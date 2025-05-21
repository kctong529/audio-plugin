#include "PluginProcessor.h"
#include "PluginEditor.h"

MainProcessorEditor::MainProcessorEditor(MainProcessor& p) :
    AudioProcessorEditor(&p), audioProcessor(p),
    genericParameterEditor(audioProcessor.getParameterManager())
{
    addAndMakeVisible(genericParameterEditor);
    const int numOfParams { static_cast<int>(audioProcessor.getParameterManager().getParameters().size()) };
    setSize(300, numOfParams * genericParameterEditor.parameterWidgetHeight);
}

MainProcessorEditor::~MainProcessorEditor()
{
}

//==============================================================================
void MainProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainProcessorEditor::resized()
{
    genericParameterEditor.setBounds(getLocalBounds());
}