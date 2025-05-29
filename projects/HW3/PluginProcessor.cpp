#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "JuceHeader.h"
#include "Oscillator.h"
#include "PluginEditor.h"
#include "SynthVoice.h"
#include "EnvelopeGenerator.h"

static const std::vector<mrta::ParameterInfo> ParameterInfos
{
    // Oscillator parameters
    { Param::ID::OscType,       Param::Name::OscType,       { "Sine", "Triangle", "Saw", "Triangle AA", "Saw AA" }, 0 },
    { Param::ID::OscGain,       Param::Name::OscGain,       "dB", 0.0f, -60.f, 12.f, 0.1f, 3.8018f },
    
    // Filter parameters
    { Param::ID::FilterEnabled, Param::Name::FilterEnabled, "Off", "On", true },
    { Param::ID::Cutoff,        Param::Name::Cutoff,        "Hz", 1000.f, 20.f, 20000.f, 1.f, 0.3f },
    { Param::ID::Resonance,     Param::Name::Resonance,     "", 0.5f, 0.1f, 10.f, 0.01f, 1.f },
    { Param::ID::FilterType,    Param::Name::FilterType,    { "LPF", "BPF", "HPF" }, 0 },
    
    // ADSR parameters
    { Param::ID::Attack,        Param::Name::Attack,        "ms", 10.f, 0.1f, 5000.f, 0.1f, 0.3f },
    { Param::ID::Decay,         Param::Name::Decay,         "ms", 100.f, 0.1f, 5000.f, 0.1f, 0.3f },
    { Param::ID::Sustain,       Param::Name::Sustain,       "", 0.7f, 0.0f, 1.0f, 0.01f, 1.0f },
    { Param::ID::Release,       Param::Name::Release,       "ms", 500.f, 0.1f, 5000.f, 0.1f, 0.3f },
    { Param::ID::EnvAnalog,     Param::Name::EnvAnalog,     "Digital", "Analog", false },
    
    // Master gain
    { Param::ID::MasterGain,    Param::Name::MasterGain,    "dB", 0.0f, -60.f, 12.f, 0.1f, 3.8018f },
};

MainProcessor::MainProcessor() :
    parameterManager(*this, ProjectInfo::projectName, ParameterInfos),
    lastMidiNoteFreq(440.0f),
    noteActive(false),
    currentNoteNumber(-1)
{
    // Set up oscillator
    parameterManager.registerParameterCallback(Param::ID::OscType,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::OscType + ": " + juce::String { value });
        osc.setType(static_cast<DSP::Oscillator::OscType>(std::floor(value)));
    });
    
    parameterManager.registerParameterCallback(Param::ID::OscGain,
    [this] (float value, bool forced)
    {
        DBG(Param::Name::OscGain + ": " + juce::String { value });
        float dbValue { 0.f };
        if (value > -60.f)
            dbValue = std::pow(10.f, value * 0.05f);

        if (forced)
            oscGain.setCurrentAndTargetValue(dbValue);
        else
            oscGain.setTargetValue(dbValue);
    });

    // Set up filter
    parameterManager.registerParameterCallback(Param::ID::FilterEnabled,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::FilterEnabled + ": " + juce::String { value });
        filterEnabled = (value > 0.5f);
    });

    parameterManager.registerParameterCallback(Param::ID::Cutoff,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Cutoff + ": " + juce::String { value });
        filterCutoff = value;
    });

    parameterManager.registerParameterCallback(Param::ID::Resonance,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Resonance + ": " + juce::String { value });
        filterResonance = value;
    });

    parameterManager.registerParameterCallback(Param::ID::FilterType,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::FilterType + ": " + juce::String { value });
        filterType = static_cast<int>(std::floor(value));
    });

    // Set up ADSR envelope
    parameterManager.registerParameterCallback(Param::ID::Attack,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Attack + ": " + juce::String { value });
        env.setAttackTime(value);
    });

    parameterManager.registerParameterCallback(Param::ID::Decay,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Decay + ": " + juce::String { value });
        env.setDecayTime(value);
    });

    parameterManager.registerParameterCallback(Param::ID::Sustain,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Sustain + ": " + juce::String { value });
        env.setSustainLevel(value);
    });

    parameterManager.registerParameterCallback(Param::ID::Release,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Release + ": " + juce::String { value });
        env.setReleaseTime(value);
    });

    parameterManager.registerParameterCallback(Param::ID::EnvAnalog,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::EnvAnalog + ": " + juce::String { value });
        env.setAnalogStyle(value > 0.5f);
    });

    // Set up master gain
    parameterManager.registerParameterCallback(Param::ID::MasterGain,
    [this] (float value, bool forced)
    {
        DBG(Param::Name::MasterGain + ": " + juce::String { value });
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

void MainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    //juce::uint32 numChannels { static_cast<juce::uint32>(std::max(getMainBusNumInputChannels(), getMainBusNumOutputChannels())) };
    juce::uint32 numChannels = std::max(1u, static_cast<juce::uint32>(
        std::max(getMainBusNumInputChannels(), getMainBusNumOutputChannels())));

    // Initialize the oscillator
    osc.prepare(sampleRate);
    
    // Initialize the filter
    filter.prepare(sampleRate);
    
    // Initialize the envelope generator
    env.prepare(sampleRate);
    
    // Initialize smoothed gain values
    oscGain.reset(sampleRate, 0.01f);
    outputGain.reset(sampleRate, 0.01f);
    
    // Prepare audio buffers
    oscBuffer.setSize(numChannels, samplesPerBlock);
    envBuffer.setSize(1, samplesPerBlock);
    filterL.setSize(1, samplesPerBlock);
    filterB.setSize(1, samplesPerBlock);
    filterH.setSize(1, samplesPerBlock);
    
    // Prepare vector buffers
    filterFreqBuffer.resize(samplesPerBlock);
    filterResoBuffer.resize(samplesPerBlock);
    envelopeBuffer.resize(samplesPerBlock);
    
    // Default filter parameters
    filterEnabled = true;
    filterCutoff = 1000.0f;
    filterResonance = 0.5f;
    filterType = 0; // LPF
    
    // Update parameters from the parameter manager
    parameterManager.updateParameters(true);
}

void MainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    parameterManager.updateParameters();
    
    // Clear the output buffer
    buffer.clear();
    oscBuffer.clear();
    
    // Process MIDI messages
    processMidiMessages(midiMessages);
    
    // Get number of samples for this block
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    // Initialize filter frequency and resonance buffers
    for (int i = 0; i < numSamples; ++i)
    {
        filterFreqBuffer[i] = filterCutoff;
        filterResoBuffer[i] = filterResonance;
    }
    
    // Process the envelope
    env.process(envelopeBuffer.data(), numSamples);
    
    // Generate oscillator output
    float* oscData = oscBuffer.getWritePointer(0);
    for (int i = 0; i < numSamples; ++i)
    {
        oscData[i] = osc.process();
    }
    
    // Apply oscillator gain
    for (int channel = 0; channel < oscBuffer.getNumChannels(); ++channel)
    {
        if (channel < oscBuffer.getNumChannels())
        {
            float* channelData = oscBuffer.getWritePointer(channel);
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    channelData[sample] *= oscGain.getNextValue();
                }
        }
     
    }
    
    // Apply filter if enabled
    if (filterEnabled)
    {
        float* lpfData = filterL.getWritePointer(0);
        float* bpfData = filterB.getWritePointer(0);
        float* hpfData = filterH.getWritePointer(0);
        
        // Process the filter
        filter.process(lpfData, bpfData, hpfData, oscBuffer.getReadPointer(0), 
                      filterFreqBuffer.data(), filterResoBuffer.data(), numSamples);
        
        // Copy filtered data back based on filter type
        float* filteredSource;
        switch (filterType)
        {
            case 0: // LPF
                filteredSource = lpfData;
                break;
            case 1: // BPF
                filteredSource = bpfData;
                break;
            case 2: // HPF
                filteredSource = hpfData;
                break;
            default:
                filteredSource = lpfData;
                break;
        }
        
        // Copy filtered data back to oscillator buffer
        for (int channel = 0; channel < oscBuffer.getNumChannels(); ++channel)
        {
            float* channelData = oscBuffer.getWritePointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                channelData[sample] = filteredSource[sample];
            }
        }
    }
    
    // Apply envelope and master gain, write to output buffer
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* outData = buffer.getWritePointer(channel);
        const float* sourceData = oscBuffer.getReadPointer(std::min(channel, oscBuffer.getNumChannels() - 1));
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Apply envelope
            float envValue = envelopeBuffer[sample];
            
            // Apply master gain
            float gainValue = outputGain.getNextValue();
            
            // Write to output
            outData[sample] = sourceData[sample] * envValue * gainValue;
        }
    }
}

