#pragma once

#include <JuceHeader.h>

namespace Param
{
    namespace ID
    {
        static const juce::String Enabled { "enabled" };
        static const juce::String Drive { "drive" };
        static const juce::String Frequency { "frequency" };
        static const juce::String Resonance { "resonance" };
        static const juce::String Mode { "mode" };
        static const juce::String PostGain { "post_gain" };

        static const juce::String ModulationFreq { "modulation_freq" };

    }

    namespace Name
    {
        static const juce::String Enabled { "Enabled" };
        static const juce::String Drive { "Drive" };
        static const juce::String Frequency { "Frequency" };
        static const juce::String Resonance { "Resonance" };
        static const juce::String Mode { "Mode" };
        static const juce::String PostGain { "Post-Gain" };

        static const juce::String ModulationFreq { "Modulation Frequency" };

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
    juce::dsp::LadderFilter<float> filter;
    juce::SmoothedValue<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessor)

    float modulationPhase = 0.0f;
    float modulationFreqHz = 30.0f; // default in Hz
    double currentSampleRate = 44100.0;
    bool isEnabled = true;
    float diodeSaturationAmount = 1.5f;

    juce::dsp::ProcessorDuplicator<
        juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>
    > dcBlocker;


};
