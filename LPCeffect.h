#pragma once
#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>

#ifndef AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#endif //AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
using namespace kfr;

class LPCeffect {
public:
    LPCeffect();
    float sendSample(float sample);

private:
    void doLPC();
    void autocorrelation();
    void levinsonDurbin();
    void filterFFT();

//    const int windowSize = 8192;
    const int windowSize = 4096;
//    const int windowSize = 2048;

    const int modelOrder = 160;
    int index = 0;

    const float overlap = 0.5;
    const int overlapSize = windowSize * overlap;
    const int hopSize = windowSize - overlapSize;

    univector<float> inputBuffer;
    univector<float> corrCoeff;
    univector<float> LPCcoeffs;
    univector<float> filteredBuffer;

    juce::dsp::WindowingFunction<float> hannWindow;
};