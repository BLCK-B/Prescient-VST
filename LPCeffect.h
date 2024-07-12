#pragma once
#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include "ShiftEffect.cpp"

#ifndef AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#endif //AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
using namespace kfr;

struct ChainSettings {
    float modelorder{70}, flangerBase {0}, passthrough {0}, shift {1}, voice2 {1}, voice3 {1}, stereowidth {1};
    bool enableLPC {true}, preshift {true};
};

class LPCeffect {
public:
    LPCeffect();
    [[nodiscard]] int getLatency() const {
        return windowSize;
    }
    float sendSample(float carrierSample, float voiceSample, const ChainSettings& chainSettings);
    void sendRands(univector<float> rands);

private:
    enum class FFToperation {
        Convolution, IIR
    };

    univector<float> processLPC(const univector<float>& voice, const univector<float>& carrier);
    void processing(univector<float>& overwrite, const univector<float>& voice, const univector<float>& carrier, const ChainSettings& chainSettings);

    univector<float> FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients);
    univector<float> autocorrelation(const univector<float>& fromBufer, bool saveFFT);
    [[nodiscard]] univector<float> levinsonDurbin(const univector<float>& ofBuffer) const;
    univector<float> getResiduals(const univector<float>& ofBuffer);

    static void normalise(univector<float>& input);
    void matchPower(univector<float>& input, const univector<float>& reference) const;

    static void mulVectorWith(univector<float>& vec1, const univector<float>& vec2);
    static void mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);
    static void divVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);

//    const int windowSize = 8192;
//    const int windowSize = 4096;
    const int windowSize = 2048;
//    const int windowSize = 1024;
    univector<fbase, 2048> hannWindow = window_hann(2048);

    int index = 0;
    int index2 = 0;

    const float overlap = 0.5;
    const int overlapSize = round(windowSize * overlap);
    const int hopSize = windowSize - overlapSize;

    int frameModelOrder = 70;

    univector<float> carrierBuffer1;
    univector<float> carrierBuffer2;
    univector<float> sideChainBuffer1;
    univector<float> sideChainBuffer2;
    univector<float> filteredBuffer1;
    univector<float> filteredBuffer2;

    univector<std::complex<float>> FFTcache;

    univector<float> randValues;

    ShiftEffect shiftEffect;
};