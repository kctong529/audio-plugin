#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

static const std::vector<mrta::ParameterInfo> Parameters
{
    
        //{ Param::ID::Enabled,  Param::Name::Enabled,   "Off", "On", true },
        //{ Param::ID::Enabled,     Param::Name::Enabled,  Param::Ranges::EnabledOff, Param::Ranges::EnabledOn, true },

        { Param::ID::Drive,            Param::Name::Drive,     "", 1.f, 1.f, 10.f, 0.1f, 1.f }, // Drive taken from MyFirstRealTimeAudioApp

        { Param::ID::BitDepth,         Param::Name::BitDepth,     "bits", 16.f, 1.f, 24.f, 1.f, 1.f }, // 1-24 bits, default 16.
        { Param::ID::RateReduce,       Param::Name::RateReduce,     "x", 1.f, 1.f, 64.f, 1.f, 2.f }, // 1x to 64x downsampling factor, default 1.

        // Flanger based on course Flanger project
        // { Param::ID::Offset,      Param::Name::Offset,   "ms",  2.f,  Param::Ranges::OffsetMin,   Param::Ranges::OffsetMax,   Param::Ranges::OffsetInc,   Param::Ranges::OffsetSkw },
        // { Param::ID::Depth,       Param::Name::Depth,    "ms",  2.f,  Param::Ranges::DepthMin,    Param::Ranges::DepthMax,    Param::Ranges::DepthInc,    Param::Ranges::DepthSkw },
        // { Param::ID::Rate,        Param::Name::Rate,     "Hz",  0.5f, Param::Ranges::RateMin,     Param::Ranges::RateMax,     Param::Ranges::RateInc,     Param::Ranges::RateSkw },
        // { Param::ID::ModType,     Param::Name::ModType,  Param::Ranges::ModLabels, 0 },
        { Param::ID::FlangerIntensity, Param::Name::FlangerIntensity, "%", 0.0f, Param::Ranges::FlangerIntensityMin, Param::Ranges::FlangerIntensityMax, Param::Ranges::FlangerIntensityInc, Param::Ranges::FlangerIntensitySkw },
    
        { Param::ID::PostGain,         Param::Name::PostGain,  "dB", 0.0f, -60.f, 12.f, 0.1f, 3.8018f },

        { Param::ID::TremoloEnabled,   Param::Name::TremoloEnabled, Param::Ranges::EnabledOff, Param::Ranges::EnabledOn, true },
        { Param::ID::TremoloRate,      Param::Name::TremoloRate, "Hz", 1.0f, Param::Ranges::TremoloRateMin, Param::Ranges::TremoloRateMax, Param::Ranges::TremoloRateInc, Param::Ranges::TremoloRateSkw },
        { Param::ID::TremoloDepth,     Param::Name::TremoloDepth, "%", 0.0f, Param::Ranges::TremoloDepthMin, Param::Ranges::TremoloDepthMax, Param::Ranges::TremoloDepthInc, Param::Ranges::TremoloDepthSkw },
        
        { Param::ID::VinylNoise,       Param::Name::VinylNoise, "%", 0.0f, Param::Ranges::VinylNoiseMin, Param::Ranges::VinylNoiseMax, Param::Ranges::VinylNoiseInc, Param::Ranges::VinylNoiseSkw }
};

MainProcessor::MainProcessor() :
    parameterManager(*this, ProjectInfo::projectName, Parameters),
    flanger(MaxDelaySizeMs, 2),
    enableRamp(0.05f),

    flangerMinOffsetMs(1.0f),       // e.g., minimum average delay for "intense" effect
    flangerMaxOffsetMs(7.0f),       // e.g., maximum average delay for "subtle" effect
    flangerMinDepthMs(0.1f),       // e.g., minimum sweep depth
    flangerMaxDepthMs(5.0f),       // e.g., maximum sweep depth (could be Param::Ranges::DepthMax)
    flangerMinRateHz(0.1f),        // e.g., minimum LFO rate
    flangerMaxRateHz(1.0f),         // e.g., a moderate maximum LFO rate

    tremoloEffectEnabled(true), // Default from Parameters vector
    tremoloRateHz(1.0f),        // Default from Parameters vector
    tremoloDepth(0.0f / 100.0f), // Default from Parameters (0.0%) scaled to 0.0-1.0

    vinylNoiseLevel(0.0f) // <<< NEW: Initialize vinylNoiseLevel

