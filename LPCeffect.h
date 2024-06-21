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
    [[nodiscard]] int getLatency() const;
    float sendSample(float sample, float sidechain, int modelorder, bool stutter, float passthrough);

private:
    enum class FFToperation {
        Convolution, IIR
    };

    void doLPC(bool firstBuffers, float passthrough);
    univector<float> autocorrelation(const univector<float>& fromBufer, bool saveFFT);
    [[nodiscard]] univector<float> levinsonDurbin(const univector<float>& ofBuffer) const;
    univector<float> getResiduals(const univector<float>& ofBuffer);
    [[nodiscard]] float matchPower(const univector<float>& original, const univector<float>& output) const;
    univector<float> FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients);
    static void mulVectorWith(univector<float>& vec1, const univector<float>& vec2);
    static void mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);
    static void divVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);

//    const int windowSize = 8192;
//    const int windowSize = 4096;
    const int windowSize = 2048;
    univector<fbase, 2048> hannWindow = window_hann(2048);

    int index = 0;
    int index2 = 0;

    const float overlap = 0.5;
    const int overlapSize = round(windowSize * overlap);
    const int hopSize = windowSize - overlapSize;

    int tempmodelorder = 70;

    univector<float> carrierBuffer1;
    univector<float> carrierBuffer2;
    univector<float> sideChainBuffer1;
    univector<float> sideChainBuffer2;
    univector<float> filteredBuffer1;
    univector<float> filteredBuffer2;
    univector<float> tempBuffer1;
    univector<float> tempBuffer2;

    bool tempFill1 = false;
    bool tempFill2 = false;

    univector<std::complex<float>> FFTcache;
};