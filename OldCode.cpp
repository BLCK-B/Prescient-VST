#include "OldCode.h"

// calculate filter parameters using this algorithm
void LPCeffect::levinsonDurbin()
{
    // levinson durbin algorithm
    std::vector<float> k (modelOrder + 1);
    std::vector<float> E (modelOrder + 1);
    // matrix of LPC coefficients a[j][i] j = row, i = column
    std::vector<std::vector<float>> a (modelOrder + 1, std::vector<float>(modelOrder + 1));

    // initialization with i = 1
//    E[0] = a[0][0] = corrCoeff[0];
    // R[0] = 1 in any case
    E[0] = a[0][0] = 1.0;
    k[1] = - corrCoeff[1] / E[0];
    a[1][1] = k[1];
    float kPw2 = std::pow(k[1], 2);
    E[1] = (1 - kPw2) * E[0];

    for (int i = 0; i <= modelOrder; ++i) {
        a[0][i] = 1;
    }

    // loop beginning with i = 2
    for (int i = 2; i <= modelOrder; ++i) {
        float sumResult = 0.f;
        for (int j = 1; j < i; ++j) {
            sumResult += a[j][i - 1] * corrCoeff[i - j];
        }
        k[i] = - (corrCoeff[i] + sumResult) / E[i - 1];

        jassert(abs(k[i]) <= 1);

        a[i][i] = k[i];
        for (int j = 1; j < i; ++j) {
            a[j][i] = a[j][i - 1] + k[i] * a[i - j][i - j];
        }
        kPw2 = std::pow(k[i], 2);
        E[i] = (1 - kPw2) * E[i - 1];
    }
    /*  result in the form of
     *  a[1][1] a[1][2] a[1][3]
     *          a[2][2] a[2][3]
     *                  a[3][3]
     *  formant frequency information (envelope)
     *  saving the reflection diagonal coefficients: */
    for (int x = 0; x <= modelOrder; ++x) {
        LPCcoeffs.push_back(a[x][modelOrder]);
//        std::cout << a[x][modelOrder] << "\n";
    }
    a.clear();
    E.clear();
    k.clear();
}

// synthesis filter
// FIFO buffer of modelOrder previous samples
    std::vector<float> previousSamples(modelOrder + 1, 0.0f)
    for (int n = 0; n < windowSize; ++n) {
        float sum = inputBuffer.getSample(0, n);
        for (int i = 0; i < modelOrder; ++i) {
            sum -= LPCcoeffs[i] * previousSamples[i];
        }
        // move all to right
        for (int g = modelOrder - 1; g > 0; --g) {
            previousSamples[g] = previousSamples[g - 1];
        }
        // add last to beginning
        previousSamples[0] = sum;
        // output
        filteredBuffer.setSample(0,n,sum);

        // log autocorr coeff values: 0 < x <= 1
        //filteredBuffer.setSample(0, n, corrCoeff[n % corrCoeff.size()]);
        // log LPC coeffs values
        //filteredBuffer.setSample(0, n, LPCcoeffs[n % modelOrder]);
    }

void OLAtest() {
    // data setup
    const int windowSize = 24;
    const int Nframes = 4;
    const int frameSize = windowSize / Nframes;
    int index = 0;
    juce::AudioBuffer<float> inputBuffer(1, windowSize);
    juce::AudioBuffer<float> overlapBuffer(1, windowSize);
    juce::AudioBuffer<float> frameBuffer(1, frameSize);
    juce::dsp::WindowingFunction<float> hannWindow(frameSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann);
    // input emulation
    for (int s = 1; s < 999; ++s) {
        int sample = s;

        // logic
        inputBuffer.setSample(0, index, sample);
        ++index;
        if (index == windowSize) {
            index = 0;
            // inputBuffer: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
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
                // perform OLA one frame at a time
                hannWindow.multiplyWithWindowingTable(frameChPtr, frameSize);
                /* frame values:
                    0 1.65836 6.51246 8.68328 4.1459 0
                    0 6.63344 19.5374 21.7082 9.12097 0
                    0 11.6085 32.5623 34.7331 14.0961 0
                    0 16.5836 45.5872 47.758 19.0711 0
                */
                if (i == 0)
                    overlapBuffer.addFrom(0, 0, frameBuffer, 0, 0, frameSize);
                else {
                    int startSample = i * frameSize - (frameSize / 2) * i;
                    overlapBuffer.addFrom(0, startSample, frameBuffer, 0, 0, frameSize);
                }
            }

            // verification
            bool pass = true;
            for (int g = 0; g < windowSize; ++g) {
                std::array<float, 24> expected = {0,1.66,6.51,8.68,10.78,19.54,21.71,20.73,32.56,34.73,30.67,45.59,47.76,19.07,0,0,0,0,0,0,0,0,0,0};
                float val = overlapBuffer.getSample(0, g);
                if (std::abs(val - expected[g]) > 0.01)
                    pass = false;
            }
            if (pass)
                std::cout << "PASSED OLA test\n";
            else
                std::cout << "DID NOT PASS OLA test\n";
            break;
        }
    }
}