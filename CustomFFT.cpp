#include <iostream>
#include <cmath>
#include <juce_audio_formats/codecs/flac/compat.h>

using namespace std;

//this is my own implementation of a forward FFT since JUCE's was not clear how to use
//thanks to comparing, I managed to get the JUCE implementation to produce correct
//results, at a faster rate, so I'm leaving this here just in case I will need it
class CustomFFT {
public:
    CustomFFT(int size) {
        N = size;
    }

    void CustomFFT::forwardTransform(int inputArray[]) {
        //precalculating twiddle factors because they can be reused
        //real and imaginary values are interleaved: 0 = real, 1 = imaginary, 2 = real, ...
        //total number of Wi is n(max)*k(max), since we assume that FFT size and N are equal, but to avoid
        //calculating (N-1)^2 values, we use the fact that Wi start repeating every N
        //thus we calculate 0 to N factors and later refer to them as %N
        for (int kn = 0; kn < N; ++kn) {
            //euler formula
            float Wreal = cos(-2 * M_PI * kn / N);
            float Wimag = sin(-2 * M_PI * kn / N);
            if (abs(Wreal) < 0.01)
                Wreal = 0;
            if (abs(Wimag) < 0.01)
                Wimag = 0;
            WrealVector.push_back(Wreal);
            WimagVector.push_back(Wimag);
            //cout << "factor W" << kn << ": " << Wreal << " " << Wimag << "j" << "\n";
        }
        //formula
        for (int k = 0; k <= N - 1; ++k) {
            float Xreal = 0;
            float Ximag = 0;
            //sum of x[n] * W[kn]
            for (int n = 0; n <= N - 1; ++n) {
                Xreal += inputArray[n] * WrealVector[(k * n) % N];
                Ximag += inputArray[n] * WimagVector[(k * n) % N];
            }
            if (abs(Xreal) < 0.01)
                Xreal = 0;
            if (abs(Ximag) < 0.01)
                Ximag = 0;
            outputArray.push_back(Xreal);
            outputArray.push_back(Ximag);
        }
        WrealVector.clear();
        WimagVector.clear();

        /*cout << "result of forward FFT:\n";
        for (int f = 0; f < N; ++f) {
            cout << "x" << f << " = " << outputArray[2 * f] << " ";
            cout << outputArray[2 * f + 1] << "j\n";
        }*/
    }

private:
    //FFT size
    int N;
    //lists with calculated twiddle factors
    vector<float> WrealVector;
    vector<float> WimagVector;
    //list of results of forward FFT with interleaved real and imaginary numbers
    vector<float> outputArray;
};


