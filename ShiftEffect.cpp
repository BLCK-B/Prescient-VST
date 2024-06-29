#include "ShiftEffect.h"

ShiftEffect::ShiftEffect() {
}

univector<float> ShiftEffect::shiftSignal(univector<float> input) {
    int analysisHop = 200;
    int synthesisHop = 150;

    univector<float> psi(LEN, 0.f);
    univector<float> previousPhi(LEN, 0.f);
    univector<int> ramp(LEN);
    for (int i = 0; i < LEN; ++i)
        ramp[i] = std::floor((i * analysisHop) / synthesisHop) + 1;
    int resampledLEN = std::floor(LEN * analysisHop / synthesisHop);
    univector<int> x;
    for (int i = 0; i < resampledLEN; ++i)
        x.push_back(1 + i * LEN / resampledLEN);

    input /= absmaxof(input);

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
        univector<float> phi = carg(fftGrain);
        univector<float> delta = (phi - previousPhi - omega + pi) % (-2 * pi) + omega + pi;
        psi = (psi + delta * synthesisHop / analysisHop + pi) % (-2 * pi) + pi;
        previousPhi = phi;
        // shifting: output correction factor
        univector<float> temp;
        for (int i : ramp)
            temp.push_back(i + anCycle + 1);
        temp *= hannWindow;
        temp /= (LEN * 0.5);
        univector<std::complex<float>> f1 = std::abs(realdft(temp));
        univector<std::complex<float>> logarithmic = std::log(0.00001 + f1) - std::log(0.00001 + std::abs(fftGrain)) / 2;
        univector<float> realLog = irealdft(logarithmic);
        univector<std::complex<float>> corrected = std::abs(fftGrain) * std::exp(realLog[0]) * std::exp(1j * psi);
        // interpolation
        grain = real(irealdft(corrected) * hannWindow);
        output(...) += grain(...);
    }
    return output(...);
}

