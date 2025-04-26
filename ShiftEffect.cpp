#include "ShiftEffect.h"
#include <iostream>
#include <fstream>

ShiftEffect::ShiftEffect(const int sampleRate) {
    LEN = 1024;
    synthesisHop = 250;
    windowLenEnum = WindowLenEnum::M;
    jassert(LEN % 2 == 0);
    psi.resize(LEN, 0.f);
    ramp.resize(LEN, 0.f);
    omega.resize(LEN, 0.f);
    fftGrain.resize(LEN / 2 + 1, 0.f);
    phi.resize(LEN, 0.f);
    previousPhi.resize(LEN, 0.f);
    delta.resize(LEN, 0.f);
    f1.resize(LEN, 0.f);
    corrected.resize(LEN, 0.f);
}

univector<float> ShiftEffect::shiftSignal(const univector<float>& input, float shift) {
    univector<float> finalOutput(input.size(), 0.f);
    int analysisHop = static_cast<int>(synthesisHop / shift);
    const int resampledLEN = std::floor(LEN * analysisHop / synthesisHop);

    std::iota(ramp.begin(), ramp.end(), 1);
    std::transform(ramp.begin(), ramp.end(), ramp.begin(), [&](int i) {
        return std::floor((i * analysisHop) / synthesisHop) + 1;
    });

    univector<float> x(resampledLEN, 0.f);
    for (int i = 0; i < resampledLEN; ++i)
        x[i] = (1 + (float) i * LEN / resampledLEN);

    for (int i = 0; i < LEN; ++i)
        omega[i] = 2 * pi * analysisHop * i / LEN;

    univector<float> overLapOut(input.size() + resampledLEN, 0.f);
    univector<float> grain(LEN);
    int endCycle = std::floor(input.size() - std::max(LEN, ramp[LEN - 1]));
    for (int anCycle = 0; anCycle < endCycle; anCycle += analysisHop) {
        std::copy(input.begin() + anCycle, input.begin() + anCycle + LEN, grain.begin());
        mulVectorWith(grain, hannWindow[windowLenEnum]);
        fftGrain = padFFT(grain);

        // phase information: output psi
        phi = carg(fftGrain);
        delta = modulo(phi - previousPhi - omega + pi, -2 * pi) + omega + pi;
        psi = modulo(psi + delta * synthesisHop / analysisHop + pi, -2 * pi) + pi;
        previousPhi = phi;

        // shifting: output correction factor
        f1 = absOf(padFFT(mul(input[anCycle], hannWindow[windowLenEnum])) / LEN);
        corrected = mul(absOf(fftGrain), std::exp(cutIFFT(f1 - fftGrain)[0]));
        mulVectorWith(corrected, expComplex(makeComplex(psi)));

        // overlap
        grain = real(cutIFFT(corrected));
        mulVectorWith(grain, hannWindow[windowLenEnum]);
        for (int ai = anCycle; ai < anCycle + resampledLEN; ++ai)
            overLapOut[ai] += grain[std::floor(x[ai - anCycle]) - 1];
    }
    return {overLapOut.begin(), overLapOut.begin() + input.size()};
}

void ShiftEffect::mulVectorWith(univector<float>& vec1, const univector<float>& vec2) {
    std::transform(vec1.begin(), vec1.end(), vec2.begin(), vec1.begin(), std::multiplies<>());
}

void ShiftEffect::mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2) {
    std::transform(vec1.begin(), vec1.end(), vec2.begin(), vec1.begin(), std::multiplies<>());
}

univector<float> ShiftEffect::absOf(const univector<std::complex<float>>& ofBuffer) {
    univector<float> result(ofBuffer.size());
    std::transform(ofBuffer.begin(), ofBuffer.end(), result.begin(),
                   [](const std::complex<float>& x) { return std::abs(x); });
    return result;
}

univector<std::complex<float>> ShiftEffect::makeComplex(const univector<float>& ofBuffer) {
    univector<std::complex<float>> result;
    result.reserve(ofBuffer.size());
    std::transform(ofBuffer.begin(), ofBuffer.end(), std::back_inserter(result), [](float x) {
        return std::complex<float>(0.f, x);
    });
    return result;
}

univector<std::complex<float>> ShiftEffect::expComplex(const univector<std::complex<float>>& ofBuffer) {
    univector<std::complex<float>> result;
    result.reserve(ofBuffer.size());
    std::transform(ofBuffer.begin(), ofBuffer.end(), std::back_inserter(result), [](const std::complex<float>& x) {
        return std::exp(x);
    });
    return result;
}

univector<float> ShiftEffect::modulo(const univector<float>& a, float b) {
    univector<float> result(a.size());
    std::transform(a.begin(), a.end(), result.begin(),
                   [b](float x) { return std::fmod(x, b); });
    return result;
}

/** FFT and filling the other half with zeroes */
univector<std::complex<float>> ShiftEffect::padFFT(const univector<float>& input) const {
    univector<std::complex<float>> buff = realdft(input);
    univector<std::complex<float>> result(LEN, 0.f);
    std::copy(buff.begin(), buff.end(), result.begin());
    return result;
}

/** cutting the other half (of zeroes) and IFFT */
univector<float> ShiftEffect::cutIFFT(const univector<std::complex<float>>& input) {
    univector<std::complex<float>> buff(input.begin(), input.begin() + input.size() / 2 + 1);
    return irealdft(buff);
}