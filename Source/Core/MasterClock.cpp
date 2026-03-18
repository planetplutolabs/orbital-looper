#include "MasterClock.h"

MasterClock::MasterClock()
{
}

MasterClock::~MasterClock()
{
}

void MasterClock::prepare(double sr)
{
    sampleRate = sr;
    currentPositionInCycle.store(0);
}

void MasterClock::advance(int numSamples)
{
    int cycleLength = masterCycleLengthSamples.load();
    int position = currentPositionInCycle.load();

    if (cycleLength <= 0)
    {
        // No master cycle established yet - clock still advances for metronome
        currentPositionInCycle.store(position + numSamples);
        return;
    }

    // Advance position and wrap around master cycle
    position += numSamples;
    if (position >= cycleLength)
    {
        position %= cycleLength;
    }
    currentPositionInCycle.store(position);
}

void MasterClock::reset()
{
    currentPositionInCycle.store(0);
    masterCycleLengthSamples.store(0);
    masterLoopIndex.store(-1);
}

void MasterClock::setBPM(int bpm)
{
    currentBPM.store(juce::jlimit(40, 300, bpm));
}

void MasterClock::setTimeSignature(int numerator, int denominator)
{
    timeSignatureNumerator = numerator;
    timeSignatureDenominator = denominator;
}

void MasterClock::setMasterCycleLength(int lengthInSamples)
{
    masterCycleLengthSamples.store(lengthInSamples);
    currentPositionInCycle.store(0); // Reset position when master changes
}

double MasterClock::getCurrentPositionBeats() const
{
    if (!hasMasterCycle())
        return 0.0;

    int samplesPerBeat = getSamplesPerBeat();
    if (samplesPerBeat == 0)
        return 0.0;

    return static_cast<double>(currentPositionInCycle.load()) / samplesPerBeat;
}

int MasterClock::getSamplesPerBeat() const
{
    // Samples per beat = (60 / BPM) * sampleRate
    int bpm = currentBPM.load();
    if (bpm <= 0)
        return 0;

    return static_cast<int>((60.0 / bpm) * sampleRate);
}

int MasterClock::getSamplesPerBar() const
{
    // Samples per bar = samples per beat * beats per bar
    return getSamplesPerBeat() * timeSignatureNumerator;
}

int MasterClock::getNextBeatPosition(int fromPosition) const
{
    int samplesPerBeat = getSamplesPerBeat();
    if (samplesPerBeat == 0)
        return fromPosition;

    // Find next beat boundary
    int currentBeat = fromPosition / samplesPerBeat;
    int nextBeat = currentBeat + 1;
    return nextBeat * samplesPerBeat;
}

int MasterClock::getNextBarPosition(int fromPosition) const
{
    int samplesPerBar = getSamplesPerBar();
    if (samplesPerBar == 0)
        return fromPosition;

    // Find next bar boundary
    int currentBar = fromPosition / samplesPerBar;
    int nextBar = currentBar + 1;
    return nextBar * samplesPerBar;
}

int MasterClock::quantizeToNearestBeat(int samplePosition) const
{
    int samplesPerBeat = getSamplesPerBeat();
    if (samplesPerBeat == 0)
        return samplePosition;

    // Round to nearest beat
    int beatNumber = (samplePosition + samplesPerBeat / 2) / samplesPerBeat;
    return beatNumber * samplesPerBeat;
}

int MasterClock::quantizeToNearestBar(int samplePosition) const
{
    int samplesPerBar = getSamplesPerBar();
    if (samplesPerBar == 0)
        return samplePosition;

    // Round to nearest bar
    int barNumber = (samplePosition + samplesPerBar / 2) / samplesPerBar;
    return barNumber * samplesPerBar;
}

int MasterClock::getCurrentBarNumber() const
{
    int samplesPerBar = getSamplesPerBar();
    if (samplesPerBar == 0)
        return 0;

    return currentPositionInCycle.load() / samplesPerBar;
}

bool MasterClock::isAtBarBoundary(int toleranceSamples) const
{
    int samplesPerBar = getSamplesPerBar();
    if (samplesPerBar == 0)
        return false;

    int positionInBar = currentPositionInCycle.load() % samplesPerBar;
    // At boundary if we're within tolerance of start of bar
    return positionInBar <= toleranceSamples;
}

int MasterClock::getSamplesUntilNextBar() const
{
    int samplesPerBar = getSamplesPerBar();
    if (samplesPerBar == 0)
        return 0;

    int positionInBar = currentPositionInCycle.load() % samplesPerBar;
    return samplesPerBar - positionInBar;
}

int MasterClock::getLengthForMultiplier(LoopMultiplier multiplier) const
{
    int cycleLength = masterCycleLengthSamples.load();
    if (cycleLength <= 0)
        return 0;

    switch (multiplier)
    {
        case LoopMultiplier::Quarter:
            return cycleLength / 4;
        case LoopMultiplier::Half:
            return cycleLength / 2;
        case LoopMultiplier::One:
            return cycleLength;
        case LoopMultiplier::Two:
            return cycleLength * 2;
        case LoopMultiplier::Four:
            return cycleLength * 4;
        default:
            return cycleLength;
    }
}

MasterClock::LoopMultiplier MasterClock::findClosestMultiplier(int lengthInSamples) const
{
    if (!hasMasterCycle() || lengthInSamples <= 0)
        return LoopMultiplier::One;

    // Calculate distances to each valid multiplier
    int distQuarter = std::abs(lengthInSamples - getLengthForMultiplier(LoopMultiplier::Quarter));
    int distHalf    = std::abs(lengthInSamples - getLengthForMultiplier(LoopMultiplier::Half));
    int distOne     = std::abs(lengthInSamples - getLengthForMultiplier(LoopMultiplier::One));
    int distTwo     = std::abs(lengthInSamples - getLengthForMultiplier(LoopMultiplier::Two));
    int distFour    = std::abs(lengthInSamples - getLengthForMultiplier(LoopMultiplier::Four));

    // Find minimum distance
    int minDist = distQuarter;
    LoopMultiplier result = LoopMultiplier::Quarter;

    if (distHalf < minDist) { minDist = distHalf; result = LoopMultiplier::Half; }
    if (distOne < minDist)  { minDist = distOne;  result = LoopMultiplier::One; }
    if (distTwo < minDist)  { minDist = distTwo;  result = LoopMultiplier::Two; }
    if (distFour < minDist) { result = LoopMultiplier::Four; }

    return result;
}

juce::String MasterClock::getMultiplierString(LoopMultiplier multiplier) const
{
    switch (multiplier)
    {
        case LoopMultiplier::Quarter:
            return "1/4x";
        case LoopMultiplier::Half:
            return "1/2x";
        case LoopMultiplier::One:
            return "Master";
        case LoopMultiplier::Two:
            return "2x";
        case LoopMultiplier::Four:
            return "4x";
        default:
            return "1x";
    }
}
