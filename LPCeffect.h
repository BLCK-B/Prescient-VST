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
    void levinsonDurbin();
    void residuals();

    const int windowSize = 4000;
    const int numChannels = 1;
    const int modelOrder = 80;
    int index = 0;

    const float overlap = 0.25;
    const int overlapSize = windowSize * overlap;
    const int hopSize = windowSize - overlapSize;

    std::vector<float> inputBuffer;
    std::vector<float> corrCoeff;
    std::vector<float> LPCcoeffs;
    juce::AudioBuffer<float> filteredBuffer;

    juce::dsp::WindowingFunction<float> hannWindow;
};