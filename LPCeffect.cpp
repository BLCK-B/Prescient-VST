#include "LPCeffect.h"
#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/dft.hpp>
using namespace kfr;
using namespace std::chrono;

// constructor
LPCeffect::LPCeffect() :
        carrierBuffer1(windowSize),
        carrierBuffer2(windowSize),
        sideChainBuffer1(windowSize),
        sideChainBuffer2(windowSize),
        filteredBuffer1(windowSize, 0.f),
        filteredBuffer2(windowSize, 0.f)
{
    jassert(windowSize % 2 == 0); // real-to-complex and complex-to-real transforms are only available for even sizes
}

// add received samples to buffers, process once buffer full
float LPCeffect::sendSample(float carrierSample, float voiceSample) {
    carrierBuffer1[index] = carrierSample;
    sideChainBuffer1[index] = voiceSample;
    if (index2 >= hopSize & overlap != 0) {
        carrierBuffer2[index2 - hopSize] = carrierSample;
        sideChainBuffer2[index2 - hopSize] = voiceSample;
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

    return output;
}

void LPCeffect::doLPC(bool firstBuffers) {
    if (firstBuffers) {
        univector<float> LPCvoice = levinsonDurbin(autocorrelation(sideChainBuffer1, false));
        univector<float> e = getResiduals(carrierBuffer1);
        FFTcache.clear();
        filteredBuffer1 = FFToperations(FFToperation::IIR, e, LPCvoice);
        filteredBuffer1 = mul(filteredBuffer1, matchPower(sideChainBuffer1, filteredBuffer1));
    }
    else {
        univector<float> LPCvoice = levinsonDurbin(autocorrelation(sideChainBuffer2, false));
        univector<float> e = getResiduals(carrierBuffer2);
        FFTcache.clear();
        filteredBuffer2 = FFToperations(FFToperation::IIR, e, LPCvoice);
        filteredBuffer2 = mul(filteredBuffer2, matchPower(sideChainBuffer2, filteredBuffer2));
    }
}

univector<float> LPCeffect::FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients) {
    univector<float> paddedCoeff(windowSize);
    std::copy(coefficients.begin(), coefficients.end(), paddedCoeff.begin());

    univector<std::complex<float>> fftInp = o == FFToperation::Convolution ? FFTcache : realdft(inputBuffer);
    univector<std::complex<float>> fftCoeff = realdft(paddedCoeff);
    univector<std::complex<float>> fftResult(fftInp.size());

    switch (o) {
        case FFToperation::Convolution:
            std::transform(fftInp.begin(), fftInp.end(), fftCoeff.begin(), fftResult.begin(), std::multiplies<>());
            return irealdft(fftResult);
        case FFToperation::IIR:
            std::transform(fftInp.begin(), fftInp.end(), fftCoeff.begin(), fftResult.begin(), std::divides<>());
            univector<float> filtered = mul(irealdft(fftResult), 0.0001);
            // faster than filtered *= hannWindow
            std::transform(filtered.begin(), filtered.end(), hannWindow.begin(), filtered.begin(), std::multiplies<>());
            return filtered;
    }
}

univector<float> LPCeffect::getResiduals(const univector<float>& ofBuffer) {
    univector<float> corrCoeff = autocorrelation(ofBuffer, true);
    univector<float> LPC = levinsonDurbin(corrCoeff);
    univector<float> e = FFToperations(FFToperation::Convolution, ofBuffer, LPC);
    e = mul(e, 0.1);
    return e;
}

univector<float> LPCeffect::autocorrelation(const univector<float>& ofBuffer, bool saveFFT) {
    // Wiener–Khinchin theorem
    // TODO: better nominal and padding?
    univector<std::complex<float>> fftBuffer = realdft(ofBuffer);
    if (saveFFT)
        FFTcache = fftBuffer;
    univector<std::complex<float>> fftBufferConj = cconj(fftBuffer);
    std::transform(fftBuffer.begin(), fftBuffer.end(), fftBufferConj.begin(), fftBuffer.begin(), std::multiplies<>());
    univector<float> coeffs = irealdft(fftBuffer);
    coeffs /= coeffs.size();
    return coeffs;
}

univector<float> LPCeffect::levinsonDurbin(const univector<float>& corrCoeff) {
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

float LPCeffect::matchPower(const univector<float>& original, const univector<float>& output) {
    float sumOfSquares = 0.0;
    for (const auto& sample : original)
        sumOfSquares += sample * sample;
    float signalPower = std::sqrt(sumOfSquares / windowSize);

    sumOfSquares = 0.0;
    for (const auto& sample : output)
        sumOfSquares += sample * sample;
    float signalPower2 = std::sqrt(sumOfSquares / windowSize);

    return signalPower / signalPower2;
}

//     white noise
//    univector<float> newE(windowSize);
//    for (int n = 0; n < windowSize; ++n) {
//        float rnd = (float) (rand() % 1000);
//        rnd /= 100000000 * 0.5;
//        newE[n] = rnd;
//    }