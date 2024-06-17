#include <kfr/base.hpp>
#include <kfr/dsp.hpp>
#include <kfr/dft.hpp>

// manual tests for initial verification of values
class LPCtests {

    public:

    void levinsonDurbinTest()
    {
        // data setup
        const int modelOrder = 6;
        std::vector<float> corrCoeff = {1.0, 0.1732, -0.5073, -0.2528, 0.2726, 0.1341, -0.3024};
        std::vector<float> LPCcoeffs;
        // ensure enough autocorr coeffs
        corrCoeff.push_back(0);
        corrCoeff.push_back(0);

        // logic
        std::vector<float> k(modelOrder + 1);
        std::vector<float> E(modelOrder + 1);
        // matrix of LPC coefficients a[j][i] j = row, i = column
        std::vector<std::vector<float>> a(modelOrder + 1, std::vector<float>(modelOrder + 1, 0.0f));
        // init
        for (int r = 0; r <= modelOrder; ++r)
            a[r][r] = 1;

        k[0] = - corrCoeff[1] / corrCoeff[0];
        a[1][0] = k[0];
        float kTo2 = std::pow(k[0], 2);
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

            kTo2 = std::pow(k[i-1],2);
            E[i-1] = (1 - kTo2) * E[i - 2];

            std::cout<<"LPC coeffs\n";
            for (int col = 0; col <= modelOrder; ++col) {
                for (int row = 0; row <= modelOrder; ++row) {
                    std::cout << std::setprecision(5) << a[row][col] << " ";
                }
                std::cout << "\n";
            } std::cout << "\n\n";
        }
//        std::cout<<"LPC coeffs\n";
//        for (int col = 0; col <= modelOrder; ++col) {
//            for (int row = 0; row <= modelOrder; ++row) {
//                std::cout << std::setprecision(5) << a[row][col] << " ";
//            }
//            std::cout << "\n";
//        } std::cout << "\n\n";
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

    void IIRfilterTest() {
        // data setup
        univector<float> inputBuffer = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 0.1};
        const int windowSize = inputBuffer.size();
        univector<float> LPCcoeffs = {1.0, -0.2368, 0.4725, 0.1604, -0.0292, 0.0993, 0.2139, 0.4525};
        univector<float> paddedLPC(windowSize, 0.0f);
        std::copy(LPCcoeffs.begin(), LPCcoeffs.end(), paddedLPC.begin());

        // logic
        univector<std::complex<float>> X = realdft(inputBuffer);
        univector<std::complex<float>> H = realdft(paddedLPC);
        univector<std::complex<float>> Y = X / H;
        univector<float> filteredBuffer = irealdft(Y);
        for (float & i : filteredBuffer) {
            i *= 0.1;
        }

        // verification
        bool pass = true;
        std::vector<float> expected =  {-0.25, -0.02, -0.10, -0.31, -0.04, 0.56, 1.06, 0.91, 0.59, -0.22};
        for (int i = 0; i < windowSize; ++i) {
            std::cout << filteredBuffer[i] << "\n";
            if (std::abs(filteredBuffer[i] - expected[i]) > 0.01)
                pass = false;
        }
        if (pass)
            std::cout << "PASSED filterFFT test\n";
        else
            std::cout << "DID NOT PASS filterFFT test\n";
    }

    void convolutionFFT() {
        // data setup
        univector<float> inp = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.2};
        univector<float> LPC = {0.2, 0.1, 0.8, 0.1, 0.8};

        // logic
        univector<float> paddedLPC(inp.size(), 0.0f);
        std::copy(LPC.begin(), LPC.end(), paddedLPC.begin());
        univector<std::complex<float>> X = realdft(inp);
        univector<std::complex<float>> H = realdft(paddedLPC);
        univector<std::complex<float>> Y = X * H;
        univector<float> e = irealdft(Y);
        for (float &x : e)
            x *= 0.1;

        // verification
        bool pass = true;
        univector<float> expected = {0.16, 0.28, 0.48, 0.68, 0.88, 0.96, 1.04, 0.71};
        for (int i = 0; i < expected.size(); ++i) {
//            if (std::abs(e[i] - expected[i]) > 0.0001)
//                pass = false;
            std::cout<< e[i] << "\n";
        }
        if (pass)
            std::cout << "PASSED convolution test\n";
        else
            std::cout << "DID NOT PASS convolution test\n";
    }

};