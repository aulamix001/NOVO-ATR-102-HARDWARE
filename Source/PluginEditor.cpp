#include "PluginEditor.h"
using namespace hardware;
namespace {
constexpr float W=900,H=1070;
const std::array<const char*,5> calNames {"HF EQ","SHELF EQ","REPRO HF","REPRO LF","BIAS"};
const std::array<const char*,5> calIds {"hfEq","shelfEq","reproHf","reproLf","bias"};
}
ReelGlueEditor::ReelGlueEditor(ReelGlueProcessor& p):AudioProcessorEditor(p),processor(p) {
    setLookAndFeel(&look);setResizable(true,true);getConstrainer()->setFixedAspectRatio(W/H);setResizeLimits(675,803,1800,2140);setSize(810,963);
    addKnob("repL",{219,151,76,76});addKnob("recL",{338,151,76,76});
    addKnob("repR",{663,151,76,76});addKnob("recR",{782,151,76,76});
    addKnob("speed",{501,679,65,65},true);
    addKey("autogain","AUTO GAIN",{68,735,55,48});addKey("link","LINK",{133,735,55,48});
    addKey("emphasis","CCIR",{133,879,55,48});
    addKey("power","POWER",{365,678,54,48},true);
    addKey("thru","THRU",{798,915,52,48});
    addKey("meterIn","METER IN",{68,800,55,48});
    addKey("autoCal","AUTO CAL",{133,800,55,48});
    addKey("wow","WOW / FLUTTER",{242,800,106,48},true);
    addKey("noise","HISS / HUM",{355,800,96,48},true);
    addKey("crosstalk","CROSSTALK",{460,800,96,48},true);
    addKey("transformer","TRANSFORMER",{565,800,102,48},true);
    for(size_t i=0;i<5;++i)addKnob(calIds[i],{234+static_cast<int>(i)*72,862,68,68});
    addKnob("mix",{594,862,68,68},true);
    for(int col=0;col<3;++col)for(int row=0;row<4;++row){auto b=std::make_unique<hardware::Key>("",false);b->compact=true;
        b->onClick=[this,col,row]{const char* id=col==0?"slot":(col==1?"cal":"head");processor.set(id,static_cast<float>(col==2?std::min(row,2):row));processor.syncControls(id);};addAndMakeVisible(*b);selectors.push_back(std::move(b));}
    for(int row=0;row<3;++row){auto b=std::make_unique<hardware::Key>(juce::StringArray{"SYNC","REPRO","INPUT"}[row],false);b->compact=true;
        b->onClick=[this,row]{processor.set("path",static_cast<float>(row));};addAndMakeVisible(*b);selectors.push_back(std::move(b));}
    auto quality=std::make_unique<hardware::Key>("NORMAL",false);quality->compact=true;
    quality->onClick=[this]{processor.set("quality",std::fmod(processor.value("quality")+1,3));};addAndMakeVisible(*quality);selectors.push_back(std::move(quality));
    for(int i=0;i<12;++i)preset.addItem(processor.getProgramName(i),i+1);
    preset.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);preset.setTooltip("Factory presets. Recalling a preset replaces current settings.");
    preset.setColour(juce::ComboBox::backgroundColourId,colour(0xff343332));preset.setColour(juce::ComboBox::outlineColourId,colour(0xff585756));preset.setColour(juce::ComboBox::textColourId,cream);
    preset.onChange=[this]{processor.setCurrentProgram(preset.getSelectedId()-1);};addAndMakeVisible(preset);
    previous.onClick=[this]{preset.setSelectedId((preset.getSelectedId()+10)%12+1);};next.onClick=[this]{preset.setSelectedId(preset.getSelectedId()%12+1);};addAndMakeVisible(previous);addAndMakeVisible(next);
    for(auto* b:{&previous,&next}){b->setColour(juce::TextButton::buttonColourId,colour(0xff343332));b->setColour(juce::TextButton::textColourOffId,cream);}
    open.onClick=[this]{advanced=true;layoutControls();repaint();};close.onClick=[this]{advanced=false;layoutControls();repaint();};addAndMakeVisible(open);addAndMakeVisible(close);
    open.setTooltip("Open the calibration and tape imperfections panel");close.setTooltip("Close the calibration panel");
    layoutControls();startTimerHz(40);
}
ReelGlueEditor::~ReelGlueEditor(){stopTimer();parameterWatchers.clear();setLookAndFeel(nullptr);}
void ReelGlueEditor::addKnob(const char* id,juce::Rectangle<int> bounds,bool silver) {
    Knob k;k.id=id;k.bounds=bounds;k.component=std::make_unique<juce::Slider>();auto& s=*k.component;
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    s.setRotaryParameters(juce::MathConstants<float>::pi*1.25f,juce::MathConstants<float>::pi*2.75f,true);
    s.setVelocityModeParameters(0.5,1,0.1,true,juce::ModifierKeys::shiftModifier);s.setPopupDisplayEnabled(true,true,this);s.setName(processor.state.getParameter(id)->getName(50));s.setTooltip(s.getName()+" / Drag to adjust / Shift for fine control / Double-click to reset");s.setWantsKeyboardFocus(true);s.getProperties().set("silver",silver);
    k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,s);addAndMakeVisible(s);knobs.push_back(std::move(k));
}
void ReelGlueEditor::addKey(const char* id,const char* label,juce::Rectangle<int> bounds,bool dark) {
    Key k;k.id=id;k.bounds=bounds;k.component=std::make_unique<hardware::Key>(label);k.component->dark=dark;
    if(juce::String(id)=="emphasis") {k.component->onClick=[this]{processor.set("emphasis",processor.value("emphasis")>0.5f?0.f:1.f);};}
    else k.attachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,id,*k.component);
    k.component->setTooltip(processor.state.getParameter(id)->getName(50)+(juce::String(id)=="autoCal"?" / Enabling replaces manual calibration. Save a preset first.":""));addAndMakeVisible(*k.component);keys.push_back(std::move(k));
}
void ReelGlueEditor::resized(){layoutControls();}
void ReelGlueEditor::layoutControls() {
    const float scale=getWidth()/W;
    auto place=[scale](juce::Component& c,juce::Rectangle<int> b){c.setBounds(b);c.setTransform(juce::AffineTransform::scale(scale));};
    for(auto& k:knobs){place(*k.component,k.bounds);k.component->setVisible(k.id=="speed"||k.id.startsWith("rec")||k.id.startsWith("repL")||k.id=="repR"||advanced);if(k.id=="reproHf"||k.id=="reproLf")k.component->setVisible(advanced);}
    for(auto& k:keys){place(*k.component,k.bounds);k.component->setVisible(k.id!="wow"&&k.id!="noise"&&k.id!="crosstalk"&&k.id!="transformer"?true:advanced);}
    place(open,{133,685,55,48});place(close,{68,685,55,48});
    place(preset,{326,39,232,29});place(previous,{294,39,28,29});place(next,{562,39,28,29});
    int i=0;for(auto& b:selectors){if(i<12)place(*b,{698+(i/4)*51,724+(i%4)*27,49,25});else if(i<15)place(*b,{696+(i-12)*52,869,50,29});else {place(*b,{547,945,116,25});b->setVisible(advanced);}++i;}
    repaint();
}
void ReelGlueEditor::timerCallback(){
    if(!paused&&processor.value("power")>0.5f)rotation+=0.009f*static_cast<float>(reelglue::constants::speeds[static_cast<int>(processor.value("speed"))]/15.0);
    for(int c=0;c<2;++c){const auto gain=processor.meters[c].load();const float target=juce::jlimit(-22.f,6.f,juce::Decibels::gainToDecibels(gain,-60.f)+12.f);levels[c]+=0.18f*(target-levels[c]);}
    const auto cfg=reelglue::resolve(static_cast<int>(processor.value("speed")),static_cast<int>(processor.value("head")),static_cast<int>(processor.value("slot")),static_cast<int>(processor.value("cal")),static_cast<int>(processor.value("emphasis")));
    const std::array<juce::StringArray,3> options {juce::StringArray{},juce::StringArray{"0","+3","+6","+9"},juce::StringArray{"1/4\"","1/2\"","1\"","-"}};
    int i=0;for(auto& b:selectors){if(i<12){const auto col=i/4,row=i%4;auto name=col==0?juce::String(reelglue::constants::formulaNames[reelglue::resolve(cfg.speed,cfg.head,row,cfg.cal,cfg.emphasis).formula]):options[col][row];b->setButtonText(name);b->setToggleState(row==(col==0?cfg.slot:(col==1?cfg.cal:cfg.head)),juce::dontSendNotification);b->setEnabled(!(col==2&&(row==3||(cfg.speed<=1&&row>0))));}else if(i<15)b->setToggleState(i-12==static_cast<int>(processor.value("path")),juce::dontSendNotification);else b->setButtonText(juce::StringArray{"ECO / PREVIEW","NORMAL","HIGH / PREVIEW"}[static_cast<int>(processor.value("quality"))]);++i;}
    for(auto& k:keys)if(k.id=="emphasis")k.component->setToggleState(cfg.emphasis==1,juce::dontSendNotification);
    auto ideal=reelglue::idealFor(cfg);
    bool calibrated=true;
    for(size_t n=0;n<5;++n){if(std::abs(processor.value(calIds[n])-ideal[n])>0.002)calibrated=false;}
    for(auto& k:keys)if(k.id=="autoCal")k.component->red=!calibrated;
    for(auto& k:knobs)for(size_t n=0;n<5;++n)if(k.id==calIds[n])k.component->setDoubleClickReturnValue(true,ideal[n]);
    for(auto& b:selectors)b->setTooltip("Select "+b->getButtonText());
    preset.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
    repaint();
}
void ReelGlueEditor::mouseDown(const juce::MouseEvent& e){const auto p=e.position/(getWidth()/W);if(p.y>265&&p.y<635){paused=!paused;repaint();}
    if(advanced&&p.y>=849&&p.y<864){const int index=static_cast<int>((p.x-234)/72);if(p.x>=234&&index>=0&&index<5){auto cfg=reelglue::resolve(static_cast<int>(processor.value("speed")),static_cast<int>(processor.value("head")),static_cast<int>(processor.value("slot")),static_cast<int>(processor.value("cal")),static_cast<int>(processor.value("emphasis")));processor.set(calIds[static_cast<size_t>(index)],static_cast<float>(reelglue::idealFor(cfg)[static_cast<size_t>(index)]));}}
}
void ReelGlueEditor::meter(juce::Graphics& g,juce::Rectangle<float> r,int channel) {
    gradient(g,r,colour(0xff464544),colour(0xff292827));g.setColour(colour(0xffb1b1a2));g.drawRect(r,1);g.setColour(colour(0xff100f0e));g.drawRect(r.reduced(3),2);
    auto window=juce::Rectangle<float>(r.getX()+18,r.getY()+28,164,82);
    juce::Path frame;frame.startNewSubPath(window.getX()-4,window.getY()-6);frame.lineTo(window.getRight()+3,window.getY()-4);frame.lineTo(window.getRight()-1,window.getBottom()+3);frame.lineTo(window.getX()+3,window.getBottom()+5);frame.closeSubPath();
    g.setColour(colour(0xff11100f));g.fillPath(frame);
    gradient(g,window,colour(0xffdc8a22),colour(0xffffd175));
    juce::ColourGradient glow(colour(0xffffe096),window.getCentreX(),window.getCentreY(),colour(0xffbb6814),window.getX()-15,window.getY(),true);g.setGradientFill(glow);g.fillRect(window);
    const float cx=window.getCentreX(),cy=window.getBottom()+24,rad=91;
    for(int i=0;i<=26;++i){const float a=(-50+i*3.8f)*juce::MathConstants<float>::pi/180;const bool red=i>19;
        g.setColour(red?colour(0xffbc351a):colour(0xff49351c));const float len=i%5==0?9.f:5.f;g.drawLine(cx+std::sin(a)*(rad-len),cy-std::cos(a)*(rad-len),cx+std::sin(a)*rad,cy-std::cos(a)*rad,i%5==0?1.5f:0.8f);
        if(i%5==0)text(g,juce::StringArray{"20","10","5","0","3","6"}[i/5],{juce::jlimit(window.getX()+2,window.getRight()-20,cx+std::sin(a)*(rad+9)-10),cy-std::cos(a)*(rad+9)-6,20,12},8,red?colour(0xffa52616):colour(0xff49351c));}
    text(g,"VU",{cx-20,window.getY()+39,40,19},15,colour(0xff73501d),juce::Justification::centred,true);
    text(g,channel==0?"LEFT":"RIGHT",{window.getX()+3,window.getBottom()-13,45,10},7,colour(0xff7f501d));
    const auto angle=juce::jmap(juce::jlimit(-22.f,6.f,levels[channel]),-22.f,6.f,-50.f,48.8f)*juce::MathConstants<float>::pi/180;
    {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(window.toNearestInt());
        g.setColour(juce::Colours::black.withAlpha(0.18f));g.drawLine(cx+2,cy,cx+std::sin(angle)*(rad-3)+2,cy-std::cos(angle)*(rad-3)+2,2.5f);
        g.setColour(colour(0xff322a1e));g.drawLine(cx,cy,cx+std::sin(angle)*(rad-3),cy-std::cos(angle)*(rad-3),1.4f);
        juce::Path reflection;reflection.addTriangle(window.getX(),window.getY(),window.getRight(),window.getY(),window.getX(),window.getBottom());g.setColour(juce::Colours::white.withAlpha(0.08f));g.fillPath(reflection);}
    text(g,"REPRODUCE",{r.getX()+204,r.getY()+13,92,12},9,muted,juce::Justification::centred,true);
    text(g,"RECORD",{r.getX()+322,r.getY()+13,92,12},9,muted,juce::Justification::centred,true);
    for(int j=0;j<2;++j){const float x=r.getX()+212+j*119;gradient(g,{x,r.getY()+112,66,19},colour(0xff151413),colour(0xff373635));const auto db=processor.value(channel==0?(j==0?"repL":"recL"):(j==0?"repR":"recR"));text(g,juce::String(db,1)+" dB",{x,r.getY()+112,66,19},11,amber);}
    screw(g,r.getX()+8,r.getY()+9,3);screw(g,r.getRight()-9,r.getBottom()-9,3);
    led(g,window.getCentreX(),r.getBottom()-17,processor.clips[channel].load(),colour(0xffee4c28),3);text(g,"PEAK",{window.getCentreX()+9,r.getBottom()-23,39,12},8,muted);
}
void ReelGlueEditor::reel(juce::Graphics& g,float cx,float cy,float radius,float angle,bool right) {
    const auto outer=juce::Rectangle<float>(cx-radius,cy-radius,radius*2,radius*2);
    juce::DropShadow(juce::Colours::black.withAlpha(0.75f),16,{3,13}).drawForPath(g,[&]{juce::Path p;p.addEllipse(outer);return p;}());
    // Visible tape winding below the machined aluminium flange.
    g.setGradientFill(juce::ColourGradient(colour(0xff443020),cx-radius,cy-radius,colour(0xff100e0a),cx+radius,cy+radius,false));g.fillEllipse(outer.reduced(11));
    for(int i=0;i<110;++i){float r=radius*0.37f+i*(radius*0.56f/110);g.setColour(colour(i%3?0xff3f2b1e:0xff765339).withAlpha(0.42f));g.drawEllipse(cx-r,cy-r,r*2,r*2,0.6f);}
    juce::Path metal;metal.addEllipse(outer);metal.setUsingNonZeroWinding(false);
    for(int i=0;i<3;++i){const float a=angle+i*juce::MathConstants<float>::twoPi/3;
        juce::Path hole;hole.startNewSubPath(-32,-radius*0.76f);hole.quadraticTo(-39,-radius*0.82f,-47,-radius*0.72f);hole.lineTo(-67,-radius*0.39f);hole.quadraticTo(-67,-radius*0.32f,-53,-radius*0.31f);hole.lineTo(36,-radius*0.31f);hole.quadraticTo(52,-radius*0.33f,47,-radius*0.43f);hole.lineTo(8,-radius*0.76f);hole.closeSubPath();hole.applyTransform(juce::AffineTransform::rotation(a).translated(cx,cy));metal.addPath(hole);}
    juce::ColourGradient aluminium(colour(0xffddddcf),cx-radius,cy-radius,colour(0xff747372),cx+radius*0.9f,cy+radius,false);aluminium.addColour(0.35,colour(0xffabaaa9));aluminium.addColour(0.65,colour(0xffc6c5c4));g.setGradientFill(aluminium);g.fillPath(metal);
    g.setColour(colour(0xffe3e2e1).withAlpha(0.45f));g.strokePath(metal,juce::PathStrokeType(1));
    {juce::Graphics::ScopedSaveState save(g);g.reduceClipRegion(metal);for(int i=0;i<420;++i){float y=cy-radius+i;g.setColour((i%3==0?juce::Colours::black:juce::Colours::white).withAlpha(i%3==0?0.018f:0.025f));g.drawHorizontalLine(static_cast<int>(y),cx-radius,cx+radius);}}
    // Fine concentric lathe marks are drawn as vectors, not image textures.
    for(int i=0;i<5;++i){g.setColour(colour(0xffe9e8e7).withAlpha(0.12f));g.drawEllipse(outer.reduced(3+i),0.5f);}
    const float hub=radius*0.245f;auto hb=juce::Rectangle<float>(cx-hub,cy-hub,hub*2,hub*2);
    g.setColour(colour(0xff272625));g.fillEllipse(hb.expanded(3));g.setGradientFill(juce::ColourGradient(colour(0xff0a0908),cx-hub,cy-hub,colour(0xff585756),cx+hub,cy+hub,false));g.fillEllipse(hb);
    g.setColour(colour(0xff8a8988).withAlpha(0.55f));g.drawEllipse(hb.reduced(2),0.8f);
    g.setGradientFill(juce::ColourGradient(colour(0xff4f4e4d),cx-hub*0.6f,cy-hub,colour(0xff100f0e),cx+hub*0.3f,cy+hub,false));g.fillEllipse(hb.reduced(8));screw(g,cx,cy,4);
    for(int i=0;i<3;++i){const auto a=angle+(i*2+1)*juce::MathConstants<float>::pi/3;const auto x=cx+std::sin(a)*radius*0.34f,y=cy-std::cos(a)*radius*0.34f;screw(g,x,y,3);}
    juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::rotation(angle,cx,cy));text(g,right?"PRECISION REEL  /  10.5\"":"R G  /  STUDIO SERIES",{cx-80,cy+radius*0.58f,160,14},9,colour(0xff656463));
}
void ReelGlueEditor::deck(juce::Graphics& g) {
    auto face=juce::Rectangle<float>(25,265,850,725);
    gradient(g,face,colour(0xff393837),colour(0xff1a1918));
    for(int i=0;i<3900;++i){const float x=31+static_cast<float>((i*137)%838),y=270+static_cast<float>((i*71)%710);g.setColour((i%2?juce::Colours::white:juce::Colours::black).withAlpha(0.025f));g.fillRect(x,y,1.f,1.f);}
    // Brushed aluminium chassis rails and recessed screws.
    for(float x:{25.f,855.f}){gradient(g,{x,268,20,715},colour(0xffd5d4d3),colour(0xff706f6e),true);g.setColour(colour(0xffefeeed));g.drawVerticalLine(static_cast<int>(x+2),269,982);screw(g,x+10,643);screw(g,x+10,971);}
    // Tape path passes underneath the reels and guide rollers.
    juce::Path tape;tape.startNewSubPath(115,548);tape.lineTo(292,698);tape.lineTo(602,698);tape.lineTo(784,544);
    g.setColour(colour(0xff0b0a09));g.strokePath(tape,juce::PathStrokeType(7));g.setColour(colour(0xff6b4630));g.strokePath(tape,juce::PathStrokeType(3));g.setColour(colour(0xff98734a).withAlpha(0.4f));g.strokePath(tape,juce::PathStrokeType(0.6f));
    // Head block, capstan bridge, and precision rollers.
    juce::Path cover;cover.startNewSubPath(247,708);cover.lineTo(651,708);cover.lineTo(680,797);cover.quadraticTo(684,819,662,841);cover.lineTo(576,923);cover.lineTo(349,923);cover.lineTo(245,837);cover.quadraticTo(224,815,230,782);cover.closeSubPath();
    juce::DropShadow(juce::Colours::black.withAlpha(0.9f),9,{0,7}).drawForPath(g,cover);gradient(g,{240,710,430,210},colour(0xff4f4e4d),colour(0xff2a2928));g.setColour(colour(0xff242322));g.fillPath(cover);
    g.setGradientFill(juce::ColourGradient(colour(0xff4d4c4b),300,710,colour(0xff2b2a29),580,920,false));g.fillPath(cover);g.setColour(colour(0xff787776).withAlpha(0.45f));g.strokePath(cover,juce::PathStrokeType(1.2f));
    gradient(g,{324,920,278,14},colour(0xffcac9c8),colour(0xff605f5e));
    for(float x:{294.f,606.f}){g.setColour(colour(0xff151413));g.fillEllipse(x-29,676,58,61);g.setGradientFill(juce::ColourGradient(colour(0xfff1eed9),x-18,680,colour(0xff959493),x+20,727,false));g.fillEllipse(x-20,682,40,45);screw(g,x,707,8);g.setColour(colour(0xff797877));g.drawEllipse(x-23,679,46,51,1);}
    auto fly=juce::Rectangle<float>(566,747,100,100);g.setColour(colour(0xff0d0c0b));g.fillEllipse(fly.expanded(3));g.setGradientFill(juce::ColourGradient(colour(0xffe2e1e0),570,750,colour(0xff6d6c6b),655,842,false));g.fillEllipse(fly);g.setColour(colour(0xffcfcecd));g.drawEllipse(fly.reduced(4),1);
    g.setGradientFill(juce::ColourGradient(colour(0xff797877),592,770,colour(0xffe5e4e3),637,822,false));g.fillEllipse(fly.reduced(24));g.setColour(colour(0xff8a8988));g.drawEllipse(fly.reduced(24),1);
    {juce::Graphics::ScopedSaveState save(g);g.addTransform(juce::AffineTransform::rotation(-0.4f,638,900));gradient(g,{623,875,30,49},colour(0xffe4e3e2),colour(0xff787776),true);g.setColour(colour(0xffbfbebd));g.drawVerticalLine(629,876,923);}
    for(float x:{265.f,637.f})screw(g,x,741,4);
    text(g,"30",{483,675,23,12},10,muted);text(g,"15",{479,690,23,12},10,amber);text(g,"7.5",{476,705,26,12},10,muted);text(g,"3.75",{475,720,26,12},9,muted);
    text(g,"IPS",{522,744,33,12},9,muted);text(g,"TWO-TRACK MASTERING RECORDER",{300,868,280,14},9,colour(0xff828180));
    text(g,"R E E L G L U E",{296,817,250,35},25,colour(0xffa2a1a0),juce::Justification::centred,true);
    // Illuminated enamel control panels.
    for(float x:{50.f,687.f}){juce::DropShadow(juce::Colours::black.withAlpha(0.8f),6,{2,4}).drawForRectangle(g,juce::Rectangle<int>(static_cast<int>(x),648,166,326));gradient(g,{x,648,166,326},colour(0xffeeeddf),colour(0xffbbbab9),true);g.setColour(colour(0xff474645));g.drawRoundedRectangle({x+5,653,156,315},3,2);gradient(g,{x+9,656,148,24},colour(0xffe9e8e7),colour(0xffb9b8b7));text(g,"REELGLUE",{x+8,655,150,25},20,colour(0xff3a3938),juce::Justification::centred,true);screw(g,x+13,960,3);screw(g,x+153,960,3);}
    gradient(g,{60,853,146,17},colour(0xff4a4948),colour(0xff373635));text(g,"EMPHASIS EQ",{60,853,146,17},9,cream);
    led(g,94,908,processor.value("speed")<2.5f&&processor.value("emphasis")<0.5f);text(g,"NAB",{71,882,48,14},10,colour(0xff484746));
    text(g,"TAPE",{698,698,49,19},9,colour(0xff565554));text(g,"CAL",{749,698,49,19},9,colour(0xff565554));text(g,"HEAD",{800,698,49,19},9,colour(0xff565554));
    g.setColour(colour(0xff51504f));g.drawHorizontalLine(841,697,849);
    text(g,"MONITOR SELECT",{696,844,153,17},9,colour(0xff51504f));
    led(g,721,933,processor.value("path")<0.5f,colour(0xffee5b2e),4);text(g,"SYNC",{699,943,46,14},9,colour(0xff565554));
    led(g,771,933,processor.value("path")>0.5f&&processor.value("path")<1.5f);text(g,"REPRO",{749,943,46,14},9,colour(0xff565554));
    reel(g,220,423,220,-rotation,false);reel(g,680,423,220,rotation+0.45f,true);
}
void ReelGlueEditor::advancedPanel(juce::Graphics& g){
    auto r=juce::Rectangle<float>(227,767,452,210);juce::DropShadow(juce::Colours::black.withAlpha(0.8f),10,{0,4}).drawForRectangle(g,r.toNearestInt());gradient(g,r,colour(0xff424140),colour(0xff242322));g.setColour(colour(0xff828180));g.drawRoundedRectangle(r,2,1);
    text(g,"CALIBRATION  /  TAPE CIRCUIT",{240,775,410,15},10,cream,juce::Justification::centredLeft,true);
    for(size_t i=0;i<5;++i)text(g,calNames[i],{235+static_cast<float>(i)*72,849,66,13},9,muted);
    text(g,"MIX",{595,849,66,13},9,muted);text(g,"DOUBLE-CLICK TO RESET  /  SHIFT FOR FINE ADJUST",{242,946,294,23},8,muted,juce::Justification::centredLeft);
    screw(g,237,777,3);screw(g,669,777,3);
}
void ReelGlueEditor::paint(juce::Graphics& g){
    g.addTransform(juce::AffineTransform::scale(getWidth()/W));
    gradient(g,{0,0,W,H},colour(0xff2b2a29),colour(0xff151413));
    // Lacquered walnut cheeks: layered grain with warm reflected highlights.
    for(float x:{0.f,878.f}){gradient(g,{x,0,22,H},colour(0xff71472b),colour(0xff2f1d12),true);for(int i=0;i<190;++i){g.setColour(colour(i%3==0?0xffaf7845:0xff21140e).withAlpha(0.23f));g.drawLine(x+2,static_cast<float>(i*7),x+20,static_cast<float>(i*7+9),static_cast<float>(i%2+1));}}
    auto chassis=juce::Rectangle<float>(20,12,860,1042);g.setColour(colour(0xff0b0a09));g.fillRoundedRectangle(chassis,7);
    gradient(g,{23,15,854,80},colour(0xff646362),colour(0xff2b2a29));g.setColour(colour(0xff9a9998));g.drawHorizontalLine(16,28,872);g.setColour(colour(0xff171615));g.drawHorizontalLine(94,23,877);
    text(g,"REELGLUE",{42,29,227,28},29,cream,juce::Justification::centredLeft,true);text(g,"PRECISION TAPE MACHINE",{44,63,222,14},9,muted,juce::Justification::centredLeft);
    text(g,"FACTORY PRESET",{326,20,231,14},8,muted,juce::Justification::centredLeft);text(g,"RG-02",{731,32,100,22},20,cream,juce::Justification::centredRight,true);text(g,"STEREO  /  MASTERING",{655,62,178,13},9,muted,juce::Justification::centredRight);led(g,849,48,processor.value("power")>0.5f);
    deck(g);
    // Meter bridge casts a shadow over the reel flanges.
    juce::DropShadow(juce::Colours::black.withAlpha(0.85f),8,{0,7}).drawForRectangle(g,{21,102,858,160});gradient(g,{22,101,856,160},colour(0xffcdcccb),colour(0xff787776));
    meter(g,{28,108,418,145},0);meter(g,{454,108,418,145},1);
    g.setColour(colour(0xffe5e5d6));g.drawHorizontalLine(102,22,878);g.drawHorizontalLine(260,22,878);
    if(advanced)advancedPanel(g);
    gradient(g,{30,986,840,57},colour(0xff484746),colour(0xff282726));
    for(int i=0;i<100;++i){g.setColour(juce::Colours::white.withAlpha(0.015f));g.drawHorizontalLine(988+i/2,32,868);}
    text(g,"ANALOG CHARACTER. PRECISE CONTROL.",{52,1001,450,24},11,muted,juce::Justification::centredLeft);
    text(g,paused?"REELS PAUSED":"*   TRANSPORT RUNNING",{605,1003,236,20},9,paused?muted:amber,juce::Justification::centredRight);
    for(float x:{39.f,861.f}){screw(g,x,28,3);screw(g,x,1034,3);}
    text(g,"REELGLUE AUDIO",{37,1049,250,15},8,muted,juce::Justification::centredLeft);text(g,"0.1  /  DEVELOPMENT EDITION",{601,1049,260,15},8,muted,juce::Justification::centredRight);
}
