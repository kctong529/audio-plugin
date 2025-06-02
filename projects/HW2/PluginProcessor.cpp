#include "PluginProcessor.h"
#include "PluginEditor.h"

static const std::vector<mrta::ParameterInfo> ParameterInfos
{
    { Param::ID::DelayTime, Param::Name::DelayTime, "ms", 500.f, 1.f, 2000.f, 1.f, 0.3f },
    { Param::ID::Feedback,  Param::Name::Feedback,  "",   0.5f, 0.f, 0.95f, 0.01f, 0.3f },
    { Param::ID::WetMix,    Param::Name::WetMix,    "",   0.5f, 0.f, 1.f, 0.01f, 0.3f },
    { Param::ID::DryMix,    Param::Name::DryMix,    "",   0.5f, 0.f, 1.f, 0.01f, 0.3f },
};

MainProcessor::MainProcessor() :
    parameterManager(*this, ProjectInfo::projectName, ParameterInfos)
{
    parameterManager.registerParameterCallback(Param::ID::DelayTime,
    [this](float value, bool) {
        delayTimeSamples.setTargetValue(value * 0.001f * currentSampleRate); // ms to samples
    });

    parameterManager.registerParameterCallback(Param::ID::Feedback,
    [this](float value, bool) {
        feedback.setTargetValue(value);
    });

    parameterManager.registerParameterCallback(Param::ID::WetMix,
    [this](float value, bool) {
        wetMix.setTargetValue(value);
    });

    parameterManager.registerParameterCallback(Param::ID::DryMix,
    [this](float value, bool) {
        dryMix.setTargetValue(value);
    });

}

MainProcessor::~MainProcessor()
{
}

void MainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // juce::uint32 numChannels { static_cast<juce::uint32>(std::max(getMainBusNumInputChannels(), getMainBusNumOutputChannels())) };
    // filter.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), numChannels });
    // outputGain.reset(sampleRate, 0.01f);

    currentSampleRate = sampleRate;
    int maxDelaySamples = static_cast<int>(2.0 * sampleRate);
    delayBuffer.setSize(getTotalNumOutputChannels(), maxDelaySamples);
    delayBuffer.clear();

    delayTimeSamples.reset(sampleRate, 0.05);
    feedback.reset(sampleRate, 0.05);
    wetMix.reset(sampleRate, 0.05);
    dryMix.reset(sampleRate, 0.05);

    parameterManager.updateParameters(true);
}

void MainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    // juce::ScopedNoDenormals noDenormals;

    // {
    //     juce::dsp::AudioBlock<float> audioBlock(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    //     juce::dsp::ProcessContextReplacing<float> ctx(audioBlock);
    //     filter.process(ctx);
    // }

    // outputGain.applyGain(buffer, buffer.getNumSamples());
    
    parameterManager.updateParameters();

    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    for(int sample = 0; sample < numSamples; ++sample){
        float currentDelaySamples = delayTimeSamples.getNextValue();
        float fb = feedback.getNextValue();
        float wet = wetMix.getNextValue();
        float dry = dryMix.getNextValue();

        int delaySamples = static_cast<int>(currentDelaySamples);
        int delayBufferSize = delayBuffer.getNumSamples();

        for(int ch = 0; ch < numChannels; ++ch){
            float* channelData = buffer.getWritePointer(ch);
            float* delayData = delayBuffer.getWritePointer(ch);

            int readPos = (writePosition - delaySamples + delayBufferSize) % delayBufferSize;

            float delayedInput = delayData[readPos];  // x[n-D]
            float delayedOutput = delayData[readPos]; // approximation for y[n-D]

            float input = channelData[sample];
            float out = wet * (delayedInput + fb * delayedOutput) + dry * input;

            delayData[writePosition] = input + fb * delayedOutput;
            channelData[sample] = out;



        }
        writePosition = (writePosition + 1) % delayBuffer.getNumSamples();

    }
}

void MainProcessor::releaseResources()
{
    // filter.reset();
    // No resources need to be released.
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
const juce::String MainProcessor::getName() const { return "Homework2"; }
bool MainProcessor::acceptsMidi() const { return false; }
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
