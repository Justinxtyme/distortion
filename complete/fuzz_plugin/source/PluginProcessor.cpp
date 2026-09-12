
namespace fuzz {

PluginProcessor::PluginProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
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

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midi)
{
    juce::ignoreUnused(midi);
    juce::ScopedNoDenormals noDenormals;

    const int totalIn  = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();

    for (int ch = totalIn; ch < totalOut; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Update engine parameters from current values
    engine.setSustain(parameters.sustain.get());
    engine.setTone(parameters.tone.get());
    engine.setOutputLevel(parameters.outputLevel.get());
    engine.setMode(parameters.mode.getIndex());

    // Bypass smoothing
    const bool bypassed = parameters.bypassed.get();
    bypassTransitionSmoother.setBypass(bypassed);

    const bool fullyBypassed = bypassed && !bypassTransitionSmoother.isTransitioning();
    if (fullyBypassed)
        return;

    // Store dry buffer for crossfade
    bypassTransitionSmoother.setDryBuffer(buffer);

    // Process fuzz
    engine.process(buffer);

    // Crossfade wet/dry according to bypass transition
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
    juce::MemoryOutputStream out(destData, true);
    JsonSerializer::serialize(parameters, out);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    juce::MemoryInputStream in(data, static_cast<size_t>(sizeInBytes), false);
    const auto result = JsonSerializer::deserialize(in, parameters);

    if (result.failed())
        DBG(result.getErrorMessage());

    // Ensure bypass smoother matches restored bypass state
    bypassTransitionSmoother.setBypassForced(parameters.bypassed.get());
}

Parameters& PluginProcessor::getParameterRefs() noexcept {
    return parameters;
}

juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const noexcept {
    return &parameters.bypassed;
}

double PluginProcessor::getSampleRateThreadSafe() const noexcept {
    return currentSampleRate.load();
}

} // namespace fuzz

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new fuzz::PluginProcessor();
}