{
    // parameterManager.registerParameterCallback(Param::ID::Enabled,
    // [this](float newValue, bool force)
    // {
    //     enabled = newValue > 0.5f;
    //     enableRamp.setTarget(enabled ? 1.f : 0.f, force);
    // });

    parameterManager.registerParameterCallback(Param::ID::Drive,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Drive + ": " + juce::String { value });
        filter.setDrive(value);
    });

     parameterManager.registerParameterCallback(Param::ID::BitDepth,
    [this](float newValue, bool /*force*/)
    {
        DBG(Param::Name::BitDepth + ": " + juce::String{ newValue });
        bitCrusher.setBitDepth(newValue);
    });

    parameterManager.registerParameterCallback(Param::ID::RateReduce,
    [this](float newValue, bool /*force*/)
    {
        DBG(Param::Name::RateReduce + ": " + juce::String{ newValue });
        bitCrusher.setRateReduction(static_cast<int>(std::max(1.0f, newValue)));

    });

    // parameterManager.registerParameterCallback(Param::ID::Offset,
    // [this] (float newValue, bool /*force*/)
    // {
    //     flanger.setOffset(newValue);
    // });

    // parameterManager.registerParameterCallback(Param::ID::Depth,
    // [this](float newValue, bool /*force*/)
    // {
    //     flanger.setDepth(newValue);
    // });

    // parameterManager.registerParameterCallback(Param::ID::Rate,
    // [this] (float newValue, bool /*force*/)
    // {
    //     flanger.setModulationRate(newValue);
    // });

    // parameterManager.registerParameterCallback(Param::ID::ModType,
    // [this](float newValue, bool /*force*/)
    // {
    //     DSP::Flanger::ModulationType modType = static_cast<DSP::Flanger::ModulationType>(std::round(newValue));
    //     flanger.setModulationType(std::min(std::max(modType, DSP::Flanger::Sin), DSP::Flanger::Tri));
    // });
     parameterManager.registerParameterCallback(Param::ID::FlangerIntensity,
    [this](float newValue, bool /*force*/)
    {
        // newValue is typically 0-100 from the parameter definition. Convert to 0.0-1.0.
        float intensity = newValue / Param::Ranges::FlangerIntensityMax; // Or just / 100.0f if max is 100
        intensity = std::max(0.0f, std::min(intensity, 1.0f)); // Clamp

        // --- Mapping Logic ---
        // This is where you define how the intensity controls the flanger's character.
        // Example: As intensity increases:
        // - Depth increases from minDepth to maxDepth.
        // - Rate might increase from a slow rate to a moderate rate.
        // - Offset might decrease (shorter delay for more metallic flange at high intensity).

        // Depth: simple linear mapping
        // When intensity is 0, depth is 0 (or flangerMinDepthMs if you always want some).
        // When intensity is 1, depth is flangerMaxDepthMs.
        float currentDepth = intensity * flangerMaxDepthMs; 
        // If you want a minimum depth even at low intensity (but > 0), you could do:
        // float currentDepth = (intensity == 0.0f) ? 0.0f : juce::jmap(intensity, flangerMinDepthMs, flangerMaxDepthMs);


        // Rate: linear mapping
        float currentRate = juce::jmap(intensity, flangerMinRateHz, flangerMaxRateHz);
        // For very low intensity, you might want rate to be 0 or very slow.
        if (intensity < 0.01f) currentRate = 0.0f; // or flangerMinRateHz

        // Offset: inverse linear mapping (higher intensity = lower offset)
        // When intensity is 0, offset is flangerMaxOffsetMs (more subtle).
        // When intensity is 1, offset is flangerMinOffsetMs (more pronounced/metallic).
        float currentOffset = juce::jmap(intensity, flangerMaxOffsetMs, flangerMinOffsetMs);
        if (intensity < 0.01f) currentOffset = flangerMaxOffsetMs; // Ensure max offset at zero intensity

        flanger.setDepth(currentDepth);
        flanger.setModulationRate(currentRate);
        flanger.setOffset(currentOffset);

        // You can also decide on a fixed ModType or even change it based on intensity
        // For simplicity, let's keep it fixed, e.g., Sine.
        // This would have been set during flanger.prepare or an initial setup.
        // If not, you can set it here:
        // flanger.setModulationType(DSP::Flanger::Sin); // Or make it part of the mapping

        DBG(Param::Name::FlangerIntensity + ": " + juce::String(newValue) + "% -> "
            + "Offset: " + juce::String(currentOffset, 2) + "ms, "
            + "Depth: " + juce::String(currentDepth, 2) + "ms, "
            + "Rate: " + juce::String(currentRate, 2) + "Hz");

        // The Param::ID::Enabled for the flanger (if you still have one)
        // might now be controlled by whether intensity > 0
        // For example, you might want enableRamp to target 0 if intensity is 0
        // and 1 if intensity > 0.
        // This depends on how your 'enabled' flag for the flanger effect is used.
        // If 'enableRamp' is for the *entire* flanger effect block:
        bool flangerShouldBeOn = intensity > 0.001f; // Small threshold
        // And if 'enabled' member variable controls the flanger's bypass via enableRamp:
        // enabled = flangerShouldBeOn; // This might conflict if 'enabled' is global
        // It's better if 'enableRamp' is specifically for the flanger effect itself.
        // If your existing 'enableRamp' controls the flanger's wet signal:
        enableRamp.setTarget(flangerShouldBeOn ? 1.f : 0.f, false /*not forced here, let ramp do its job*/);
    });

    parameterManager.registerParameterCallback(Param::ID::PostGain,
    [this] (float value, bool forced)
    {
        DBG(Param::Name::PostGain + ": " + juce::String { value });
        float dbValue { 0.f };
        if (value > -60.f)
            dbValue = std::pow(10.f, value * 0.05f);

        if (forced)
            outputGain.setCurrentAndTargetValue(dbValue);
        else
            outputGain.setTargetValue(dbValue);
    });

    parameterManager.registerParameterCallback(Param::ID::TremoloEnabled,
    [this](float newValue, bool /*force*/)
    {
        tremoloEffectEnabled = newValue > 0.5f;
        DBG("Tremolo Enabled: " + juce::String(tremoloEffectEnabled ? "On" : "Off"));
        // Optional: reset LFO phase when enabled/disabled if desired
        // if (tremoloEffectEnabled && getSampleRate() > 0) tremoloLFO.reset();
    });

    parameterManager.registerParameterCallback(Param::ID::TremoloRate,
    [this] (float newValue, bool /*force*/)
    {
        tremoloRateHz = newValue;
        if (getSampleRate() > 0) // Ensure LFO is prepared
        {
            tremoloLFO.setFrequency(tremoloRateHz); // Smooth frequency update
        }
        DBG("Tremolo Rate: " + juce::String(tremoloRateHz) + " Hz");
    });

    parameterManager.registerParameterCallback(Param::ID::TremoloDepth,
    [this](float newValue, bool /*force*/)
    {
        // Assuming the parameter's range is 0-100 for percentage
        // If Param::Ranges::TremoloDepthMax is 1.0, remove / 100.0f
        tremoloDepth = newValue / 100.0f; 
        // Clamp to ensure it's within 0.0 to 1.0 after division, just in case.
        tremoloDepth = std::max(0.0f, std::min(tremoloDepth, 1.0f)); 
        DBG("Tremolo Depth: " + juce::String(newValue) + "% (Internal: " + juce::String(tremoloDepth) + ")");
    });

    parameterManager.registerParameterCallback(Param::ID::VinylNoise,
    [this](float newValue, bool /*force*/)
    {
        vinylNoiseLevel = newValue / Param::Ranges::VinylNoiseMax;
        vinylNoiseLevel = std::max(0.f, std::min(vinylNoiseLevel, 1.f));
        DBG("Vinyl Noise: " + juce::String(newValue) + "% (Internal: " + juce::String(vinylNoiseLevel) + ")");
    });
}

