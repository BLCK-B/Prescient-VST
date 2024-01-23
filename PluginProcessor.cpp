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

    flangerLFO.initialise([](float x) { return std::sin(x); }, 128);
    flangerDelayLine.reset();
    flangerDelayLine.prepare(spec);
    flangerDelayLine.setMaximumDelayInSamples(samplesPerBlock * 5);

    pitchLine.reset();
    pitchLine.prepare(spec);

    flangerLFO.initialise ([] (float x) { return std::sin(x); }, 128);

    //multi-channel buffer containing float audio samples
    pitchBuffer.setSize(2, 64);

    samplesInBuffer = 0;

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

    flangerLFO.setFrequency (chainSettings.flangerLFO);
    float flangerDepth = chainSettings.flangerDepth;
    float flangerRatio = chainSettings.flangerRatio;
    float flangerInvert = chainSettings.flangerInvert;
    float factor = chainSettings.flangerSmooth;

    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {

            float lfoOutput = flangerLFO.processSample(0.0f);
            float currentDelay = 0 + flangerDepth * lfoOutput;

            float currentSample = buffer.getSample(channel, sample);
            //currentSample = (float) samplesInBuffer;

            pitchBuffer.setSample(channel, samplesInBuffer, currentSample);

            samplesInBuffer++;
            if (samplesInBuffer >= pitchSamples / 2) {
               /*
                for (float x = 0; x < pitchSamples/2; ++x) {
                    pitchBuffer.setSample(channel, x, x + 1);
                }*/
                for (int d = pitchSamples / 2; d < pitchSamples; d++) {
                    pitchBuffer.setSample(channel, d, 0);
                }
                samplesInBuffer = 0;
                pitchShift(pitchBuffer, channel, factor);
            }
            //push sample onto delay line
            /*flangerDelayLine.pushSample(channel, currentSample);
            pitchLine.pushSample(channel, currentSample);
            //retrieving a sample from delayline with delay
            int currentDelayInSamples = static_cast<int>(0.5 + currentDelay * getSampleRate() / 1000.0f);
            float delayedSample = flangerDelayLine.popSample(channel, currentDelayInSamples, true) * (1.0 - flangerRatio);
            */
            //float finalSample = flangerRatio * currentSample + delayedSample * flangerInvert;
            //buffer.setSample(channel, sample, finalSample);

            float finalSample = pitchBuffer.getSample(channel, samplesInBuffer);
            //std::cout << "resulting " << finalSample << "\n";
            buffer.setSample(channel, sample, finalSample);
        }
    }

}

void::MyAudioProcessor::pitchShift(juce::AudioBuffer<float>& pitchBuffer, int channel, float factor) {
    //commented-out attempt using STFT but I just cant get it to work
    juce::dsp::FFT pitchFourier(5); // 2^order
    juce::dsp::WindowingFunction<float> hannWindow(pitchSamples, juce::dsp::WindowingFunction<float>::hann, false);

    /*std::cout << "\n\n this is input samples\n";
    for (int x = 0; x < pitchSamples; x++) {
        std::cout << pitchBuffer.getSample(channel, x) << "  ";
    }*/

    float *startOfBuffer = pitchBuffer.getWritePointer(channel);
    size_t sizeOfBuffer = pitchBuffer.getNumSamples();
    //hannWindow.multiplyWithWindowingTable(startOfBuffer, sizeOfBuffer);

    //std::cout << "\n now onto after FFT \n";
    /*
    The size of the array passed in must be 2 * getSize(), and the first half
    should contain your raw input sample data. On return, if
    onlyCalculateNonNegativeFrequencies is false, the array will contain size
    complex real + imaginary parts data interleaved.
    */
    pitchFourier.performRealOnlyForwardTransform(startOfBuffer, false);
    /*
     To change the perceived pitch while maintaining the original length and timbre, we need to modify the phase factor applied to these components.
     By modifying the phases, we are essentially altering the timing relationship between the components.
     Calculate a new phase angle based on the pitch shift and apply it as follows:
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
    //std::cout << "these are after FFT\n";
    for (int i = 0; i < pitchSamples / 2; ++i) {                        //i.. frequency bins
        float real = pitchBuffer.getSample(channel, 2 * i);     //even indices.. magnitude
        float imag = pitchBuffer.getSample(channel,2 * i + 1); //odd indices... phase .. after IFFT manifests in time delay
        //std::cout << real << " " << imag << "\n";
        //here we assume the highest magnitude is always the first
        float originalMaxMag = pitchBuffer.getSample(channel, 0);
        float originalFund = getSampleRate() / pitchSamples;
        float newFund = originalFund * factor;
        float phaseFactor = 2 * juce::MathConstants<float>::pi * i * newFund / pitchSamples;

        float newReal = real * cos(phaseFactor) - imag * sin(phaseFactor);
        float newImag = real * sin(phaseFactor) + imag * cos(phaseFactor);
        //set the new values
        pitchBuffer.setSample(channel, 2 * i, newReal);
        pitchBuffer.setSample(channel, 2 * i + 1, newImag);
    }
    pitchFourier.performRealOnlyInverseTransform(startOfBuffer);
    /*std::cout << "\n after IFFT \n";
    for (int i = 0; i < pitchSamples / 2; ++i) {                        //i.. frequency bins
        float real = pitchBuffer.getSample(channel, 2 * i);     //even indices.. magnitude
        float imag = pitchBuffer.getSample(channel,2 * i + 1); //odd indices... phase .. after IFFT manifests in time delay
        std::cout << real << " " << imag << "\n";
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

