#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         frameBuffer(numChannels, frameSize),
                         overlapBuffer(numChannels, windowSize),
                         hannWindow(frameSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann)
{
}

// add received sample to buffer, send to process once buffer full
float LPCeffect::sendSample(float sample, int channel)
{
    inputBuffer.setSample(channel, index, sample);
    if (channel == 1) {
        ++index;
        if (index == windowSize) {
            index = 0;
            LPCeffect();
        }
    }
}

// split into frames > hann > calculate autocorrelation for each frame > OLA
void LPCeffect::overlapAdd()
{
    // Nframes in each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        float* frameChPtr = inputBuffer.getWritePointer(channel);

        for (int i = 0; i < Nframes; ++i) {
            int startOfFrame = i / Nframes * windowSize;
            frameBuffer.clear();
            frameBuffer.copyFrom(channel, 0, inputBuffer, channel, startOfFrame, frameSize);
            hannWindow.multiplyWithWindowingTable(frameChPtr, frameSize);
            autocorrelation(channel);
            // add frames with overlap to overlapBuffer
            // ...
        }
    }
}

// calculate autocorrelation coefficients
float LPCeffect::autocorrelation(int channel)
{
    corrCoeff.clear();
    // cycling lag "k" 0 up to modelOrder
    for (int k = 0; k < modelOrder; ++k) {
        float sumResult = 0;
        // loop representing sum
        for (int n = 0; n < frameSize - 1 - k; ++n) {
            float sample = frameBuffer.getSample(channel, n);
            float lagSample = frameBuffer.getSample(channel, n + k);
            sumResult += sample * lagSample;
        }
        sumResult /= frameSize;
        corrCoeff.push_back(sumResult);
    }
    // now we have R(0 - modelOrder) vector
}

// calculate filter parameters using this algorithm
void LPCeffect::levinsonDurbin() {
    // levinson durbin algorithm
    std::vector<float> k (modelOrder);
    std::vector<float> E (modelOrder);
    // two-dimensional vector because matrix
    std::vector<std::vector<float>> a;
    E[0] = corrCoeff[0];
    for (int i = 1; i < modelOrder; ++i) {
        // sum
        float sumResult = 0;
        if (i > 1) {
            for (int j = 1; j < i; ++j) {
                sumResult += a[j][i - 1] * corrCoeff[i - j];
            }
        }
        k[i] = - (corrCoeff[i] + sumResult) / E[i - 1];
        a[i][i] = k[i];
        if (i > 1) {
            for (int j = 1; j < i; ++j) {
                a[j][i] = a[j][i - 1] + k[i] * a[i - j][i - j];
            }
        }
        auto k2 = static_cast<float>(std::pow(k[i], 2));
        E[i] = (1 - k2) * E[i - 1];
    }
    /*  result in the form of
     *  a[1][1] a[1][2] a[1][3]
     *          a[2][2] a[2][3]
     *                  a[3][3]
    */

    for (int row = 1; row < modelOrder; ++row) {
        for (int col = 1; col < modelOrder; ++col) {
            printf("a[%d][%d] = %lf\n", row, col, a[row][col]);
        }
    }
}