#pragma once
using namespace kfr;

class ShiftEffect {
public:
    inline explicit ShiftEffect(const int sampleRate);

    /**
     * @brief Shifts the input signal by the ratio.
     *
     * @param input The input signal.
     * @param shift Shift ratio.
     *
     * @return The shifted signal.
     */
    inline univector<float> shiftSignal(const univector<float>& input, float shift);

private:
    inline static void mulVectorWith(univector<float>& vec1, const univector<float>& vec2);
    inline static void mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);
    inline static univector<float> absOf(const univector<std::complex<float>>& ofBuffer);

    /**
     * @brief Converts a real-valued buffer to a complex buffer.
     *
     * @param ofBuffer The real-valued buffer.
     *
     * @return The complex buffer.
     */
    inline static univector<std::complex<float>> makeComplex(const univector<float>& ofBuffer);

    /**
      * @brief Applies the exponential function element-wise to a complex buffer.
      *
      * @param ofBuffer The input complex buffer.
      *
      * @return Element-wise exponentials of the input complex numbers.
      */
    inline static univector<std::complex<float>> expComplex(const univector<std::complex<float>>& ofBuffer);

    /**
    * @brief Modulo of a buffer with a scalar value.
    *
    * @param a The buffer to compute the modulo of.
    * @param b The modulus.
    *
    * @return Modulo-ed buffer.
    */
    inline static univector<float> modulo(const univector<float>& a, float b);

    /**
        * @brief FFT transform and filling the other half with zeroes.
        *
        * @param input The input signal.
        *
        * @return The output frequency domain coefficients.
        */
    inline univector<std::complex<float>> padFFT(const univector<float>& input) const;

    /**
       * @brief Cutting the other half (of zeroes) before performing IFFT.
       *
       * @param input The input frequency domain coefficients.
       *
       * @return A real-valued signal.
       */
    inline static univector<float> cutIFFT(const univector<std::complex<float>>& input) ;

    const float pi = 2 * acos(0.0);
    int LEN = 0;
    univector<fbase, 512> hannWindowS = window_hann(512);
    univector<fbase, 1024> hannWindowM = window_hann(1024);
    univector<fbase, 2048> hannWindowL = window_hann(2048);
    int synthesisHop = 0;

    enum class WindowLenEnum {
        S, M, L
    };
    WindowLenEnum windowLenEnum;

    std::unordered_map<WindowLenEnum, univector<fbase>> hannWindow = {
            {WindowLenEnum::S, hannWindowS},
            {WindowLenEnum::M, hannWindowM},
            {WindowLenEnum::L, hannWindowL}
    };

    univector<float> psi;
    univector<int> ramp;
    univector<float> omega;
    univector<std::complex<float>> fftGrain;
    univector<float> phi;
    univector<float> previousPhi;
    univector<float> delta;
    univector<float> f1;
    univector<std::complex<float>> corrected;
};