#include "LPCeffect.h"
#include <kfr/base.hpp>
#include <kfr/dft.hpp>
using namespace kfr;
using namespace std::chrono;

// constructor
LPCeffect::LPCeffect(const int sampleRate) {
    shiftEffect = new ShiftEffect(sampleRate);
    if (sampleRate < 44100) {
        windowSize = 1024;
        windowSizeEnum = WindowSizeEnum::S;
    }
    else if (44100 < sampleRate && sampleRate < 88200) {
        windowSize = 2048;
        windowSizeEnum = WindowSizeEnum::M;
    }
    else {
        windowSize = 4096;
        windowSizeEnum = WindowSizeEnum::L;
    }
    jassert(windowSize % 2 == 0); // real-to-complex and complex-to-real transforms are only available for even sizes
    overlapSize = round(windowSize * overlap);
    hopSize = windowSize - overlapSize;

    carrierBuffer1.resize(windowSize);
    carrierBuffer2.resize(windowSize);
    sideChainBuffer1.resize(windowSize);
    sideChainBuffer2.resize(windowSize);
    filteredBuffer1.resize(windowSize, 0.f);
    filteredBuffer2.resize(windowSize, 0.f);
}

// add received samples to buffers, process once buffer full
float LPCeffect::sendSample(float carrierSample, float voiceSample, float modelOrder, float shiftVoice1,
                            float shiftVoice2, float shiftVoice3, bool enableLPC, float passthrough) {
    carrierBuffer1[index1] = carrierSample;
    sideChainBuffer1[index1] = voiceSample;
    if (index2 >= hopSize & overlap != 0) {
        carrierBuffer2[index2 - hopSize] = carrierSample;
        sideChainBuffer2[index2 - hopSize] = voiceSample;
    }
    ++index1;
    ++index2;
    if (index1 == windowSize) {
        index1 = 0;
        processing(filteredBuffer1, sideChainBuffer1, carrierBuffer1, shiftVoice1, shiftVoice2, shiftVoice3, enableLPC, passthrough);
        frameModelOrder = static_cast<int>(modelOrder);
    }
    else if (index2 == hopSize + windowSize && overlap != 0) {
        index2 = hopSize;
        processing(filteredBuffer2, sideChainBuffer2, carrierBuffer2, shiftVoice1, shiftVoice2, shiftVoice3, enableLPC, passthrough);
    }
    float output = filteredBuffer1[index1];
    if (index2 >= hopSize & overlap != 0)
        output += filteredBuffer2[index2 - hopSize];
    return output;
}

void LPCeffect::processing(univector<float>& toOverwrite, const univector<float>& voice, const univector<float>& carrier,
                           float shiftVoice1,  float shiftVoice2, float shiftVoice3, bool enableLPC, float passthrough) {
    univector<float> result = voice;

    if (shiftVoice1 >= 1.01 || shiftVoice1 <= 0.99)
        result = shiftEffect -> shiftSignal(result, shiftVoice1);
    if (shiftVoice2 >= 1.01 || shiftVoice2 <= 0.99)
        result += shiftEffect -> shiftSignal(voice, shiftVoice2);
    if (shiftVoice3 >= 1.01  || shiftVoice3 <= 0.99)
        result += shiftEffect -> shiftSignal(voice, shiftVoice3);
    matchPower(result, voice);

    if (enableLPC) {
        result = processLPC(result, carrier);
        matchPower(result, voice);
    }

    result = mul(result, passthrough) + mul(voice, 1 - passthrough);

    std::memcpy(toOverwrite.data(), result.data(), result.size() * sizeof(float));
}

univector<float> LPCeffect::processLPC(const univector<float>& voice, const univector<float>& carrier) {
    univector<float> LPCvoice = levinsonDurbin(autocorrelation(voice));
    return FFToperations(FFToperation::IIR, getResiduals(carrier), LPCvoice);
}

