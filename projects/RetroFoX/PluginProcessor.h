#pragma once

#include <JuceHeader.h>
#include "Flanger.h"
#include "BitCrusher.h"

namespace Param
{
    namespace ID
    {   // Enable entire effect
        //static const juce::String Enabled { "enabled" };

        // Drive/distortion params
        static const juce::String Drive { "drive" };

        // Bitcrusher params
        static const juce::String BitDepth { "bit_depth" };
        static const juce::String RateReduce { "rate_reduce" };

        // Flanger/chorus params
        // static const juce::String Offset { "offset" };
        // static const juce::String Depth { "depth" };
        // static const juce::String Rate { "rate" };
        // static const juce::String ModType { "mod_type" };
        const juce::String FlangerIntensity { "flangerIntensity" }; // New ID

        // Gain at the end of FX chain
        static const juce::String PostGain { "post-gain" };
        
        // Tremolo params

        static const juce::String TremoloRate { "tremolo_rate" };
        static const juce::String TremoloDepth { "tremolo_depth" };
        static const juce::String TremoloEnabled { "tremolo_enabled"};

        static const juce::String VinylNoise { "vinyl_noise" };

    }

    namespace Name
    {
        //static const juce::String Enabled { "Enabled" };

        static const juce::String Drive { "Drive" };
    
        static const juce::String BitDepth { "Bit Depth" };
        static const juce::String RateReduce { "Rate Reduce" };

        // static const juce::String Offset { "Offset" };
        // static const juce::String Depth { "Depth" };
        // static const juce::String Rate { "Rate" };
        // static const juce::String ModType { "Mod. Type" };
        const juce::String FlangerIntensity { "Flanger Mix" }; // Or "Flanger Amount", "Flanger Intensity"
        
        static const juce::String PostGain { "Post-Gain" };

        static const juce::String TremoloRate { "Tremolo Rate" };
        static const juce::String TremoloDepth { "Tremolo Depth" };
        static const juce::String TremoloEnabled { "Tremolo Enabled"};

        static const juce::String VinylNoise { "Vinyl Noise" };
    }

    namespace Ranges
    {
    // {
    //     static constexpr float OffsetMin { 0.f };
    //     static constexpr float OffsetMax { 10.f };
    //     static constexpr float OffsetInc { 0.01f };
    //     static constexpr float OffsetSkw { 0.5f };

    //     static constexpr float DepthMin { 0.f };
    //     static constexpr float DepthMax { 10.f };
    //     static constexpr float DepthInc { 0.01f };
    //     static constexpr float DepthSkw { 0.5f };

    //     static constexpr float RateMin { 0.1f };
    //     static constexpr float RateMax { 5.f };
    //     static constexpr float RateInc { 0.01f };
    //     static constexpr float RateSkw { 0.5f };

    //     static const juce::StringArray ModLabels { "Sine", "Triangle" };

        static const float FlangerIntensityMin = 0.0f;  // 0%
        static const float FlangerIntensityMax = 100.0f; // 100%
        static const float FlangerIntensityInc = 1.0f;
        static const float FlangerIntensitySkw = 1.0f; // Linear

        static const juce::String EnabledOff { "Off" };
        static const juce::String EnabledOn { "On" };

        static constexpr float TremoloRateMin = 0.1f;
        static constexpr float TremoloRateMax = 20.0f; // Hz
        static constexpr float TremoloRateInc = 0.01f;
        static constexpr float TremoloRateSkw = 0.5f; // Example skew

        static constexpr float TremoloDepthMin = 0.0f;
        static constexpr float TremoloDepthMax = 100.0f; // 0% to 100%
        static constexpr float TremoloDepthInc = 0.01f;
        static constexpr float TremoloDepthSkw = 1.0f; // Linear for depth

        static const inline juce::StringArray ModLabelsTremolo { "Sine", "Triangle", "Saw", "Square" }; // Example

        static constexpr float VinylNoiseMin = 0.0f;      // 0%
        static constexpr float VinylNoiseMax = 100.0f;    // 100%
        static constexpr float VinylNoiseInc = 1.0f;
        static constexpr float VinylNoiseSkw = 1.0f;      // Linear
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

    const float flangerMinOffsetMs;
    const float flangerMaxOffsetMs;
    const float flangerMinDepthMs;
    const float flangerMaxDepthMs;
    const float flangerMinRateHz;
    const float flangerMaxRateHz;

    juce::SmoothedValue<float> outputGain;

    bool enabled { true };
    juce::AudioBuffer<float> fxBuffer;

    juce::dsp::Oscillator<float> tremoloLFO;
    float tremoloRateHz { 1.0f };         // Current LFO rate
    float tremoloDepth { 0.0f };          // Current depth (0.0 to 1.0)
    bool tremoloEffectEnabled { false };  // Is the tremolo effect active?
    juce::AudioBuffer<float> tremoloLfoOutputBuffer; // To store LFO output for the current block
    
    juce::Random randomGenerator; // For generating noise
    juce::dsp::StateVariableFilter::Filter<float> noiseFilter; // For shaping the noise
    float vinylNoiseLevel { 0.0f }; // 0.0 to 1.0

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessor)
};