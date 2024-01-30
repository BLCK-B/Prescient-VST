#include "FFTProcessor.h"

FFTProcessor::FFTProcessor() :
    fft(fftOrder),
    window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false)
{
    // Note that the window is of length `fftSize + 1` because JUCE's windows
    // are symmetrical, which is wrong for overlap-add processing. To make the
    // window periodic, set size to 1025 but only use the first 1024 samples.
}

void FFTProcessor::reset()
{
    count = 0;
    pos = 0;
    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
    std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);
}

void FFTProcessor::processBlock(float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i) {
        data[i] = processSample(data[i], 1.0);
    }
}

float FFTProcessor::processSample(float sample, float factor)
{   // pos = position
    inputFifo[pos] = sample;
    float outputSample = outputFifo[pos];
    outputFifo[pos] = 0.0f;
    // Advance the FIFO index and wrap around if necessary.
    pos += 1;
    if (pos == fftSize)
        pos = 0;
    // Process the FFT frame once we've collected hopSize samples.
    count += 1;
    if (count == hopSize) {
        count = 0;
        processFrame(factor);
    }
    return outputSample;
}

void FFTProcessor::processFrame(float factor)
{
    const float* inputPtr = inputFifo.data();
    float* fftPtr = fftData.data();
    // Copy the input FIFO into the FFT working space in two parts.
    std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
    if (pos > 0)
        std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));

    window.multiplyWithWindowingTable(fftPtr, fftSize);
    fft.performRealOnlyForwardTransform(fftPtr, true);
    processSpectrum(fftPtr, numBins, factor);
    fft.performRealOnlyInverseTransform(fftPtr);
    window.multiplyWithWindowingTable(fftPtr, fftSize);

    // Scale down the output samples because of the overlapping windows.
    for (int i = 0; i < fftSize; ++i) {
        fftPtr[i] *= windowCorrection;
    }
    // Add the IFFT results to the output FIFO.
    for (int i = 0; i < pos; ++i) {
        outputFifo[i] += fftData[i + fftSize - pos];
    }
    for (int i = 0; i < fftSize - pos; ++i) {
        outputFifo[i + pos] += fftData[i];
    }
}

void FFTProcessor::processSpectrum(float* data, int numBins, float factor)
{
    auto* cdata = reinterpret_cast<std::complex<float>*>(data);

    for (int i = 0; i < numBins; ++i) {
        float magn = std::abs(cdata[i]);
        float phase = std::arg(cdata[i]);
        //int newIndex = static_cast<int>(i * factor);
        int newIndex = std::floor(i * factor);

        while (newIndex > numBins) {
            newIndex -= numBins;
        }
        while (newIndex < 0) {
            newIndex += (numBins + newIndex);
        }

        cdata[newIndex] = std::polar(magn, phase);
    }
}
