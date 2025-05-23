// BitCrusher.cpp
#include "BitCrusher.h"

BitCrusher::BitCrusher() :
    bitDepth(16.0f),           // Default bit depth
    rateReductionFactor(1),    // Default no rate reduction
    quantizationStep(0.0f),
    lastSample(0.0f),
    sampleCounter(0)
{
    updateQuantizationStep(); // Initialize quantizationStep based on default bitDepth
}

BitCrusher::~BitCrusher()
{
    // Cleanup if needed
}

void BitCrusher::setBitDepth(float newBitDepth)
{
    bitDepth = std::max(1.0f, newBitDepth); // Ensure bit depth is at least 1
    updateQuantizationStep();
}

void BitCrusher::setRateReduction(int newRateReductionFactor)
{
    //rateReductionFactor = std::max(1, newRateReductionFactor); // Ensure factor is at least 1
    int newFactorClamped = std::max(1, newRateReductionFactor);
    if (newFactorClamped != rateReductionFactor) { 
        sampleCounter = 0; 
    }
    rateReductionFactor = newFactorClamped;

}

void BitCrusher::prepare(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    // Reset internal state. If your bitcrusher logic depended on sampleRate,
    // you'd store and use it here.
    reset();
}

void BitCrusher::reset()
{
    lastSample = 0.0f;
    sampleCounter = 0;
    // Potentially re-initialize other state if needed
}

float BitCrusher::processSample(float inputSample)
{
    float processedSample = inputSample;

    // Rate Reduction (Downsampling)
    if (rateReductionFactor > 1)
    {
        if (sampleCounter == 0)
        {
            lastSample = processedSample; // Hold the sample
        }
        processedSample = lastSample; // Output the held sample
        sampleCounter = (sampleCounter + 1) % rateReductionFactor;
    }
    else // No rate reduction, so always reset counter
    {
        sampleCounter = 0;
    }


    // Bit Reduction (Quantization)
    if (quantizationStep > 0.0f) // Avoid division by zero if bitDepth is huge or invalid
    {
        // Quantize: scale, floor, then scale back. Adding 0.5f before floor rounds to nearest.
        processedSample = quantizationStep * std::floor(processedSample / quantizationStep + 0.5f);
    }
    // Optional: Clamp to a specific range if your audio isn't guaranteed to be, e.g. -1.0 to 1.0
    // processedSample = std::max(-1.0f, std::min(1.0f, processedSample));

    return processedSample;
}

void BitCrusher::processBlock(float* channelData, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        channelData[i] = processSample(channelData[i]);
    }
}

void BitCrusher::updateQuantizationStep()
{
    // Calculate the number of quantization levels
    // For bipolar signals typically in the range [-1.0, 1.0], the total dynamic range is 2.0.
    // numLevels = 2^bitDepth
    // quantizationStep = TotalRange / numLevels
    if (bitDepth >= 1.0f && bitDepth <= 32.0f) // Practical limits for float precision
    {
        // For bit depths like 1 or 2, std::pow(2.0f, bitDepth) is small, leading to large steps.
        // For high bit depths, std::pow becomes very large, step becomes very small.
        quantizationStep = 2.0f / std::pow(2.0f, bitDepth);
    }
    else if (bitDepth > 32.0f) // Effectively no quantization for very high bit depths
    {
        quantizationStep = 0.0f; // Or a very small number if you want to avoid the if in processSample
    }
    else // bitDepth < 1, treat as 1 bit (sign bit essentially)
    {
        quantizationStep = 2.0f / std::pow(2.0f, 1.0f); // step will be 1.0
    }
}