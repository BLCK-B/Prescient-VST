#pragma once
#ifndef AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#endif //AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H

class LPCeffect {
public:
    LPCeffect();
    float sendSample(float sample, int channel);
    void overlapAdd();
    float autocorrelation();


private:
    // 30 ms worth of samples in given sample rate
    //const int windowSize = std::round(0.030 * getSampleRate());
    const int windowSize = std::pow(2, 10); // 2^10 = 1024
    const int numChannels = 2;
    const int Nframes = 4;
    int index = 0;

    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> frameBuffer;
    juce::AudioBuffer<float> overlapBuffer;
    juce::dsp::WindowingFunction<float> hannWindow;

};