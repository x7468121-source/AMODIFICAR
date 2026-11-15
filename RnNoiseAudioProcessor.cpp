#include "RnNoiseAudioProcessor.h"
#include "RnNoisePluginEditor.h"

#include "common/RnNoiseCommonPlugin.h"

//==============================================================================
RnNoiseAudioProcessor::RnNoiseAudioProcessor()
        : AudioProcessor(BusesProperties()
                                 .withInput("Input", juce::AudioChannelSet::namedChannelSet(NUM_CHANNELS), true)
                                 .withOutput("Output", juce::AudioChannelSet::namedChannelSet(NUM_CHANNELS), true)
), m_parameters(*this, nullptr, juce::Identifier("RNNoise"),
                {
                        std::make_unique<juce::AudioParameterFloat>("vad_threshold",
                                                                    "VAD Threshold",
                                                                    0.0f,
                                                                    1.0f,
                                                                    0.6f),
                        std::make_unique<juce::AudioParameterInt>("vad_grace_period",
                                                                  "VAD Grace Period (10ms per unit)",
                                                                  0,
                                                                  500,
                                                                  20),
                        std::make_unique<juce::AudioParameterInt>("vad_retroactive_grace_period",
                                                                  "Retroactive VAD Grace Period (10ms per unit)",
                                                                  0,
                                                                  10,
                                                                  0)
                }) {
    m_vadThresholdParam = (juce::AudioParameterFloat *) m_parameters.getParameter("vad_threshold");
    m_vadGracePeriodParam = (juce::AudioParameterInt *) m_parameters.getParameter("vad_grace_period");
    m_vadRetroactiveGracePeriodParam = (juce::AudioParameterInt *) m_parameters.getParameter(
            "vad_retroactive_grace_period");
}

RnNoiseAudioProcessor::~RnNoiseAudioProcessor() = default;

//==============================================================================
const juce::String RnNoiseAudioProcessor::getName() const { return JucePlugin_Name; }
bool RnNoiseAudioProcessor::acceptsMidi() const { return false; }
bool RnNoiseAudioProcessor::producesMidi() const { return false; }
double RnNoiseAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int RnNoiseAudioProcessor::getNumPrograms() { return 1; }
int RnNoiseAudioProcessor::getCurrentProgram() { return 0; }
void RnNoiseAudioProcessor::setCurrentProgram(int index) { juce::ignoreUnused(index); }
const juce::String RnNoiseAudioProcessor::getProgramName(int index) { juce::ignoreUnused(index); return {}; }
void RnNoiseAudioProcessor::changeProgramName(int index, const juce::String &newName) { juce::ignoreUnused(index, newName); }

//==============================================================================
void RnNoiseAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::ignoreUnused(sampleRate, samplesPerBlock);
    inputLevel.store(0.0f, std::memory_order_relaxed);
    outputLevel.store(0.0f, std::memory_order_relaxed);
    m_rnNoisePlugin = std::make_shared<RnNoiseCommonPlugin>(static_cast<uint32_t>(getTotalNumInputChannels()));
    m_rnNoisePlugin->init();
}

void RnNoiseAudioProcessor::releaseResources() {
    m_rnNoisePlugin.reset();
}

bool RnNoiseAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const {
    if (layouts.getMainInputChannelSet() == juce::AudioChannelSet::disabled()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::disabled())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void RnNoiseAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                         juce::MidiBuffer &midiMessages) {
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Measure the input peak before RNNoise processes the block.
    float inputPeak = 0.0f;
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        inputPeak = juce::jmax(inputPeak, buffer.getMagnitude(channel, 0, numSamples));

    const float *in[8] = {nullptr};
    float *out[8] = {nullptr};
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        in[channel] = buffer.getReadPointer(channel);
        out[channel] = buffer.getWritePointer(channel);
    }

    if (m_rnNoisePlugin != nullptr) {
        m_rnNoisePlugin->process(in, out, static_cast<size_t>(numSamples), m_vadThresholdParam->get(),
                                 static_cast<uint32_t>(m_vadGracePeriodParam->get()),
                                 static_cast<uint32_t>(m_vadRetroactiveGracePeriodParam->get()));
    }

    // Measure the processed output peak. These atomics are read by the UI timer.
    float outputPeak = 0.0f;
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
        outputPeak = juce::jmax(outputPeak, buffer.getMagnitude(channel, 0, numSamples));

    inputLevel.store(inputPeak, std::memory_order_relaxed);
    outputLevel.store(outputPeak, std::memory_order_relaxed);
}

//==============================================================================
bool RnNoiseAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *RnNoiseAudioProcessor::createEditor() {
    return new RnNoiseAudioProcessorEditor(*this, m_parameters);
}

void RnNoiseAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto state = m_parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RnNoiseAudioProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (!xml || !xml->hasTagName(m_parameters.state.getType()))
        return;
    m_parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new RnNoiseAudioProcessor();
}
