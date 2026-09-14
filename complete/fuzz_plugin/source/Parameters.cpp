
namespace fuzz {
namespace {

// --------------------------------------------------------------
// Utility: add parameter to processor and return reference
// --------------------------------------------------------------
template <typename ParamType>
auto& addParameterToProcessor(juce::AudioProcessor& processor,
                              std::unique_ptr<ParamType> parameter)
{
    auto& ref = *parameter;
    processor.addParameter(parameter.release());
    return ref;
}

// --------------------------------------------------------------
// Sustain (input gain → clipping stages)
// --------------------------------------------------------------
juce::AudioParameterFloat& createSustainParameter(juce::AudioProcessor& processor)
{
    constexpr int versionHint = 1;

    return addParameterToProcessor(
        processor,
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fuzz.sustain", versionHint},
            "Sustain",
            juce::NormalisableRange<float>{0.1f, 3.0f, 0.0f},
            1.5f,
            juce::AudioParameterFloatAttributes{}.withLabel("dB")
        )
    );
}

// --------------------------------------------------------------
// Tone (LP/HP blend)
// --------------------------------------------------------------
juce::AudioParameterFloat& createToneParameter(juce::AudioProcessor& processor)
{
    constexpr int versionHint = 1;

    return addParameterToProcessor(
        processor,
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fuzz.tone", versionHint},
            "Tone",
            juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f},
            0.5f
        )
    );
}

// --------------------------------------------------------------
// Output Level (post‑tone gain)
// --------------------------------------------------------------
juce::AudioParameterFloat& createOutputLevelParameter(juce::AudioProcessor& processor)
{
    constexpr int versionHint = 1;

    return addParameterToProcessor(
        processor,
        std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"fuzz.output", versionHint},
            "Output Level",
            juce::NormalisableRange<float>{0.0f, 1.5f, 0.001f},
            0.75f,
            juce::AudioParameterFloatAttributes{}.withLabel("x")
        )
    );
}


// --------------------------------------------------------------
// Bypass
// --------------------------------------------------------------
juce::AudioParameterBool& createBypassedParameter(juce::AudioProcessor& processor)
{
    constexpr int versionHint = 1;

    return addParameterToProcessor(
        processor,
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"fuzz.bypassed", versionHint},
            "Bypass",
            false
        )
    );
}

// --------------------------------------------------------------
// Mode (Triangle / Rams Head / Russian / NYC)
// --------------------------------------------------------------
juce::AudioParameterChoice& createModeParameter(juce::AudioProcessor& processor)
{
    constexpr int versionHint = 1;

    return addParameterToProcessor(
        processor,
        std::make_unique<juce::AudioParameterChoice>(
            juce::ParameterID{"fuzz.mode", versionHint},
            "Mode",
            juce::StringArray{"Fuzz", "Tube", "Cream", "Hard"},
            0 // default: Triangle
        )
    );
}

juce::AudioParameterBool& createOversamplingParameter(juce::AudioProcessor& processor)
{
    constexpr int versionHint = 1;

    return addParameterToProcessor(
        processor,
        std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"fuzz.oversampling", versionHint},
            "Oversampling",
            false
        )
    );
}

} // anonymous namespace

// ============================================================================
// Parameters constructor
// ============================================================================
Parameters::Parameters(juce::AudioProcessor& processor)
    : sustain{createSustainParameter(processor)},
      tone{createToneParameter(processor)},
      outputLevel{createOutputLevelParameter(processor)},
      bypassed{createBypassedParameter(processor)},
      mode{createModeParameter(processor)},
      oversampling{createOversamplingParameter(processor)}
{
}




} // namespace fuzz
