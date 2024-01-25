#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "CustomFFT.cpp"
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
            juce::NormalisableRange<float>(0.1f, 8.f, 0.1f, 1.f), 1.f));

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

    flangerLFO.initialise([](float x) { return std::sin(x); }, 256);
    flangerDelayLine.reset();
    flangerDelayLine.prepare(spec);
    flangerDelayLine.setMaximumDelayInSamples(samplesPerBlock * 5);

    pitchLine.reset();
    pitchLine.prepare(spec);

    //CustomFFT customFFT(32);
    //int inputArray[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,26, 27, 28, 29, 30, 31, 32};
    //customFFT.forwardTransform(inputArray);

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

    float factor = chainSettings.flangerSmooth;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        int blockSizeRemaining = juce::jmin(blockSize, buffer.getNumSamples() - sample);
        std::vector<float*> channelData(totalNumInputChannels);
        for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            channelData[channel] = buffer.getWritePointer(channel, sample);
            //flanger calling
            float currentSample = buffer.getSample(channel, sample);
            flangerLFO.setFrequency (chainSettings.flangerLFO);
            float flangerDepth = chainSettings.flangerDepth;
            float flangerRatio = chainSettings.flangerRatio;
            float flangerInvert = chainSettings.flangerInvert;
            float lfoOutput = flangerLFO.processSample(0.0f);
            float currentDelay = 0 + flangerDepth * lfoOutput;
            float processedSample = flangerEffect(channel, currentSample, currentDelay, flangerInvert, flangerRatio);
            buffer.setSample(channel, sample, processedSample);
        }
        //the block must be twice the size of number of useful data and FFT due to the real and imaginary values after FFT
        juce::dsp::AudioBlock<float> pitchBlock(&channelData[0], totalNumInputChannels, blockSizeRemaining);
        //setting sample values for testing
        /*for (int channel = 0; channel < totalNumInputChannels; ++channel) {
            for (int i = 0; i < blockSizeRemaining / 2; ++i) {
                float* startOfBuffer = pitchBlock.getChannelPointer(channel);
                startOfBuffer[i] = static_cast<float>(i + 1);
            }
        }*/
        if (counter >= blockSize / 2) {
            //pitchShift(pitchBlock, factor);
            counter = 0;
        } else {
            counter++;
        }
    }
}

void::MyAudioProcessor::pitchShift(juce::dsp::AudioBlock<float>& pitchBlock, float factor) {
    juce::dsp::FFT pitchFourier(10); // 2^order
    juce::dsp::WindowingFunction<float> hannWindow(blockSize, juce::dsp::WindowingFunction<float>::hann, false);

    for (int channel = 0; channel < pitchBlock.getNumChannels(); ++channel) {
        float* startOfBuffer = pitchBlock.getChannelPointer(channel);
        size_t sizeOfBuffer = pitchBlock.getNumSamples();
        //applying hann window to avoid spectral leakage
        //hannWindow.multiplyWithWindowingTable(startOfBuffer, sizeOfBuffer);
        //forward FFT
        pitchFourier.performRealOnlyForwardTransform(startOfBuffer, false);
        //pitch shifting: FFT produces interleaved real (magnitudes) and imaginary numbers (phases)
        /*for (int i = 0; i < blockSize / 2; ++i) {
            float real = startOfBuffer[2 * i];
            float imag = startOfBuffer[2 * i + 1];
            //std::cout << real << " " << imag << "\n";
            float originalFund = getSampleRate() / blockSize;
            float newFund = originalFund * factor;
            float phaseFactor = 2 * juce::MathConstants<float>::pi * i * newFund / blockSize;

            float newReal = real * cos(phaseFactor) - imag * sin(phaseFactor);
            float newImag = real * sin(phaseFactor) + imag * cos(phaseFactor);
            //set the new values
            startOfBuffer[2 * i] = newReal;
            startOfBuffer[2 * i + 1] = newImag;
        }*/
        //inverse FFT
        pitchFourier.performRealOnlyInverseTransform(startOfBuffer);

        //printing values
        /*for (int channel = 0; channel < pitchBlock.getNumChannels() / 2; ++channel) {
            float* startOfBuffer = pitchBlock.getChannelPointer(channel);
            for (size_t sample = 0; sample < pitchBlock.getNumSamples(); ++sample) {
                std::cout << startOfBuffer[sample] << "\n";
            }
            std::cout << "\n";
        }*/
    }

    /*
    When we perform a pitch shift, we're essentially multiplying the magnitude of each frequency
    component by a constant factor and adding a phase shift to it. This phase shift is represented by
    the imaginary part of the FFT result.
    1. Calculate the new fundamental frequency (pitch) for each frame (i) in the input by identifying the peak magnitude.
    2. Determine the phase factor
        φ(i, n) = 2π * i * fn / N  ... phase factor
            i is the index of the frequency component (i = 0, ..., N-1), please note: real/imaginary distinction
            N is the FFT size
            fn is the new fundamental frequency
        fn = f0 * factor .. f0 (first harmonic) represents the highest magnitude, factor is user-selected
        f0 = fs / N .. fs sampling frequency
    3.  Apply phase factor to each frequency component by multiplying their real, imaginary parts with phase factor
            Re(k) = Re(k) * cos(φ(k, n)) - Im(k) * sin(φ(k, n))
            Im(k) = Re(k) * sin(φ(k, n)) + Im(k) * cos(φ(k, n))
    */

}

float MyAudioProcessor::flangerEffect(int channel, float currentSample, float currentDelay, float flangerInvert, float flangerRatio) {
    //push sample onto delay line
    flangerDelayLine.pushSample(channel, currentSample);
    pitchLine.pushSample(channel, currentSample);
    //retrieving a sample from delayline with delay
    int currentDelayInSamples = static_cast<int>(0.5 + currentDelay * getSampleRate() / 1000.0f);
    float delayedSample = flangerDelayLine.popSample(channel, currentDelayInSamples, true) * (1.0 - flangerRatio);

    float finalSample = flangerRatio * currentSample + delayedSample * flangerInvert;
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

