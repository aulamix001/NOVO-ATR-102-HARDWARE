#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;
 ReelGlueProcessor p;p.prepareToPlay(48000,512);
 juce::MidiBuffer midi;juce::AudioBuffer<float> audio(2,512);
 for(int block=0;block<150;++block){for(int c=0;c<2;++c)for(int i=0;i<512;++i)audio.setSample(c,i,0.3f*std::sin(static_cast<float>(block*512+i)*0.13f));p.processBlock(audio,midi);}
 auto editor=std::unique_ptr<juce::AudioProcessorEditor>(p.createEditor());editor->setSize(900,1070);editor->setVisible(true);
 juce::MessageManager::getInstance()->runDispatchLoopUntil(150);
 const auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f);
 const auto path=argc>1?juce::File(argv[1]):juce::File::getCurrentWorkingDirectory().getChildFile("reelglue.png");
 path.deleteFile();juce::FileOutputStream out(path);if(!out.openedOk())return 1;juce::PNGImageFormat png;if(!png.writeImageToStream(image,out))return 2;
 juce::MemoryBlock state;p.set("recL",4.5f);p.getStateInformation(state);ReelGlueProcessor restored;restored.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
 if(std::abs(restored.value("recL")-4.5f)>0.001f){std::cerr<<"Restored Record: "<<restored.value("recL")<<"\n";return 3;}
 auto near=[](float a,float b){return std::abs(a-b)<0.001f;};
 p.set("autogain",1);p.set("recL",7);p.syncControls("recL");
 if(!near(p.value("repL"),-7)||!near(p.value("recR"),7))return 4;
 p.set("link",0);p.set("recR",2);p.syncControls("recR");
 if(!near(p.value("repR"),-2)||!near(p.value("recL"),7))return 5;
 p.set("autoCal",1);p.set("speed",1);p.syncControls("speed");
 auto cfg=reelglue::resolve(1,1,0,2,0);const auto ideal=reelglue::idealFor(cfg);
 const std::array<const char*,5> ids {"hfEq","shelfEq","reproHf","reproLf","bias"};
 for(size_t i=0;i<5;++i)if(!near(p.value(ids[i]),static_cast<float>(ideal[i])))return 6;
 p.setCurrentProgram(0);
 bool opened=false;
 for(auto* child:editor->getChildren())if(auto* button=dynamic_cast<juce::Button*>(child))if(button->getButtonText()=="OPEN"){button->triggerClick();opened=true;break;}
 juce::MessageManager::getInstance()->runDispatchLoopUntil(100);
 if(!opened)return 7;
 auto expanded=path.getSiblingFile("reelglue-expanded.png");expanded.deleteFile();juce::FileOutputStream extra(expanded);
 if(!extra.openedOk()||!png.writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds(),true,1.5f),extra))return 8;
 editor->setSize(675,803);juce::MessageManager::getInstance()->runDispatchLoopUntil(60);
 auto small=path.getSiblingFile("reelglue-small.png");small.deleteFile();juce::FileOutputStream compact(small);
 if(!compact.openedOk()||!png.writeImageToStream(editor->createComponentSnapshot(editor->getLocalBounds(),true),compact))return 9;
 std::cout<<"UI render (closed, expanded, resized), audio, state, Link/Auto Gain and Auto Cal passed\n";
 return 0;
}
