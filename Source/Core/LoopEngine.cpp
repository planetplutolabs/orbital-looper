/*
  ==============================================================================
    LoopEngine.cpp

    Orbital Looper v01.06.01 - SAVE / LOAD

    Implementation of core loop recording and playback.

    Planet Pluto Effects
  ==============================================================================
*/

#include "LoopEngine.h"

LoopEngine::LoopEngine()
{
}

void LoopEngine::prepare(double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;

    // Pre-allocate 30 seconds of buffer to avoid reallocation on the audio thread
    // during the first 30s of recording. Buffer grows geometrically beyond this.
    loopBuffer.setSize(numChannels, static_cast<int>(sampleRate * 30.0), false, true, false);
    loopBuffer.clear();

    // v01.03.01: Clear undo/redo stacks
    undoStack.clear();
    redoStack.clear();
}

void LoopEngine::reset()
{
    state = LoopState::Stopped;
    loopLengthSamples = 0;
    recordPosition = 0;
    loopBuffer.clear();
    overdubCount = 0;  // v01.07.02
    multiplier = MasterClock::LoopMultiplier::One;
    armedCountdownSamples = 0;

    // v01.03.01: Clear undo/redo stacks when clearing loop
    undoStack.clear();
    redoStack.clear();
}

//==============================================================================
// v01.00.01 - RECORD FUNCTIONALITY
//==============================================================================

void LoopEngine::startRecording()
{
    // Can only start recording when stopped
    if (state != LoopState::Stopped)
        return;

    // v01.03.01: Save current state to undo stack (even if empty)
    // This allows UNDO to work on base loop recording too
    undoStack.push_back(LoopSnapshot(loopBuffer, loopLengthSamples));

    // v01.03.01: Clear redo stack when starting new operation
    redoStack.clear();

    // Reset recording state
    recordPosition = 0;
    loopLengthSamples = 0;
    loopBuffer.clear();

    // Start recording
    state = LoopState::Recording;
}

void LoopEngine::stopRecording()
{
    // Can only stop if we're recording
    if (state != LoopState::Recording)
        return;

    // Finalize the loop
    loopLengthSamples = recordPosition;

    // Trim buffer to exact size to free excess pre-allocated memory
    loopBuffer.setSize(loopBuffer.getNumChannels(), loopLengthSamples, true, false, false);

    // v01.01.04: Set to Stopped first, THEN call startPlayback()
    // (startPlayback has a guard that prevents it from running if state == Recording)
    state = LoopState::Stopped;

    overdubCount = 1;  // v01.07.02: base recording = layer 1

    // v01.01.04: Auto-start playback after recording (like v01.00.03)
    // This provides the fluid workflow users expect from looper pedals
    startPlayback();  // This sets state to Playing and resets playbackPosition
}

//==============================================================================
// v01.01.01 - MANUAL PLAYBACK CONTROL
//==============================================================================

void LoopEngine::startPlayback()
{
    // Can only start playback if we have a recording and we're not recording
    if (!hasRecording() || state == LoopState::Recording)
        return;

    // v01.01.02: If resuming from pause, keep current position
    // If starting fresh (from Stopped), reset to beginning
    if (state == LoopState::Stopped)
        playbackPosition = 0;  // Start from beginning
    // else: state is Paused, keep playbackPosition as-is (resume)

    state = LoopState::Playing;
}

void LoopEngine::stopPlayback()
{
    // Can only stop if we're playing or paused
    if (state != LoopState::Playing && state != LoopState::Paused)
        return;

    // Stop playback and reset position to beginning
    playbackPosition = 0;  // v01.01.02: Reset when truly stopping
    state = LoopState::Stopped;
}

//==============================================================================
// v01.01.02 - PAUSE/RESUME
//==============================================================================

void LoopEngine::pausePlayback()
{
    // Can only pause if we're playing
    if (state != LoopState::Playing)
        return;

    // Pause playback but preserve playbackPosition for resume
    state = LoopState::Paused;
    // playbackPosition is NOT reset - this is the key to resume functionality
}

//==============================================================================
// v01.01.03 - RESTART
//==============================================================================

void LoopEngine::restartPlayback()
{
    // Can only restart if we're playing or paused
    // (Stopped is already at position 0, Recording shouldn't be interrupted)
    if (state != LoopState::Playing && state != LoopState::Paused)
        return;

    // Reset position to beginning but keep current state
    playbackPosition = 0;
    // State stays as-is (Playing continues playing, Paused stays paused)
}

//==============================================================================
// v01.03.01 - MULTIPLY
//==============================================================================

