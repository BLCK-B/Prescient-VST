// manual, copy-over tests for initial verification of logic
// every result value is reached with pen & paper and other tools when applicable
class LPCtests {

    const int channel = 0;

    public:

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
        std::vector<float> corrCoeff = {2, 3, 1, 5};
        std::vector<float> LPCcoeffs;

        // logic
        std::vector<float> k (modelOrder);
        std::vector<float> E (modelOrder);
        // two-dimensional vector because matrix
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

        for (int row = 1; row < modelOrder; ++row) {
            for (int col = 1; col < modelOrder; ++col) {
                std::cout << a[row][col] << " | ";
            }
            std::cout << "\n";
        }

        for (int col = 1; col < modelOrder; ++col) {
            for (int row = 1; row < modelOrder; ++row) {
                LPCcoeffs.push_back(a[row][col]);
            }
        }
        a.clear();
        E.clear();
        k.clear();
        for (float coeff : LPCcoeffs) {
        //    std::cout << coeff << "\n";
        }

        // verification

    }


};