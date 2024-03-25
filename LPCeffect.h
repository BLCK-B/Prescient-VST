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
    void logValues(float input, float output);
    void logAC();
    void matlabLevinson();

    //const int windowSize = std::pow(2, 10); // 2^10 = 1024
    const int windowSize = 1024;
    const int numChannels = 1;
    const int modelOrder = 4;
    int index = 0;

    juce::AudioBuffer<float> inputBuffer;
    std::vector<float> corrCoeff;
    std::vector<float> LPCcoeffs;
    juce::AudioBuffer<float> filteredBuffer;

};