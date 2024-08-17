#pragma once
#include "PluginProcessor.h"
//==============================================================================
class MyAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MyAudioProcessorEditor (MyAudioProcessor&);
    ~MyAudioProcessorEditor() override;

    //==============================================================================
    void resized() override;
private:
    using Resource = juce::WebBrowserComponent::Resource;
    static std::optional<Resource> getResource(const juce::String& url) ;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MyAudioProcessor& processorRef;

    juce::WebSliderRelay modelOrderRelay;
    juce::WebSliderRelay passthroughRelay;
    juce::WebSliderRelay shiftVoice1Relay;
    juce::WebSliderRelay shiftVoice2Relay;
    juce::WebSliderRelay shiftVoice3Relay;
    juce::WebSliderRelay monostereoRelay;
    juce::WebSliderRelay enableLPCRelay;

    juce::WebBrowserComponent webView;

    juce::WebSliderParameterAttachment modelOrderSliderAttachment;
    juce::WebSliderParameterAttachment passthroughSliderAttachment;
    juce::WebSliderParameterAttachment shiftVoice1SliderAttachment;
    juce::WebSliderParameterAttachment shiftVoice2SliderAttachment;
    juce::WebSliderParameterAttachment shiftVoice3SliderAttachment;
    juce::WebSliderParameterAttachment monostereoSliderAttachment;
    juce::WebSliderParameterAttachment enableLPCSliderAttachment;

    const juce::ParameterID modelOrder{"model order", 1};
    const juce::ParameterID passthrough{"passthrough", 1};
    const juce::ParameterID shiftVoice1{"shiftVoice1", 1};
    const juce::ParameterID shiftVoice2{"shiftVoice2", 1};
    const juce::ParameterID shiftVoice3{"shiftVoice3", 1};
    const juce::ParameterID monostereo{"monostereo", 1};
    const juce::ParameterID enableLPC{"enableLPC", 1};


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MyAudioProcessorEditor)
};
