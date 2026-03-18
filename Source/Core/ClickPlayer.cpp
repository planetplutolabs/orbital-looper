/*
  ==============================================================================
    ClickPlayer.cpp — v03.04.01 CLICK TRACK

    Planet Pluto Effects
  ==============================================================================
*/

#include "ClickPlayer.h"

//==============================================================================
ClickPlayer::ClickPlayer()
{
    formatManager.registerBasicFormats();
}

//==============================================================================
void ClickPlayer::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate    = sr;
    clickDuration = juce::roundToInt(CLICK_DURATION_MS / 1000.0 * sr);
    reset();
}

void ClickPlayer::reset()
{
    clickActive      = false;
    clickSamplesLeft = 0;
    clickStartSample = 0;
    synthPhase       = 0.0f;
    userFilePlayPos  = 0;
}

//==============================================================================
void ClickPlayer::setVolume(float v)
{
    volume.store(juce::jlimit(0.0f, 1.0f, v));
}

float ClickPlayer::getVolume() const
{
    return volume.load();
}

//==============================================================================
bool ClickPlayer::loadUserFile(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    // Load into a local buffer first, then swap under the lock.
    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples  = static_cast<int>(reader->lengthInSamples);

    juce::AudioBuffer<float> tmp(numChannels, numSamples);
    reader->read(&tmp, 0, numSamples, 0, true, true);

    {
        const juce::ScopedLock sl(userFileLock);
        userFileSample  = std::move(tmp);
        userFileName    = file.getFileNameWithoutExtension();
    }

    useUserFile.store(true);
    return true;
}

void ClickPlayer::clearUserFile()
{
    useUserFile.store(false);
    {
        const juce::ScopedLock sl(userFileLock);
        userFileSample.setSize(0, 0);
        userFileName = {};
    }
}

juce::String ClickPlayer::getUserFileName() const
{
    const juce::ScopedLock sl(userFileLock);
    return userFileName;
}

bool ClickPlayer::hasUserFile() const
{
    return useUserFile.load();
}

//==============================================================================
// Audio thread — called when a beat boundary falls within the current block
void ClickPlayer::triggerClick(int sampleOffset, bool accented)
{
    clickActive      = true;
    clickStartSample = sampleOffset;
    clickSamplesLeft = clickDuration;
    clickAccented    = accented;
    synthPhase       = 0.0f;
    userFilePlayPos  = 0;
}

//==============================================================================
// Audio thread — render any active click into buf (already cleared by caller)
void ClickPlayer::processBlock(juce::AudioBuffer<float>& buf, int numSamples)
{
    if (!clickActive || clickSamplesLeft <= 0)
        return;

    if (useUserFile.load())
        renderUserFile(buf, numSamples);
    else
        renderBuiltin(buf, numSamples);

    // After the first block, subsequent carry-over blocks always start at 0
    clickStartSample = 0;

    if (clickSamplesLeft <= 0)
        clickActive = false;
}

//==============================================================================
// Built-in woodblock synthesis
// Dual-layer: short sine burst (tone) + noise body, both exponentially decayed.
// - Tone layer:  sin wave at clickFreq, fast decay (–18× t)
// - Noise layer: broadband noise * sin harmonic (rough bandpass), slower decay (–12× t)
// Combined they produce a pitched knock — not a pure sine wave.
void ClickPlayer::renderBuiltin(juce::AudioBuffer<float>& buf, int numSamples)
{
    const float freq   = clickAccented ? ACCENT_FREQ : NORMAL_FREQ;
    const float gain   = clickAccented ? accentGain() : normalGain();
    const float twoPi  = juce::MathConstants<float>::twoPi;
    const float phaseInc = twoPi * freq / static_cast<float>(sampleRate);
    const int   nCh    = buf.getNumChannels();

    // Deterministic noise — use a simple LCG so the click sounds consistent
    uint32_t noiseSeed = 0xDEADBEEFu;

    const int startSample = clickStartSample;
    const int endSample   = juce::jmin(startSample + clickSamplesLeft, numSamples);

    for (int s = startSample; s < endSample; ++s)
    {
        // Progress t: 0 at start of click → 1 at end
        const float t         = 1.0f - static_cast<float>(clickSamplesLeft)
                                        / static_cast<float>(clickDuration);
        const float toneDecay  = std::exp(-18.0f * t);
        const float noiseDecay = std::exp(-12.0f * t);

        // Tone component
        const float tone = std::sin(synthPhase) * toneDecay * 0.4f;

        // Noise component — LCG noise shaped by a harmonic of the oscillator
        noiseSeed = noiseSeed * 1664525u + 1013904223u;
        const float noise = (static_cast<float>(noiseSeed) / 2147483648.0f - 1.0f)
                            * std::sin(synthPhase * 1.5f + 0.3f)
                            * noiseDecay * 0.6f;

        const float out = (tone + noise) * gain;

        for (int ch = 0; ch < nCh; ++ch)
            buf.getWritePointer(ch)[s] += out;

        synthPhase += phaseInc;
        if (synthPhase >= twoPi)
            synthPhase -= twoPi;

        --clickSamplesLeft;
    }
}

//==============================================================================
// User file playback — reads from userFileSample into buf
void ClickPlayer::renderUserFile(juce::AudioBuffer<float>& buf, int numSamples)
{
    // Snapshot the read-pointer under the lock (minimal hold time)
    const float* srcL   = nullptr;
    const float* srcR   = nullptr;
    int          srcLen = 0;
    {
        const juce::ScopedLock sl(userFileLock);
        if (userFileSample.getNumSamples() == 0)
            return;
        srcLen = userFileSample.getNumSamples();
        srcL   = userFileSample.getReadPointer(0);
        srcR   = (userFileSample.getNumChannels() > 1)
                    ? userFileSample.getReadPointer(1)
                    : srcL;
    }

    const float gain = (clickAccented ? accentGain() : normalGain()) * 2.0f;
    const int   nCh        = buf.getNumChannels();
    const int   startSample = clickStartSample;
    const int   endSample   = juce::jmin(startSample + clickSamplesLeft, numSamples);

    for (int s = startSample; s < endSample; ++s)
    {
        if (userFilePlayPos >= srcLen)
            break;

        const float outL = srcL[userFilePlayPos] * gain;
        const float outR = srcR[userFilePlayPos] * gain;

        if (nCh > 0) buf.getWritePointer(0)[s] += outL;
        if (nCh > 1) buf.getWritePointer(1)[s] += outR;

        ++userFilePlayPos;
        --clickSamplesLeft;
    }
}
