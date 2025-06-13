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

        // Pitch wobble params
        static const juce::String PitchWobbleIntensity { "pitch_wobble_intensity" }; // <<< NEW ID


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

        static const juce::String PitchWobbleIntensity { "Wobble" }; // <<< NEW Name (or "Pitch Wobble")
    
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
        static constexpr float PitchWobbleIntensityMin   = 0.0f;   // 0%
        static constexpr float PitchWobbleIntensityMax   = 100.0f; // 100%
        static constexpr float PitchWobbleIntensityInc   = 1.0f;
        static constexpr float PitchWobbleIntensitySkw   = 1.0f;   // Linear

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

    juce::dsp::Oscillator<float> pitchWobbleLFO;
    std::vector<juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>> pitchWobbleDelayLines;
    float currentPitchWobbleIntensity { 0.0f }; // Internal state, 0.0 to 1.0

    static constexpr float PITCH_WOBBLE_CENTRAL_DELAY_MS { 15.0f }; // Base delay around which to modulate
    static constexpr float PITCH_WOBBLE_MAX_MOD_DEPTH_MS { 3.0f };  // Max delay swing (in ms) from central point at 100% intensity
    static constexpr float PITCH_WOBBLE_MIN_RATE_HZ      { 0.25f }; // LFO rate when intensity is just above 0
    static constexpr float PITCH_WOBBLE_MAX_RATE_HZ      { 1.8f };  // LFO rate at 100% intensity


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessor)
};