MainProcessor::~MainProcessor()
{
}

void MainProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
    const unsigned int numChannels { static_cast<unsigned int>(std::max(getMainBusNumInputChannels(), getMainBusNumOutputChannels())) };

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = newSampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = numChannels;

    
    filter.prepare(spec);
    filter.setMode(juce::dsp::LadderFilterMode::LPF24); // Or LPF12. LPF is often default.
                                                        // Using LPF24 for a bit more character if desired.
    filter.setCutoffFrequencyHz(20000.0f); // Set cutoff very high to minimize filtering.
                                           // Or more robustly: (float)(newSampleRate / 2.0 * 0.99)
                                           // to ensure it's just below Nyquist for any sample rate.
    filter.setResonance(0.0f);             // Set resonance to minimum to avoid peaking.
                                           // Some filters might prefer a tiny non-zero value like 0.1f
                                           // but 0.0f is usually fine for no resonance with Ladder.
    // The drive will be set by the parameter callback when updateParameters(true) is called below.
    // Or you could set a default initial drive here: filter.setDrive(1.0f);

    bitCrusher.prepare(newSampleRate, samplesPerBlock); // Call your custom class's method

    flanger.prepare(newSampleRate, MaxDelaySizeMs, numChannels);
    enableRamp.prepare(newSampleRate, true, enabled ? 1.f : 0.f);
    //filter.prepare({ newSampleRate, static_cast<juce::uint32>(samplesPerBlock), numChannels });
    //flanger.prepare(newSampleRate, MaxDelaySizeMs, numChannels);
    //enableRamp.prepare(newSampleRate, true, enabled ? 1.f : 0.f);

    outputGain.reset(newSampleRate, 0.01f);

    juce::dsp::ProcessSpec lfoSpec; // LFO is mono, but prepare with overall spec
    lfoSpec.sampleRate = newSampleRate;
    lfoSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    lfoSpec.numChannels = 1; // LFO itself processes mono
    
    tremoloLFO.prepare(lfoSpec);
    tremoloLFO.initialise([](float x){ return std::sin(x); }); // Sine wave LFO
    // Set initial frequency, force phase reset for consistent start
    tremoloLFO.setFrequency(tremoloRateHz, true); 

    // Prepare LFO output buffer
    tremoloLfoOutputBuffer.setSize(1, samplesPerBlock, false, true, false);
    // (numChannels=1, numSamples=samplesPerBlock, keepExistingContent=false, clearExtraSpace=true, avoidReallocating=false)

    juce::dsp::ProcessSpec noiseFilterSpec;
    noiseFilterSpec.sampleRate = newSampleRate;
    noiseFilterSpec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    noiseFilterSpec.numChannels = 1;

    noiseFilter.prepare(noiseFilterSpec);
    noiseFilter.reset();

    noiseFilter.parameters->type = juce::dsp::StateVariableFilter::Parameters<float>::Type::lowPass;
    noiseFilter.parameters->setCutOffFrequency(newSampleRate, 3000.0f, static_cast<float>(1.0 / std::sqrt(2.0))); // Cutoff 3kHz, Q ~0.707

    parameterManager.updateParameters(true);

    fxBuffer.setSize(static_cast<int>(numChannels), samplesPerBlock);
    fxBuffer.clear();
}

