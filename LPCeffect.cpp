#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         frameBuffer(numChannels, frameSize),
                         overlapBuffer(numChannels, windowSize),
                         hannWindow(frameSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann)
{
}
// add received sample to buffer, send to process once buffer full
float LPCeffect::sendSample(float sample, int channel)
{
    inputBuffer.setSample(channel, index, sample);
    if (channel == 1) {
        ++index;
        if (index == windowSize) {
            index = 0;
            LPCeffect();
        }
    }
}
// split into frames > hann > calculate autocorrelation for each frame > OLA
void LPCeffect::overlapAdd()
{
    // Nframes in each channel
    for (int channel = 0; channel < numChannels; ++channel) {
        float* frameChPtr = inputBuffer.getWritePointer(channel);

        for (int i = 0; i < Nframes; ++i) {
            int startOfFrame = i / Nframes * windowSize;
            frameBuffer.clear();
            frameBuffer.copyFrom(channel, 0, inputBuffer, channel, startOfFrame, frameSize);
            hannWindow.multiplyWithWindowingTable(frameChPtr, frameSize);
            autocorrelation(channel);
            // add frames with overlap to overlapBuffer
            // ...
        }
    }
}
// calculate autocorrelation coefficients
float LPCeffect::autocorrelation(int channel)
{
    // cycling lag "k" 0 up to modelOrder
    for (int k = 0; k < modelOrder; ++k) {
        float sumResult = 0;
        // loop representing sum
        for (int n = 0; n < frameSize - 1 - k; ++n) {
            float sample = frameBuffer.getSample(channel, n);
            float lagSample = frameBuffer.getSample(channel, n + k);
            sumResult += sample * lagSample;
        }
        sumResult /= frameSize;
        autocorrCoeffs.push_back(sumResult);
    }
    // now we have R(0 - modelOrder) vector

}