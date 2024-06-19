#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "LPCeffect.cpp"
#include "LPCtests.cpp"
//==============================================================================
MyAudioProcessor::MyAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput("Input",  juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        )
{
    treeState.addParameterListener("flanger ratio", this);
    treeState.addParameterListener("flanger lfo", this);
    treeState.addParameterListener("flanger invert", this);
    treeState.addParameterListener("flanger depth", this);
    treeState.addParameterListener("flanger base", this);
    treeState.addParameterListener("pitch shift", this);
}

MyAudioProcessor::~MyAudioProcessor()
{
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& treeState) {
    ChainSettings settings;
    settings.flangerRatio = treeState.getRawParameterValue("flanger ratio")->load();
    settings.flangerLFO = treeState.getRawParameterValue("flanger lfo")->load();
    settings.flangerInvert = treeState.getRawParameterValue("flanger invert")->load();
    settings.flangerDepth = treeState.getRawParameterValue("flanger depth")->load();
    settings.flangerBase = treeState.getRawParameterValue("flanger base")->load();
    settings.pitchShift = treeState.getRawParameterValue("pitch shift")->load();

    return settings;
}

//defining parameters of the plugin
juce::AudioProcessorValueTreeState::ParameterLayout MyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger ratio", "Flanger Ratio",
            juce::NormalisableRange<float>(0.f, 1.f, 0.1f, 1.f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger lfo", "Flanger Lfo",
            juce::NormalisableRange<float>(0.f, 5.f, 0.001f, 0.5f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger invert", "Flanger Invert",
            juce::NormalisableRange<float>(-1.f, 1.f, 2.f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger depth", "Flanger Depth",
            juce::NormalisableRange<float>(0.f, 25.f, 0.1f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger base", "Flanger Base",
            juce::NormalisableRange<float>(0.f, 25.f, 0.1f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("pitch shift", "Pitch Shift",
            juce::NormalisableRange<float>(0.1f, 4.f, 0.01f, 0.5f), 1.f));

    return layout;
}
//listener for parameterID change
void MyAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    chainSettings = getChainSettings(treeState);
    flangerLFO.setFrequency (chainSettings.flangerLFO);
}

//==============================================================================
const juce::String MyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MyAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MyAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MyAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MyAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MyAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs, so this should be at least 1, even if you're not really implementing programs.
}

int MyAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MyAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String MyAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void MyAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
//on init
void MyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    setLatencySamples(4096);
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec{};
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;

    flangerLFO.initialise([](float x) { return std::sin(x); }, 256);
    flangerDelayLine.reset();
    flangerDelayLine.prepare(spec);
    flangerDelayLine.setMaximumDelayInSamples(samplesPerBlock * 5);

    LPCtests lpCtests;

//    lpCtests.levinsonDurbinTest();
//    lpCtests.IIRfilterTest();
//    lpCtests.convolutionFFT();
}

void MyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any spare memory, etc.
}

bool MyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
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
        if (abs(sampleL) < 0.000000001 || abs(sampleR) < 0.000000001)
            continue;

        // flanger
        //channelL[sample] = flangerEffect(sampleL);
        //channelR[sample] = flangerEffect(sampleR);

        // LPC
//        sampleL = sampleR = rand() % 1000 / 1000.0;
//        sampleSideChainL = sampleSideChainR = rand() % 1000 / 1000.0;
//        std::cout<<"orig: "<< std::fixed << setprecision(5) <<sampleL;

        sampleL = lpcEffect[0].sendSample(sampleL, sampleSideChainL);
        sampleR = lpcEffect[1].sendSample(sampleR, sampleSideChainR);

//        std::cout<<" out: "<< std::fixed << setprecision(5) << sampleL<<"\n";

        channelL[sample] = sampleL;
        channelR[sample] = sampleR;
    }
}

float MyAudioProcessor::flangerEffect(float currentSample) {
    float depth = chainSettings.flangerDepth;
    float ratio = chainSettings.flangerRatio;
    float invert = chainSettings.flangerInvert;
    float base = chainSettings.flangerBase;
    float lfoOutput = flangerLFO.processSample(0.0f);

    float currentDelay = base + depth * lfoOutput;

    // push sample onto delay line
    flangerDelayLine.pushSample(0, currentSample);
    // retrieving a sample from delayline with delay
    float currentDelayInSamples = currentDelay * getSampleRate() / 1000.f;
    float delayedSample = flangerDelayLine.popSample(0, currentDelayInSamples, true) * (1.f - ratio);

    float finalSample = ratio * currentSample + delayedSample * invert;
    return finalSample;
}

//==============================================================================
bool MyAudioProcessor::hasEditor() const
{
    return true; //change this to false if you choose to not supply an editor
}

juce::AudioProcessorEditor* MyAudioProcessor::createEditor()
{
    //return new MyAudioProcessorEditor (*this);
    //generic UI:
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
// storing parameters in memory block
// destination data from DAW
void MyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ignoreUnused (destData);
    juce::MemoryOutputStream stream(destData, false);
    treeState.state.writeToStream(stream);
}
//when plugin is opened - restore saved parameters
void MyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    juce::ignoreUnused (data, sizeInBytes);
    juce::ValueTree tree = juce::ValueTree::readFromData(data, size_t(sizeInBytes));
    treeState.state = tree;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MyAudioProcessor();
}