void LoopEngine::multiply()
{
    // Can only multiply when playing or overdubbing
    if (state != LoopState::Playing && state != LoopState::Overdubbing)
        return;

    // Must have a loop to multiply
    if (loopLengthSamples <= 0)
        return;

    const int newLength = loopLengthSamples * 2;

    // Enforce optional user-configured max length
    if (maxLoopLengthSeconds > 0)
    {
        const int maxSamples = static_cast<int>(sampleRate * maxLoopLengthSeconds);
        if (newLength > maxSamples)
            return;
    }

    // v01.03.01: Save current state to undo stack before multiplying
    undoStack.push_back(LoopSnapshot(loopBuffer, loopLengthSamples));

    // v01.03.01: Clear redo stack when starting new operation
    redoStack.clear();

    // v01.04.01: Grow buffer to fit the doubled content if needed
    if (newLength > loopBuffer.getNumSamples())
        loopBuffer.setSize(loopBuffer.getNumChannels(), newLength, true, true, false);

    // Copy existing loop content to second half
    for (int channel = 0; channel < loopBuffer.getNumChannels(); ++channel)
    {
        loopBuffer.copyFrom(channel,            // destination channel
                           loopLengthSamples,   // destination start sample
                           loopBuffer,          // source buffer
                           channel,             // source channel
                           0,                   // source start sample
                           loopLengthSamples);  // number of samples to copy
    }

    loopLengthSamples = newLength;
}

bool LoopEngine::canMultiply() const
{
    if (state != LoopState::Playing && state != LoopState::Overdubbing)
        return false;

    if (loopLengthSamples <= 0)
        return false;

    // If no user-configured limit, always allow
    if (maxLoopLengthSeconds == 0)
        return true;

    const int maxSamples = static_cast<int>(sampleRate * maxLoopLengthSeconds);
    return (loopLengthSamples * 2) <= maxSamples;
}

//==============================================================================
// v01.06.01 - LOAD BUFFER
//==============================================================================

void LoopEngine::loadBuffer(int numChannels, int numSamples, juce::AudioFormatReader& reader)
{
    // Reset to clean state
    state = LoopState::Stopped;
    playbackPosition = 0;
    recordPosition   = 0;
    undoStack.clear();
    redoStack.clear();

    // Size the buffer and read audio
    loopBuffer.setSize(numChannels, numSamples, false, true, false);
    reader.read(&loopBuffer, 0, numSamples, 0, true, true);

    loopLengthSamples = numSamples;
}

//==============================================================================
// v01.02.01 - OVERDUB
//==============================================================================

void LoopEngine::startOverdub()
{
    // Can only overdub if we have a recording and we're playing or paused
    if (!hasRecording() || (state != LoopState::Playing && state != LoopState::Paused))
        return;

    // v01.03.01: Save current loop state to undo stack before overdubbing
    undoStack.push_back(LoopSnapshot(loopBuffer, loopLengthSamples));

    // v01.03.01: Clear redo stack when starting new operation
    redoStack.clear();

    // Start recording at current playback position for perfect sync
    recordPosition = playbackPosition;

    state = LoopState::Overdubbing;
}

void LoopEngine::stopOverdub()
{
    // Can only stop overdub if we're overdubbing
    if (state != LoopState::Overdubbing)
        return;

    overdubCount++;  // v01.07.02: each completed overdub adds a layer

    // Return to playing state (loop continues)
    // Note: recordPosition and playbackPosition continue advancing independently
    state = LoopState::Playing;
}

//==============================================================================
// v05.01 - ARMED RECORD / SYNC
//==============================================================================

void LoopEngine::armRecord()
{
    if (state != LoopState::Stopped)
        return;

    if (masterClock != nullptr && masterClock->getSamplesPerBar() > 0)
    {
        int samplesUntil = masterClock->getSamplesUntilNextBar();
        int samplesPerBar = masterClock->getSamplesPerBar();
        armedCountdownSamples = (samplesUntil >= samplesPerBar) ? 0 : samplesUntil;
    }
    else
    {
        armedCountdownSamples = 0;
    }

    // Save undo state (matching startRecording pattern)
    undoStack.push_back(LoopSnapshot(loopBuffer, loopLengthSamples));
    redoStack.clear();

    state = LoopState::RecordArmed;
}

void LoopEngine::cancelArm()
{
    if (state == LoopState::RecordArmed)
    {
        state = LoopState::Stopped;
        armedCountdownSamples = 0;
        // Restore undo state
        if (!undoStack.empty())
            undoStack.pop_back();
    }
}

void LoopEngine::setLoopLength(int lengthInSamples)
{
    if (lengthInSamples <= 0) return;
    if (lengthInSamples > loopBuffer.getNumSamples())
        loopBuffer.setSize(loopBuffer.getNumChannels(), lengthInSamples, true, true, false);
    loopLengthSamples = lengthInSamples;
    if (playbackPosition >= loopLengthSamples)
        playbackPosition %= loopLengthSamples;
}

