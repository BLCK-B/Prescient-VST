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
            juce::NormalisableRange<float>(0.f, 0.9f, 0.05f, 1.f), 0.f));

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

    //multi-channel buffer containing float audio samples
    juce::AudioBuffer<float> pitchBuffer(2, 32);

    int samplesSet = 0;
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {

            float lfoOutput = abs(flangerLFO.processSample(0.0f));
            float currentDelay = 0 + flangerDepth * lfoOutput;

            float currentSample = block.getSample(channel, sample);
            //fill buffer to process using FFT
            //currentSample = 1;
            pitchBuffer.setSample(channel, sample % 32, currentSample);
            samplesSet++;
            if (samplesSet == 32) {
                pitchShift(pitchBuffer, channel);
                samplesSet = 0;
            }

            //push sample onto delay line
            flangerDelayLine.pushSample(channel, currentSample);
            pitchLine.pushSample(channel, currentSample);
            //retrieving a sample from delayline with delay
            int currentDelayInSamples = static_cast<int>(0.5 + currentDelay * getSampleRate() / 1000.0f);
            float delayedSample = flangerDelayLine.popSample(channel, currentDelayInSamples, true) * (1.0 - flangerRatio);

            //float finalSample = flangerRatio * currentSample + delayedSample * flangerInvert;
            //block.setSample(channel, sample, finalSample);

            float finalSample = pitchBuffer.getSample(channel, sample % 32);
            //std::cout << finalSample << "\n";
            block.setSample(channel, sample, finalSample);
        }
    }

}

void::MyAudioProcessor::pitchShift(juce::AudioBuffer<float>& pitchBuffer, int channel)
{
    //object for FFT and IFFT: number of points the FFT will operate on is 2^order
    juce::dsp::FFT pitchFourier(5);
    //window hann functon object
    juce::dsp::WindowingFunction<float> hannWindow(32, juce::dsp::WindowingFunction<float>::hann, false);

    float *startOfBuffer = pitchBuffer.getWritePointer(channel);
    size_t sizeOfBuffer = pitchBuffer.getNumSamples();
    hannWindow.multiplyWithWindowingTable(startOfBuffer, sizeOfBuffer);

    //display hann func
    std::cout << "\nhann window\n";
    for (float g = 0.8; g >= 0; g) {
        std::cout << "\n";
        for (int i = 0; i < 32; i++) {
            if (pitchBuffer.getSample(channel, i) > g)
                std::cout << "|";
            else
                std::cout << "-";
        }
        if (g >= 0.5)
            g -= 0.2;
        else
            g-= 0.1;
    }

    float *bufferFFTStart = pitchBuffer.getWritePointer(channel);
    pitchFourier.performFrequencyOnlyForwardTransform(bufferFFTStart);
    float pitchShiftFactor = 2;
    for (size_t i = 0; i < pitchBuffer.getNumSamples() / 2; ++i) {
        float frequency = i * getSampleRate() / pitchBuffer.getNumSamples();
        float real = pitchBuffer.getSample(channel, 2 * i);     //magnitude
        float imag = pitchBuffer.getSample(channel, 2 * i + 1); //phase
        real *= std::pow(pitchShiftFactor, frequency);
        imag *= std::pow(pitchShiftFactor, frequency);
        pitchBuffer.setSample(channel, 2 * i, real);
        pitchBuffer.setSample(channel, 2 * i + 1, imag);
    }
    pitchFourier.performRealOnlyInverseTransform(bufferFFTStart);

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

