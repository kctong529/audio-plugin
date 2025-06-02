#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "CustomParameterEditor.h"

class MainProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MainProcessorEditor(MainProcessor&);
    ~MainProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    juce::ResizableCornerComponent resizer{ this, &resizeLimits };
    juce::ComponentBoundsConstrainer resizeLimits;

private:
    MainProcessor& audioProcessor;
    CustomParameterEditor customParameterEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainProcessorEditor)
};
