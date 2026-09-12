#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace fuzz
{

class PluginProcessor; // forward declare, or include PluginProcessor.h

// Simple attachment that binds a Slider to a AudioParameterFloat
struct SliderAttachment
{
    SliderAttachment(juce::AudioParameterFloat& p, juce::Slider& s)
        : param(p), slider(s)
    {
        slider.setRange(param.range.start, param.range.end);
        slider.setValue(param.get(), juce::dontSendNotification);

        slider.onValueChange = [this]
        {
            param.setValueNotifyingHost((float) slider.getValue());
        };
    }

    juce::AudioParameterFloat& param;
    juce::Slider& slider;
};

// Simple attachment that binds a ToggleButton to an AudioParameterBool
struct ButtonAttachment
{
    ButtonAttachment(juce::AudioParameterBool& p, juce::ToggleButton& b)
        : param(p), button(b)
    {
        button.setToggleState(param.get(), juce::dontSendNotification);

        button.onClick = [this]
        {
            param.setValueNotifyingHost(button.getToggleState() ? 1.0f : 0.0f);
        };
    }

    juce::AudioParameterBool& param;
    juce::ToggleButton& button;
};

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    PluginEditor(PluginProcessor& p);
    ~PluginEditor() override = default;

    void resized() override;

private:
    PluginProcessor& processor;

    juce::Slider sustainSlider;
    juce::Slider toneSlider;
    juce::Slider levelSlider;

    juce::Label sustainLabel;
    juce::Label toneLabel;
    juce::Label levelLabel;

    juce::ToggleButton bypassButton;
    juce::Label bypassLabel;

    SliderAttachment sustainAttachment;
    SliderAttachment toneAttachment;
    SliderAttachment levelAttachment;
    ButtonAttachment bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

// ======================================================================
// Implementation
// ======================================================================

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      sustainAttachment(p.getParameterRefs().sustain,      sustainSlider),
      toneAttachment   (p.getParameterRefs().tone,         toneSlider),
      levelAttachment  (p.getParameterRefs().outputLevel,  levelSlider),
      bypassAttachment (p.getParameterRefs().bypassed,     bypassButton)
{
    // === Sustain ===
    sustainSlider.setSliderStyle(juce::Slider::Rotary);
    sustainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(sustainSlider);

    sustainLabel.setText("Sustain", juce::dontSendNotification);
    sustainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sustainLabel);

    // === Tone ===
    toneSlider.setSliderStyle(juce::Slider::Rotary);
    toneSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(toneSlider);

    toneLabel.setText("Tone", juce::dontSendNotification);
    toneLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(toneLabel);

    // === Level ===
    levelSlider.setSliderStyle(juce::Slider::Rotary);
    levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(levelSlider);

    levelLabel.setText("Level", juce::dontSendNotification);
    levelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(levelLabel);

    // === Bypass ===
    bypassButton.setButtonText("Bypass");
    addAndMakeVisible(bypassButton);

    bypassLabel.setText("Bypass", juce::dontSendNotification);
    bypassLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bypassLabel);

    setSize(600, 200);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto topRow = area.removeFromTop(100);
    auto bottomRow = area;

    // Three knobs across the top
    sustainLabel.setBounds(topRow.removeFromLeft(100).removeFromTop(20));
    sustainSlider.setBounds(topRow.removeFromLeft(100));

    toneLabel.setBounds(topRow.removeFromLeft(100).removeFromTop(20));
    toneSlider.setBounds(topRow.removeFromLeft(100));

    levelLabel.setBounds(topRow.removeFromLeft(100).removeFromTop(20));
    levelSlider.setBounds(topRow.removeFromLeft(100));

    // Bypass centered at bottom
    bypassLabel.setBounds(bottomRow.removeFromTop(20));
    bypassButton.setBounds(bottomRow.withSizeKeepingCentre(80, 30));
}

} // namespace fuzz