void MainProcessor::releaseResources()
{
    filter.reset();
    bitCrusher.reset();
    flanger.clear();
}

void MainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    parameterManager.updateParameters();

    const unsigned int numChannels { static_cast<unsigned int>(buffer.getNumChannels()) };
    const unsigned int numSamples { static_cast<unsigned int>(buffer.getNumSamples()) };

    // Drive (via filter)
    {
        juce::dsp::AudioBlock<float> audioBlock(buffer);
        //juce::dsp::AudioBlock<float> audioBlock(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
        juce::dsp::ProcessContextReplacing<float> ctx(audioBlock);
        filter.process(ctx);
    }

    // Bitcrushing
    for(int ch = 0; ch < static_cast<int>(numChannels); ++ch){
        bitCrusher.reset();
        auto* channelData = buffer.getWritePointer(ch);
        bitCrusher.processBlock(channelData, numSamples);
    }

    // Flanger
    for (int ch = 0; ch < static_cast<int>(numChannels); ++ch)
        fxBuffer.copyFrom(ch, 0, buffer, ch, 0, static_cast<int>(numSamples));

    flanger.process(fxBuffer.getArrayOfWritePointers(), fxBuffer.getArrayOfReadPointers(), numChannels, numSamples);
    enableRamp.applyGain(fxBuffer.getArrayOfWritePointers(), numChannels, numSamples);

    for (int ch = 0; ch < static_cast<int>(numChannels); ++ch)
        buffer.addFrom(ch, 0, fxBuffer, ch, 0, static_cast<int>(numSamples));
    
    // Apply if enabled and depth is significant
    if (tremoloEffectEnabled && tremoloDepth > 0.001f) 
    {
        // Ensure LFO output buffer is correctly sized (should be by prepareToPlay)
        // but good to assert or check if block size can change dynamically.
        jassert(tremoloLfoOutputBuffer.getNumSamples() >= numSamples);
        jassert(tremoloLfoOutputBuffer.getNumChannels() == 1);

        auto* lfoSamples = tremoloLfoOutputBuffer.getWritePointer(0);

        // Generate LFO samples for the block (mono LFO)
        for (int i = 0; i < numSamples; ++i)
        {
            float lfoSampleRaw = tremoloLFO.processSample(0.0f); // LFO outputs ~ -1 to 1
            lfoSamples[i] = (lfoSampleRaw + 1.0f) * 0.5f;      // Scale to 0 to 1
        }

        // Apply tremolo gain to each processing channel
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* channelData = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                // Gain: 1.0 when LFO is at bottom (lfoSamples[i]=0), (1.0 - depth) when LFO is at top (lfoSamples[i]=1)
                float gain = 1.0f - (tremoloDepth * lfoSamples[i]);
                channelData[i] *= gain;
            }
        }
    }

    // Vinyl noise
    if(vinylNoiseLevel > 0.001f){
        for(int sample = 0; sample < numSamples; ++sample){
            float noiseSample = randomGenerator.nextFloat()*2.f - 1.f;
            noiseSample = noiseFilter.processSample(noiseSample);
            noiseSample *= vinylNoiseLevel;
            noiseSample *= 0.1f;

            for(int ch = 0; ch < numChannels; ++ch){
                buffer.addSample(ch, sample, noiseSample);
            }
        #if JUCE_DSP_ENABLE_SNAP_TO_ZERO // As seen in the filter's processBlock
        noiseFilter.snapToZero();       // Good practice if processing sample-by-sample
       #endif
        }

    // Output gain
    outputGain.applyGain(buffer, buffer.getNumSamples());

    // Tremolo
    
    }
}

void MainProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    parameterManager.getStateInformation(destData);
}

void MainProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    parameterManager.setStateInformation(data, sizeInBytes);
}

//==============================================================================
bool MainProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* MainProcessor::createEditor() { return new MainProcessorEditor(*this); }
const juce::String MainProcessor::getName() const { return JucePlugin_Name; }
bool MainProcessor::acceptsMidi() const { return false; }
bool MainProcessor::producesMidi() const { return false; }
bool MainProcessor::isMidiEffect() const { return false; }
double MainProcessor::getTailLengthSeconds() const { return 0.0; }
int MainProcessor::getNumPrograms() { return 1; }
int MainProcessor::getCurrentProgram() { return 0; }
void MainProcessor::setCurrentProgram(int) { }
const juce::String MainProcessor::getProgramName (int) { return {}; }
void MainProcessor::changeProgramName (int, const juce::String&) { }
//==============================================================================

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MainProcessor();
}