univector<float> LPCeffect::FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients) {
    univector<float> paddedCoeff(windowSize);
    std::copy(coefficients.begin(), coefficients.end(), paddedCoeff.begin());

    univector<std::complex<float>> fftInp = realdft(inputBuffer);
    univector<std::complex<float>> fftCoeff = realdft(paddedCoeff);
    switch (o) {
        case FFToperation::Convolution:
            mulVectorWith(fftInp, fftCoeff);
            return irealdft(fftInp);
        case FFToperation::IIR:
            divVectorWith(fftInp, fftCoeff);
            univector<float> filtered = irealdft(fftInp);
            mulVectorWith(filtered, hannWindow[windowSizeEnum]);
            return filtered;
    }
}

univector<float> LPCeffect::getResiduals(const univector<float>& ofBuffer) {
    univector<float> LPC = levinsonDurbin(autocorrelation(ofBuffer));
    return FFToperations(FFToperation::Convolution, ofBuffer, LPC);
}

univector<float> LPCeffect::autocorrelation(const univector<float>& ofBuffer) {
    // Wiener–Khinchin theorem
    univector<std::complex<float>> fftBuffer = realdft(ofBuffer);
    univector<std::complex<float>> fftBufferConj = cconj(fftBuffer);
    mulVectorWith(fftBuffer, fftBufferConj);
    univector<float> coeffs = irealdft(fftBuffer);
    return coeffs;
}

univector<float> LPCeffect::levinsonDurbin(const univector<float>& corrCoeff) const {
    std::vector<float> k(frameModelOrder + 1);
    std::vector<float> E(frameModelOrder + 1);
    // matrix of coefficients a[j][i] j = row, i = column
    std::vector<std::vector<float>> a(frameModelOrder + 1, std::vector<float>(frameModelOrder + 1, 0.0f));
    for (int r = 0; r <= frameModelOrder; ++r)
        a[r][r] = 1;

    k[0] = - corrCoeff[1] / corrCoeff[0];
    a[1][0] = k[0];
    auto kPow2 = std::pow(k[0], 2);
    E[0] = static_cast<float>((1 - kPow2) * corrCoeff[0]);

    for (int i = 2; i <= frameModelOrder; ++i) {
        float sum = 0.f;
        for (int j = 0; j <= i; ++j) {
            sum += a[i-1][j] * corrCoeff[j + 1];
        }
        k[i - 1] = - sum / E[i - 1-1];
        a[i][0] = k[i - 1];

        for (int j = 1; j < i; ++j) {
            a[i][j] = a[i - 1][j - 1] + k[i - 1] * a[i - 1][i - j - 1];
        }

        kPow2 = std::pow(k[i - 1],2);
        E[i - 1] = static_cast<float>((1 - kPow2) * E[i - 2]);
    }
    univector<float> LPCcoeffs(frameModelOrder);
    for (int x = 0; x <= frameModelOrder; ++x)
        LPCcoeffs.push_back(a[frameModelOrder][frameModelOrder - x]);

    return LPCcoeffs;
}

void LPCeffect::matchPower(univector<float>& input, const univector<float>& reference) const {
    float sumOfSquares = std::inner_product(reference.begin(), reference.end(), reference.begin(), 0.0f);
    float refPower = std::sqrt(sumOfSquares / static_cast<float>(windowSize));

    sumOfSquares = std::inner_product(input.begin(), input.end(), input.begin(), 0.0f);
    float inputPower = std::sqrt(sumOfSquares / static_cast<float>(windowSize));

    input = mul(input, min(refPower, inputPower) / max(refPower, inputPower));
}

void LPCeffect::mulVectorWith(univector<float>& vec1, const univector<float>& vec2) {
    std::transform(vec1.begin(), vec1.end(), vec2.begin(), vec1.begin(), std::multiplies<>());
}
void LPCeffect::mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2) {
    std::transform(vec1.begin(), vec1.end(), vec2.begin(), vec1.begin(), std::multiplies<>());
}
void LPCeffect::divVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2) {
    std::transform(vec1.begin(), vec1.end(), vec2.begin(), vec1.begin(), std::divides<>());
}