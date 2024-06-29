#ifndef AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H
#define AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H

#endif //AUDIO_PLUGIN_EXAMPLE_SHIFTEFFECT_H

using namespace kfr;

class ShiftEffect {
public:
    ShiftEffect();
    univector<float> shiftSignal(univector<float> input);

private:
    const float pi = 2 * acos(0.0);
    const int LEN = 100;
    // or hanning there TODO
    univector<fbase, 1000> hannWindow = window_hann(1000);
};