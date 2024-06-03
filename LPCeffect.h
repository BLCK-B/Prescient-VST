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
    univector<fbase, 4096> hannWindow = window_hann(windowSize);

    const int modelOrder = 100;
    int index = 0;
    int index2 = 0;
    int activeBuffer = 0;

    const float overlap = 0.9;
    const int overlapSize = round(windowSize * overlap);
    const int hopSize = windowSize - overlapSize;

    univector<float> inputBuffer;
    univector<float> inputBuffer2;
    univector<float> corrCoeff;
    univector<float> LPCcoeffs;
    univector<float> filteredBuffer;
    univector<float> filteredBuffer2;

};