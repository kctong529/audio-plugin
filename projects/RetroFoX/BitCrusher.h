#ifndef BITCRUSHER_H_INCLUDED
#define BITCRUSHER_H_INCLUDED

#include <cmath>       // For std::pow, std::floor
#include <algorithm>   // For std::max, std::min

class BitCrusher
{
public:
    BitCrusher();
    ~BitCrusher();

    void setBitDepth(float newBitDepth);
    void setRateReduction(int newRateReductionFactor);

    // Call this in your PluginProcessor's prepareToPlay
    void prepare(double sampleRate, int samplesPerBlock);

    // Call this in your PluginProcessor's releaseResources
    void reset();

    // Process a single sample (option 1)
    float processSample(float inputSample);

    // Process a block of samples for a single channel (option 2)
    void processBlock(float* channelData, int numSamples);

private:
    void updateQuantizationStep();

    float bitDepth;
    int rateReductionFactor;
    float quantizationStep;

    // For rate reduction
    float lastSample;
    int sampleCounter;

    // You might add other members if needed, like sampleRate if calculations depend on it
};

#endif // BITCRUSHER_H_INCLUDED