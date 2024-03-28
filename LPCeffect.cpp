#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         filteredBuffer(numChannels, windowSize),
                         LPCcoeffs(modelOrder)
{
    // initialize filteredBuffer with 0s
    for (int j = 0; j < windowSize; ++j)
        filteredBuffer.setSample(0, j, 0);
}

// add received sample to buffer, send to processing once buffer full
float LPCeffect::sendSample(float sample)
{
    inputBuffer.setSample(0, index, sample);
    ++index;
    if (index == windowSize) {
        index = 0;
        doLPC();
    }
    float output = filteredBuffer.getSample(0, index);

//    logValues(sample, output); // analysis and debug logging

    if (output == NULL)
        return 0.f;

    return output;
}

// split into frames > R coeff > hann > OLA > LPC coeffs > residuals
void LPCeffect::doLPC()
{
    // calculate coefficients
    autocorrelation();
    levinsonDurbin();
    // filter overlapBuffer with all-pole filter (AR) from LPC coefficients
    residuals();
    corrCoeff.clear();
    LPCcoeffs.clear();
}

// calculate autocorrelation coefficients of a single frame
void LPCeffect::autocorrelation()
{
    // coefficients go up to frameSize, but since levinson needs modelOrder: cycling lag "k" 0 to modelOrder
    // denominator and mean remain the same
    float denominator = 0.f;
    float mean = 0.f;
    for (int i = 0; i < windowSize; ++i) {
        mean += inputBuffer.getSample(0, i);
    }
    mean /= (float) windowSize;
    for (int n = 0; n < windowSize; ++n) {
        float sample = inputBuffer.getSample(0, n);
        denominator += std::pow((sample - mean), 2.f);
    }
    for (int k = 0; k <= modelOrder; ++k) {
        float numerator = 0;
        for (int n = 0; n < windowSize - k; ++n) {
            float sample = inputBuffer.getSample(0, n);
            float lagSample = inputBuffer.getSample(0, n + k);
            numerator += (sample - mean) * (lagSample - mean);
        }
        jassert(abs(numerator / denominator) <= 1);
        corrCoeff.push_back(numerator / denominator);
    }
//    logAC();
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
    float kTo2 = (float) pow(k[0], 2);
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

        kTo2 = (float) pow(k[i-1],2);
        E[i-1] = (1 - kTo2) * E[i - 2];
    }

    for (int x = 0; x <= modelOrder; ++x) {
        LPCcoeffs.push_back(a[modelOrder][modelOrder - x]);
    }
}

// filtering the buffer with all-pole filter from LPC coefficients
void LPCeffect::residuals()
{
    for (int j = 0; j < windowSize; ++j) {
        filteredBuffer.setSample(0, j, 0);
    }

    // white noise carrier signal
    for (int n = 0; n < windowSize; ++n) {
        float rnd = (float) (rand() % 1000);
        rnd /= 10000;
        inputBuffer.setSample(0, n, rnd);
    }

    for (int n = 0; n < windowSize; ++n) {
        float sum = inputBuffer.getSample(0, n);
        for (int i = 1; i <= modelOrder; ++i) {
            if (n - i + 1 > 0)
                sum += LPCcoeffs[i] * inputBuffer.getSample(0, n - i);
        }
        filteredBuffer.setSample(0, n, sum);
    }
}

std::vector<float> logInpBuffer;
std::vector<float> logOutBuffer;
void LPCeffect::logValues(float input, float output)
{   // txt file value logger
    logInpBuffer.push_back(input);
    logOutBuffer.push_back(output);
    if (logInpBuffer.size() >= windowSize)
    {
        juce::File logFile("C:/Users/legionntb/Desktop/jucelog.txt");
        std::unique_ptr<juce::FileOutputStream> stream(logFile.createOutputStream());
        if (stream) {
            stream->setPosition(0);
            for (int i = 0; i < logInpBuffer.size(); ++i) {
                juce::String logMessage = juce::String(logInpBuffer[i], 5)+"   -> "+juce::String(logOutBuffer[i], 5)+"\n";
                stream -> writeString(logMessage);
            }
        }
        logInpBuffer.clear();
        logOutBuffer.clear();
    }
}

void LPCeffect::logAC() {
    juce::File logFile("C:/Users/legionntb/Desktop/jucelog.txt");
    std::unique_ptr<juce::FileOutputStream> stream(logFile.createOutputStream());
    if (stream) {
        stream->setPosition(0);
        for (float i : corrCoeff) {
            juce::String logMessage = juce::String(i, 5) + ",\n";
            stream -> writeString(logMessage);
        }
    }
}