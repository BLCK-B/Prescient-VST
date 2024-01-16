#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

struct ChainSettings {
    float phaserFreq {0}, phaserDepth {0}, phaserCenterFreq {0}, phaserFeedback {0}, phaserMix {0};
    float flangerRatio {0}, flangerLFO {0}, flangerInvert {0}, flangerDepth {0}, flangerSmooth {0};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& treeState);

//==============================================================================
//this class inherits from AudioProcessor and a valuetreestate listener
class MyAudioProcessor final : public juce::AudioProcessor, juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    MyAudioProcessor();
    ~MyAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState treeState {*this, nullptr, "PARAMETERS", createParameterLayout()};

private:

    //listener for parameter change : now irrelevant not used
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients) {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }

    void updatePhaser(const ChainSettings &chainSettings);
    void updateFlanger(const ChainSettings &chainSettings);
    ChainSettings updateFilters();

    juce::dsp::Phaser<float> phaser;
    juce::dsp::Oscillator<float> flangerLFO;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> flangerDelayLine;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyAudioProcessor)
};
