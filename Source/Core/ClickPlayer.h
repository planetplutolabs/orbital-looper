/*
  ==============================================================================
    ClickPlayer.h

    Orbital Looper v03.04.01 - CLICK TRACK

    Synthesizes or plays back a percussive woodblock click on each beat.
    Beat triggering is driven by PluginProcessor (which owns the beat phase
    accumulator). ClickPlayer is responsible only for audio synthesis and
    sample playback.

    Sound modes:
      Built-in  — synthesized woodblock (tone + noise burst, exponential decay)
      UserFile  — loaded WAV/AIFF, played back on each trigger

    Thread safety:
      prepare()       — call before audio starts (any thread)
      reset()         — call on audio thread (resetBeatIndex) or message thread
                        before audio starts
      triggerClick()  — AUDIO THREAD only
      processBlock()  — AUDIO THREAD only
      setVolume()     — atomic, safe from any thread
      loadUserFile()  — MESSAGE THREAD only (may block briefly on I/O)
      clearUserFile() — MESSAGE THREAD only

    Planet Pluto Effects
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ClickPlayer
{
public:
    ClickPlayer();
    ~ClickPlayer() = default;

    //==========================================================================
    // Setup — call from any thread before audio starts
    void prepare(double sampleRate, int maxBlockSize);
    void reset();   // clears active playback state

    //==========================================================================
    // Volume — atomic: UI thread writes, audio thread reads
    void  setVolume(float v);   // clamped 0.0–1.0
    float getVolume() const;

    //==========================================================================
    // User file — call from MESSAGE THREAD only
    // Returns true on success; switches mode to UserFile automatically.
    bool         loadUserFile(const juce::File& file);
    void         clearUserFile();           // reverts to built-in woodblock
    juce::String getUserFileName() const;
    bool         hasUserFile() const;

    //==========================================================================
    // Audio thread only
    // Call triggerClick() when a beat boundary falls within the current block.
    //   sampleOffset — index within the current processBlock buffer where the
    //                  beat fires (sub-block accurate placement)
    //   accented     — true for downbeat / user-marked beat (louder, higher pitch)
    void triggerClick(int sampleOffset, bool accented);

    // Writes click audio into buf (caller must clear buf first).
    // May carry over from the previous block (clickSamplesLeft > 0).
    void processBlock(juce::AudioBuffer<float>& buf, int numSamples);

private:
    //==========================================================================
    // Built-in woodblock synthesis
    // Dual-layer: short sine burst (tone) + filtered noise body, both with
    // exponential decay.  NOT a pure sine wave.
    void renderBuiltin(juce::AudioBuffer<float>& buf, int numSamples);

    // User file playback
    void renderUserFile(juce::AudioBuffer<float>& buf, int numSamples);

    //==========================================================================
    // Constants
    static constexpr float CLICK_DURATION_MS = 50.0f;   // ~50ms click
    static constexpr float ACCENT_FREQ       = 1200.0f; // Hz — downbeat
    static constexpr float NORMAL_FREQ       =  800.0f; // Hz — normal beat
    float accentGain() const { return volume.load(); }
    float normalGain() const { return volume.load() * 0.75f; }

    //==========================================================================
    // Audio-thread-only state (plain members — no atomics needed)
    double sampleRate        = 44100.0;
    int    clickDuration     = 0;       // computed in prepare()
    bool   clickActive       = false;   // true while a click is rendering
    int    clickStartSample  = 0;       // sample offset within the current block
    int    clickSamplesLeft  = 0;       // samples remaining for current click
    bool   clickAccented     = false;   // is the current click an accent?
    float  synthPhase        = 0.0f;   // oscillator phase (radians)
    int    userFilePlayPos   = 0;       // read head into userFileSample

    //==========================================================================
    // Shared state (atomic) — written by UI, read by audio thread
    std::atomic<float> volume      { 0.75f };
    std::atomic<bool>  useUserFile { false };

    //==========================================================================
    // User file buffer — protected by userFileLock during load/clear.
    // Audio thread takes the lock only to snapshot the read-pointer, so the
    // hold time is minimal.
    juce::CriticalSection    userFileLock;
    juce::AudioBuffer<float> userFileSample;
    juce::String             userFileName;

    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClickPlayer)
};
