#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LPCeffect.cpp"
#include "LPCtests.cpp"

//==============================================================================
MyAudioProcessor::MyAudioProcessor() :
    randValues1(32, 0.f),
    randValues2(32, 0.f),
    randValues3(32, 0.f),
    AudioProcessor (BusesProperties()
        .withInput("Input",  juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        )
{
    treeState.addParameterListener("model order", this);
    treeState.addParameterListener("passthrough", this);
    treeState.addParameterListener("enableLPC", this);
    treeState.addParameterListener("shift", this);
    treeState.addParameterListener("voice2", this);
    treeState.addParameterListener("voice3", this);
    treeState.addParameterListener("monostereo", this);
}

MyAudioProcessor::~MyAudioProcessor()
{
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& treeState) {
    ChainSettings settings;
    settings.modelorder = treeState.getRawParameterValue("model order")->load();
    settings.passthrough = treeState.getRawParameterValue("passthrough")->load();
    settings.shift = treeState.getRawParameterValue("shift")->load();
    settings.enableLPC = treeState.getRawParameterValue("enableLPC")->load();
    settings.voice2 = treeState.getRawParameterValue("voice2")->load();
    settings.voice3 = treeState.getRawParameterValue("voice3")->load();
    settings.monostereo = treeState.getRawParameterValue("monostereo")->load();
    return settings;
}

//defining parameters of the plugin
juce::AudioProcessorValueTreeState::ParameterLayout MyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("model order", "model order",
           juce::NormalisableRange<float>(5.f, 125.f, 5.f, 1.f), 70.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("passthrough", "passthrough",
           juce::NormalisableRange<float>(0.f, 2.f, 0.1f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterBool>("enableLPC", "enableLPC", false));

    layout.add(std::make_unique<juce::AudioParameterFloat>("shift", "shift",
           juce::NormalisableRange<float>(0.55f, 2.f, 0.01f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("voice2", "voice2",
            juce::NormalisableRange<float>(0.55f, 2.f, 0.01f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("voice3", "voice3",
            juce::NormalisableRange<float>(0.55f, 2.f, 0.01f, 1.f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("monostereo", "monostereo",
           juce::NormalisableRange<float>(0.f, 1.f, 0.01f, 1.f), 1.f));

    return layout;
}
//listener for parameterID change
void MyAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue) {
    chainSettings = getChainSettings(treeState);
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

//on init
void MyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    chainSettings.modelorder = 70;
    chainSettings.shift = 1.f;
    chainSettings.voice2 = 1.f;
    chainSettings.voice3 = 1.f;
    chainSettings.enableLPC = false;
    chainSettings.monostereo = 1.f;
    setLatencySamples(lpcEffect[0].getLatency());
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec{};
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;

//    LPCtests lpCtests;

//    lpCtests.levinsonDurbinTest();
//    lpCtests.IIRfilterTest();
//    lpCtests.convolutionFFT();
//    lpCtests.shiftSignalTest();
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
        float sampleSideChainL = sideChain.getReadPointer(0)[sample];
        float sampleSideChainR = sideChain.getReadPointer(1)[sample];
//        float sampleSideChainL = 0;
//        float sampleSideChainR = 0;

        // LPC
//        sampleL = sampleR = rand() % 1000 / 1000.0;
//        sampleSideChainL = sampleSideChainR = rand() % 1000 / 1000.0;

//        lpcEffect[0].sendRands({0.3724, 0.1478, 0.2923, -0.1537, 0.2440});
//        lpcEffect[1].sendRands({0.1306, -0.0509, 0.2920, -0.3429, 0.4543});

        float side = 0.5 * (sampleSideChainL - sampleSideChainR);
        float mid = 0.5 * (sampleSideChainL + sampleSideChainR);
        float width = chainSettings.stereowidth;
        float sideNew = width * side;
        float midNew = (2 - width) * mid;
        sampleSideChainL = midNew + sideNew;
        sampleSideChainR = midNew - sideNew;


        ++sampleCount;
        if (sampleCount > 2048) {
            sampleCount = 0;
            for (float &a : randValues1) {
                float rnd = (float) rand();
                a += 0.2 * (rnd / RAND_MAX - 0.5);
            }
            randValues1 /= absmaxof(randValues1);
            for (float &a : randValues2) {
                float rnd = (float) rand();
                a += 0.2 * (rnd / RAND_MAX - 0.5);
            }
            randValues2 /= absmaxof(randValues1);
            for (float &a : randValues3) {
                float rnd = (float) rand();
                a += 0.2 * (rnd / RAND_MAX - 0.5);
            }
            randValues3 /= absmaxof(randValues1);
            lpcEffect[0].sendRands(randValues1, randValues2, randValues3);
            if (chainSettings.preshift)
                lpcEffect[1].sendRands(randValues1, randValues2 - 0.01, randValues3 + 0.02);
            else
                lpcEffect[1].sendRands(randValues1 - 0.01, randValues2 - 0.01, randValues3 + 0.02);
        }

        sampleL = lpcEffect[0].sendSample(sampleL, sampleSideChainL, chainSettings);
        sampleR = lpcEffect[1].sendSample(sampleR, sampleSideChainR, chainSettings);

        float width = chainSettings.monostereo;
        float sideNew = width * 0.5 * (sampleL - sampleR);
        float midNew = (2 - width) * 0.5 * (sampleL + sampleR);
        sampleL = (midNew + sideNew) / 2 - chainSettings.monostereo;
        sampleR = (midNew - sideNew) / 2 - chainSettings.monostereo;

        channelL[sample] = sampleL;
        channelR[sample] = sampleR;
    }
}

//==============================================================================
bool MyAudioProcessor::hasEditor() const {
    return true; //change this to false if you choose to not supply an editor
}

juce::AudioProcessorEditor* MyAudioProcessor::createEditor() {
    //return new MyAudioProcessorEditor (*this);
    //generic UI:
    return new juce::GenericAudioProcessorEditor(*this);
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
