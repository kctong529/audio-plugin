#pragma once

#include <JuceHeader.h>

namespace Param
{
    namespace ID
    {
        static const juce::String DelayTime { "delay_time" };
        static const juce::String Feedback { "feedback" };
        static const juce::String WetMix { "wet_mix" };
        static const juce::String DryMix { "dry_mix" };
    }

    namespace Name
    {
        static const juce::String DelayTime { "Delay Time" };
        static const juce::String Feedback { "Feedback" };
        static const juce::String WetMix { "Wet Mix" };
        static const juce::String DryMix { "Dry Mix" };
    }
}

class MainProcessor : public juce::AudioProcessor
{
public:
    MainProcessor();
    ~MainProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    mrta::ParameterManager& getParameterManager() { return parameterManager; }

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    //==============================================================================

private:
    mrta::ParameterManager parameterManager;
    // juce::dsp::LadderFilter<float> filter;
    // juce::SmoothedValue<float> outputGain;
    juce::AudioBuffer<float> delayBuffer;
    int writePosition = 0;
    double currentSampleRate = 44100.0;

    juce::SmoothedValue<float> delayTimeSamples;
    juce::SmoothedValue<float> feedback;
    juce::SmoothedValue<float> wetMix;
    juce::SmoothedValue<float> dryMix;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessor)
};