//==============================================================================
// v01.02.02 - UNDO
//==============================================================================

void LoopEngine::undo()
{
    // Can only undo if we have snapshots in the undo stack
    if (undoStack.empty())
        return;

    // Can't undo while recording or overdubbing (finish current operation first)
    if (state == LoopState::Recording || state == LoopState::Overdubbing)
        return;

    // v01.03.01: Save current state to redo stack BEFORE undoing
    redoStack.push_back(LoopSnapshot(loopBuffer, loopLengthSamples));

    // Pop the most recent snapshot from undo stack
    LoopSnapshot snapshot = undoStack.back();
    undoStack.pop_back();

    // Restore loop buffer and length from snapshot
    loopBuffer.makeCopyOf(snapshot.buffer);
    loopLengthSamples = snapshot.lengthSamples;

    // If restoring to empty state (undoing base loop), stop playback
    if (loopLengthSamples == 0)
    {
        state = LoopState::Stopped;
        playbackPosition = 0;
        overdubCount = 0;  // v01.07.02
    }
    else
    {
        // Decrement layer count on undo, minimum 1 (base recording)
        overdubCount = juce::jmax(1, overdubCount - 1);  // v01.07.02
    }
    // Otherwise, if currently playing or paused, stay in that state
    // Loop continues playing with restored content
    // playbackPosition wraps at loopLengthSamples, so it will adjust automatically
}

//==============================================================================
// v01.02.03 - REDO
//==============================================================================

void LoopEngine::redo()
{
    // Can only redo if we have snapshots in the redo stack
    if (redoStack.empty())
        return;

    // Can't redo while recording or overdubbing (finish current operation first)
    if (state == LoopState::Recording || state == LoopState::Overdubbing)
        return;

    // v01.03.01: Save current state to undo stack BEFORE redoing
    undoStack.push_back(LoopSnapshot(loopBuffer, loopLengthSamples));

    // Pop the most recent snapshot from redo stack
    LoopSnapshot snapshot = redoStack.back();
    redoStack.pop_back();

    // Restore loop buffer and length from snapshot
    loopBuffer.makeCopyOf(snapshot.buffer);
    loopLengthSamples = snapshot.lengthSamples;

    // If restoring to empty state, stop playback
    if (loopLengthSamples == 0)
    {
        state = LoopState::Stopped;
        playbackPosition = 0;
    }
    // Otherwise, if currently playing or paused, stay in that state
    // Loop continues playing with restored content
}

//==============================================================================
// AUDIO PROCESSING
//==============================================================================

