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

    juce::ComboBox modeSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    juce::ToggleButton oversamplingButton;

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

    //bypassLabel.setText("Bypass", juce::dontSendNotification);
    bypassLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bypassLabel);

    modeSelector.addItem("Fuzz", 1);
    modeSelector.addItem("Tube", 2);
    modeSelector.addItem("Cream", 3);
    modeSelector.addItem("Hard", 4);

    modeSelector.setSelectedId(1);

    addAndMakeVisible(modeSelector);

    // Bind UI → Processor parameter
    modeSelector.onChange = [this]
    {
        auto& params = processor.getParameterRefs();
        params.mode.setValueNotifyingHost(static_cast<float>(modeSelector.getSelectedId() - 1));
    };

    oversamplingButton.setButtonText("Oversampling");
    //oversamplingButton.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(oversamplingButton);


    oversamplingButton.onClick = [this]
    {
        auto& params = processor.getParameterRefs();
        params.oversampling.setValueNotifyingHost(oversamplingButton.getToggleState());
    };


    setSize(650, 250);
}

    void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    // === Top row: 3 vertical columns ===
    auto topRow = area.removeFromTop(120);   // slightly taller to fit label + knob
    auto colWidth = topRow.getWidth() / 3;

    auto sustainArea = topRow.removeFromLeft(colWidth);
    auto toneArea    = topRow.removeFromLeft(colWidth);
    auto levelArea   = topRow.removeFromLeft(colWidth);

    // Sustain column
    sustainLabel.setBounds(sustainArea.removeFromTop(20));
    sustainSlider.setBounds(sustainArea);

    // Tone column
    toneLabel.setBounds(toneArea.removeFromTop(20));
    toneSlider.setBounds(toneArea);

    // Level column
    levelLabel.setBounds(levelArea.removeFromTop(20));
    levelSlider.setBounds(levelArea);

    // === Middle row: Mode selector ===
    auto modeRow = area.removeFromTop(50);
    modeSelector.setBounds(modeRow.withSizeKeepingCentre(140, 30));

    // === Bottom row: Oversampling + Bypass (vertical stack) ===
    auto oversamplingArea = area.removeFromTop(50);
    //oversamplingLabel.setBounds(oversamplingArea.removeFromTop(20));
    oversamplingButton.setBounds(oversamplingArea.withSizeKeepingCentre(100, 30));

    auto bypassArea = area.removeFromTop(50);
    //bypassLabel.setBounds(bypassArea.removeFromTop(20));
    bypassButton.setBounds(bypassArea.withSizeKeepingCentre(100, 30));
}




} // namespace fuzz
