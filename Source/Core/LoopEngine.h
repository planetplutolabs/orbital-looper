/*
  ==============================================================================
    LoopEngine.h

    Orbital Looper v01.06.01 - SAVE / LOAD

    Core loop recording and playback engine.
    Clean implementation with no legacy baggage.

    Planet Pluto Effects
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "MasterClock.h"

// Sync mode (v02.04.01) — Free = any length, Sync = will quantize to bar boundaries.
enum class LoopSyncMode
{
    Free,   // Record freely, any length (default, current behaviour)
    Sync    // Will snap end-point to bar boundary (requires MasterClock — future)
};

// Loop engine states
enum class LoopState
{
    Stopped,      // No loop recorded OR loop stopped at beginning (position = 0)
    RecordArmed,  // Waiting for bar boundary to start recording (Sync mode)
    Recording,    // Currently recording audio (initial recording)
    Playing,      // Playing back recorded loop
    Paused,       // v01.01.02: Playback paused (position preserved)
    Overdubbing   // v01.02.01: Recording new audio while playing existing loop
};

class LoopEngine
{
public:
    LoopEngine();
    ~LoopEngine() = default;

    //==============================================================================
    // SETUP
    //==============================================================================
    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    //==============================================================================
    // v01.00.01 - RECORD FUNCTIONALITY
    //==============================================================================
    void startRecording();
    void stopRecording();
    bool isRecording() const { return state == LoopState::Recording; }

    //==============================================================================
    // v01.01.01 - MANUAL PLAYBACK CONTROL
    //==============================================================================
    void startPlayback();
    void stopPlayback();

    //==============================================================================
    // v01.01.02 - PAUSE/RESUME
    //==============================================================================
    void pausePlayback();

    //==============================================================================
    // v01.01.03 - RESTART
    //==============================================================================
    void restartPlayback();

    //==============================================================================
    // v01.03.01 - MULTIPLY
    //==============================================================================
    void multiply();
    bool canMultiply() const;

    //==============================================================================
    // v02.04.01 - SYNC MODE
    //==============================================================================
    void setSyncMode(LoopSyncMode mode) { syncMode = mode; }
    LoopSyncMode getSyncMode() const { return syncMode; }

    //==============================================================================
    // v05.01 - MASTER CLOCK INTEGRATION
    //==============================================================================
    void setMasterClock(MasterClock* clock) { masterClock = clock; }
    MasterClock::LoopMultiplier getMultiplier() const { return multiplier; }
    void setMultiplier(MasterClock::LoopMultiplier mult) { multiplier = mult; }

    // Armed record (Sync mode — waits for bar boundary)
    void armRecord();
    void cancelArm();
    bool isArmed() const { return state == LoopState::RecordArmed; }

    // Post-recording quantization
    void setLoopLength(int lengthInSamples);

    //==============================================================================
    // v02.05.01/02 - MUTE and SOLO
    //==============================================================================
    void setMute(bool shouldMute) { isMuted = shouldMute; }
    bool isMute()  const { return isMuted; }
    void setSolo(bool shouldSolo) { isSoloed = shouldSolo; }
    bool isSolo()  const { return isSoloed; }

    //==============================================================================
    // v01.02.01 - OVERDUB
    //==============================================================================
    void startOverdub();
    void stopOverdub();

    //==============================================================================
    // v01.02.02 - UNDO (v01.03.01: Multi-level)
    //==============================================================================
    void undo();
    bool canUndo() const { return !undoStack.empty(); }

    //==============================================================================
    // v01.02.03 - REDO (v01.03.01: Multi-level)
    //==============================================================================
    void redo();
    bool canRedo() const { return !redoStack.empty(); }

    //==============================================================================
    // STATE
    //==============================================================================
    LoopState getState() const { return state; }
    int getLoopLengthSamples() const { return loopLengthSamples; }
    bool hasRecording() const { return loopLengthSamples > 0; }

    // v01.07.02 — UI getters for LOOP 1 card
    // Returns current playback/record position in samples (0 when stopped/recording initial)
    int getCurrentPosition() const { return playbackPosition; }
    // Returns current position as seconds
    double getCurrentPositionSeconds() const
    {
        return (sampleRate > 0.0) ? (double)playbackPosition / sampleRate : 0.0;
    }
    // Returns loop length as seconds
    double getLoopLengthSeconds() const
    {
        return (sampleRate > 0.0) ? (double)loopLengthSamples / sampleRate : 0.0;
    }
    // Returns number of overdub layers added (1 = base recording, increments per overdub)
    int getOverdubCount() const { return overdubCount; }

    //==============================================================================
    // v01.04.01 - LOOP VOLUME
    //==============================================================================
    void setVolume(float vol) { volume = juce::jlimit(0.0f, 1.0f, vol); }
    float getVolume() const { return volume; }

    //==============================================================================
    // v01.04.02 - LOOP PAN
    //==============================================================================
    // pan: -1.0 = full left, 0.0 = center, +1.0 = full right
    void setPan(float p) { pan = juce::jlimit(-1.0f, 1.0f, p); }
    float getPan() const { return pan; }

    //==============================================================================
    // CONFIGURATION
    //==============================================================================
    // 0 = unlimited (default). Set to e.g. 60 to cap at 60 seconds.
    void setMaxLoopLengthSeconds(int seconds) { maxLoopLengthSeconds = seconds; }

    //==============================================================================
    // v01.06.01 - SAVE / LOAD BUFFER ACCESS
    //==============================================================================
    // Read-only access to the loop buffer for saving
    const juce::AudioBuffer<float>& getLoopBuffer() const { return loopBuffer; }

    // Load audio from a reader (called by PluginProcessor::loadSession)
    // Resets engine to Stopped, fills loopBuffer from the reader, and sets loopLengthSamples
    void loadBuffer(int numChannels, int numSamples, juce::AudioFormatReader& reader);

    //==============================================================================
    // AUDIO PROCESSING
    //==============================================================================
    void processBlock(juce::AudioBuffer<float>& buffer);

private:
    //==============================================================================
    // STATE
    //==============================================================================
    LoopState state = LoopState::Stopped;

    //==============================================================================
    // AUDIO BUFFER
    //==============================================================================
    juce::AudioBuffer<float> loopBuffer;       // Current mixed loop
    int loopLengthSamples = 0;
    int recordPosition = 0;
    int playbackPosition = 0;  // v01.00.02: Current read position in loop

    //==============================================================================
    // MULTI-LEVEL UNDO/REDO STACKS - v01.03.01
    //==============================================================================
    struct LoopSnapshot
    {
        juce::AudioBuffer<float> buffer;
        int lengthSamples;

        LoopSnapshot() : lengthSamples(0) {}

        LoopSnapshot(const juce::AudioBuffer<float>& buf, int length)
            : lengthSamples(length)
        {
            buffer.makeCopyOf(buf);
        }
    };

    std::vector<LoopSnapshot> undoStack;  // Stack of previous states (can undo multiple times)
    std::vector<LoopSnapshot> redoStack;  // Stack of undone states (can redo after undo)

    //==============================================================================
    // CONFIGURATION
    //==============================================================================
    double sampleRate = 44100.0;
    // v01.04.01: No default max loop length - buffer grows dynamically.
    // Optional user-configurable limit can be set via setMaxLoopLengthSeconds().
    int maxLoopLengthSeconds = 0;   // 0 = unlimited

    //==============================================================================
    // v01.07.02 - OVERDUB COUNT (layer tracking for LOOP 1 UI)
    //==============================================================================
    int overdubCount = 0;  // 0 = no recording; 1 = base recording; +1 per overdub

    //==============================================================================
    // v01.04.01 - VOLUME / v01.04.02 - PAN
    //==============================================================================
    float volume = 1.0f;            // 0.0 (silent) to 1.0 (full, default)
    float pan    = 0.0f;            // -1.0 (full left) to +1.0 (full right), 0 = center

    //==============================================================================
    // v02.04.01 - SYNC MODE
    //==============================================================================
    LoopSyncMode syncMode = LoopSyncMode::Free;
    MasterClock* masterClock = nullptr;
    MasterClock::LoopMultiplier multiplier = MasterClock::LoopMultiplier::One;
    int armedCountdownSamples = 0;

    //==============================================================================
    // v02.05.01/02 - MUTE and SOLO
    //==============================================================================
    bool isMuted  = false;
    bool isSoloed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopEngine)
};
