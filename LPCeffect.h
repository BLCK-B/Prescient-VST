#pragma once
#include <kfr/base.hpp>
#include <kfr/dft.hpp>
#include <kfr/dsp.hpp>
#include "ShiftEffect.cpp"

using namespace kfr;

class LPCeffect {
public:
    explicit LPCeffect(const int sampleRate);
    [[nodiscard]] int getLatency() const {
        return windowSize;
    }

    /**
     * @brief Sends sample to buffer collection to be eventually processed in effect chain.
     *
     * @param carrierSample The input carrier (excitation) sample.
     * @param voiceSample The input voice sample.
     * @param modelOrder The model order for LPC analysis.
     * @param shiftVoice1 The first voice shift ratio.
     * @param shiftVoice2 The second voice shift ratio.
     * @param shiftVoice3 The third voice shift ratio.
     * @param enableLPC Enable or disable LPC vocoder effect.
     * @param passthrough Dry / wet control.
     *
     * @return The processed output sample.
     */
    float sendSample(float carrierSample, float voiceSample, float modelOrder, float shiftVoice1,  float shiftVoice2, float shiftVoice3, bool enableLPC, float passthrough);

private:
    enum class FFToperation {
        Convolution, IIR
    };

    /**
     * @brief The LPC effect.
     *
     * @param voice The voice signal.
     * @param carrier The carrier (excitation) signal.
     *
     * @return The cross-synthesis processed signal.
     */
    univector<float> processLPC(const univector<float>& voice, const univector<float>& carrier);

    /**
     * @brief Processes collected buffers using the effect chain.
     *
     * @param overwrite The buffer to overwrite with the output.
     * @param voice The voice signal.
     * @param carrier The carrier (excitation) signal.
     * @param shiftVoice1 The first voice shift ratio.
     * @param shiftVoice2 The second voice shift ratio.
     * @param shiftVoice3 The third voice shift ratio..
     * @param enableLPC Enable or disable LPC vocoder effect.
     * @param passthrough Dry / wet control.
     */
    void processing(univector<float>& overwrite, const univector<float>& voice, const univector<float>& carrier, float shiftVoice1,  float shiftVoice2, float shiftVoice3, bool enableLPC, float passthrough);

    /**
   * @brief Performs FFT-based operations (convolution or IIR filtering).
   *
   * @param o FFT operation (Convolution or IIR filter).
   * @param inputBuffer The input signal to which the operation is applied.
   * @param coefficients The LPC coefficients.
   *
   * @return Convolution or filter output.
   */
    univector<float> FFToperations(FFToperation o, const univector<float>& inputBuffer, const univector<float>& coefficients);

    /**
     * @brief Calculates the autocorrelation of a signal.
     *
     * @param fromBufer Input signal.
     *
     * @return Equal length output coefficients vector.
     */
    static univector<float> autocorrelation(const univector<float>& fromBufer);

    /**
    * @brief Performs the Levinson-Durbin recursion for LPC analysis.
    *
    * @param ofBuffer Autocorrelation coefficients.
    *
    * @return LPC coefficients.
    */
    [[nodiscard]] univector<float> levinsonDurbin(const univector<float>& ofBuffer) const;

    /**
      * @brief Extracts the residual signal after LPC analysis.
      *
      * @param ofBuffer An input signal.
      *
      * @return Residual signal.
      */
    univector<float> getResiduals(const univector<float>& ofBuffer);

    /**
     * @brief Matches the power of the input signal to the reference signal.
     *
     * @param input The signal to be adjusted.
     * @param reference The reference signal.
     */
    void matchPower(univector<float>& input, const univector<float>& reference) const;

    static void mulVectorWith(univector<float>& vec1, const univector<float>& vec2);
    static void mulVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);
    static void divVectorWith(univector<std::complex<float>>& vec1, const univector<std::complex<float>>& vec2);

    int windowSize = 0;

    univector<fbase, 1024> hannWindowS = window_hann(1024);
    univector<fbase, 2048> hannWindowM = window_hann(2048);
    univector<fbase, 4096> hannWindowL = window_hann(4096);

    enum class WindowSizeEnum {
        S, M, L
    };
    WindowSizeEnum windowSizeEnum;

    std::unordered_map<WindowSizeEnum, univector<fbase>> hannWindow = {
            {WindowSizeEnum::S, hannWindowS},
            {WindowSizeEnum::M, hannWindowM},
            {WindowSizeEnum::L, hannWindowL}
    };

    int index1 = 0;
    int index2 = 0;

    const float overlap = 0.5;
    int overlapSize = 0;
    int hopSize = 0;

    int frameModelOrder = 70;

    univector<float> carrierBuffer1;
    univector<float> carrierBuffer2;
    univector<float> sideChainBuffer1;
    univector<float> sideChainBuffer2;
    univector<float> filteredBuffer1;
    univector<float> filteredBuffer2;

    ShiftEffect* shiftEffect;
};