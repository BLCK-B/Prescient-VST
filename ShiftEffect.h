#ifndef AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H

#endif //AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H

using namespace kfr;

class ShiftEffect {
public:
    ShiftEffect();
    univector<float> shiftSignal(const univector<float>& input);

private:
    univector<float> absOf(const univector<std::complex<float>>& ofBuffer);
    univector<std::complex<float>> makeComplex(const univector<float>& ofBuffer);
    univector<std::complex<float>> expComplex(const univector<std::complex<float>>& ofBuffer);
    univector<float> modulo(const univector<float>& a, float b);

    const float pi = 2 * acos(0.0);
    const int LEN = 1000;
    // or hanning there TODO
    univector<fbase, 1000> hannWindow = window_hann(1000);
};