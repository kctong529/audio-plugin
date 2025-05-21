#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>

void modeMix(float mode, float& lpf, float& bpf, float& hpf)
{
    mode = std::clamp(mode, -1.f, 1.f);
    lpf = std::fmax(-mode, 0.f);
    bpf = std::fmax(1.f - std::fabs(mode), 0.f);
    hpf = std::fmax(mode, 0.f);
}

static const std::vector<mrta::ParameterInfo> parameters
{
    { Param::ID::OscType, Param::Name::OscType, Param::Range::OscTypeLabels, 0 },
    { Param::ID::Volume,  Param::Name::Volume,  Param::Unit::dB, -12.f, Param::Range::VolumeMin,  Param::Range::VolumeMax,  Param::Range::VolumeInc,  Param::Range::VolumeSkw },

    { Param::ID::Freq,        Param::Name::Freq,      Param::Unit::Hz, 500.0f,  Param::Range::FreqMin,        Param::Range::FreqMax,        Param::Range::FreqInc,        Param::Range::FreqSkw },
    { Param::ID::FreqModAmt,  Param::ID::FreqModAmt,  "",                0.25f, Param::Range::FreqModAmtMin,  Param::Range::FreqModAmtMax,  Param::Range::FreqModAmtInc,  Param::Range::FreqModAmtSkw },
    { Param::ID::FreqModRate, Param::ID::FreqModRate, Param::Unit::Hz,   0.5f,  Param::Range::FreqModRateMin, Param::Range::FreqModRateMax, Param::Range::FreqModRateInc, Param::Range::FreqModRateSkw },
    { Param::ID::Reso,        Param::Name::Reso,      "",                1.0f,  Param::Range::ResoMin,        Param::Range::ResoMax,        Param::Range::ResoInc,        Param::Range::ResoSkw },
    { Param::ID::Mode,        Param::Name::Mode,      "",                0.0f,  Param::Range::ModeMin,        Param::Range::ModeMax,        Param::Range::ModeInc,        Param::Range::ModeSkw },
};

MainProcessor::MainProcessor() :
    parameterManager(*this, ProjectInfo::projectName, parameters),
    freqRamp(0.005f),
    freqModAmtRamp(0.005f)
{
    parameterManager.registerParameterCallback(Param::ID::OscType,
    [this] (float value, bool /*force*/)
    {
        DSP::Oscillator::OscType type = static_cast<DSP::Oscillator::OscType>(std::rint(value));
        oscLeft.setType(type);
        oscRight.setType(type);
    });

    parameterManager.registerParameterCallback(Param::ID::Volume,
    [this] (float value, bool force)
    {
        volumeDb = value;
        float volumeLin = std::pow(10.f, 0.05f * volumeDb);
        volumeRamp.setTarget(volumeLin, force);
    });

    parameterManager.registerParameterCallback(Param::ID::FreqModAmt,
    [this] (float value, bool force)
    {
        freqModAmt = std::clamp(value, 0.f, 1.f);
        freqModAmtRamp.setTarget(freqModAmt, force);
    });

    parameterManager.registerParameterCallback(Param::ID::FreqModRate,
    [this] (float value, bool force)
    {
        lfo.setFrequency(value);
    });

    parameterManager.registerParameterCallback(Param::ID::Reso,
    [this] (float value, bool force)
    {
        reso = value;
        resoRamp.setTarget(value, force);
    });

    parameterManager.registerParameterCallback(Param::ID::Mode,
    [this] (float value, bool force)
    {
        mode = value;
        float lpf(0.f), bpf(0.f), hpf(0.f);
        modeMix(mode, lpf, bpf, hpf);
        lpfRamp.setTarget(lpf, force);
        bpfRamp.setTarget(bpf, force);
        hpfRamp.setTarget(hpf, force);
    });

    lfo.setType(DSP::Oscillator::Sin);
}

MainProcessor::~MainProcessor()
{
}

void MainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    parameterManager.updateParameters(true);
    oscLeft.prepare(sampleRate);
    oscRight.prepare(sampleRate);
    volumeRamp.prepare(sampleRate, true, std::pow(10.f, 0.05f * volumeDb));

    svfLeft.prepare(sampleRate);
    svfRight.prepare(sampleRate);
    lfo.prepare(sampleRate);

    freqRamp.prepare(sampleRate, true, freqHz);
    freqModAmtRamp.prepare(sampleRate, true, freqModAmt);
    resoRamp.prepare(sampleRate, true, reso);

    float lpf, bpf, hpf;
    modeMix(mode, lpf, bpf, hpf);
    lpfRamp.prepare(sampleRate, true, lpf);
    bpfRamp.prepare(sampleRate, true, bpf);
    hpfRamp.prepare(sampleRate, true, hpf);

    // resize the aux buffers
    freqInBuffer.setSize(1, samplesPerBlock);
    freqModAmtBuffer.setSize(1, samplesPerBlock);
    lfoBuffer.setSize(1, samplesPerBlock);
    resoInBuffer.setSize(1, samplesPerBlock);
    lpfOutBuffer.setSize(2, samplesPerBlock);
    bpfOutBuffer.setSize(2, samplesPerBlock);
    hpfOutBuffer.setSize(2, samplesPerBlock);
}