void MainProcessor::processMidiMessages(juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const auto time = metadata.samplePosition;
        
        if (message.isNoteOn())
        {
            // Get MIDI note number and convert to frequency
            const int noteNumber = message.getNoteNumber();
            lastMidiNoteFreq = juce::MidiMessage::getMidiNoteInHertz(noteNumber);
            
            // Set oscillator frequency
            osc.setFrequency(lastMidiNoteFreq);
            
            // Start envelope
            env.start();
            
            // Store current note information
            currentNoteNumber = noteNumber;
            noteActive = true;
        }
        else if (message.isNoteOff())
        {
            const int noteNumber = message.getNoteNumber();
            
            // Only react to the currently playing note
            if (noteNumber == currentNoteNumber)
            {
                // End envelope
                env.end();
                noteActive = false;
                currentNoteNumber = -1;
            }
        }
        else if (message.isAllNotesOff())
        {
            // End envelope
            env.end();
            noteActive = false;
            currentNoteNumber = -1;
        }
    }
}

void MainProcessor::releaseResources()
{
    //filter.reset();
}

void MainProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    parameterManager.getStateInformation(destData);
}

void MainProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    parameterManager.setStateInformation(data, sizeInBytes);
}

juce::AudioProcessorEditor* MainProcessor::createEditor()
{
    return new MainProcessorEditor(*this);
}

//==============================================================================
const juce::String MainProcessor::getName() const { return JucePlugin_Name; }
bool MainProcessor::isSynth() const { return true ; }
bool MainProcessor::acceptsMidi() const { return true; }
bool MainProcessor::producesMidi() const { return false; }
bool MainProcessor::isMidiEffect() const { return false; }
double MainProcessor::getTailLengthSeconds() const { return 0.0; }
int MainProcessor::getNumPrograms() { return 1; }
int MainProcessor::getCurrentProgram() { return 0; }
void MainProcessor::setCurrentProgram (int) { }
const juce::String MainProcessor::getProgramName(int) { return {}; }
void MainProcessor::changeProgramName(int, const juce::String&) { }
bool MainProcessor::hasEditor() const { return true; }
//==============================================================================

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MainProcessor();
}
