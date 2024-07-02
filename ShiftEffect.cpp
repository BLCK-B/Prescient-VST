#include "ShiftEffect.h"

ShiftEffect::ShiftEffect():
    psi(LEN, 0.f),
    ramp(LEN, 0.f),
    omega(LEN, 0.f),
    fftGrain(LEN / 2 + 1, 0.f),
    phi(LEN, 0.f),
    previousPhi(LEN, 0.f),
    delta(LEN, 0.f),
    f1(LEN, 0.f),
    corrected(LEN, 0.f)
{
    jassert(LEN % 2 == 0);
}

univector<float> ShiftEffect::shiftSignal(const univector<float>& input, float shift) {
    if (shift < 1.03 && shift > 0.98)
        return input;
    const int analysisHop = static_cast<int>(synthesisHop / shift);
    for (int i = 0; i < LEN; ++i)
        ramp[i] = std::floor((i * analysisHop) / synthesisHop) + 1;
    int resampledLEN = std::floor(LEN * analysisHop / synthesisHop);
    univector<float> x(resampledLEN, 0.f);
    for (int i = 0; i < resampledLEN; ++i)
            x[i] = (1 + (float) i * LEN / resampledLEN);
    univector<float> output(input.size() + resampledLEN, 0.f);
    for (int oi = 0; oi < LEN; ++oi)
        omega[oi] = 2 * pi * analysisHop * oi / LEN;

    univector<float> grain(input.begin() + 0, input.begin() + 0 + LEN);
    mulVectorWith(grain, hannWindow);
    fftGrain = padFFT(grain);
    // phase information: output psi
    phi = -carg(fftGrain);
    delta = modulo(phi - previousPhi - omega + pi, -2 * pi) + omega + pi;
    psi = modulo(psi + mul(delta, synthesisHop / analysisHop) + pi, -2 * pi) + pi;
    previousPhi = phi;

    // shifting: output correction factor
//    univector<float> temp(LEN, 0.f);
//    std::transform(ramp.begin(), ramp.begin() + LEN, temp.begin(),
//                   [&](int r) { return input[0 + r - 1] / (LEN * 0.5); });

    f1 = absOf(padFFT(input * hannWindow) / LEN);
    corrected = mul(absOf(fftGrain), std::exp(cutIFFT(f1 - fftGrain)[0] / 2));
    mulVectorWith(corrected, expComplex(makeComplex(psi)));

    // interpolation
    grain = real(cutIFFT(corrected)) / 2;
    mulVectorWith(grain, hannWindow);
    for (int ai = 0; ai < 0 + resampledLEN; ++ai)
        output[ai] += grain[std::floor(x[ai - 0]) - 1];
    return {output.begin(), output.begin() + input.size()};
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
univector<float> ShiftEffect::cutIFFT(const univector<std::complex<float>>& input) const {
    univector<std::complex<float>> buff(input.begin(), input.begin() + input.size() / 2 + 1);
    return irealdft(buff);
}