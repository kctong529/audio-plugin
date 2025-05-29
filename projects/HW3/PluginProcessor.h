#pragma once

#include <JuceHeader.h>
#include "Oscillator.h"
#include "StateVariableFilter.h"
#include "EnvelopeGenerator.h"
namespace Param
{
    namespace ID
    {
        static const juce::String Cutoff { "cutoff" };
        static const juce::String Resonance { "resonance" };
        static const juce::String Attack { "attack" };
        static const juce::String Decay { "decay" };
        static const juce::String Sustain { "sustain" };
        static const juce::String Release { "release" };

        static const juce::String OscType { "osc_type" };
        static const juce::String OscGain { "osc_gain" };
        static const juce::String FilterEnabled { "filter_enabled" };
        static const juce::String FilterType { "filter_type" };
        static const juce::String EnvAnalog { "env_analog" };
        static const juce::String MasterGain { "master_gain" };
    }

    namespace Name
    {
        static const juce::String Cutoff { "Cutoff" };
        static const juce::String Resonance { "Resonance" };
        static const juce::String Attack { "Attack" };
        static const juce::String Decay { "Decay" };
        static const juce::String Sustain { "Sustain" };
        static const juce::String Release { "Release" };

        static const juce::String OscType { "Oscillator Type" };
        static const juce::String OscGain { "Oscillator Gain" };
        static const juce::String FilterEnabled { "Filter Enabled" };
        static const juce::String FilterType { "Filter Type" };
        static const juce::String EnvAnalog { "Envelope Style" };
        static const juce::String MasterGain { "Master Gain" };
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
    bool isSynth() const;
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
    //juce::dsp::LadderFilter<float> filter;
    juce::SmoothedValue<float> outputGain;

    void processMidiMessages(juce::MidiBuffer& midiMessages);

    DSP::Oscillator osc;
    juce::SmoothedValue<float> oscGain;

    DSP::StateVariableFilter filter;
    bool filterEnabled;
    float filterCutoff;
    float filterResonance;
    int filterType;

    DSP::EnvelopeGenerator env;

    float lastMidiNoteFreq;
    bool noteActive;
    int currentNoteNumber;

    juce::AudioBuffer<float> oscBuffer;
    juce::AudioBuffer<float> envBuffer;
    juce::AudioBuffer<float> filterL, filterB, filterH;

    std::vector<float> filterFreqBuffer;
    std::vector<float> filterResoBuffer;
    std::vector<float> envelopeBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessor)
};
