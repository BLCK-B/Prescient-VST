#pragma once
#ifndef AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#endif //AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H

class LPCeffect {
public:
    LPCeffect();
    float sendSample(float sample);

private:
    void doLPC();
    void autocorrelation();
    void levinsonDurbin(int startS);
    void residuals();

    const int windowSize = std::pow(2, 10); // 2^10 = 1024
    const int numChannels = 1;
    const int Nframes = 4;
    const int frameSize = windowSize / Nframes;
    const int modelOrder = 12;
    int index = 0;

    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> frameBuffer;
    juce::AudioBuffer<float> overlapBuffer;
    juce::dsp::WindowingFunction<float> hannWindow;
    std::vector<float> corrCoeff;
    std::vector<float> LPCcoeffs;
    juce::AudioBuffer<float> filteredBuffer;

};