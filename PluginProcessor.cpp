#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LPCeffect.cpp"

//==============================================================================
MyAudioProcessor::MyAudioProcessor() :
    AudioProcessor (BusesProperties()
        .withInput("Input",  juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
    ),
    treeState{*this, nullptr, "PARAMETERS", createParameterLayout()},
    modelOrder{treeState.getRawParameterValue("model order")},
    passthrough{treeState.getRawParameterValue("passthrough")},
    shiftVoice1{treeState.getRawParameterValue("shiftVoice1")},
    shiftVoice2{treeState.getRawParameterValue("shiftVoice2")},
    shiftVoice3{treeState.getRawParameterValue("shiftVoice3")},
    monostereo{treeState.getRawParameterValue("monostereo")},
    enableLPC{treeState.getRawParameterValue("enableLPC")}
{ }

MyAudioProcessor::~MyAudioProcessor() { }

//defining parameters of the plugin
juce::AudioProcessorValueTreeState::ParameterLayout MyAudioProcessor::createParameterLayout() {
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterFloat>("model order", "model order",
           NormalisableRange<float>(6.f, 76.f, 1.f, 1.f), 76.f));

    layout.add(std::make_unique<AudioParameterFloat>("passthrough", "passthrough",
           NormalisableRange<float>(0.f, 1.f, 0.1f, 1.f), 1.f));

    layout.add(std::make_unique<AudioParameterFloat>("enableLPC", "enableLPC",
            NormalisableRange<float>(0.f, 1.f, 1.f, 1.f), 0.f));

    layout.add(std::make_unique<AudioParameterFloat>("shiftVoice1", "shiftVoice1",
           NormalisableRange<float>(0.6f, 2.f, 0.01f, 0.55f), 1.f));

    layout.add(std::make_unique<AudioParameterFloat>("shiftVoice2", "shiftVoice2",
            NormalisableRange<float>(0.6f, 2.f, 0.01f, 0.55f), 1.f));

    layout.add(std::make_unique<AudioParameterFloat>("shiftVoice3", "shiftVoice3",
            NormalisableRange<float>(0.6f, 2.f, 0.01f, 0.55f), 1.f));

    layout.add(std::make_unique<AudioParameterFloat>("monostereo", "monostereo",
           NormalisableRange<float>(0.f, 1.f, 0.01f, 1.f), 0.5f));

    return layout;
}

//==============================================================================
const juce::String MyAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool MyAudioProcessor::acceptsMidi() const {
    return false;
}

bool MyAudioProcessor::producesMidi() const {
    return false;
}

bool MyAudioProcessor::isMidiEffect() const {
    return false;
}

double MyAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int MyAudioProcessor::getNumPrograms() {
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs, so this should be at least 1, even if you're not really implementing programs.
}

int MyAudioProcessor::getCurrentProgram() {
    return 0;
}

void MyAudioProcessor::setCurrentProgram (int index) {
    juce::ignoreUnused (index);
}

const juce::String MyAudioProcessor::getProgramName (int index) {
    juce::ignoreUnused (index);
    return {};
}

void MyAudioProcessor::changeProgramName (int index, const juce::String& newName) {
    juce::ignoreUnused (index, newName);
}

void MyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    setLatencySamples(lpcEffect[0].getLatency());
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec{};
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;
}

void MyAudioProcessor::releaseResources() {

}

bool MyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const {
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    //=================================================================================
    auto mainInput = getBusBuffer (buffer, true, 0);
    auto sideChain = getBusBuffer (buffer, true, 1);

    float* channelL = mainInput.getWritePointer(0);
    float* channelR = mainInput.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float sampleL = channelL[sample];
        float sampleR = channelR[sample];
        // standalone cant use these pointers
        float sampleSideChainL = sideChain.getReadPointer(0)[sample];
        float sampleSideChainR = sideChain.getReadPointer(1)[sample];
//        float sampleSideChainL = 0;
//        float sampleSideChainR = 0;
        //==============================================

        sampleL = lpcEffect[0].sendSample(sampleL, sampleSideChainL, *modelOrder,
                                          *shiftVoice1, *shiftVoice2, *shiftVoice3, *enableLPC > 0.99, *passthrough);
        sampleR = lpcEffect[1].sendSample(sampleR, sampleSideChainR, *modelOrder,
                                          *shiftVoice1, *shiftVoice2, *shiftVoice3, *enableLPC > 0.99, *passthrough);

        float width = *monostereo;
        auto sideNew = width * 0.5 * (sampleL - sampleR);
        auto midNew = (2 - width) * 0.5 * (sampleL + sampleR);
        sampleL = static_cast<float>(midNew + sideNew);
        sampleR = static_cast<float>(midNew - sideNew);

        channelL[sample] = sampleL;
        channelR[sample] = sampleR;
    }
}

//==============================================================================
bool MyAudioProcessor::hasEditor() const {
    return true; // false if you choose to not supply an editor
}

juce::AudioProcessorEditor* MyAudioProcessor::createEditor() {
    return new MyAudioProcessorEditor (*this);
    //generic UI:
//    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
// storing parameters in memory block
// destination data from DAW
void MyAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    juce::ignoreUnused (destData);
    juce::MemoryOutputStream stream(destData, false);
    treeState.state.writeToStream(stream);
}
//when plugin is opened - restore saved parameters
void MyAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
    juce::ignoreUnused (data, sizeInBytes);
    juce::ValueTree tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    treeState.state = tree;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new MyAudioProcessor();
}
