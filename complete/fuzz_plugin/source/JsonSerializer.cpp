
namespace {

// This struct mirrors the fuzz Parameters in a serializable form
struct SerializableParameters {
    float sustain;
    float tone;
    float outputLevel;
    bool bypassed;
    juce::String mode;

    static constexpr auto marshallingVersion = 1;

    template <typename Archive, typename T>
    static void serialise(Archive& archive, T& p)
    {
        using namespace juce;

        if (archive.getVersion() != marshallingVersion)
            return;

        std::string pluginName = FUZZ_PLUGIN_NAME;
        archive(named("pluginName", pluginName));

        if (pluginName != FUZZ_PLUGIN_NAME)
            return;

        archive(named("sustain", p.sustain),
                named("tone", p.tone),
                named("outputLevel", p.outputLevel),
                named("bypassed", p.bypassed),
                named("mode", p.mode));
    }
};

// Convert from live Parameters → SerializableParameters
SerializableParameters from(const fuzz::Parameters& p)
{
    return {
        .sustain     = p.sustain.get(),
        .tone        = p.tone.get(),
        .outputLevel = p.outputLevel.get(),
        .bypassed    = p.bypassed.get(),
        .mode        = p.mode.getCurrentChoiceName()
    };
}

} // namespace

namespace fuzz {

void JsonSerializer::serialize(const Parameters& parameters,
                               juce::OutputStream& output)
{
    const auto json = juce::ToVar::convert(from(parameters));

    if (!json.has_value())
        return;

    juce::JSON::writeToStream(output, *json,
        juce::JSON::FormatOptions{}
            .withSpacing(juce::JSON::Spacing::multiLine)
            .withMaxDecimalPlaces(2));
}

juce::Result JsonSerializer::deserialize(juce::InputStream& input,
                                         Parameters& parameters)
{
    juce::var parsedResult;
    auto parsingResult =
        juce::JSON::parse(input.readEntireStreamAsString(), parsedResult);

    if (parsingResult.failed())
        return parsingResult;

    const auto parsedParameters =
        juce::FromVar::convert<SerializableParameters>(parsedResult);

    if (!parsedParameters.has_value())
        return juce::Result::fail("failed to parse parameters from JSON");

    // Validate mode choice
    const auto modeIndex =
        parameters.mode.choices.indexOf(parsedParameters->mode);

    if (modeIndex < 0)
    {
        return juce::Result::fail(
            "invalid mode name; supported values are: " +
            parameters.mode.choices.joinIntoString(", "));
    }

    // Apply parameters
    parameters.sustain     = parsedParameters->sustain;
    parameters.tone        = parsedParameters->tone;
    parameters.outputLevel = parsedParameters->outputLevel;
    parameters.bypassed    = parsedParameters->bypassed;
    parameters.mode        = modeIndex;

    return juce::Result::ok();
}

} // namespace fuzz

