#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "LPCeffect.h"
//==============================================================================
class MyAudioProcessor final : public juce::AudioProcessor {
public:
    MyAudioProcessor();
    ~MyAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState treeState;

private:
    std::atomic<float>* modelOrder{nullptr};
    std::atomic<float>* passthrough{nullptr};
    std::atomic<float>* shiftVoice1{nullptr};
    std::atomic<float>* shiftVoice2{nullptr};
    std::atomic<float>* shiftVoice3{nullptr};
    std::atomic<float>* monostereo{nullptr};
    std::atomic<float>* enableLPC{nullptr};

    template<int Index, typename ChainType, typename CoefficientType>
    void update(ChainType& chain, const CoefficientType& coefficients) {
        updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
        chain.template setBypassed<Index>(false);
    }

    LPCeffect lpcEffect[2] = {
            LPCeffect(static_cast<int>(getSampleRate())),
            LPCeffect(static_cast<int>(getSampleRate()))
    };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyAudioProcessor)
};
