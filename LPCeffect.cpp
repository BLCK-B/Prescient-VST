#include "LPCeffect.h"
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/dft.hpp>
using namespace kfr;

// constructor
LPCeffect::LPCeffect() : inputBuffer(windowSize),
                         filteredBuffer( windowSize),
                         LPCcoeffs(modelOrder),
                         hannWindow(windowSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann)
{
    jassert(inputBuffer.size() % 2 == 0); // real-to-complex and complex-to-real transforms are only available for even sizes
    // initialize filteredBuffer with 0s
    for (int j = 0; j < windowSize; ++j)
        filteredBuffer[j] = 0;
}

// add received sample to buffer, send to processing once buffer full
float LPCeffect::sendSample(float sample) {
    inputBuffer[index] = sample;
    ++index;
    if (index == windowSize) {
        index = 0;
        doLPC();
    }
    float output = filteredBuffer[index];

    if (output == NULL)
        return 0.f;

    return output;
}

void LPCeffect::doLPC() {
    autocorrelation();
    levinsonDurbin();
    filterFFT();
    corrCoeff.clear();
    LPCcoeffs.clear();
}

void::LPCeffect::autocorrelation() {
    float avg = std::accumulate(inputBuffer.begin(), inputBuffer.end(), 0.0) / windowSize;
    univector<float> subtracted(windowSize);
    for (size_t i = 0; i < windowSize; ++i)
        subtracted[i] = inputBuffer[i] - avg;
    float stdDeviation = std::sqrt(std::inner_product(subtracted.begin(), subtracted.end(), subtracted.begin(), 0.0) / windowSize);

    univector<float> normalised(windowSize);
    for (size_t i = 0; i < windowSize; ++i)
        normalised[i] = subtracted[i] / stdDeviation;

    univector<std::complex<float>> fftBuffer = realdft(normalised);
    univector<std::complex<float>> powerSpDen(fftBuffer.size());
    for (int i = 0; i < fftBuffer.size(); ++i)
        powerSpDen[i] = std::abs(fftBuffer[i]) * std::abs(fftBuffer[i]);
    univector<float> ifftBuffer = irealdft(powerSpDen);
    int resLen = windowSize / 2;
    for (int i = 0; i < resLen; ++i) {
        corrCoeff.push_back(ifftBuffer[i] / windowSize * 0.1);
    }
}

void LPCeffect::levinsonDurbin() {
    // ensure enough autocorr coeffs, not ideal
    corrCoeff.push_back(0);
    corrCoeff.push_back(0);

    std::vector<float> k(modelOrder + 1);
    std::vector<float> E(modelOrder + 1);
    // matrix of LPC coefficients a[j][i] j = row, i = column
    std::vector<std::vector<float>> a(modelOrder + 1, std::vector<float>(modelOrder + 1));
    // init
    for (int r = 0; r <= modelOrder; ++r) {
        for (int c = 0; c < modelOrder; ++c) {
            a[r][c] = 0.f;
        }
    }
    for (int r = 0; r <= modelOrder; ++r) {
        a[r][r] = 1;
    }

    k[0] = - corrCoeff[1] / corrCoeff[0];
    a[1][0] = k[0];
    float kTo2 = (float) std::pow(k[0], 2);
    E[0] = (1 - kTo2) * corrCoeff[0];

    for (int i = 2; i <= modelOrder; ++i) {
        float sum = 0.f;
        for (int j = 0; j <= i; ++j) {
            sum += a[i-1][j] * corrCoeff[j + 1];
        }
        k[i-1] = - sum / E[i - 1-1];
        a[i][0] = k[i-1];

        for (int j = 1; j < i; ++j) {
            a[i][j] = a[i-1][j-1] + k[i-1] * a[i-1][i-j-1];
        }

        kTo2 = (float) std::pow(k[i-1],2);
        E[i-1] = (1 - kTo2) * E[i - 2];
    }

    for (int x = 0; x <= modelOrder; ++x) {
        LPCcoeffs.push_back(a[modelOrder][modelOrder - x]);
    }
}

void LPCeffect::filterFFT() {
    for (int j = 0; j < windowSize; ++j) {
        filteredBuffer[j] = 0.f;
    }
    // white noise carrier
    for (int n = 0; n < windowSize; ++n) {
        float rnd = (float) (rand() % 1000);
        rnd /= 10000000;
        inputBuffer[n] = rnd;
    }

    univector<float> paddedLPC(windowSize);
    std::fill(paddedLPC.begin(), paddedLPC.end(), 0);
    std::copy(LPCcoeffs.begin(), LPCcoeffs.end(), paddedLPC.begin());

    univector<std::complex<float>> X = realdft(inputBuffer);
    univector<std::complex<float>> H = realdft(paddedLPC);
    univector<std::complex<float>> Y = X / H;
    filteredBuffer = irealdft(Y);
    for (float & i : filteredBuffer) {
        i *= 0.1;
    }
}