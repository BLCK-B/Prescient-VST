#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         filteredBuffer(numChannels, windowSize)
{
    // initialize filteredBuffer with 0s
    for (int j = 0; j < windowSize; ++j) {
        filteredBuffer.setSample(0, j, 0);
    }
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

    // debug logging
    //if (output != 0) logValues(sample, output);

    return output == NULL ? 0 : output;
}

// split into frames > R coeff > hann > OLA > LPC coeffs > residuals
void LPCeffect::doLPC()
{
    // calculate coefficients
    autocorrelation();
    levinsonDurbin();

    // filter overlapBuffer with all-pole filter from LPC coefficients
    residuals();
    corrCoeff.clear();
    LPCcoeffs.clear();
}

// calculate autocorrelation coefficients of a single frame
void LPCeffect::autocorrelation()
{
    // coefficients go up to frameSize, but since levinson needs modelOrder: cycling lag "k" 0 to modelOrder
    // denominator and mean remain the same
    float denominator = 0;
    float mean = 0;
    for (int i = 0; i < windowSize; ++i) {
        mean += inputBuffer.getSample(0, i);
    }
    mean /= windowSize;
    for (int n = 0; n < windowSize; ++n) {
        float sample = inputBuffer.getSample(0, n);
        denominator += std::pow((sample - mean), 2);
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
}

// calculate filter parameters using this algorithm
void LPCeffect::levinsonDurbin()
{
    // levinson durbin algorithm
    std::vector<float> k (modelOrder + 1);
    std::vector<float> E (modelOrder + 1);
    // two-dimensional vector because matrix
    // j = row, i = column
    std::vector<std::vector<float>> a (modelOrder + 1, std::vector<float>(modelOrder + 1));

    // initialization with i = 1
    E[0] = a[0][0] = corrCoeff[0];
    k[1] = - corrCoeff[1] / E[0];
    a[1][1] = k[1];
    auto k2 = static_cast<float>(std::pow(k[1], 2));
    E[1] = (1 - k2) * E[0];

    for (int i = 0; i <= modelOrder; ++i) {
        a[0][1] = 1;
    }

    // loop beginning with i = 2
    for (int i = 2; i <= modelOrder; ++i) {
        float sumResult = 0;
        for (int j = 1; j < i; ++j) {
            sumResult += a[j][i - 1] * corrCoeff[i - j];
        }
        k[i] = - (corrCoeff[i] + sumResult) / E[i - 1];
        jassert(abs(k[i]) <= 1);
        a[i][i] = k[i];
        for (int j = 1; j < i; ++j) {
            a[j][i] = a[j][i - 1] + k[i] * a[i - j][i - j];
        }
        k2 = static_cast<float>(std::pow(k[i], 2));
        E[i] = (1 - k2) * E[i - 1];
    }
    /*  result in the form of
     *  a[1][1] a[1][2] a[1][3]
     *          a[2][2] a[2][3]
     *                  a[3][3]
     *  formant frequency information (envelope)
     *  saving the reflection diagonal coefficients: */
    for (int x = 0; x <= modelOrder; ++x) {
        LPCcoeffs.push_back(a[x][modelOrder]);
    }
    a.clear();
    E.clear();
    k.clear();
}

// filtering the buffer with all-pole filter from LPC coefficients
void LPCeffect::residuals()
{
    filteredBuffer.clear();

    // white noise carrier signal
    for (int n = 0; n < windowSize; ++n) {
        float rnd = rand() % 1000;
        rnd /= 10000;
        inputBuffer.setSample(0, n, rnd);
    }

    // synthesis filter
    // assuming g=1
    // FIFO buffer of modelOrder previous samples
    std::vector<float> previousSamples(modelOrder + 1, 0.0f);

//    for (int n = 0; n < windowSize; ++n) {
//        float sum = inputBuffer.getSample(0, n);
//        for (int i = 0; i < modelOrder; ++i) {
//            sum -= LPCcoeffs[i] * previousSamples[i];
//        }
//        // move all to right
//        for (int g = modelOrder - 1; g > 0; --g) {
//            previousSamples[g] = previousSamples[g - 1];
//        }
//        // add last to beginning
//        previousSamples[0] = sum;
//        // output
//        filteredBuffer.setSample(0,n,sum);
//
//        // log autocorr coeff values: 0 < x <= 1
//        //filteredBuffer.setSample(0, n, corrCoeff[n % corrCoeff.size()]);
//        // log LPC coeffs values
//        //filteredBuffer.setSample(0, n, LPCcoeffs[n % modelOrder]);
//        // bypass carrier
//        //filteredBuffer.setSample(0,n,overlapBuffer.getSample(0, n));
//    }

    // try copy of vocoderproj
    for (int i = 0; i < windowSize; ++i) {
        filteredBuffer.setSample(0, i, 0);
    }
    for (int n = 0; n < windowSize; ++n) {
        float sum = inputBuffer.getSample(0, n);
        for (int i = 0; i < modelOrder; ++i) {
            if (n - i >= 0)
                sum -= LPCcoeffs[i] * filteredBuffer.getSample(0, n - i);
            else
                break;
        }
        filteredBuffer.setSample(0,n,sum);
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
            for (int i = 0; i < windowSize; ++i) {
                juce::String logMessage = juce::String(logInpBuffer[i]) + " -> " + juce::String(logOutBuffer[i]) + "\n";
                stream->writeString(logMessage);
            }
        }
        logInpBuffer.clear();
        logOutBuffer.clear();
    }
}