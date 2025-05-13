#include "PluginProcessor.h"
#include "PluginEditor.h"

static const std::vector<mrta::ParameterInfo> ParameterInfos
{
    { Param::ID::Enabled,        Param::Name::Enabled,   "Off", "On", true },
    { Param::ID::ModulationFreq, Param::Name::ModulationFreq, "Hz", 30.0f, 0.1f, 2000.0f, 0.1f, 0.3f },
    
};

MainProcessor::MainProcessor() :
    parameterManager(*this, ProjectInfo::projectName, ParameterInfos)
{
    parameterManager.registerParameterCallback(Param::ID::Enabled,
    [this] (float value, bool /*forced*/)
    {
        DBG(Param::Name::Enabled + ": " + juce::String { value });
        isEnabled = (value > 0.5f);

        filter.setEnabled(isEnabled);
    });

    parameterManager.registerParameterCallback(Param::ID::ModulationFreq, 
    [this](float value, bool /*forced*/) 
    {
        DBG(Param::Name::ModulationFreq + juce::String(value));
        modulationFreqHz = value;
    });
}

MainProcessor::~MainProcessor()
{
}

void MainProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::uint32 numChannels { static_cast<juce::uint32>(std::max(getMainBusNumInputChannels(), getMainBusNumOutputChannels())) };
    filter.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), numChannels });
    outputGain.reset(sampleRate, 0.01f);

    modulationPhase = 0.0;

    parameterManager.updateParameters(true);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), static_cast<juce::uint32>(numChannels) };
    dcBlocker.prepare(spec);
    dcBlocker.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);  // 20 Hz cutoff

}

void MainProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    parameterManager.updateParameters();

    float modFreq = modulationFreqHz;

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    //juce::dsp::AudioBlock<float> audioBlock(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), buffer.getNumSamples());
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(audioBlock);
    filter.process(ctx);

    if(isEnabled){
        for(int sample = 0; sample < numSamples; ++sample){
            float modSignal = std::cos(modulationPhase);
            modulationPhase += 2.0f * juce::MathConstants<float>::pi * modFreq / currentSampleRate;
            
            // Wrap phase to avoid precision issues
            if (modulationPhase > juce::MathConstants<float>::twoPi)
                modulationPhase -= juce::MathConstants<float>::twoPi;

            // Apply ring modulation to each channel
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* channelData = buffer.getWritePointer(ch);
                //channelData[sample] *= modSignal;
                float inSample = channelData[sample];

                float diodeMod =
                    std::tanh(diodeSaturationAmount * (inSample + modSignal)) -
                    std::tanh(diodeSaturationAmount * (inSample - modSignal));

                channelData[sample] = diodeMod;
            }

        }
    }
    

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> dcCtx(block);
    dcBlocker.process(dcCtx);

    //outputGain.applyGain(buffer, buffer.getNumSamples());
    outputGain.applyGain(buffer, numSamples);
}

void MainProcessor::releaseResources()
{
    filter.reset();
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
const juce::String MainProcessor::getName() const { return "Homework1"; }
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
