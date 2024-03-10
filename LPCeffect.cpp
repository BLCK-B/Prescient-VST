#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         frameBuffer(numChannels, frameSize),
                         overlapBuffer(numChannels, windowSize),
                         hannWindow(frameSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann),
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
    float finalSample = filteredBuffer.getSample(0, index);
    if (abs(finalSample) > 100)
        finalSample = 0;
    if (finalSample == NULL)
        return 0;
    else
        return filteredBuffer.getSample(0, index);
}

// split into frames > R coeff > hann > OLA > LPC coeffs > residuals
void LPCeffect::doLPC()
{
    //TODO: OLA may not be necessary *within* a window

    // Nframes in each channel
    float* frameChPtr = frameBuffer.getWritePointer(0);
    // initialize overlapBuffer with 0s
    for (int j = 0; j < windowSize; ++j) {
        overlapBuffer.setSample(0, j, 0);
    }

    for (int i = 0; i < Nframes; ++i) {
        frameBuffer.clear();
        int startOfFrame = i * windowSize / Nframes ;
        // copy a frame from inputBuffer to frameBuffer
        frameBuffer.copyFrom(0, 0, inputBuffer, 0, startOfFrame, frameSize);
        // calculate coefficients
        autocorrelation();
        int startS = i * modelOrder / Nframes;
        levinsonDurbin(startS);

        // perform OLA one frame at a time
        hannWindow.multiplyWithWindowingTable(frameChPtr, frameSize);
        if (i == 0)
            overlapBuffer.addFrom(0, 0, frameBuffer, 0, 0, frameSize);
        else {
            int startSample = i * frameSize - (frameSize / 2) * i;
            overlapBuffer.addFrom(0, startSample, frameBuffer, 0, 0, frameSize);
        }
    }
    // we have 1024 samples 48 LPC coeffs.. 21 to 1 compression
    // filter overlapBuffer with all-pole filter from LPC coefficients
    residuals();
    corrCoeff.clear();
    LPCcoeffs.clear();
}

// calculate autocorrelation coefficients of a single frame
void LPCeffect::autocorrelation() //TODO: needed also R(0) = 1?
{
    // coefficients go up to frameSize - 1, but since levinson needs only modelOrder: cycling lag "k" 0 up to modelOrder
    // denominator and mean remain the same
    float denominator = 0;
    float mean = 0;
    for (int i = 0; i < frameSize; ++i) {
        mean += frameBuffer.getSample(0, i);
    }
    mean /= frameSize;
    for (int n = 0; n < frameSize; ++n) {
        float sample = frameBuffer.getSample(0, n);
        denominator += std::pow((sample - mean), 2);
    }
    for (int k = 0; k < modelOrder; ++k) {
        float numerator = 0;
        for (int n = 0; n < frameSize - k; ++n) {
            float sample = frameBuffer.getSample(0, n);
            float lagSample = frameBuffer.getSample(0, n + k);
            numerator += (sample - mean) * (lagSample - mean);
        }
        corrCoeff.push_back(numerator / denominator);
    }
    // now we have a vector of coefficients R(0 - modelOrder)
}

// calculate filter parameters using this algorithm
void LPCeffect::levinsonDurbin(int startS)
{
    // levinson durbin algorithm
    std::vector<float> k (modelOrder);
    std::vector<float> E (modelOrder);
    // two-dimensional vector because matrix
    // j = row, i = column
    std::vector<std::vector<float>> a (modelOrder, std::vector<float>(modelOrder));

    // initialization with i = 1
    E[0] = corrCoeff[0 + startS];
    k[1] = - corrCoeff[1 + startS] / E[0];
    a[1][1] = k[1];
    auto k2 = static_cast<float>(std::pow(k[1], 2));
    E[1] = (1 - k2) * E[0];

    // loop beginning with i = 2
    for (int i = 2; i < modelOrder; ++i) {
        float sumResult = 0;
        for (int j = 1; j < i; ++j) {
            sumResult += a[j][i - 1] * corrCoeff[i - j + startS];
        }
        k[i] = - (corrCoeff[i + startS] + sumResult) / E[i - 1];
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
            if (a[row][col] != 0)
                LPCcoeffs.push_back(a[row][col]);
        }
    }
    a.clear();
    E.clear();
    k.clear();
}

// filtering the OLA-ed buffer with all-pole filter from LPC coefficients
void LPCeffect::residuals()
{
    filteredBuffer.clear();

    // white noise carrier signal
    // TODO: carrier needs be modulated by gain?
    for (int n = 0; n < windowSize; ++n) {
        //float rnd = (float) (rand()) / (float) (rand());
        //if (rnd > 10)
        //    rnd /= 10;
        float rnd = rand() % 1000;
        rnd /= 1000;
        overlapBuffer.setSample(0, n, rnd);
    }


    std::vector<float> previousOutput(modelOrder);
    for (int n = 0; n <= modelOrder; ++n) {
        previousOutput.push_back(0);
    }
    for (int n = 0; n < windowSize; ++n) {
        float sum = 0;
        for (int i = 0; i < modelOrder; ++i) {
            // modelOrder (12) previous samples required
            sum += LPCcoeffs[i] * previousOutput[modelOrder - i];
        }
        for (int i = modelOrder; i > 0; i--) {
            previousOutput[i] = previousOutput[i - 1];
        }
        float input = overlapBuffer.getSample(0, n);
        float output = input + sum;
        previousOutput[0] = output;
        filteredBuffer.setSample(0, n, output);
    }

    // residual/filter equation page 372
    /*for (int n = 0; n < windowSize; ++n) {
        float sum = 0;
        //LPCcoeffs[0] = 1;
        for (int m = 0; m < LPCcoeffs.size(); ++m) {
            if (n - m >= 0)
                sum += -LPCcoeffs[m] * overlapBuffer.getSample(0, n - m);
            //else
                //sum += -LPCcoeffs[m] * overlapBuffer.getSample(0, 0);
        }
        //sum -= overlapBuffer.getSample(0, n);
        filteredBuffer.setSample(0, n, sum);
        // without filter
        //filteredBuffer.setSample(0,n,overlapBuffer.getSample(0, n));
    }*/

}

// filter white noise or other carrier with all-pole/all-zero filter from LPC coefficients
void LPCeffect::filterCarrier() {

}