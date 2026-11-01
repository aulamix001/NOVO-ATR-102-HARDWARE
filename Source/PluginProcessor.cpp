#include "PluginProcessor.h"
#include "PluginEditor.h"
namespace {
const juce::StringArray presets {"Studio Alignment", "Fifteen & Warm", "Low Speed Patina", "Midnight Master", "Soft Landing", "Copper Drive", "Wide Open", "Clean Circuit", "Drum Room", "Velvet Print", "Slow Motion", "Final Pass"};
const std::array<const char*,12> controlIds {"recL","recR","repL","repR","link","autogain","autoCal","speed","head","slot","emphasis","cal"};
const std::array<const char*,5> calIds {"hfEq","shelfEq","reproHf","reproLf","bias"};
}
juce::AudioProcessorValueTreeState::ParameterLayout ReelGlueProcessor::layout() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    auto choice=[&](const char* id,const char* name,juce::StringArray items,int initial){p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id,1},name,items,initial));};
    auto boolean=[&](const char* id,const char* name,bool initial){p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},name,initial));};
    auto number=[&](const char* id,const char* name,float lo,float hi,float initial){p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,0.001f),initial));};
    number("recL","Record left",-20,20,0);number("recR","Record right",-20,20,0);
    number("repL","Reproduce left",-20,20,0);number("repR","Reproduce right",-20,20,0);
    choice("speed","Tape speed",{"3.75","7.5","15","30"},2);
    choice("head","Head width",{"1/4 inch","1/2 inch","1 inch"},1);
    choice("slot","Tape formula slot",{"I","II","III","IV"},0);
    choice("cal","Calibration",{"0 dB","+3 dB","+6 dB","+9 dB"},2);
    choice("emphasis","Emphasis",{"NAB","CCIR"},0);
    choice("path","Monitor path",{"SYNC","REPRO","INPUT"},1);
    choice("quality","Quality",{"Eco","Normal","High"},1);
    boolean("link","Link channels",true);boolean("autogain","Auto gain",false);
    boolean("power","Power",true);boolean("thru","Thru",false);
    boolean("meterIn","Meter input",false);boolean("wow","Wow and flutter",true);
    boolean("noise","Hiss and hum",false);boolean("crosstalk","Crosstalk",true);
    boolean("transformer","Transformers",true);boolean("autoCal","Auto calibration",false);
    for(auto id:calIds)number(id,id,0,1,0.5f);
    number("mix","Mix",0,1,1);return p;
}
ReelGlueProcessor::ReelGlueProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"ReelGlueState",layout()) {
    for(auto* p:getParameters())if(auto* a=dynamic_cast<juce::AudioProcessorParameterWithID*>(p))state.addParameterListener(a->paramID,this);
    startTimerHz(40);
}
ReelGlueProcessor::~ReelGlueProcessor(){stopTimer();for(auto* p:getParameters())if(auto* a=dynamic_cast<juce::AudioProcessorParameterWithID*>(p))state.removeParameterListener(a->paramID,this);}
void ReelGlueProcessor::parameterChanged(const juce::String& id,float){
    configChanged.store(true);controlsChanged.store(true);
    if(!controlsSyncing.load())for(size_t i=0;i<controlIds.size();++i)if(id==controlIds[i]){pendingControl.store(static_cast<int>(i));break;}
}
void ReelGlueProcessor::timerCallback(){const int i=pendingControl.exchange(-1);if(i>=0)syncControls(controlIds[static_cast<size_t>(i)]);}
void ReelGlueProcessor::set(const char* id,float v){auto* p=state.getParameter(id);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(v));p->endChangeGesture();}
void ReelGlueProcessor::syncControls(const juce::String& id) {
    if(controlsSyncing.exchange(true))return;
    const bool linked=value("link")>0.5f;
    if(value("autogain")>0.5f && !(linked&&id.endsWith("R"))) {
        if(id.startsWith("rep"))set(id=="repR"&&!linked?"recR":"recL",-value(id.toRawUTF8()));
        else {set("repL",-value("recL"));if(!linked)set("repR",-value("recR"));}
    }
    if(linked){set("recR",value("recL"));set("repR",value("repL"));}
    if(value("autoCal")>0.5f && (id=="autoCal"||id=="speed"||id=="head"||id=="slot"||id=="emphasis")) {
        auto ideal=reelglue::idealFor(reelglue::resolve(static_cast<int>(value("speed")),static_cast<int>(value("head")),static_cast<int>(value("slot")),static_cast<int>(value("cal")),static_cast<int>(value("emphasis"))));
        for(size_t i=0;i<5;++i)set(calIds[i],static_cast<float>(ideal[i]));
    }
    controlsSyncing.store(false);
}
bool ReelGlueProcessor::isBusesLayoutSupported(const BusesLayout& b)const{auto in=b.getMainInputChannelSet(),out=b.getMainOutputChannelSet();return (in==juce::AudioChannelSet::mono()&&(out==in||out==juce::AudioChannelSet::stereo()))||(in==juce::AudioChannelSet::stereo()&&out==in);}
void ReelGlueProcessor::prepareToPlay(double sr,int){for(int c=0;c<2;++c)channels[c].prepare(sr,c);setLatencySamples(channels[0].latency());bleed[0]=bleed[1]=0;bleedCoeff=1-std::exp(-2*reelglue::constants::pi*reelglue::constants::crosstalkCutoff/sr);configChanged.store(true);}
void ReelGlueProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals guard;
    if(configChanged.exchange(false)){
        reelglue::Settings s;
        s.config=reelglue::resolve(static_cast<int>(value("speed")),static_cast<int>(value("head")),static_cast<int>(value("slot")),static_cast<int>(value("cal")),static_cast<int>(value("emphasis")));
        s.power=value("power")>0.5f;s.thru=value("thru")>0.5f;s.transformer=value("transformer")>0.5f;
        s.noise=value("noise")>0.5f;s.wow=value("wow")>0.5f;s.mix=value("mix");s.path=static_cast<int>(value("path"));s.quality=static_cast<int>(value("quality"));
        for(size_t i=0;i<5;++i)s.calibration[i]=value(calIds[i]);
        const bool linked=value("link")>0.5f||getTotalNumInputChannels()==1;
        for(int c=0;c<2;++c){s.record=value(c==0||linked?"recL":"recR");s.reproduce=value(c==0||linked?"repL":"repR");if(value("autogain")>0.5f)s.reproduce=-s.record;channels[c].configure(s);}
    }
    auto* left=buffer.getWritePointer(0);auto* right=buffer.getNumChannels()>1?buffer.getWritePointer(1):nullptr;
    const bool cross=value("crosstalk")>0.5f&&value("power")>0.5f&&value("thru")<0.5f&&value("path")<1.5f;
    float inputLevel[2] {0,0};
    for(int i=0;i<buffer.getNumSamples();++i){float il=left[i],ir=getTotalNumInputChannels()>1&&right?right[i]:il;
        inputLevel[0]=std::max(inputLevel[0],std::abs(il));inputLevel[1]=std::max(inputLevel[1],std::abs(ir));
        float l=channels[0].process(il),r=channels[1].process(ir);
        bleed[0]+=bleedCoeff*(l-bleed[0]);bleed[1]+=bleedCoeff*(r-bleed[1]);
        if(cross){l+=static_cast<float>(bleed[1]*reelglue::constants::crosstalkGain);r+=static_cast<float>(bleed[0]*reelglue::constants::crosstalkGain);}
        left[i]=l;if(right)right[i]=r;
    }
    for(int c=0;c<2;++c){meters[c].store(value("meterIn")>0.5f?inputLevel[c]*0.70710678f:static_cast<float>(channels[c].level()));clips[c].store(channels[c].clipping());}
}
juce::AudioProcessorEditor* ReelGlueProcessor::createEditor(){return new ReelGlueEditor(*this);}
const juce::String ReelGlueProcessor::getProgramName(int i){return presets[juce::jlimit(0,11,i)];}
void ReelGlueProcessor::setCurrentProgram(int i){currentProgram=juce::jlimit(0,11,i);set("link",0);set("recL",0);set("recR",0);set("repL",0);set("repR",0);set("speed",2);set("head",1);set("slot",0);set("cal",2);set("emphasis",0);set("path",1);set("thru",0);set("transformer",1);set("noise",0);set("wow",1);set("mix",1);set("autogain",0);for(auto id:calIds)set(id,0.5f);
    const int n=currentProgram;
    if(n==1||n==5||n==8){set("recL",n==5?9.f:4.f);set("repL",n==5?-5.f:-2.f);}
    if(n==2||n==10)set("speed",n==2?1.f:0.f);
    if(n==3||n==6||n==7)set("speed",3);
    if(n==7)set("transformer",0);
    if(n==4||n==9)set("cal",1);
    if(n==6)set("head",2);
    if(n==10)set("noise",1);
    set("autoCal",1);syncControls("autoCal");set("link",1);syncControls("link");
}
void ReelGlueProcessor::getStateInformation(juce::MemoryBlock& dest){auto tree=state.copyState();tree.setProperty("version",1,nullptr);tree.setProperty("program",currentProgram,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,dest);}
void ReelGlueProcessor::setStateInformation(const void* data,int bytes){if(auto xml=getXmlFromBinary(data,bytes))if(xml->hasTagName(state.state.getType())){auto tree=juce::ValueTree::fromXml(*xml);currentProgram=static_cast<int>(tree.getProperty("program",0));state.replaceState(tree);configChanged.store(true);}}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new ReelGlueProcessor();}
