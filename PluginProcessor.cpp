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
    //flanger
    settings.flangerRatio = treeState.getRawParameterValue("flanger ratio")->load();
    settings.flangerLFO = treeState.getRawParameterValue("flanger lfo")->load();
    settings.flangerInvert = treeState.getRawParameterValue("flanger invert")->load();
    settings.flangerDepth = treeState.getRawParameterValue("flanger depth")->load();
    settings.flangerSmooth = treeState.getRawParameterValue("flanger smooth")->load();

    return settings;
}

void MyAudioProcessor::updateFlanger(const ChainSettings &chainSettings) {

}

ChainSettings MyAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(treeState);
    updateFlanger(chainSettings);
    return chainSettings;
}

//defining parameters of the plugin
juce::AudioProcessorValueTreeState::ParameterLayout MyAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    //flanger
    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger ratio", "Flanger Ratio",
            juce::NormalisableRange<float>(0.f, 1.f, 0.1f, 1.f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger lfo", "Flanger Lfo",
            juce::NormalisableRange<float>(0.f, 5.f, 0.001f, 0.5f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger invert", "Flanger Invert",
            juce::NormalisableRange<float>(-1.f, 1.f, 2.f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger depth", "Flanger Depth",
            juce::NormalisableRange<float>(0.f, 20.f, 0.1f, 1.f), 0.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("flanger smooth", "Flanger Smooth",
            juce::NormalisableRange<float>(0.5f, 5.f, 0.1f, 1.f), 1.f));

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

    flangerLFO.initialise([](float x) { return std::sin(x); }, 128);
    flangerDelayLine.reset();
    flangerDelayLine.prepare(spec);
    flangerDelayLine.setMaximumDelayInSamples(samplesPerBlock * 5);

    pitchLine.reset();
    pitchLine.prepare(spec);

    flangerLFO.initialise ([] (float x) { return std::sin(x); }, 128);

    //multi-channel buffer containing float audio samples
    pitchBuffer = juce::AudioBuffer<float>(2, 128);
    samplesSet = 0;

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

    flangerLFO.setFrequency (chainSettings.flangerLFO);
    float flangerDepth = chainSettings.flangerDepth;
    float flangerRatio = chainSettings.flangerRatio;
    float flangerInvert = chainSettings.flangerInvert;

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {

            float lfoOutput = flangerLFO.processSample(0.0f);
            float currentDelay = 0 + flangerDepth * lfoOutput;

            float currentSample = buffer.getSample(channel, sample);

            //currentSample = samplesSet;
            //fill buffer to process using FFT
            pitchBuffer.setSample(channel, samplesSet, currentSample);
            samplesSet++;
            if (samplesSet == pitchBuffer.getNumSamples()) {
                //pitchShift(pitchBuffer, channel, chainSettings.flangerSmooth);
                samplesSet = 0;
            }

            //push sample onto delay line
            /*flangerDelayLine.pushSample(channel, currentSample);
            pitchLine.pushSample(channel, currentSample);
            //retrieving a sample from delayline with delay
            int currentDelayInSamples = static_cast<int>(0.5 + currentDelay * getSampleRate() / 1000.0f);
            float delayedSample = flangerDelayLine.popSample(channel, currentDelayInSamples, true) * (1.0 - flangerRatio);
            */
            //float finalSample = flangerRatio * currentSample + delayedSample * flangerInvert;
            //std::cout << finalSample <<" ";
            //buffer.setSample(channel, sample, finalSample);

            float finalSample = fabs(pitchBuffer.getSample(channel, sample % pitchBuffer.getNumSamples()));
            //std::cout << finalSample << "\n";
            buffer.setSample(channel, sample, finalSample);

        }
    }

}

void::MyAudioProcessor::pitchShift(juce::AudioBuffer<float>& pitchBuffer, int channel, float factor)
{
    /*juce::dsp::FFT pitchFourier(32);
    juce::dsp::WindowingFunction<float> hannWindow(32, juce::dsp::WindowingFunction<float>::hann, false);

    float *startOfBuffer = pitchBuffer.getWritePointer(channel);
    size_t sizeOfBuffer = pitchBuffer.getNumSamples();
    hannWindow.multiplyWithWindowingTable(startOfBuffer, sizeOfBuffer);

    pitchFourier.performFrequencyOnlyForwardTransform(startOfBuffer);
    float pitchShiftFactor = factor;
    for (int i = 0; i < 32 / 2; ++i) {
        float real = pitchBuffer.getSample(channel, 2 * i);     //even indices.. magnitude
        float imag = pitchBuffer.getSample(channel, 2 * i + 1); //odd indices..  phase
        float normalizedIndex = static_cast<float>(i) / (32 / 2);
        float phaseShift = normalizedIndex * pitchShiftFactor * 2.0 * juce::MathConstants<float>::pi;
        float newReal = real * std::cos(phaseShift) - imag * std::sin(phaseShift);
        float newImag = real * std::sin(phaseShift) + imag * std::cos(phaseShift);

        pitchBuffer.setSample(channel, 2 * i, newReal);
        pitchBuffer.setSample(channel, 2 * i + 1, newImag);
    }
    pitchFourier.performRealOnlyInverseTransform(startOfBuffer);*/

    /*for (int x = 0; x < pitchBuffer.getNumSamples(); x++) {
        std::cout << pitchBuffer.getSample(channel, x) << "  ";
    }*/
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

