#include "LPCeffect.h"
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/dft.hpp>
using namespace kfr;

// constructor
LPCeffect::LPCeffect() : inputBuffer(windowSize),
                         inputBuffer2(windowSize),
                         filteredBuffer(windowSize),
                         filteredBuffer2(windowSize),
                         LPCcoeffs(modelOrder)
{
    jassert(windowSize % 2 == 0); // real-to-complex and complex-to-real transforms are only available for even sizes
    std::fill(filteredBuffer.begin(), filteredBuffer.end(), 0);
    std::fill(filteredBuffer2.begin(), filteredBuffer2.end(), 0);
}

// add received sample to buffer, send to processing once buffer full
float LPCeffect::sendSample(float sample, float sidechain) {
    inputBuffer[index] = sample + sidechain;
    if (index2 >= hopSize & overlap != 0)
        inputBuffer2[index2 - hopSize] = sample + sidechain;
    ++index;
    ++index2;

    if (index == windowSize) {
        index = 0;
        activeBuffer = 1;
        doLPC();
    }
    else if (index2 == hopSize + windowSize && overlap != 0) {
        index2 = hopSize;
        activeBuffer = 2;
        doLPC();
    }

    float output = filteredBuffer[index];
    if (index2 >= hopSize & overlap != 0)
        output += filteredBuffer2[index2 - hopSize];

    if (output == NULL)
        return 0.f;

    return output;
}

void LPCeffect::doLPC() {
    autocorrelation();
    levinsonDurbin();
    filterFFT();

//    if (activeBuffer == 1)
//        filteredBuffer = hannWindow * inputBuffer;
//    else
//        filteredBuffer2 = hannWindow * inputBuffer2;

    corrCoeff.clear();
    LPCcoeffs.clear();

}

void::LPCeffect::autocorrelation() {
    float avg = std::accumulate(inputBuffer.begin(), inputBuffer.end(), 0.0) / windowSize;
    univector<float> subtracted = inputBuffer - avg;
    float stdDeviation = std::sqrt(std::inner_product(subtracted.begin(), subtracted.end(), subtracted.begin(), 0.0) / windowSize);

    univector<float> normalised = subtracted / stdDeviation;

    univector<std::complex<float>> fftBuffer = realdft(normalised);
    univector<std::complex<float>> powerSpDen(fftBuffer.size());
    for (int i = 0; i < fftBuffer.size(); ++i)
        powerSpDen[i] = std::abs(fftBuffer[i]) * std::abs(fftBuffer[i]);
    univector<float> ifftBuffer = irealdft(powerSpDen);
    for (int i = 0; i < windowSize / 2; ++i)
        corrCoeff.push_back(ifftBuffer[i] / windowSize * 0.1);
}

void LPCeffect::levinsonDurbin() {
    std::vector<float> k(modelOrder + 1);
    std::vector<float> E(modelOrder + 1);
    // matrix of LPC coefficients a[j][i] j = row, i = column
    std::vector<std::vector<float>> a(modelOrder + 1, std::vector<float>(modelOrder + 1, 0.0f));
    // init
    for (int r = 0; r <= modelOrder; ++r)
        a[r][r] = 1;

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
    // white noise carrier
    for (int n = 0; n < windowSize; ++n) {
        float rnd = (float) (rand() % 1000);
        rnd /= 100000000 * 0.5;
        if (activeBuffer == 1)
            inputBuffer[n] = rnd;
        else
            inputBuffer2[n] = rnd;
    }

    univector<float> paddedLPC(windowSize, 0.0f);
    std::copy(LPCcoeffs.begin(), LPCcoeffs.end(), paddedLPC.begin());

    univector<std::complex<float>> X;
    if (activeBuffer == 1)
        X = realdft(inputBuffer);
    else
        X = realdft(inputBuffer2);
    univector<std::complex<float>> H = realdft(paddedLPC);
    univector<std::complex<float>> Y = X / H;
    if (activeBuffer == 1) {
        filteredBuffer = irealdft(Y);
        for (float &i: filteredBuffer) {
            i *= 0.1;
        }
        filteredBuffer *= hannWindow;
    }
    else {
        filteredBuffer2 = irealdft(Y);
        for (float &i: filteredBuffer2) {
            i *= 0.1;
        }
        filteredBuffer2 *= hannWindow;
    }
}