// rather manual tests for initial verification of values
class LPCtests {

    public:

    void autocorrelationTest()
    {
        // data setup
        std::vector<float> input = {0.5, 0.4, 0.3, 0.2, 0.5, 0.4, 0.2, 0.1, 0.5, 0.8};
        const int windowSize = input.size();
        const int modelOrder = 6;
        juce::AudioBuffer<float> inputBuffer(1, windowSize);
        std::vector<float> corrCoeff;
        for (int i = 0; i < windowSize; ++i) {
            inputBuffer.setSample(0, i, input[i]);
        }
        // logic
        // coefficients go up to frameSize, but since levinson needs modelOrder: cycling lag "k" 0 to modelOrder
        // denominator and mean remain the same
        float denominator = 0.f;
        float mean = 0.f;
        for (int i = 0; i < windowSize; ++i) {
            mean += inputBuffer.getSample(0, i);
        }
        mean /= windowSize;
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

        // verification
        bool pass = true;
        std::vector<float> expected = {1.0, 0.1732, -0.5073, -0.2528, 0.2726, 0.1341, -0.3024};
        for (int k = 0; k <= modelOrder; ++k) {
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
        const int modelOrder = 6;
        std::vector<float> corrCoeff = {1.0, 0.1732, -0.5073, -0.2528, 0.2726, 0.1341, -0.3024};
        std::vector<float> LPCcoeffs;
        // ensure enough autocorr coeffs, not ideal
        corrCoeff.push_back(0);
        corrCoeff.push_back(0);

        // logic
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
        float kTo2 = pow(k[0], 2);
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

            kTo2 = pow(k[i-1],2);
            E[i-1] = (1 - kTo2) * E[i - 2];
        }
    //    std::cout<<"LPC coeffs\n";
    //    for (int col = 0; col <= modelOrder; ++col) {
    //        for (int row = 0; row <= modelOrder; ++row) {
    //            std::cout << setprecision(5) << a[row][col] << " ";
    //        }
    //        std::cout << "\n";
    //    } std::cout << "\n\n";
        for (int x = 0; x <= modelOrder; ++x) {
            LPCcoeffs.push_back(a[modelOrder][modelOrder - x]);
        }

        // verification
        bool pass = true;
        std::vector<float> expected = {1.0, -0.2368, 0.4725, 0.1604, -0.0292, 0.0993, 0.2139};
        for (int i = 0; i <= modelOrder; ++i) {
            if (std::abs(LPCcoeffs[i] - expected[i]) > 0.01)
                pass = false;
        }
        if (pass)
            std::cout << "PASSED levinsonDurbin test\n";
        else
            std::cout << "DID NOT PASS levinsonDurbin test\n";

    }

    void filterTest() {
        // data setup
        const int modelOrder = 6;
        std::vector<float> LPCcoeffs = {1.0, -0.2368, 0.4725, 0.1604, -0.0292, 0.0993, 0.2139};
        std::vector<float> carrier = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.1};
        const int windowSize = carrier.size();
        juce::AudioBuffer<float> inputBuffer(1, windowSize);
        juce::AudioBuffer<float> filteredBuffer(1, windowSize);
        for (int i = 0; i < windowSize; ++i) {
            inputBuffer.setSample(0, i, carrier[i]);
            filteredBuffer.setSample(0, i, 0);
        }
        // logic
        for (int n = 0; n < windowSize; ++n) {
            float sum = inputBuffer.getSample(0, n);
            for (int i = 1; i <= modelOrder; ++i) {
                if (n - i + 1 > 0)
                    sum += LPCcoeffs[i] * inputBuffer.getSample(0, n - i);
            }
            filteredBuffer.setSample(0, n, sum);
        }

        // verification
        bool pass = true;
        std::vector<float> expected = {0.1, 0.1763, 0.2999, 0.4395, 0.5762, 0.7228, 0.8908, 1.0588, 1.2269, 0.4949};
        for (int i = 0; i < windowSize; ++i) {
            if (std::abs(filteredBuffer.getSample(0, i) - expected[i]) > 0.01)
                pass = false;
        }
        if (pass)
            std::cout << "PASSED filter test\n";
        else
            std::cout << "DID NOT PASS filter test\n";
    }

};