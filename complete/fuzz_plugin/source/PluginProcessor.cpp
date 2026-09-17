
namespace fuzz {


PluginProcessor::PluginProcessor()
: AudioProcessor(
    BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

const juce::String PluginProcessor::getName() const {
    return "Fuzz";
}

bool PluginProcessor::acceptsMidi() const        { return false; }
bool PluginProcessor::producesMidi() const       { return false; }
bool PluginProcessor::isMidiEffect() const       { return false; }
double PluginProcessor::getTailLengthSeconds() const { return 0.0; }

int PluginProcessor::getNumPrograms()                  { return 1; }
int PluginProcessor::getCurrentProgram()               { return 0; }
void PluginProcessor::setCurrentProgram(int idx)       { juce::ignoreUnused(idx); }
const juce::String PluginProcessor::getProgramName(int idx) {
    juce::ignoreUnused(idx);
    return "None";
}
void PluginProcessor::changeProgramName(int idx, const juce::String& nm) {
    juce::ignoreUnused(idx, nm);
}

bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in  = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono()
     && out != juce::AudioChannelSet::stereo())
        return false;

    return out == in;
}

void PluginProcessor::prepareToPlay(double sampleRate,
                                    int expectedMaxFramesPerBlock)
{

    currentSampleRate = sampleRate;

    engine.setMode(0);


    engine.prepare(sampleRate, expectedMaxFramesPerBlock);

    // // THIS is the correct line:
    // engine.setSustain(parameters.sustain.get());



    juce::dsp::ProcessSpec spec {
        sampleRate,
        static_cast<uint32_t>(expectedMaxFramesPerBlock),
        static_cast<uint32_t>(juce::jmax(getTotalNumInputChannels(),
                                         getTotalNumOutputChannels()))
    };

    bypassTransitionSmoother.prepare(spec);



}

void PluginProcessor::releaseResources()
{
    engine.reset();
    bypassTransitionSmoother.reset();
}

// void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
//                                    juce::MidiBuffer& midi)
// {
//     juce::ignoreUnused(midi);
//     juce::ScopedNoDenormals noDenormals;
//
//     const int totalIn  = getTotalNumInputChannels();
//     const int totalOut = getTotalNumOutputChannels();
//
//     for (int ch = totalIn; ch < totalOut; ++ch)
//         buffer.clear(ch, 0, buffer.getNumSamples());
//
//     // Update engine parameters from current values
//     engine.setSustain(parameters.sustain.get());
//     engine.setTone(parameters.tone.get());
//     engine.setOutputLevel(parameters.outputLevel.get());
//
//
//     int mode = parameters.mode.getIndex();
//
//     if (mode != lastMode)
//     {
//         engine.setMode(mode);                     // switch DSP engine
//         engine.prepare(getSampleRate(), getBlockSize());  // re-init DSP
//         lastMode = mode;
//     }
//
//     //engine.setMode(parameters.mode.getIndex());
//
//     // Oversampling
//     bool os = parameters.oversampling.get();
//
//     if (os != engine.isOversamplingEnabled)
//     {
//         engine.setOversampling(os);
//         engine.prepare(getSampleRate(), getBlockSize());
//     }
//     // Bypass smoothing
//     const bool bypassed = parameters.bypassed.get();
//     bypassTransitionSmoother.setBypass(bypassed);
//
//     const bool fullyBypassed = bypassed && !bypassTransitionSmoother.isTransitioning();
//     if (fullyBypassed)
//         return;
//
//     // Store dry buffer for crossfade
//     bypassTransitionSmoother.setDryBuffer(buffer);
//
//     // Process fuzz
//     engine.process(buffer);
//
//     // Crossfade wet/dry according to bypass transition
//     bypassTransitionSmoother.mixToWetBuffer(buffer);
// }

    void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();

    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // ============================
    // Read APVTS parameters (atomic)
    // ============================
    const float sustain = *apvts.getRawParameterValue("fuzz.sustain");
    const float tone    = *apvts.getRawParameterValue("fuzz.tone");
    const float level   = *apvts.getRawParameterValue("fuzz.output");

    const bool bypassed = apvts.getRawParameterValue("fuzz.bypassed")->load();
    const bool os       = apvts.getRawParameterValue("fuzz.oversampling")->load();

    float mode      = apvts.getRawParameterValue("fuzz.mode")->load();

    // ============================
    // Mode switching (re-init DSP)
    // ============================
    if (mode != lastMode)
    {
        engine.setMode(mode);
        engine.prepare(getSampleRate(), getBlockSize());
        lastMode = mode;
    }


    // ============================
    // Push parameters into DSP engine
    // ============================
    engine.setSustain(sustain);
    engine.setTone(tone);
    engine.setOutputLevel(level);



    // ============================
    // Oversampling toggle (re-init DSP)
    // ============================
    if (os != engine.isOversamplingEnabled)
    {
        engine.setOversampling(os);
        engine.prepare(getSampleRate(), getBlockSize());
    }

    // ============================
    // Bypass smoothing
    // ============================
    bypassTransitionSmoother.setBypass(bypassed);

    const bool fullyBypassed = bypassed && !bypassTransitionSmoother.isTransitioning();
    if (fullyBypassed)
        return;

    // Store dry buffer for crossfade
    bypassTransitionSmoother.setDryBuffer(buffer);

    // ============================
    // Process fuzz engine
    // ============================
    engine.process(buffer);

    // ============================
    // Crossfade wet/dry
    // ============================
    bypassTransitionSmoother.mixToWetBuffer(buffer);
}


bool PluginProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor() {
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}


juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const noexcept {
    return apvts.getParameter("fuzz.bypassed");
}

double PluginProcessor::getSampleRateThreadSafe() const noexcept {
    return currentSampleRate.load();
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fuzz.sustain", "Sustain",
        juce::NormalisableRange<float>{0.1f, 3.0f, 0.0f},
        1.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fuzz.tone", "Tone",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f},
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "fuzz.output", "Output Level",
        juce::NormalisableRange<float>{0.0f, 1.0f, 0.001f},
        1.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "fuzz.bypassed", "Bypass",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "fuzz.oversampling", "Oversampling",
        false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "fuzz.mode", "Mode",
        juce::StringArray{"Fuzz", "Tube", "Cream", "Hard"},
        0));

    return { params.begin(), params.end() };
}


} // namespace fuzz

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new fuzz::PluginProcessor();
}
