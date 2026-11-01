#pragma once
#include <JuceHeader.h>
#include "Dsp/TapeChannel.h"
class ReelGlueProcessor final : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener, private juce::Timer {
public:
    ReelGlueProcessor();
    ~ReelGlueProcessor() override;
    void prepareToPlay(double,int) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    const juce::String getName() const override {return "ReelGlue";}
    bool acceptsMidi() const override {return false;}
    bool producesMidi() const override {return false;}
    double getTailLengthSeconds() const override {return 0.002;}
    int getNumPrograms() override {return 12;}
    int getCurrentProgram() override {return currentProgram;}
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int,const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    float value(const char* id) const {return state.getRawParameterValue(id)->load();}
    void set(const char*,float);
    void syncControls(const juce::String&);
    juce::AudioProcessorValueTreeState state;
    std::array<std::atomic<float>,2> meters {0.f,0.f};
    std::array<std::atomic<bool>,2> clips {false,false};
    std::atomic<bool> configChanged {true};
    std::atomic<bool> controlsChanged {true};
private:
    void timerCallback() override;
    std::atomic<int> pendingControl {-1};
    std::atomic<bool> controlsSyncing {false};
    void parameterChanged(const juce::String&,float) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout layout();
    reelglue::TapeChannel channels[2];
    int currentProgram=0; double bleed[2] {0,0}; double bleedCoeff=0.5;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReelGlueProcessor)
};