void MainProcessor::releaseResources()
{
}

void MainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    parameterManager.updateParameters();

    // clear all aux buffers
    freqInBuffer.clear();
    freqModAmtBuffer.clear();
    lfoBuffer.clear();
    resoInBuffer.clear();
    lpfOutBuffer.clear();
    bpfOutBuffer.clear();
    hpfOutBuffer.clear();

    const unsigned int numChannels{ static_cast<unsigned int>(buffer.getNumChannels()) };
    const unsigned int numSamples{ static_cast<unsigned int>(buffer.getNumSamples()) };

    // Process MIDI messages to update oscillator frequency
    oscLeft.renderNextBlock(buffer, midiMessages, 0, numSamples);
    oscRight.renderNextBlock(buffer, midiMessages, 0, numSamples);

    oscLeft.process(buffer.getWritePointer(0), numSamples);
    if (numChannels == 2)
        oscRight.process(buffer.getWritePointer(1), numSamples);

    volumeRamp.applyGain(buffer.getArrayOfWritePointers(), numChannels, numSamples);
    
    // // Fill freqInBuffer with currentFreq for all samples
    // for (unsigned int n = 0; n < numSamples; ++n)
    // {
    //     freqInBuffer.setSample(0, n, (float)currentFreq);
    // }

    // // get the freq and reso controls
    // freqRamp.applySum(freqInBuffer.getWritePointer(0), numSamples);
    // resoRamp.applySum(resoInBuffer.getWritePointer(0), numSamples);
    // freqModAmtRamp.applySum(freqModAmtBuffer.getWritePointer(0), numSamples);

    // // calculate LFO in Hz
    // lfo.process(lfoBuffer.getWritePointer(0), numSamples);
    // for (unsigned int n = 0; n < numSamples; ++n)
    // {
    //     const float curFreq = freqInBuffer.getSample(0, n);
    //     const float modAmtHz = curFreq * FreqModAmtMax * freqModAmtBuffer.getSample(0, n);
    //     const float modFreq = modAmtHz * lfoBuffer.getSample(0, n) + curFreq;
    //     freqInBuffer.setSample(0, n, modFreq);
    // }

    // // always process left channel
    // svfLeft.process(lpfOutBuffer.getWritePointer(0),
    //                 bpfOutBuffer.getWritePointer(0),
    //                 hpfOutBuffer.getWritePointer(0),
    //                 buffer.getReadPointer(0),
    //                 freqInBuffer.getReadPointer(0),
    //                 resoInBuffer.getReadPointer(0),
    //                 numSamples);

    // // if stereo, also process right channel
    // if (numChannels > 1)
    // {
    //     svfRight.process(lpfOutBuffer.getWritePointer(1),
    //                      bpfOutBuffer.getWritePointer(1),
    //                      hpfOutBuffer.getWritePointer(1),
    //                      buffer.getReadPointer(1),
    //                      freqInBuffer.getReadPointer(0),
    //                      resoInBuffer.getReadPointer(0),
    //                      numSamples);
    // }

    // // mix outputs
    // lpfRamp.applyGain(lpfOutBuffer.getArrayOfWritePointers(), numChannels, numSamples);
    // bpfRamp.applyGain(bpfOutBuffer.getArrayOfWritePointers(), numChannels, numSamples);
    // hpfRamp.applyGain(hpfOutBuffer.getArrayOfWritePointers(), numChannels, numSamples);
    // buffer.clear();
    // for (int ch = 0; ch < numChannels; ++ch)
    // {
    //     buffer.addFrom(ch, 0, lpfOutBuffer, ch, 0, numSamples);
    //     buffer.addFrom(ch, 0, bpfOutBuffer, ch, 0, numSamples);
    //     buffer.addFrom(ch, 0, hpfOutBuffer, ch, 0, numSamples);
    // }
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
const juce::String MainProcessor::getName() const { return JucePlugin_Name; }
bool MainProcessor::acceptsMidi() const { return true; }
bool MainProcessor::producesMidi() const { return false; }
bool MainProcessor::isMidiEffect() const { return false; }
double MainProcessor::getTailLengthSeconds() const { return 0.0; }
int MainProcessor::getNumPrograms() { return 1; }
int MainProcessor::getCurrentProgram() { return 0; }
void MainProcessor::setCurrentProgram(int) { }
const juce::String MainProcessor::getProgramName(int) { return {}; }
void MainProcessor::changeProgramName(int, const juce::String&) { }
bool MainProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* MainProcessor::createEditor() { return new MainProcessorEditor(*this); }
//==============================================================================

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MainProcessor();
}