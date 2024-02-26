// manual, copy-over tests for initial verification of logic
// every result value is reached with pen & paper and other tools when applicable
class LPCtests {

    const int channel = 0;

    public:

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
            inputBuffer.setSample(channel, index, sample);
            ++index;
            if (index == windowSize) {
                index = 0;
                // inputBuffer: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
                // Nframes in each channel
                for (int channel = 0; channel < 1; ++channel) {
                    float* frameChPtr = frameBuffer.getWritePointer(channel);
                    // initialize overlapBuffer with 0s
                    for (int j = 0; j < windowSize; ++j) {
                        overlapBuffer.setSample(channel, j, 0);
                    }

                    for (int i = 0; i < Nframes; ++i) {
                        frameBuffer.clear();
                        int startOfFrame = i * windowSize / Nframes ;
                        // copy a frame from inputBuffer to frameBuffer
                        frameBuffer.copyFrom(channel, 0, inputBuffer, channel, startOfFrame, frameSize);
                        // perform OLA one frame at a time
                        hannWindow.multiplyWithWindowingTable(frameChPtr, frameSize);
                        /* frame values:
                            0 1.65836 6.51246 8.68328 4.1459 0
                            0 6.63344 19.5374 21.7082 9.12097 0
                            0 11.6085 32.5623 34.7331 14.0961 0
                            0 16.5836 45.5872 47.758 19.0711 0
                        */
                        if (i == 0)
                            overlapBuffer.addFrom(channel, 0, frameBuffer, channel, 0, frameSize);
                        else {
                            int startSample = i * frameSize - (frameSize / 2) * i;
                            overlapBuffer.addFrom(channel, startSample, frameBuffer, channel, 0, frameSize);
                        }
                    }
                }

                // verification
                bool pass = true;
                for (int g = 0; g < windowSize; ++g) {
                    std::array<float, 24> expected = {0,1.66,6.51,8.68,10.78,19.54,21.71,20.73,32.56,34.73,30.67,45.59,47.76,19.07,0,0,0,0,0,0,0,0,0,0};
                    float val = overlapBuffer.getSample(channel, g);
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

    void autocorrelationTest()
    {
        // data setup
        std::array<float, 8> input = {2,3,4,6,9,7,2,1};
        const int frameSize = input.size();
        juce::AudioBuffer<float> frameBuffer(1, frameSize);
        std::vector<float> corrCoeff;
        for (int i = 0; i < frameSize; ++i) {
            frameBuffer.setSample(channel, i, input[i]);
        }

        // logic
        corrCoeff.clear();
        // coefficients go up to frameSize - 1
        // denominator and mean remain the same
        float denominator = 0;
        float mean = 0;
        for (int i = 0; i < frameSize; ++i) {
            mean += frameBuffer.getSample(channel, i);
        }
        mean /= frameSize;
        for (int n = 0; n < frameSize; ++n) {
            float sample = frameBuffer.getSample(channel, n);
            denominator += std::pow((sample - mean), 2);
        }

        for (int k = 0; k < frameSize; ++k) {
            float numerator = 0;
            for (int n = 0; n < frameSize - k; ++n) {
                float sample = frameBuffer.getSample(channel, n);
                float lagSample = frameBuffer.getSample(channel, n + k);
                numerator += (sample - mean) * (lagSample - mean);
            }
            corrCoeff.push_back(numerator / denominator);
        }

        // verification
        bool pass = true;
        std::array<float, 8> expected = {1, 0.454, -0.318, -0.539, -0.347, -0.046, 0.164, 0.132};
        for (int k = 0; k < frameSize; ++k) {
            if (std::abs(corrCoeff[k] - expected[k]) > 0.01)
                pass = false;
        }
        if (pass)
            std::cout << "PASSED autocorrelation test\n";
        else
            std::cout << "DID NOT PASS autocorrelation test\n";
    }

    void levinsonDurbinTest()
    {
        // data setup
        const int modelOrder = 4;
        std::vector<float> corrCoeff = {1, 0.7, 0.5, 0.8};
        std::vector<float> LPCcoeffs;

        // logic
        std::vector<float> k (modelOrder);
        std::vector<float> E (modelOrder);
        // two-dimensional vector because matrix
        // j = row, i = column
        std::vector<std::vector<float>> a (modelOrder, std::vector<float>(modelOrder));

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

        /*for (int row = 1; row < modelOrder; ++row) {
            for (int col = 1; col < modelOrder; ++col) {
                std::cout << a[row][col] << " | ";
            }
            std::cout << "\n";
        }*/

        for (int col = 1; col < modelOrder; ++col) {
            for (int row = 1; row < modelOrder; ++row) {
                if (a[row][col] != 0)
                    LPCcoeffs.push_back(a[row][col]);
            }
        }
        a.clear();
        E.clear();
        k.clear();

        // verification
        bool pass = true;
        std::array<float, 6> expected = {-0.7, -0.686, -0.0196, -0.67, 0.59, -0.87};
        for (int i = 0; i < LPCcoeffs.size(); ++i) {
            if (std::abs(LPCcoeffs[i] - expected[i]) > 0.01)
                pass = false;
        }
        if (pass)
            std::cout << "PASSED levinsonDurbin test\n";
        else
            std::cout << "DID NOT PASS levinsonDurbin test\n";
    }

    void residualsTest() {
        // data setup
        const int modelOrder = 4;
        const int windowSize = 24;
        std::array<float, 6> LPCcoeffs = {-0.7, -0.68, -0.02, -0.67, 0.59, -0.87};
        std::array<float, 24> OLAbufferData = {0,1.66,6.51,8.68,10.78,19.54,21.71,20.73,32.56,34.73,30.67,45.59,47.76,19.07,0,0,0,0,0,0,0,0,0,0};
        juce::AudioBuffer<float> overlapBuffer(1, windowSize);
        juce::AudioBuffer<float> filteredBuffer(1, windowSize);
        for (int i = 0; i < OLAbufferData.size(); ++i)
            overlapBuffer.setSample(0, i, OLAbufferData[i]);

        // logic
        // initialize filteredBuffer with 0s
        for (int j = 0; j < windowSize; ++j) {
            filteredBuffer.setSample(channel, j, 0);
        }
        for (int channel = 0; channel < 1; ++channel) {
            for (int n = 1; n < windowSize; ++n) {
                float filtered = 0;
                for (int k = 0; k < modelOrder; ++k) {
                    if (n - k >= 0)
                        filtered += LPCcoeffs[k] * overlapBuffer.getSample(channel, n - k) + overlapBuffer.getSample(channel, n);
                }
                filteredBuffer.setSample(channel, n, filtered);
            }
        }

        for (int channel = 0; channel < 1; ++channel) {
            for (int i = 0; i < windowSize; ++i) {
                std::cout << overlapBuffer.getSample(channel, i) << " -> " << filteredBuffer.getSample(channel, i) << "\n";
            }
        }
        //TODO: residuals likely unnecessary?
        juce::AudioBuffer<float> residuals(1, windowSize);
        // subtracting the original OLA and filtered OLA signals to get residuals
        for (int channel = 0; channel < 1; ++channel) {
            for (int i = 0; i < windowSize; ++i) {
                float difference = abs(overlapBuffer.getSample(channel, i) - filteredBuffer.getSample(channel, i));
                residuals.setSample(channel, i, difference);
            }
        }

        // verification
        for (int channel = 0; channel < 1; ++channel) {
            for (int i = 0; i < windowSize; ++i) {
                std::cout << residuals.getSample(channel, i) << " ";
            }
        }
    }

};