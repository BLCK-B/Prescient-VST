#include "LPCeffect.h"
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/dft.hpp>
using namespace kfr;

// constructor
LPCeffect::LPCeffect() :
        residualsBuffer(windowSize),
        carrierBuffer1(windowSize),
        carrierBuffer2(windowSize),
        sideChainBuffer1(windowSize),
        sideChainBuffer2(windowSize),
        filteredBuffer1(windowSize),
        filteredBuffer2(windowSize)
{
    jassert(windowSize % 2 == 0); // real-to-complex and complex-to-real transforms are only available for even sizes
    std::fill(filteredBuffer1.begin(), filteredBuffer1.end(), 0);
    std::fill(filteredBuffer2.begin(), filteredBuffer2.end(), 0);
}

// add received sample to buffer, send to processing once buffer full
float LPCeffect::sendSample(float carrierSample, float voiceSample) {
    carrierBuffer1[index] = carrierSample / 100;
    sideChainBuffer1[index] = voiceSample / 100;
    if (index2 >= hopSize & overlap != 0) {
        carrierBuffer2[index2 - hopSize] = carrierSample / 100;
        sideChainBuffer2[index2 - hopSize] = voiceSample / 100;
    }
    ++index;
    ++index2;

    if (index == windowSize) {
        index = 0;
        doLPC(true);
    }
    else if (index2 == hopSize + windowSize && overlap != 0) {
        index2 = hopSize;
        doLPC(false);
    }

    float output = filteredBuffer1[index];
    if (index2 >= hopSize & overlap != 0)
        output += filteredBuffer2[index2 - hopSize];

    if (output == NULL)
        return 0.f;

    return output;
}

void LPCeffect::doLPC(bool firstBuffers) {
    univector<float> LPCvoice = firstBuffers ? levinsonDurbin(sideChainBuffer1) : levinsonDurbin(sideChainBuffer2);
    univector<float> e = firstBuffers ? getResiduals(carrierBuffer1) : getResiduals(carrierBuffer2);

    if (firstBuffers)
        filteredBuffer1 = filterFFTsidechain(LPCvoice, e);
    else
        filteredBuffer2 = filterFFTsidechain(LPCvoice, e);
}

void LPCeffect::autocorrelation(const univector<float>& ofBuffer) {
    corrCoeff.clear();
    float avg = std::accumulate(ofBuffer.begin(), ofBuffer.end(), 0.0) / windowSize;
    univector<float> subtracted = ofBuffer - avg;
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

univector<float> LPCeffect::levinsonDurbin(const univector<float>& ofBuffer) {
    autocorrelation(ofBuffer);

    std::vector<float> k(modelOrder + 1);
    std::vector<float> E(modelOrder + 1);
    // matrix of coefficients a[j][i] j = row, i = column
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
    univector<float> LPCcoeffs(modelOrder);
    for (int x = 0; x <= modelOrder; ++x)
        LPCcoeffs.push_back(a[modelOrder][modelOrder - x]);
    return LPCcoeffs;
}

univector<float> LPCeffect::getResiduals(const univector<float>& ofBuffer) {
    univector<float> LPC = levinsonDurbin(ofBuffer);
    univector<float> convRes = convolve(ofBuffer, LPC);
    float diff = ((float) (convRes.size() - windowSize)) / 2;
    univector<float> e(convRes.begin() + std::floor(diff), convRes.end() - std::ceil(diff));
    return e;
}

univector<float> LPCeffect::filterFFTsidechain(const univector<float>& LPCvoice, const univector<float>& e) {
    univector<float> paddedLPC(windowSize, 0.0f);
    std::copy(LPCvoice.begin(), LPCvoice.end(), paddedLPC.begin());

    // white noise
//    univector<float> newE(windowSize);
//    for (int n = 0; n < windowSize; ++n) {
//        float rnd = (float) (rand() % 1000);
//        rnd /= 100000000 * 0.5;
//        newE[n] = rnd;
//    }

    univector<std::complex<float>> X = realdft(e);
    univector<std::complex<float>> H = realdft(paddedLPC);
    univector<std::complex<float>> Y = X / H;

    univector<float> filtered = irealdft(Y);
    for (float &x: filtered)
        x *= 0.1;
    filtered *= hannWindow;

    return filtered;
}
