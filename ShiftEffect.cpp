#include "ShiftEffect.h"

ShiftEffect::ShiftEffect() {
}
// TODO: KFR ACCEPTS ONLY % 2 - move possible to header
univector<float> ShiftEffect::shiftSignal(const univector<float>& input) {
    int analysisHop = 200;
    int synthesisHop = 150;

    univector<float> psi(LEN, 0.f);
    univector<float> previousPhi(LEN, 0.f);
    univector<int> ramp(LEN);
    for (int i = 0; i < LEN; ++i)
        ramp[i] = std::floor((i * analysisHop) / synthesisHop) + 1;
    int resampledLEN = std::floor(LEN * analysisHop / synthesisHop);
        univector<float> x(resampledLEN, 0.f);
    for (int i = 0; i < resampledLEN; ++i)
            x[i] = (1 + (float) i * LEN / resampledLEN);
    univector<float> output(input.size() + resampledLEN, 0.f);
    univector<float> omega;
    for (int i = 0; i < LEN; ++i)
        omega.push_back(2 * pi * analysisHop * i / LEN);

    int endCycle = std::floor(input.size() - std::max(LEN, ramp[LEN - 1]));
    for (int anCycle = 0; anCycle < endCycle; anCycle += analysisHop) {
        univector<float> grain;
        for (int i = anCycle; i < anCycle + LEN; ++i)
            grain.push_back(input[i]);
        grain *= hannWindow;
        univector<std::complex<float>> fftGrain = realdft(grain);
        // phase information: output psi
        univector<float> phi = -carg(fftGrain);
        univector<float> delta = modulo(phi - previousPhi - omega + pi, -2 * pi) + omega + pi;
        psi = modulo(psi + delta * synthesisHop / analysisHop + pi, -2 * pi) + pi;
        previousPhi = phi;
        // shifting: output correction factor
        univector<float> temp;
            for (int r : ramp)
                temp.push_back(input[anCycle + r - 1] / (LEN * 0.5));
        temp *= hannWindow;
        univector<float> f1 = absOf(realdft(temp));
        univector<std::complex<float>> diff = f1 - fftGrain;
        univector<float> realDiff = irealdft(diff) / 2;
        univector<std::complex<float>> corrected = absOf(fftGrain) * std::exp(realDiff[0]) * expComplex(makeComplex(psi));
        // interpolation
        grain = real(irealdft(corrected)) / 2;
        grain *= hannWindow;
        for (int i = anCycle; i < anCycle + resampledLEN; ++i)
                output[i] += grain[std::floor(x[i - anCycle]) - 1];
    }
    univector<float> result(input.size());
    std::copy(output.begin(), output.begin() + input.size(), result.begin());
    return result;
}

univector<float> ShiftEffect::absOf(const univector<std::complex<float>>& ofBuffer) {
    univector<float> result;
    for (auto& x : ofBuffer)
        result.push_back(abs(x));
    return result;
}

univector<std::complex<float>> ShiftEffect::makeComplex(const univector<float>& ofBuffer) {
    univector<std::complex<float>> result;
    for (auto& x : ofBuffer)
        result.push_back(make_complex(0.f, x));
    return result;
}

univector<std::complex<float>> ShiftEffect::expComplex(const univector<std::complex<float>>& ofBuffer) {
    univector<std::complex<float>> result;
    for (auto& x : ofBuffer)
        result.push_back(std::exp(x));
    return result;
}

univector<float> ShiftEffect::modulo(const univector<float>& a, float b) {
    return a - floor(a / b) * b;
}