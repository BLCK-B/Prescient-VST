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
        filteredBuffer2(windowSize, 0.f),
        tempBuffer1(windowSize, 0.f),
        tempBuffer2(windowSize, 0.f)
{
    jassert(windowSize % 2 == 0); // real-to-complex and complex-to-real transforms are only available for even sizes
}

// add received samples to buffers, process once buffer full
float LPCeffect::sendSample(float carrierSample, float voiceSample, int modelorder, bool stutter, float passthrough, float shift) {
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
        if (!stutter) {
            univector<float> flow = doShift(true, sideChainBuffer1, shift);
            doLPC(flow, true, passthrough);
        }
        tempmodelorder = modelorder;
    }
    else if (index2 == hopSize + windowSize && overlap != 0) {
        index2 = hopSize;
        if (!stutter) {
            univector<float> flow = doShift(false, sideChainBuffer2, shift);
            doLPC(flow, false, passthrough);
        }
    }
    float output = filteredBuffer1[index];
    if (index2 >= hopSize & overlap != 0)
        output += filteredBuffer2[index2 - hopSize];

    return output;
}

void LPCeffect::doLPC(bool firstBuffers, float passthrough) {
    if (firstBuffers) {
        if (tempFill1) {
            tempBuffer1 += mul(sideChainBuffer1, passthrough);
            std::memcpy(filteredBuffer1.data(), tempBuffer1.data(), tempBuffer1.size() * sizeof(float));
        }
        univector<float> LPCvoice = levinsonDurbin(autocorrelation(sideChainBuffer1, false));
        tempBuffer1 = FFToperations(FFToperation::IIR, getResiduals(carrierBuffer1), LPCvoice);
        tempBuffer1 = mul(tempBuffer1, matchPower(sideChainBuffer1, tempBuffer1));
        tempFill1 = true;
    }
    else {
        if (tempFill2) {
            tempBuffer2 += mul(sideChainBuffer2, passthrough);
            std::memcpy(filteredBuffer2.data(), tempBuffer2.data(), tempBuffer2.size() * sizeof(float));
        }
        univector<float> LPCvoice = levinsonDurbin(autocorrelation(sideChainBuffer2, false));
        tempBuffer2 = FFToperations(FFToperation::IIR, getResiduals(carrierBuffer2), LPCvoice);
        tempBuffer2 = mul(tempBuffer2, matchPower(sideChainBuffer2, tempBuffer2));
        tempFill2 = true;
    }
}

void LPCeffect::doLPC(univector<float>& flow, bool firstBuffers, float passthrough) {
    if (firstBuffers) {
        if (tempFill1) {
            tempBuffer1 += mul(sideChainBuffer1, passthrough);
            std::memcpy(filteredBuffer1.data(), tempBuffer1.data(), tempBuffer1.size() * sizeof(float));
        }
        univector<float> LPCvoice = levinsonDurbin(autocorrelation(flow, false));
        tempBuffer1 = FFToperations(FFToperation::IIR, getResiduals(carrierBuffer1), LPCvoice);
        tempBuffer1 = mul(tempBuffer1, matchPower(sideChainBuffer1, tempBuffer1));
        tempFill1 = true;
    }
    else {
        if (tempFill2) {
            tempBuffer2 += mul(sideChainBuffer2, passthrough);
            std::memcpy(filteredBuffer2.data(), tempBuffer2.data(), tempBuffer2.size() * sizeof(float));
        }
        univector<float> LPCvoice = levinsonDurbin(autocorrelation(flow, false));
        tempBuffer2 = FFToperations(FFToperation::IIR, getResiduals(carrierBuffer2), LPCvoice);
        tempBuffer2 = mul(tempBuffer2, matchPower(sideChainBuffer2, tempBuffer2));
        tempFill2 = true;
    }
}

univector<float> LPCeffect::doShift(bool firstBuffers, const univector<float>& input, float shift) {
    univector<float> shifted = shiftEffect.shiftSignal(input, shift);
    if (firstBuffers)
        shifted = mul(shifted, matchPower(sideChainBuffer1, shifted));
    else
        shifted = mul(shifted, matchPower(sideChainBuffer1, shifted));
    return shifted;
}

