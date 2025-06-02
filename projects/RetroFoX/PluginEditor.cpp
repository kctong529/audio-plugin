#include "PluginProcessor.h"
#include "PluginEditor.h"

MainProcessorEditor::MainProcessorEditor(MainProcessor& p) :
    AudioProcessorEditor(&p),   // Call base class constructor with processor reference
    audioProcessor(p),
    customParameterEditor(audioProcessor.getParameterManager()) // Initialize customParameterEditor with parameter manager
{
    // Set minimum and maximum window size limits for resizing
    resizeLimits.setSizeLimits(360, 200, 1000, 800);

    // Assign the constrainer to the editor to enforce size limits during resize
    setConstrainer(&resizeLimits);

    // Allow window to be resizable, both horizontally and vertically
    setResizable(true, true);

    // Set the initial window size
    setSize(600, 400);

    // Add the custom parameter editor component and make it visible
    addAndMakeVisible(customParameterEditor);
}

MainProcessorEditor::~MainProcessorEditor()
{
    // Destructor - default behavior (no special cleanup needed)
}

void MainProcessorEditor::paint(juce::Graphics& g)
{
}

void MainProcessorEditor::resized()
{
    // Make the custom parameter editor fill the entire window
    customParameterEditor.setBounds(getLocalBounds());
}
