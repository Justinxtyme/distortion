#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace fuzz
{

class PluginProcessor;

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
    juce::ToggleButton oversamplingButton;

    juce::ComboBox modeSelector;

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> oversamplingAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

// ======================================================================
// Implementation
// ======================================================================

PluginEditor::PluginEditor(PluginProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p)
{
    // === Sustain ===
    sustainSlider.setSliderStyle(juce::Slider::Rotary);
    sustainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(sustainSlider);

    sustainLabel.setText("Sustain", juce::dontSendNotification);
    sustainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(sustainLabel);

    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "fuzz.sustain", sustainSlider);

    // === Tone ===
    toneSlider.setSliderStyle(juce::Slider::Rotary);
    toneSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(toneSlider);

    toneLabel.setText("Tone", juce::dontSendNotification);
    toneLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(toneLabel);

    toneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "fuzz.tone", toneSlider);

    // === Level ===
    levelSlider.setSliderStyle(juce::Slider::Rotary);
    levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible(levelSlider);

    levelLabel.setText("Level", juce::dontSendNotification);
    levelLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(levelLabel);

    levelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "fuzz.output", levelSlider);

    // === Bypass ===
    bypassButton.setButtonText("Bypass");
    addAndMakeVisible(bypassButton);

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "fuzz.bypassed", bypassButton);

    // === Oversampling ===
    oversamplingButton.setButtonText("Oversampling");
    addAndMakeVisible(oversamplingButton);

    oversamplingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.apvts, "fuzz.oversampling", oversamplingButton);

    // === Mode Selector ===
    modeSelector.addItem("Fuzz", 1);
    modeSelector.addItem("Tube", 2);
    modeSelector.addItem("Cream", 3);
    modeSelector.addItem("Hard", 4);
    addAndMakeVisible(modeSelector);


    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "fuzz.mode", modeSelector);


    int modeIndex = static_cast<int>(
    processor.apvts.getRawParameterValue("fuzz.mode")->load());

        modeSelector.setSelectedId(modeIndex + 1, juce::dontSendNotification);


    setSize(650, 250);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    auto topRow = area.removeFromTop(120);
    auto colWidth = topRow.getWidth() / 3;

    auto sustainArea = topRow.removeFromLeft(colWidth);
    auto toneArea    = topRow.removeFromLeft(colWidth);
    auto levelArea   = topRow.removeFromLeft(colWidth);

    sustainLabel.setBounds(sustainArea.removeFromTop(20));
    sustainSlider.setBounds(sustainArea);

    toneLabel.setBounds(toneArea.removeFromTop(20));
    toneSlider.setBounds(toneArea);

    levelLabel.setBounds(levelArea.removeFromTop(20));
    levelSlider.setBounds(levelArea);

    auto modeRow = area.removeFromTop(50);
    modeSelector.setBounds(modeRow.withSizeKeepingCentre(140, 30));

    auto oversamplingArea = area.removeFromTop(50);
    oversamplingButton.setBounds(oversamplingArea.withSizeKeepingCentre(100, 30));

    auto bypassArea = area.removeFromTop(50);
    bypassButton.setBounds(bypassArea.withSizeKeepingCentre(100, 30));
}

} // namespace fuzz
