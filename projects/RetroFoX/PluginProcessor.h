#pragma once

#include <JuceHeader.h>
#include "Flanger.h"
#include "BitCrusher.h"

namespace Param
{
    namespace ID
    {   // Enable entire effect
        static const juce::String Enabled { "enabled" };

        // Drive/distortion params
        static const juce::String Drive { "drive" };

        // Bitcrusher params
        static const juce::String BitDepth { "bit_depth" };
        static const juce::String RateReduce { "rate_reduce" };

        // Flanger/chorus params
        static const juce::String Offset { "offset" };
        static const juce::String Depth { "depth" };
        static const juce::String Rate { "rate" };
        static const juce::String ModType { "mod_type" };

        // Gain at the end of FX chain
        static const juce::String PostGain { "post-gain" };

    }

    namespace Name
    {
        static const juce::String Enabled { "Enabled" };

        static const juce::String Drive { "Drive" };
    
        static const juce::String BitDepth { "Bit Depth" };
        static const juce::String RateReduce { "Rate Reduce" };

        static const juce::String Offset { "Offset" };
        static const juce::String Depth { "Depth" };
        static const juce::String Rate { "Rate" };
        static const juce::String ModType { "Mod. Type" };
        
        static const juce::String PostGain { "Post-Gain" };

    }

    namespace Ranges
    {
        static constexpr float OffsetMin { 0.f };
        static constexpr float OffsetMax { 10.f };
        static constexpr float OffsetInc { 0.01f };
        static constexpr float OffsetSkw { 0.5f };

        static constexpr float DepthMin { 0.f };
        static constexpr float DepthMax { 10.f };
        static constexpr float DepthInc { 0.01f };
        static constexpr float DepthSkw { 0.5f };

        static constexpr float RateMin { 0.1f };
        static constexpr float RateMax { 5.f };
        static constexpr float RateInc { 0.01f };
        static constexpr float RateSkw { 0.5f };

        static const juce::StringArray ModLabels { "Sine", "Triangle" };

        static const juce::String EnabledOff { "Off" };
        static const juce::String EnabledOn { "On" };
    }

    namespace Units
    {
        static const juce::String Ms { "ms" };
        static const juce::String Hz { "Hz" };
    }
}

class MainProcessor : public juce::AudioProcessor
{
public:
    MainProcessor();
    ~MainProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void releaseResources() override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

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

    static constexpr float MaxDelaySizeMs { 20.f };
    static const unsigned int MaxChannels { 2 };
    static const unsigned int MaxProcessBlockSamples{ 32 };

private:
    mrta::ParameterManager parameterManager;

    juce::dsp::LadderFilter<float> filter;

    BitCrusher bitCrusher;

    DSP::Flanger flanger;
    DSP::Ramp<float> enableRamp;

    juce::SmoothedValue<float> outputGain;

    bool enabled { true };
    juce::AudioBuffer<float> fxBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessor)
};