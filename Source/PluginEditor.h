#pragma once
#include "PluginProcessor.h"
#include "Gui/HardwareLook.h"
class ReelGlueEditor final:public juce::AudioProcessorEditor,private juce::Timer {
public:
    explicit ReelGlueEditor(ReelGlueProcessor&);
    ~ReelGlueEditor()override;
    void paint(juce::Graphics&)override;
    void resized()override;
    void mouseDown(const juce::MouseEvent&)override;
private:
    void timerCallback()override;
    void meter(juce::Graphics&,juce::Rectangle<float>,int);
    void reel(juce::Graphics&,float,float,float,float,bool);
    void deck(juce::Graphics&);
    void advancedPanel(juce::Graphics&);
    void addKnob(const char*,juce::Rectangle<int>,bool=false);
    void addKey(const char*,const char*,juce::Rectangle<int>,bool=false);
    void layoutControls();
    ReelGlueProcessor& processor;
    hardware::Look look;
    juce::TooltipWindow tooltips {this,600};
    struct Knob {juce::String id;juce::Rectangle<int> bounds;std::unique_ptr<juce::Slider> component;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;};
    struct Key {juce::String id;juce::Rectangle<int> bounds;std::unique_ptr<hardware::Key> component;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;};
    std::vector<Knob> knobs;std::vector<Key> keys;
    juce::ComboBox preset;
    juce::TextButton previous {"<"},next {">"};
    hardware::Key open {"OPEN",false},close {"CLOSE",false};
    std::vector<std::unique_ptr<hardware::Key>> selectors;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> parameterWatchers;
    bool advanced=false,paused=false,syncing=false;
    float rotation=0;std::array<float,2> levels {-20,-20};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReelGlueEditor)
};
