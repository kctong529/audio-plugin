#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

static const std::vector<mrta::ParameterInfo> Parameters
{
    
        //{ Param::ID::Enabled,  Param::Name::Enabled,   "Off", "On", true },
        { Param::ID::Enabled,  Param::Name::Enabled,  Param::Ranges::EnabledOff, Param::Ranges::EnabledOn, true },

        { Param::ID::Drive,    Param::Name::Drive,     "", 1.f, 1.f, 10.f, 0.1f, 1.f },
    
        { Param::ID::Offset,   Param::Name::Offset,   Param::Units::Ms,  2.f,  Param::Ranges::OffsetMin,   Param::Ranges::OffsetMax,   Param::Ranges::OffsetInc,   Param::Ranges::OffsetSkw },
        { Param::ID::Depth,    Param::Name::Depth,    Param::Units::Ms,  2.f,  Param::Ranges::DepthMin,    Param::Ranges::DepthMax,    Param::Ranges::DepthInc,    Param::Ranges::DepthSkw },
        { Param::ID::Rate,     Param::Name::Rate,     Param::Units::Hz,  0.5f, Param::Ranges::RateMin,     Param::Ranges::RateMax,     Param::Ranges::RateInc,     Param::Ranges::RateSkw },
        { Param::ID::ModType,  Param::Name::ModType,  Param::Ranges::ModLabels, 0 },
    
        { Param::ID::PostGain,  Param::Name::PostGain,  "dB", 0.0f, -60.f, 12.f, 0.1f, 3.8018f },
    
};

MainProcessor::MainProcessor() :
    parameterManager(*this, ProjectInfo::projectName, Parameters),
    flanger(MaxDelaySizeMs, 2),
    enableRamp(0.05f)
{
    parameterManager.registerParameterCallback(Param::ID::Enabled,
    [this](float newValue, bool force)
    {
        enabled = newValue > 0.5f;
        enableRamp.setTarget(enabled ? 1.f : 0.f, force);
    });

    parameterManager.registerParameterCallback(Param::ID::Drive,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Drive + ": " + juce::String { value });
        filter.setDrive(value);
    });

    parameterManager.registerParameterCallback(Param::ID::Offset,
    [this] (float newValue, bool /*force*/)
    {
        flanger.setOffset(newValue);
    });

    parameterManager.registerParameterCallback(Param::ID::Depth,
    [this](float newValue, bool /*force*/)
    {
        flanger.setDepth(newValue);
    });

    parameterManager.registerParameterCallback(Param::ID::Rate,
    [this] (float newValue, bool /*force*/)
    {
        flanger.setModulationRate(newValue);
    });

    parameterManager.registerParameterCallback(Param::ID::ModType,
    [this](float newValue, bool /*force*/)
    {
        DSP::Flanger::ModulationType modType = static_cast<DSP::Flanger::ModulationType>(std::round(newValue));
        flanger.setModulationType(std::min(std::max(modType, DSP::Flanger::Sin), DSP::Flanger::Tri));
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

    flanger.prepare(newSampleRate, MaxDelaySizeMs, numChannels);
    enableRamp.prepare(newSampleRate, true, enabled ? 1.f : 0.f);
    //filter.prepare({ newSampleRate, static_cast<juce::uint32>(samplesPerBlock), numChannels });
    //flanger.prepare(newSampleRate, MaxDelaySizeMs, numChannels);
    //enableRamp.prepare(newSampleRate, true, enabled ? 1.f : 0.f);

    outputGain.reset(newSampleRate, 0.01f);

    parameterManager.updateParameters(true);

    fxBuffer.setSize(static_cast<int>(numChannels), samplesPerBlock);
    fxBuffer.clear();
}

void MainProcessor::releaseResources()
{
    filter.reset();
    flanger.clear();
}

void MainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    parameterManager.updateParameters();

    const unsigned int numChannels { static_cast<unsigned int>(buffer.getNumChannels()) };
    const unsigned int numSamples { static_cast<unsigned int>(buffer.getNumSamples()) };

    {
        juce::dsp::AudioBlock<float> audioBlock(buffer);
        //juce::dsp::AudioBlock<float> audioBlock(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
        juce::dsp::ProcessContextReplacing<float> ctx(audioBlock);
        filter.process(ctx);
    }

    for (int ch = 0; ch < static_cast<int>(numChannels); ++ch)
        fxBuffer.copyFrom(ch, 0, buffer, ch, 0, static_cast<int>(numSamples));

    flanger.process(fxBuffer.getArrayOfWritePointers(), fxBuffer.getArrayOfReadPointers(), numChannels, numSamples);
    enableRamp.applyGain(fxBuffer.getArrayOfWritePointers(), numChannels, numSamples);

    for (int ch = 0; ch < static_cast<int>(numChannels); ++ch)
        buffer.addFrom(ch, 0, fxBuffer, ch, 0, static_cast<int>(numSamples));
    
    outputGain.applyGain(buffer, buffer.getNumSamples());

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