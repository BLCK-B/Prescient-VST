#ifndef AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H

#endif //AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H


using namespace kfr;

class ShiftEffect {
public:
    explicit ShiftEffect(const int sampleRate);
    univector<float> shiftSignal(const univector<float>& input, float shift);

private:
    static void mulVectorWith(univector<float>& vec1, const univector<float>& vec2);
    static void mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);
    static univector<float> absOf(const univector<std::complex<float>>& ofBuffer);
    static univector<std::complex<float>> makeComplex(const univector<float>& ofBuffer);
    static univector<std::complex<float>> expComplex(const univector<std::complex<float>>& ofBuffer);
    static univector<float> modulo(const univector<float>& a, float b);

    [[nodiscard]] univector<std::complex<float>> padFFT(const univector<float>& input) const;
    [[nodiscard]] static univector<float> cutIFFT(const univector<std::complex<float>>& input) ;

    const float pi = 2 * acos(0.0);
    int LEN = 0;
    univector<fbase, 512> hannWindowS = window_hann(512);
    univector<fbase, 1024> hannWindowM = window_hann(1024);
    univector<fbase, 2048> hannWindowL = window_hann(2048);
    int synthesisHop = 0;

    enum class WindowLenEnum {
        S, M, L
    };
    WindowLenEnum windowLenEnum;

    std::unordered_map<WindowLenEnum, univector<fbase>> hannWindow = {
            {WindowLenEnum::S, hannWindowS},
            {WindowLenEnum::M, hannWindowM},
            {WindowLenEnum::L, hannWindowL}
    };

    univector<float> psi;
    univector<int> ramp;
    univector<float> omega;
    univector<std::complex<float>> fftGrain;
    univector<float> phi;
    univector<float> previousPhi;
    univector<float> delta;
    univector<float> f1;
    univector<std::complex<float>> corrected;
};