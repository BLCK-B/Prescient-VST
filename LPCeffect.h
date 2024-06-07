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
    float sendSample(float sample, float sidechain);

private:
    void doLPC(bool firstBuffers);
    void autocorrelation(const univector<float>& fromBufer);
    univector<float> levinsonDurbin(const univector<float>& ofBuffer);
    univector<float> getResiduals(const univector<float>& ofBuffer);
    univector<float> filterFFTsidechain(const univector<float>& LPC, const univector<float>& e);

//    const int windowSize = 8192;
    const int windowSize = 4096;
//    const int windowSize = 2048;
    univector<fbase, 4096> hannWindow = window_hann(4096);

    const int modelOrder = 70;
    int index = 0;
    int index2 = 0;

    const float overlap = 0.5;
    const int overlapSize = round(windowSize * overlap);
    const int hopSize = windowSize - overlapSize;

    univector<float> corrCoeff;
    univector<float> residualsBuffer;
    univector<float> carrierBuffer1;
    univector<float> carrierBuffer2;
    univector<float> sideChainBuffer1;
    univector<float> sideChainBuffer2;
    univector<float> filteredBuffer1;
    univector<float> filteredBuffer2;
};