univector<float> LPCeffect::FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients) {
    univector<float> paddedCoeff(windowSize);
    std::copy(coefficients.begin(), coefficients.end(), paddedCoeff.begin());

    univector<std::complex<float>> fftInp = o == FFToperation::Convolution ? FFTcache : realdft(inputBuffer);
    univector<std::complex<float>> fftCoeff = realdft(paddedCoeff);

    switch (o) {
        case FFToperation::Convolution:
            mulVectorWith(fftInp, fftCoeff);
            return irealdft(fftInp);
        case FFToperation::IIR:
            divVectorWith(fftInp, fftCoeff);
            univector<float> filtered = irealdft(fftInp);
            mulVectorWith(filtered, hannWindow);
            return filtered;
    }
}

univector<float> LPCeffect::getResiduals(const univector<float>& ofBuffer) {
    univector<float> corrCoeff = autocorrelation(ofBuffer, true);
    univector<float> LPC = levinsonDurbin(corrCoeff);
    univector<float> e = FFToperations(FFToperation::Convolution, ofBuffer, LPC);
    return e;
}

univector<float> LPCeffect::autocorrelation(const univector<float>& ofBuffer, bool saveFFT) {
    // Wiener–Khinchin theorem
    univector<std::complex<float>> fftBuffer = realdft(ofBuffer);
    if (saveFFT)
        FFTcache = fftBuffer;
    univector<std::complex<float>> fftBufferConj = cconj(fftBuffer);
    mulVectorWith(fftBuffer, fftBufferConj);
    univector<float> coeffs = irealdft(fftBuffer);
    return coeffs;
}

univector<float> LPCeffect::levinsonDurbin(const univector<float>& corrCoeff) const {
    std::vector<float> k(tempmodelorder + 1);
    std::vector<float> E(tempmodelorder + 1);
    // matrix of coefficients a[j][i] j = row, i = column
    std::vector<std::vector<float>> a(tempmodelorder + 1, std::vector<float>(tempmodelorder + 1, 0.0f));
    // init
    for (int r = 0; r <= tempmodelorder; ++r)
        a[r][r] = 1;

    k[0] = - corrCoeff[1] / corrCoeff[0];
    a[1][0] = k[0];
    auto kTo2 = std::pow(k[0], 2);
    E[0] = static_cast<float>((1 - kTo2) * corrCoeff[0]);

    for (int i = 2; i <= tempmodelorder; ++i) {
        float sum = 0.f;
        for (int j = 0; j <= i; ++j) {
            sum += a[i-1][j] * corrCoeff[j + 1];
        }
        k[i-1] = - sum / E[i - 1-1];
        a[i][0] = k[i-1];

        for (int j = 1; j < i; ++j) {
            a[i][j] = a[i-1][j-1] + k[i-1] * a[i-1][i-j-1];
        }

        kTo2 = std::pow(k[i-1],2);
        E[i-1] = static_cast<float>((1 - kTo2) * E[i - 2]);
    }
    univector<float> LPCcoeffs(tempmodelorder);
    for (int x = 0; x <= tempmodelorder; ++x)
        LPCcoeffs.push_back(a[tempmodelorder][tempmodelorder - x]);

    return LPCcoeffs;
}

float LPCeffect::matchPower(const univector<float>& original, const univector<float>& output) const {
    float sumOfSquares = std::inner_product(original.begin(), original.end(), original.begin(), 0.0f);
    float signalPower = std::sqrt(sumOfSquares / static_cast<float>(windowSize));

    sumOfSquares = std::inner_product(output.begin(), output.end(), output.begin(), 0.0f);
    float signalPower2 = std::sqrt(sumOfSquares / static_cast<float>(windowSize));

    if (signalPower < signalPower2)
        return signalPower / signalPower2;
    else
        return signalPower2 / signalPower;
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

//     white noise
//    univector<float> newE(windowSize);
//    for (int n = 0; n < windowSize; ++n) {
//        float rnd = (float) (rand() % 1000);
//        rnd /= 100000000 * 0.5;
//        newE[n] = rnd;
//    }

//auto start_time = std::chrono::high_resolution_clock::now();
//auto end = std::chrono::high_resolution_clock::now();
//std::cout << "Time difference: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_time).count() << " nanoseconds" << std::endl;