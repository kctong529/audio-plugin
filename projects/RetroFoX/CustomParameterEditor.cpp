#include "CustomParameterEditor.h"

CustomParameterEditor::CustomParameterEditor(mrta::ParameterManager& pm)
    : mrta::GenericParameterEditor(pm), parameterManager(pm)
{
    // Get the AudioProcessorValueTreeState for parameter binding
    auto& apvts = parameterManager.getAPVTS();

    // Create UI elements for each parameter in the parameter manager
    for (auto& param : parameterManager.getParameters())
    {
        // Create and configure a label displaying the parameter ID
        auto label = std::make_unique<juce::Label>();
        label->setText(param.name, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->setColour(juce::Label::textColourId, juce::Colours::saddlebrown);

        // Apply a custom font to the label
        juce::Font labelFont (FontOptions(32.0f));
        labelFont.setTypefaceName("Brush Script MT");
        labelFont.setStyleFlags(juce::Font::italic);
        label->setFont(labelFont);

        // Add label to the editor and store it
        addAndMakeVisible(label.get());
        parameterLabels.push_back(std::move(label));

        // Create and configure a vertical slider for the parameter
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::LinearVertical);
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);

        // Add slider to the editor and store it
        addAndMakeVisible(slider.get());
        parameterComponents.push_back(std::move(slider));

        // Attach the slider to the corresponding parameter in the APVTS
        sliderAttachments.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, param.ID, *static_cast<juce::Slider*>(parameterComponents.back().get())));
    }
}

CustomParameterEditor::~CustomParameterEditor() = default;

void CustomParameterEditor::resized()
{
    // Exit early if no parameters exist
    int numParams = static_cast<int>(parameterComponents.size());
    if (numParams == 0)
        return;

    // Calculate the width allocated to each slider, accounting for horizontal spacing
    int sliderWidth = (getWidth() - spacingX * (numParams + 1)) / numParams;

    // Calculate the usable slider height, leaving room for spacing, label, and textbox
    int textBoxHeight = 20;
    int sliderHeight = getHeight() - spacingY * 3 - labelHeight - textBoxHeight;

    // Compute Y positions for sliders and labels
    int sliderY = spacingY + textBoxHeight;
    int labelY  = spacingY + textBoxHeight + sliderHeight + spacingY;

    // Position each parameter's slider and label horizontally across the editor
    for (int i = 0; i < numParams; ++i)
    {
        int x = spacingX + i * (sliderWidth + spacingX);

        if (parameterComponents[i])
        {
            // Cast to Slider and position it
            auto* slider = static_cast<juce::Slider*>(parameterComponents[i].get());
            slider->setTextBoxStyle(juce::Slider::TextBoxAbove, false, sliderWidth, textBoxHeight);
            slider->setBounds(x, sliderY, sliderWidth, sliderHeight);
            slider->setColour(juce::Slider::textBoxTextColourId, juce::Colours::saddlebrown);
        }

        if (parameterLabels[i])
        {
            // Position the label below its corresponding slider
            parameterLabels[i]->setBounds(x, labelY, sliderWidth, labelHeight);
        }
    }
}

void CustomParameterEditor::paint(juce::Graphics& g)
{
    // Fill the background with an off-white vintage color
    g.fillAll(juce::Colours::antiquewhite);
}
