#pragma once

namespace fuzz {

struct Parameters {
  explicit Parameters(juce::AudioProcessor& processor);

  juce::AudioParameterFloat& sustain;       // Input gain → clipping stages
  juce::AudioParameterFloat& tone;          // Tone stack blend
  juce::AudioParameterFloat& outputLevel;   // Final output gain
  juce::AudioParameterBool& bypassed;       // Bypass state
  juce::AudioParameterChoice& mode;         // Triangle / Rams Head / Russian / NYC

  JUCE_DECLARE_NON_COPYABLE(Parameters)
  JUCE_DECLARE_NON_MOVEABLE(Parameters)
};

} // namespace fuzz