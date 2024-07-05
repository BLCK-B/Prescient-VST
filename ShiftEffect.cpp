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

univector<float> ShiftEffect::shiftSignal(const univector<float>& input, float shift, float spread, float custom) {
    if (isCloseTo(shift, 1) && isCloseTo(spread, 0))
        return input;
    univector<float> finalOutput(input.size(), 0.f);
    for (int unison = 0; unison <= 4; ++unison) {
        if (isCloseTo(spread, 0) && unison != 0)
            continue;
        if (custom < 0.25 && unison != 0)
            break;
        else if (custom < 0.5 && unison > 2)
            break;

        float newShift = shift;
        if (unison == 1)
            newShift += spread;
        if (unison == 2)
            newShift -= spread;
        if (unison == 3)
            newShift += 0.5 * spread;
        if (unison == 4)
            newShift -= 0.5 * spread;

        int analysisHop = static_cast<int>(synthesisHop / newShift);
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
            mulVectorWith(grain, hannWindow);
            fftGrain = padFFT(grain);

            // phase information: output psi
            phi = carg(fftGrain);
            delta = modulo(phi - previousPhi - omega + pi, -2 * pi) + omega + pi;
            psi = modulo(psi + delta * synthesisHop / analysisHop + pi, -2 * pi) + pi;
            previousPhi = phi;
            if (unison == 0 && isCloseTo(shift, 1)) {
                overLapOut = grain;
                break;
            }
            // shifting: output correction factor
            f1 = absOf(padFFT(mul(input[anCycle], hannWindow)) / LEN);
            corrected = mul(absOf(fftGrain), std::exp(cutIFFT(f1 - fftGrain)[0]));
            mulVectorWith(corrected, expComplex(makeComplex(psi)));

            // interpolation
            grain = real(cutIFFT(corrected));
            mulVectorWith(grain, hannWindow);
            for (int ai = anCycle; ai < anCycle + resampledLEN; ++ai)
                overLapOut[ai] += grain[std::floor(x[ai - anCycle]) - 1];
        }
        univector<float> cropped = {overLapOut.begin(), overLapOut.begin() + input.size()};

        if (unison == 1 || unison == 2)
            cropped *= 0.5;
        if (unison == 3 || unison == 4)
            cropped *= 0.25;

        finalOutput += cropped;
        if (isCloseTo(spread, 0))
            break;
    }
    return finalOutput;
}

bool ShiftEffect::isCloseTo(float x, float to) {
    return (abs(to - x) <= 0.03);
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