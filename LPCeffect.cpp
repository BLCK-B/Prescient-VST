#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         filteredBuffer(numChannels, windowSize),
                         LPCcoeffs(modelOrder)
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

    // analysis and debug logging
//    logValues(sample, output);

    if (output == NULL)
        return 0.f;

    return output;
}

// split into frames > R coeff > hann > OLA > LPC coeffs > residuals
void LPCeffect::doLPC()
{
    // calculate coefficients
    autocorrelation();
//    levinsonDurbin();
    matlabLevinson();
    // filter overlapBuffer with all-pole filter (AR) from LPC coefficients
//    residuals();
    corrCoeff.clear();
    LPCcoeffs.clear();
}

// calculate autocorrelation coefficients of a single frame
void LPCeffect::autocorrelation()
{
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
//    logAC();
}

// calculate filter parameters using this algorithm
void LPCeffect::levinsonDurbin()
{
    std::vector<float> fakeAC = {
            1.00000, 0.98614, 0.94538, 0.87996, 0.79337, 0.68996, 0.57459, 0.45222, 0.32755, 0.20485, 0.08779, -0.02058,
            -0.11773, -0.20165, -0.27073, -0.32376, -0.35994, -0.37892, -0.38088, -0.36663, -0.33756, -0.29566,
            -0.24344, -0.18371, -0.11948, -0.05375, 0.01067, 0.07127, 0.12588, 0.17271, 0.21032, 0.23761, 0.25378,
            0.25834, 0.25116, 0.23258, 0.20344, 0.16521, 0.11999, 0.07048, 0.01982, -0.02858, -0.07133, -0.10538,
            -0.12826, -0.13838, -0.13508, -0.11872, -0.09057};
    corrCoeff.clear();
    corrCoeff = fakeAC;

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
        std::cout << "i = " << i << "\n";
        std::cout<< "- (" << corrCoeff[i] << " + " << sumResult << ") / " << E[i-1] << "  =  ";
        k[i] = - (corrCoeff[i] + sumResult) / E[i - 1];
        jassert(abs(k[i]) <= 1);
        std::cout << "a" << i<< "," << i << " = " << k[i]<<"\n";
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
    std::cout << "\n\n\n";
    a.clear();
    E.clear();
    k.clear();
}

void LPCeffect::matlabLevinson() {
    std::vector<float> fakeAC = {
            1.00000, 0.98614, 0.94538, 0.87996, 0.79337, 0.68996, 0.57459, 0.45222, 0.32755, 0.20485, 0.08779, -0.02058,
            -0.11773, -0.20165, -0.27073, -0.32376, -0.35994, -0.37892, -0.38088, -0.36663, -0.33756, -0.29566,
            -0.24344, -0.18371, -0.11948, -0.05375, 0.01067, 0.07127, 0.12588, 0.17271, 0.21032, 0.23761, 0.25378,
            0.25834, 0.25116, 0.23258, 0.20344, 0.16521, 0.11999, 0.07048, 0.01982, -0.02858, -0.07133, -0.10538,
            -0.12826, -0.13838, -0.13508, -0.11872, -0.09057};
    corrCoeff.clear();
    corrCoeff = fakeAC;

    std::vector<float> k(modelOrder + 1);
    std::vector<float> E(modelOrder + 1);
    // matrix of LPC coefficients a[j][i] j = row, i = column
    std::vector<std::vector<float>> a(modelOrder + 1, std::vector<float>(modelOrder + 1));
    // init
    for (int r = 0; r < modelOrder; ++r) {
        for (int c = 0; c < modelOrder; ++c) {
            a[r][c] = 0.f;
        }
    }
    for (int r = 0; r <= modelOrder; ++r) {
        a[r][r] = 1;
    }

    k[0] = - corrCoeff[1] / corrCoeff[0];
    a[0][1] = k[0];
    float kTo2 = pow(k[0], 2);
    E[0] = (1 - kTo2) * corrCoeff[0];

    // TODO: a[j][i] j = row, i = column
    for (int i = 2; i <= modelOrder; ++i) {
        float sum = 0.f;
        for (int j = 0; j <= i; ++j) {
            sum += a[j][i-1] * corrCoeff[j + 1];
        }
        k[i-1] = - sum / E[i - 1-1];
        a[0][i+1-1] = k[i-1];
        // alright
        for (int ge = 0; ge <= modelOrder; ++ge) {
            for (int ye = 0; ye <= modelOrder; ++ye) {
                std::cout << setprecision(4) << a[ye][ge] << " ";
            }
            std::cout << "\n";
        } std::cout << "\n";

        for (int j = 1; j < i; ++j) {

            std::cout << i<<","<<j<<"\n";
            a[j][i] = 5; // a[j-1][i] + k[i] * a[i-j][i];

            for (int ge = 0; ge <= modelOrder; ++ge) {
                for (int ye = 0; ye <= modelOrder; ++ye) {
                    std::cout << setprecision(4) << a[ye][ge] << " ";
                }
                std::cout << "\n";
            } std::cout << "\n";

        }

        kTo2 = pow(k[i],2);
        E[i] = (1 - kTo2) * E[i - 1];
    }
    std::cout << "end\n\n";

//    for (int x = 0; x < modelOrder; ++x) {
//        LPCcoeffs.push_back(a[modelOrder][modelOrder-x]);
//        std::cout << LPCcoeffs[x] << " ";
//    }
//    std::cout << "\nend\n";

//    for (int j = 0; j < modelOrder; ++j) {
//        for (int i = 0; i < modelOrder; ++i) {
//            std::cout << a[i][j] << " ";
//        }
//        std::cout << "\n";
//    } std::cout << "\n";
}

// filtering the buffer with all-pole filter from LPC coefficients
void LPCeffect::residuals()
{
    for (int j = 0; j < windowSize; ++j) {
        filteredBuffer.setSample(0, j, 0);
    }

    // white noise carrier signal
    for (int n = 0; n < windowSize; ++n) {
        float rnd = rand() % 1000;
        rnd /= 10000;
        inputBuffer.setSample(0, n, rnd);
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

        // log LPC coeffs values
        filteredBuffer.setSample(0, n, LPCcoeffs[n % modelOrder]);
//        filteredBuffer.setSample(0, n, corrCoeff[n % modelOrder]);
    }

    // synthesis filter
    // FIFO buffer of modelOrder previous samples
//    std::vector<float> previousSamples(modelOrder + 1, 0.0f)
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
//    }
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
            for (int i = 0; i < logInpBuffer.size(); ++i) {
                juce::String logMessage = juce::String(logInpBuffer[i], 5) + " -> " + juce::String(logOutBuffer[i], 5) + ",\n";
                stream -> writeString(logMessage);
            }
        }
        logInpBuffer.clear();
        logOutBuffer.clear();
    }
}

void LPCeffect::logAC() {
    juce::File logFile("C:/Users/legionntb/Desktop/jucelog.txt");
    std::unique_ptr<juce::FileOutputStream> stream(logFile.createOutputStream());
    if (stream) {
        stream->setPosition(0);
        for (float i : corrCoeff) {
            juce::String logMessage = juce::String(i, 5) + ",\n";
            stream -> writeString(logMessage);
        }
    }
}