#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class CustomParameterEditor : public mrta::GenericParameterEditor
{
public:
    CustomParameterEditor(mrta::ParameterManager& parameterManager);
    ~CustomParameterEditor() override;

    void resized() override;
    void paint(juce::Graphics& g) override;

    static constexpr int labelHeight = 40;
    static constexpr int spacingX = 10;
    static constexpr int spacingY = 10;

private:
    std::vector<std::unique_ptr<juce::Label>> parameterLabels;
    std::vector<std::unique_ptr<juce::Component>> parameterComponents;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;

    mrta::ParameterManager& parameterManager;
};
