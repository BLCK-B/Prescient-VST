#pragma once
#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
//#include "ShiftEffect.cpp"

#ifndef AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
#endif //AUDIO_PLUGIN_EXAMPLE_LPCEFFECT_H
using namespace kfr;

struct ChainSettings {
    float modelorder{70}, passthrough {0}, shiftVoice1 {1}, shiftVoice2 {1}, shiftVoice3 {1}, monostereo {1}, enableLPC {1};
};

class LPCeffect {
public:
    explicit LPCeffect(const int sampleRate);
    [[nodiscard]] int getLatency() const {
        return windowSize;
    }
    float sendSample(float carrierSample, float voiceSample, const ChainSettings& chainSettings);

private:
    enum class FFToperation {
        Convolution, IIR
    };

    univector<float> processLPC(const univector<float>& voice, const univector<float>& carrier);
    void processing(univector<float>& overwrite, const univector<float>& voice, const univector<float>& carrier, const ChainSettings& chainSettings);

    univector<float> FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients);
    static univector<float> autocorrelation(const univector<float>& fromBufer);
    [[nodiscard]] univector<float> levinsonDurbin(const univector<float>& ofBuffer) const;
    univector<float> getResiduals(const univector<float>& ofBuffer);

    void matchPower(univector<float>& input, const univector<float>& reference) const;

    static void mulVectorWith(univector<float>& vec1, const univector<float>& vec2);
    static void mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);
    static void divVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);

    int windowSize = 0;

    univector<fbase, 1024> hannWindowS = window_hann(1024);
    univector<fbase, 2048> hannWindowM = window_hann(2048);
    univector<fbase, 4096> hannWindowL = window_hann(4096);

    enum class WindowSizeEnum {
        S, M, L
    };
    WindowSizeEnum windowSizeEnum;

    std::unordered_map<WindowSizeEnum, univector<fbase>> hannWindow = {
            {WindowSizeEnum::S, hannWindowS},
            {WindowSizeEnum::M, hannWindowM},
            {WindowSizeEnum::L, hannWindowL}
    };

    int index1 = 0;
    int index2 = 0;

    const float overlap = 0.5;
    int overlapSize = 0;
    int hopSize = 0;

    int frameModelOrder = 70;

    univector<float> carrierBuffer1;
    univector<float> carrierBuffer2;
    univector<float> sideChainBuffer1;
    univector<float> sideChainBuffer2;
    univector<float> filteredBuffer1;
    univector<float> filteredBuffer2;

//    ShiftEffect* shiftEffect;
};