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
    enum class FFToperation {
        Convolution, IIR
    };

    void doLPC(bool firstBuffers);
    univector<float> autocorrelation(const univector<float>& fromBufer, bool saveFFT);
    univector<float> levinsonDurbin(const univector<float>& ofBuffer);
    univector<float> getResiduals(const univector<float>& ofBuffer);
    float matchPower(const univector<float>& original, const univector<float>& output);
    univector<float> FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients);

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

    univector<float> carrierBuffer1;
    univector<float> carrierBuffer2;
    univector<float> sideChainBuffer1;
    univector<float> sideChainBuffer2;
    univector<float> filteredBuffer1;
    univector<float> filteredBuffer2;

    univector<std::complex<float>> FFTcache;
};