void LoopEngine::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // v01.00.02: Handle recording and playback
    if (state == LoopState::Recording)
    {
        // v01.04.01: Dynamic buffer growth - no hard cap by default.
        // If a user-configured maxLoopLengthSeconds > 0, stop at that limit.
        int localRecordPos = recordPosition;
        const int blocksEnd = localRecordPos + numSamples;

        // Enforce optional user-configured max length
        if (maxLoopLengthSeconds > 0)
        {
            const int maxSamples = static_cast<int>(sampleRate * maxLoopLengthSeconds);
            if (localRecordPos >= maxSamples)
            {
                recordPosition = localRecordPos;
                stopRecording();
                return;
            }
        }

        // Grow the loop buffer to fit incoming audio if needed.
        // Use geometric doubling (minimum 30s) to avoid per-block reallocation.
        if (blocksEnd > loopBuffer.getNumSamples())
        {
            int newCapacity = juce::jmax(blocksEnd,
                                         loopBuffer.getNumSamples() * 2,
                                         static_cast<int>(sampleRate * 30.0));
            loopBuffer.setSize(loopBuffer.getNumChannels(), newCapacity, true, true, false);
        }

        // Copy from input buffer to loop buffer for all channels
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float* inputData = buffer.getReadPointer(channel);
            float* loopData = loopBuffer.getWritePointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
                loopData[localRecordPos + sample] = inputData[sample];
        }

        recordPosition = blocksEnd;

        // Pass through input audio (hear what you're recording)
        // Note: buffer already contains input, so no action needed
    }
    else if (state == LoopState::RecordArmed)
    {
        // v05.01: Countdown to bar boundary before starting recording
        armedCountdownSamples -= numSamples;
        if (armedCountdownSamples <= 0)
        {
            // Bar boundary reached — start recording
            recordPosition = 0;
            loopLengthSamples = 0;
            loopBuffer.clear();
            state = LoopState::Recording;

            // Record this block
            const int blocksEnd = numSamples;
            if (blocksEnd > loopBuffer.getNumSamples())
            {
                int newCapacity = juce::jmax(blocksEnd, static_cast<int>(sampleRate * 30.0));
                loopBuffer.setSize(loopBuffer.getNumChannels(), newCapacity, true, true, false);
            }
            for (int channel = 0; channel < numChannels; ++channel)
            {
                const float* inputData = buffer.getReadPointer(channel);
                float* loopData = loopBuffer.getWritePointer(channel);
                for (int sample = 0; sample < numSamples; ++sample)
                    loopData[sample] = inputData[sample];
            }
            recordPosition = blocksEnd;
        }
        // While armed: pass through audio (buffer already has input)
    }
    else if (state == LoopState::Playing && loopLengthSamples > 0)
    {
        // v01.00.02: Play back the recorded loop
        int localPlaybackPos = playbackPosition;

        // v05.01: Sync playback — phase-lock to master clock at normal speed.
        // Uses modulo so loops play at original speed and repeat within the master cycle
        // (e.g. a 5s loop in a 10s master plays twice). Only for loops <= master length;
        // longer loops (2x, 4x) advance naturally since they span multiple master cycles.
        if (syncMode == LoopSyncMode::Sync && masterClock && masterClock->hasMasterCycle() && loopLengthSamples > 0)
        {
            int masterCycleLength = masterClock->getMasterCycleLength();
            if (masterCycleLength > 0 && loopLengthSamples <= masterCycleLength)
            {
                int masterPos = masterClock->getCurrentPosition();
                localPlaybackPos = masterPos % loopLengthSamples;
            }
        }

        // v01.04.02: Constant-power pan gains (mono: both channels use volume only)
        float leftGain  = volume;
        float rightGain = volume;
        if (numChannels >= 2)
        {
            // Map pan -1..+1 to angle 0..pi/2, then cos/sin for equal power
            float panAngle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            leftGain  = volume * std::cos(panAngle);
            rightGain = volume * std::sin(panAngle);
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                const float* loopData = loopBuffer.getReadPointer(channel);
                float* outputData = buffer.getWritePointer(channel);
                float gain = (channel == 0) ? leftGain : rightGain;
                outputData[sample] += loopData[localPlaybackPos] * gain;
            }

            localPlaybackPos++;
            if (localPlaybackPos >= loopLengthSamples)
                localPlaybackPos = 0;
        }

        playbackPosition = localPlaybackPos;
    }
    else if (state == LoopState::Overdubbing && loopLengthSamples > 0)
    {
        // v01.02.01: OVERDUBBING - Record new audio while playing existing loop
        int localPlaybackPos = playbackPosition;
        int localRecordPos = recordPosition;

        // v05.01: Sync playback in overdub mode — same modulo phase-lock as Playing
        if (syncMode == LoopSyncMode::Sync && masterClock && masterClock->hasMasterCycle() && loopLengthSamples > 0)
        {
            int masterCycleLength = masterClock->getMasterCycleLength();
            if (masterCycleLength > 0 && loopLengthSamples <= masterCycleLength)
            {
                int masterPos = masterClock->getCurrentPosition();
                localPlaybackPos = masterPos % loopLengthSamples;
                localRecordPos = localPlaybackPos;
            }
        }

        // v01.04.02: Same constant-power gains for overdub playback
        float leftGain  = volume;
        float rightGain = volume;
        if (numChannels >= 2)
        {
            float panAngle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            leftGain  = volume * std::cos(panAngle);
            rightGain = volume * std::sin(panAngle);
        }

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // PLAYBACK: Read from loop and mix to output
            for (int channel = 0; channel < numChannels; ++channel)
            {
                const float* loopData = loopBuffer.getReadPointer(channel);
                float* outputData = buffer.getWritePointer(channel);
                float gain = (channel == 0) ? leftGain : rightGain;
                outputData[sample] += loopData[localPlaybackPos] * gain;
            }

            localPlaybackPos++;
            if (localPlaybackPos >= loopLengthSamples)
                localPlaybackPos = 0;

            // RECORDING: Write input to loop buffer (additive mixing)
            for (int channel = 0; channel < numChannels; ++channel)
            {
                float* loopData = loopBuffer.getWritePointer(channel);
                const float* inputData = buffer.getReadPointer(channel);

                loopData[localRecordPos] += inputData[sample];
            }

            localRecordPos++;
            if (localRecordPos >= loopLengthSamples)
                localRecordPos = 0;
        }

        playbackPosition = localPlaybackPos;
        recordPosition = localRecordPos;
    }
    else
    {
        // Not recording or playing - pass audio through unchanged
        // (buffer already contains input audio, so no action needed)
    }
}
