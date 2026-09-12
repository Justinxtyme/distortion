
// namespace fuzz {
//
// PluginEditor::PluginEditor(PluginProcessor& p)
//     : AudioProcessorEditor(&p),
//       processor(p),
//       sustainAttachment(p.getParameterRefs().sustain, sustainSlider),
//       toneAttachment(p.getParameterRefs().tone, toneSlider),
//       levelAttachment(p.getParameterRefs().outputLevel, levelSlider),
//       bypassAttachment(p.getParameterRefs().bypassed, bypassButton)
// {
//     // === Sustain ===
//     sustainSlider.setSliderStyle(juce::Slider::Rotary);
//     sustainSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
//     addAndMakeVisible(sustainSlider);
//
//     sustainLabel.setText("Sustain", juce::dontSendNotification);
//     sustainLabel.setJustificationType(juce::Justification::centred);
//     addAndMakeVisible(sustainLabel);
//
//     // === Tone ===
//     toneSlider.setSliderStyle(juce::Slider::Rotary);
//     toneSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
//     addAndMakeVisible(toneSlider);
//
//     toneLabel.setText("Tone", juce::dontSendNotification);
//     toneLabel.setJustificationType(juce::Justification::centred);
//     addAndMakeVisible(toneLabel);
//
//     // === Level ===
//     levelSlider.setSliderStyle(juce::Slider::Rotary);
//     levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
//     addAndMakeVisible(levelSlider);
//
//     levelLabel.setText("Level", juce::dontSendNotification);
//     levelLabel.setJustificationType(juce::Justification::centred);
//     addAndMakeVisible(levelLabel);
//
//     // === Bypass ===
//     bypassButton.setButtonText("Bypass");
//     addAndMakeVisible(bypassButton);
//
//     bypassLabel.setText("Bypass", juce::dontSendNotification);
//     bypassLabel.setJustificationType(juce::Justification::centred);
//     addAndMakeVisible(bypassLabel);
//
//     setSize(400, 200);
// }
//
// void PluginEditor::resized()
// {
//     auto area = getLocalBounds().reduced(20);
//
//     auto topRow = area.removeFromTop(100);
//     auto bottomRow = area;
//
//     // Three knobs across the top
//     sustainLabel.setBounds(topRow.removeFromLeft(100).removeFromTop(20));
//     sustainSlider.setBounds(topRow.removeFromLeft(100));
//
//     toneLabel.setBounds(topRow.removeFromLeft(100).removeFromTop(20));
//     toneSlider.setBounds(topRow.removeFromLeft(100));
//
//     levelLabel.setBounds(topRow.removeFromLeft(100).removeFromTop(20));
//     levelSlider.setBounds(topRow.removeFromLeft(100));
//
//     // Bypass centered at bottom
//     bypassLabel.setBounds(bottomRow.removeFromTop(20));
//     bypassButton.setBounds(bottomRow.withSizeKeepingCentre(80, 30));
// }
//
// } // namespace fuzz
