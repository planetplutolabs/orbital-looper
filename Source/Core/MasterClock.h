#pragma once

#include <JuceHeader.h>

/**
 * Master Clock System
 *
 * Manages global tempo and synchronization for all loops.
 * - Tracks BPM and time signature
 * - Maintains master cycle length (established by first loop or metronome)
 * - Provides beat/bar quantization for loop start/stop
 * - Calculates current position in master cycle
 */
class MasterClock
{
public:
    MasterClock();
    ~MasterClock();

    void prepare(double sampleRate);
    void advance(int numSamples);
    void reset();

    // Tempo settings
    void setBPM(int bpm);
    int getBPM() const { return currentBPM.load(); }

    void setTimeSignature(int numerator, int denominator);
    int getTimeSignatureNumerator() const { return timeSignatureNumerator; }
    int getTimeSignatureDenominator() const { return timeSignatureDenominator; }
    std::pair<int, int> getTimeSignature() const { return {timeSignatureNumerator, timeSignatureDenominator}; }

    // Master cycle
    void setMasterCycleLength(int lengthInSamples);
    int getMasterCycleLength() const { return masterCycleLengthSamples.load(); }
    bool hasMasterCycle() const { return masterCycleLengthSamples.load() > 0; }

    // Master loop tracking
    void setMasterLoop(int loopIndex) { masterLoopIndex.store(loopIndex); }
    int getMasterLoopIndex() const { return masterLoopIndex.load(); }

    // Current position in master cycle
    int getCurrentPosition() const { return currentPositionInCycle.load(); }
    double getCurrentPositionBeats() const;

    // Beat/Bar calculations
    int getSamplesPerBeat() const;
    int getSamplesPerBar() const;
    int getNextBeatPosition(int fromPosition) const;
    int getNextBarPosition(int fromPosition) const;

    // Quantization
    int quantizeToNearestBeat(int samplePosition) const;
    int quantizeToNearestBar(int samplePosition) const;

    // Bar boundary detection for quantized recording
    int getCurrentBarNumber() const;
    bool isAtBarBoundary(int toleranceSamples = 0) const;
    int getSamplesUntilNextBar() const;

    // Valid loop length multipliers (1x, 2x, 4x, 1/2x, 1/4x master)
    enum class LoopMultiplier
    {
        Quarter,    // 1/4x
        Half,       // 1/2x
        One,        // 1x (master)
        Two,        // 2x
        Four        // 4x
    };

    int getLengthForMultiplier(LoopMultiplier multiplier) const;
    LoopMultiplier findClosestMultiplier(int lengthInSamples) const;
    juce::String getMultiplierString(LoopMultiplier multiplier) const;

private:
    double sampleRate = 44100.0;
    std::atomic<int> currentBPM{120};
    int timeSignatureNumerator = 4;
    int timeSignatureDenominator = 4;

    std::atomic<int> masterCycleLengthSamples{0};  // 0 = no master established yet
    std::atomic<int> currentPositionInCycle{0};
    std::atomic<int> masterLoopIndex{-1};  // -1 = no master loop

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterClock)
};
