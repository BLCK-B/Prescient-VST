#include <juce_audio_formats/codecs/flac/compat.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MyAudioProcessor::MyAudioProcessor()
    : AudioProcessor (BusesProperties()
        #if ! JucePlugin_IsMidiEffect
        #if ! JucePlugin_IsSynth
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        #endif
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
        #endif
    )
{

}

MyAudioProcessor::~MyAudioProcessor()
{

}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& treeState) {
    ChainSettings settings;
    //phaser
    settings.phaserFreq = treeState.getRawParameterValue("phaser freq")->load();
    settings.phaserDepth = treeState.getRawParameterValue("phaser depth")->load();
    settings.phaserCenterFreq = treeState.getRawParameterValue("phaser centerfreq")->load();
    settings.phaserFeedback = treeState.getRawParameterValue("phaser feedback")->load();
    settings.phaserMix = treeState.getRawParameterValue("phaser mix")->load();
    //flanger
    settings.flangerRatio = treeState.getRawParameterValue("flanger ratio")->load();
    settings.flangerLFO = treeState.getRawParameterValue("flanger lfo")->load();
    settings.flangerInvert = treeState.getRawParameterValue("flanger invert")->load();
    settings.flangerDepth = treeState.getRawParameterValue("flanger depth")->load();

    return settings;
}

void MyAudioProcessor::updatePhaser(const ChainSettings &chainSettings) {
    phaser.setRate(chainSettings.phaserFreq);
    phaser.setDepth(chainSettings.phaserDepth);
    phaser.setCentreFrequency(chainSettings.phaserCenterFreq);
    phaser.setFeedback(chainSettings.phaserFeedback);
    phaser.setMix(chainSettings.phaserMix);
}

void MyAudioProcessor::updateFlanger(const ChainSettings &chainSettings) {

}

ChainSettings MyAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(treeState);
    updatePhaser(chainSettings);
    updateFlanger(chainSettings);
    return chainSettings;
}

//defining parameters of the plugin
juce::AudioProcessorValueTreeState::ParameterLayout MyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    //phaser
    layout.add(std::make_unique<juce::AudioParameterFloat>("phaser freq", "Phaser Freq",
                            juce::NormalisableRange<float>(0.f, 50.f, 1.f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("phaser depth", "Phaser Depth",
            juce::NormalisableRange<float>(0.f, 1.f, 0.1f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("phaser centerfreq", "Phaser Centerfreq",
            juce::NormalisableRange<float>(0.f, 20000.f, 1.f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("phaser feedback", "Phaser Feedback",
            juce::NormalisableRange<float>(-0.8f, 0.8f, 0.1f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("phaser mix", "Phaser Mix",
            juce::NormalisableRange<float>(0.f, 1.f, 0.1f, 1.f), 0.f));
    //flanger
    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger ratio", "Flanger Ratio",
            juce::NormalisableRange<float>(0.f, 1.f, 0.1f, 1.f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger lfo", "Flanger Lfo",
            juce::NormalisableRange<float>(0.f, 5.f, 0.001f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger invert", "Flanger Invert",
            juce::NormalisableRange<float>(-1.f, 1.f, 2.f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger depth", "Flanger Depth",
            juce::NormalisableRange<float>(0.f, 20.f, 0.5f, 1.f), 0.f));

    return layout;
}
//listener for parameterID change
void MyAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
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
    juce::ignoreUnused (sampleRate, samplesPerBlock);
    juce::dsp::ProcessSpec spec{};
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    spec.sampleRate = sampleRate;

    phaser.prepare(spec);

    flangerLFO.initialise([](float x) { return std::sin(x); }, 128);
    flangerDelayLine.reset();
    flangerDelayLine.prepare(spec);
    flangerDelayLine.setMaximumDelayInSamples(samplesPerBlock * 5);

    flangerLFO.initialise ([] (float x) { return std::sin(x); }, 128);

    updateFilters();
}

void MyAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any spare memory, etc.
}

bool MyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
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
    auto chainSettings = updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);

    flangerLFO.setFrequency (chainSettings.flangerLFO);
    float flangerDepth = chainSettings.flangerDepth;
    float flangerRatio = chainSettings.flangerRatio;
    float flangerInvert = chainSettings.flangerInvert;

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {

            float lfoOutput = abs(flangerLFO.processSample(0.0f));
            float currentDelay = 0 + flangerDepth * lfoOutput;

            float currentSample = block.getSample(channel, sample);
            flangerDelayLine.pushSample(channel, currentSample);
            flangerDelayLine.setDelay(currentDelay);
            //retrieving a sample from delayline with delay
            float delayedSample = flangerDelayLine.popSample(channel) * 0.5 * (1.0 - flangerRatio);
            block.setSample(channel, sample, currentSample * 0.5 + flangerInvert * delayedSample);

        }
    }

    //phaser.process(block);
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
//storing parameters in memory block
//destination data from DAW
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

