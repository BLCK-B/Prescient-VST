#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         frameBuffer(numChannels, frameSize),
                         overlapBuffer(numChannels, windowSize),
                         hannWindow(frameSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann)
{
}

// add received sample to buffer, send to processing once buffer full
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
        float* frameChPtr = frameBuffer.getWritePointer(channel);

        for (int i = 0; i < Nframes; ++i) {
            frameBuffer.clear();
            int startOfFrame = i / Nframes * windowSize;
            // copy a frame from inputBuffer to frameBuffer
            frameBuffer.copyFrom(channel, 0, inputBuffer, channel, startOfFrame, frameSize);
            autocorrelation(channel);
            // perform OLA one frame at a time
            hannWindow.multiplyWithWindowingTable(frameChPtr, frameSize);
            int startSample = 0;
            if (i > 0)
                startSample = i * frameSize - frameSize / 2;
            overlapBuffer.addFrom(channel, startSample, frameBuffer, channel, 0, frameSize);
            // after last frame append 0s until frameSize
            if (i == Nframes - 1) {
                for (int j = startSample + 1; j < frameSize; ++j) {
                    overlapBuffer.setSample(channel, j, 0);
                }
            }
        }
    }
    // calculate LPC coefficients from autocorrelation coefficients
    levinsonDurbin();
    // filter overlapBuffer with all-pole filter from LPC coefficients
    residuals();
}

// calculate autocorrelation coefficients of a single frame
float LPCeffect::autocorrelation(int channel)
{
    corrCoeff.clear();
    // cycling lag "k" 0 up to modelOrder
    for (int k = 0; k < modelOrder; ++k) {
        float sumResult = 0;
        for (int n = 0; n < frameSize - 1 - k; ++n) {
            float sample = frameBuffer.getSample(channel, n);
            float lagSample = frameBuffer.getSample(channel, n + k);
            sumResult += sample * lagSample;
        }
        sumResult /= frameSize;
        corrCoeff.push_back(sumResult);
    }
    // now we have a vector of coefficients R(0 - modelOrder)
}

// calculate filter parameters using this algorithm
void LPCeffect::levinsonDurbin() {
    // levinson durbin algorithm
    std::vector<float> k (modelOrder);
    std::vector<float> E (modelOrder);
    // two-dimensional vector because matrix
    std::vector<std::vector<float>> a;

    // initialization with i = 1
    E[0] = corrCoeff[0];
    k[1] = - corrCoeff[1] / E[0];
    a[1][1] = k[1];
    auto k2 = static_cast<float>(std::pow(k[1], 2));
    E[1] = (1 - k2) * E[0];
    // loop beginning with i = 2
    for (int i = 2; i < modelOrder; ++i) {
        float sumResult = 0;
        for (int j = 1; j < i; ++j) {
            sumResult += a[j][i - 1] * corrCoeff[i - j];
        }
        k[i] = - (corrCoeff[i] + sumResult) / E[i - 1];
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
     *  sorting the coefficients for a filter column by column: */
    for (int col = 1; col < modelOrder; ++col) {
        for (int row = 1; row < modelOrder; ++row) {
            LPCcoeffs.push_back(a[row][col]);
        }
    }
    a.clear();
    E.clear();
    k.clear();
    for (float coeff : LPCcoeffs) {
        std::cout << coeff;
    }
}

// filtering the OLA-ed buffer with all-pole filter from LPC coefficients
void LPCeffect::residuals() {
    for (int channel = 0; channel < numChannels; ++channel) {
        for (int n = 1; n < modelOrder; ++n) {
            float filtered = 0;
            int cycle = 0;
            for (float coeff : LPCcoeffs) {
                if ((n - cycle) >= 0)
                    filtered += coeff * overlapBuffer.getSample(channel, n - cycle);
                ++cycle;
            }
            filteredBuffer.setSample(channel, n - 1, filtered);
        }
    }
    juce::AudioBuffer<float> residuals;
    // subtracting the original OLA-ed and filtered signals to get residuals
    for (int channel = 0; channel < numChannels; ++channel) {
        for (int i = 0; i < frameSize; ++i) {
            float difference = overlapBuffer.getSample(channel, i) - filteredBuffer.getSample(channel, i);
            residuals.setSample(channel, i, difference);
        }
    }
}