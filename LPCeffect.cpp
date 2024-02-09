#include "LPCeffect.h"

// constructor
LPCeffect::LPCeffect() : inputBuffer(numChannels, windowSize),
                         frameBuffer(numChannels, windowSize / Nframes),
                         overlapBuffer(numChannels, windowSize),
                         hannWindow(windowSize / Nframes, juce::dsp::WindowingFunction<float>::WindowingMethod::hann)
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
            frameBuffer.copyFrom(channel, 0, inputBuffer, channel, startOfFrame, windowSize / Nframes);
            hannWindow.multiplyWithWindowingTable(frameChPtr, windowSize / Nframes);
            autocorrelation();
            // add frames with overlap to overlapBuffer
            // ...
        }
    }
}
// calculate autocorrelation coefficients
float LPCeffect::autocorrelation()
{

}