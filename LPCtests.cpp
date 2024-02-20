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
        for (int i = 1; i < 999; ++i) {
            int sample = i;

            // logic
            inputBuffer.setSample(channel, index, sample);
            ++index;
            if (index == frameSize) {
                index = 0;
                // Nframes in each channel
                for (int channel = 0; channel < 1; ++channel) {
                    float* frameChPtr = frameBuffer.getWritePointer(channel);

                    for (int i = 0; i < Nframes; ++i) {
                        frameBuffer.clear();
                        int startOfFrame = i / Nframes * windowSize;
                        // copy a frame from inputBuffer to frameBuffer
                        frameBuffer.copyFrom(channel, 0, inputBuffer, channel, startOfFrame, frameSize);
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

                // verification
                for (int g = 0; g < overlapBuffer.getNumSamples(); ++g) {
                    float val = overlapBuffer.getSample(channel, g);
                    std::cout << val << " ";
                }

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
            std::cout << "PASSED autocorrelation test";
        else
            std::cout << "DID NOT PASS autocorrelation test";
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
            std::cout << LPCcoeffs[i] << "   " << expected[i] << "\n";
            if (std::abs(LPCcoeffs[i] - expected[i]) > 0.01)
                pass = false;
        }
        if (pass)
            std::cout << "PASSED levinsonDurbin test";
        else
            std::cout << "DID NOT PASS levinsonDurbin test";
    }

    void residualsTest() {
        // data setup
        const int modelOrder = 4;
        const int frameSize = 16;
        std::array<float, 6> LPCcoeffs = {-0.7, -0.68, -0.02, -0.67, 0.59, -0.87};
        juce::AudioBuffer<float> overlapBuffer(1, frameSize);
        juce::AudioBuffer<float> filteredBuffer(1, frameSize);

        // logic
        for (int channel = 0; channel < 1; ++channel) {
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
        for (int channel = 0; channel < 1; ++channel) {
            for (int i = 0; i < frameSize; ++i) {
                float difference = overlapBuffer.getSample(channel, i) - filteredBuffer.getSample(channel, i);
                residuals.setSample(channel, i, difference);
            }
        }

        // verification

    }

};