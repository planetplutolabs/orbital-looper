/*
  ==============================================================================
    PluginProcessor.cpp

    Orbital Looper v02.07.01 - SAVE ALL LOOPS

    Multi-loop processor. All loops mix simultaneously to output.
    Buttons target the currently selected loop or variable selection.
    v02.04.01: Global LoopSyncMode (Free/Sync). Sync is a no-op until
    MasterClock/Metronome integration.
    v02.06.01: MIDI CC for volume/pan; note toggles for mute/solo on
    current loop. All assignments stored in MidiMapping (reassignable).

    Planet Pluto Effects
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OrbitalLooperAudioProcessor::OrbitalLooperAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    // Create initial loop engine (prepared in prepareToPlay)
    loopEngines.push_back(std::make_unique<LoopEngine>());
    loadGlobalDefaults();   // v05.00 — apply before DAW restores project state
}

OrbitalLooperAudioProcessor::~OrbitalLooperAudioProcessor()
{
}

//==============================================================================
const juce::String OrbitalLooperAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OrbitalLooperAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OrbitalLooperAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OrbitalLooperAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OrbitalLooperAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OrbitalLooperAudioProcessor::getNumPrograms()
{
    return 1;
}

int OrbitalLooperAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OrbitalLooperAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String OrbitalLooperAudioProcessor::getProgramName(int index)
{
    return {};
}

void OrbitalLooperAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void OrbitalLooperAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    {
        const juce::ScopedLock sl(loopsLock);
        for (auto& eng : loopEngines)
        {
            eng->prepare(sampleRate, samplesPerBlock, getTotalNumInputChannels());
            eng->setSyncMode(globalSyncMode);
            eng->setMasterClock(&masterClock);
        }
    }

    // v05.01 - MASTER CLOCK
    masterClock.prepare(sampleRate);
    masterClock.setBPM(globalBPM.load());
    masterClock.setTimeSignature(timeSigNumerator.load(), timeSigDenominator.load());

    // v03.04.01 - CLICK TRACK
    clickPlayer.prepare(sampleRate, samplesPerBlock);
    clickBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock, false, true, true);
    resetAccentMaskToDownbeats();
}

void OrbitalLooperAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OrbitalLooperAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
// v02.00.01 - MULTI-LOOP AUDIO PROCESSING
// All loop engines process the same buffer.
// Recording engines pass through and write to their own loopBuffer.
// Playing engines ADD their output to the buffer (+=).
// The combined result is the mix of all active loops.
//==============================================================================
void OrbitalLooperAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Clear any unused channels
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // v05.00 — MIDI Learn: capture next note-on when in learn mode
    if (midiLearnFunctionIndex.load() >= 0)
    {
        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
            {
                midiLearnResult.store(msg.getNoteNumber());
                midiLearnFunctionIndex.store(-1);
                break;
            }
        }
    }

    // Process incoming MIDI messages before audio
    for (const auto metadata : midiMessages)
        processMidiMessage(metadata.getMessage());

    // v05.01 — Advance master clock every audio block
    masterClock.advance(buffer.getNumSamples());

    // Try to lock loop vector — skip this block if UI is modifying the list
    const juce::ScopedTryLock sl(loopsLock);
    if (!sl.isLocked())
        return;

    // Run all loop engines — each adds its playback output to the buffer.
    // v02.05.01/02: Muted/un-soloed loops still advance their position so they
    // stay in sync; their output is directed to a silent temp buffer (discarded).
    juce::AudioBuffer<float> silentBuffer;
    bool silentBufferReady = false;

    for (auto& eng : loopEngines)
    {
        const bool audible = anyLoopSoloed ? eng->isSolo()
                                           : !eng->isMute();
        if (audible)
        {
            eng->processBlock(buffer);
        }
        else
        {
            // Keep position advancing without contributing audio output
            if (!silentBufferReady)
            {
                silentBuffer.setSize(buffer.getNumChannels(),
                                     buffer.getNumSamples(), false, true, true);
                silentBufferReady = true;
            }
            silentBuffer.clear();
            eng->processBlock(silentBuffer);
        }
    }

    // v03.03.01 / v03.04.01 — BEAT ACCUMULATOR + CLICK TRACK
    // Replaces the former while-loop with a per-sample scan so we capture the
    // exact sample offset of each beat boundary for sub-block-accurate click placement.
    {
        // Service a UI-thread reset request
        if (resetBeatPhaseRequested.exchange(false))
        {
            beatPhaseAccumulator = 0;
            currentBeatIndex.store(0);
            clickPlayer.reset();
        }

        const bool clickOn = clickTrackOn.load();

        // v04.00 — GroundLoop-style: time-based, silent count-in (block-level decrement)
        // beatCounterActive is false during count-in, so click logic below won't run.
        {
            int ciRemaining = countInSamplesRemaining.load();
            if (ciRemaining > 0)
            {
                ciRemaining -= buffer.getNumSamples();
                if (ciRemaining <= 0)
                {
                    ciRemaining = 0;
                    countInJustCompleted.store(true);
                }
                countInSamplesRemaining.store(ciRemaining);
            }
        }

        // Allocate + clear click buffer only when click track is active
        if (clickOn)
        {
            clickBuffer.setSize(buffer.getNumChannels(), buffer.getNumSamples(),
                                false, true, true);
            clickBuffer.clear();
        }

        if (metronomeOn.load())
        {
            const int bpm = globalBPM.load();
            const int sr  = static_cast<int>(getSampleRate());

            // Reset phase accumulator if BPM changed to avoid beat-phase drift
            if (bpm != lastBPMCache)
            {
                lastBPMCache         = bpm;
                beatPhaseAccumulator = 0;
            }

            // Only advance when UI has signalled "playing" (prevents catch-up on resume)
            if (beatCounterActive.load() && sr > 0 && bpm > 0)
            {
                const int      samplesPerBeat  = juce::roundToInt(60.0 * sr / bpm);
                const int      beatsPerMeasure = timeSigNumerator.load();
                const int      totalBeats      = 4 * beatsPerMeasure;  // 4-bar grid
                const uint32_t mask            = accentMask.load();
                int beatIdx = currentBeatIndex.load();
                int phase   = beatPhaseAccumulator;

                // Per-sample scan — O(blockSize) but lightweight (compare + branch only).
                // This gives us the exact sample offset 's' when each beat fires,
                // enabling sub-block-accurate click trigger placement.
                for (int s = 0; s < buffer.getNumSamples(); ++s)
                {
                    if (++phase >= samplesPerBeat)
                    {
                        phase -= samplesPerBeat;
                        ++beatIdx;

                        if (clickOn)
                        {
                            const int  patternBeat = (beatIdx - 1) % totalBeats;
                            const bool accented    = (mask & (1u << patternBeat)) != 0u;
                            clickPlayer.triggerClick(s, accented);
                        }
                    }
                }

                beatPhaseAccumulator = phase;
                currentBeatIndex.store(beatIdx);
            }
        }

        // Render click audio AFTER all loop-engine recording is complete.
        // Click track audio never reaches the loop record buffer.
        if (clickOn)
        {
            clickPlayer.processBlock(clickBuffer, buffer.getNumSamples());
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addFrom(ch, 0, clickBuffer, ch, 0, buffer.getNumSamples());
        }
    }
}

//==============================================================================
// v03.03.01 - BEAT VISUALIZATION
//==============================================================================
void OrbitalLooperAudioProcessor::resetBeatIndex()
{
    // Signal the audio thread to reset its phase accumulator on the next block.
    // Setting the atomic flag is safe from any thread.
    resetBeatPhaseRequested.store(true);
}

//==============================================================================
// v03.04.01 - CLICK TRACK
//==============================================================================
void OrbitalLooperAudioProcessor::resetAccentMaskToDownbeats()
{
    // Compute a bitmask where bit N is set for every beat at position N that is
    // the first beat of a measure (i.e. N % beatsPerMeasure == 0).
    // The 4-bar grid has totalBeats = 4 * beatsPerMeasure positions (bits 0..31).
    const int beats = timeSigNumerator.load();
    uint32_t mask = 0u;
    for (int bar = 0; bar < 4; ++bar)
    {
        const int bit = bar * beats;
        if (bit < 32)
            mask |= (1u << bit);
    }
    accentMask.store(mask);
}

void OrbitalLooperAudioProcessor::resetClickTrack()
{
    // Called from UI when CLICK TRACK is turned OFF or CLEAR is pressed.
    // Resets volume, user file, and accent mask to defaults.
    clickPlayer.clearUserFile();
    clickPlayer.setVolume(1.0f);   // v03.06.01 — default 100% like loop volume
    resetAccentMaskToDownbeats();
}

//==============================================================================
// v02.00.01 - ADD / DELETE LOOPS
//==============================================================================
void OrbitalLooperAudioProcessor::addNewLoop()
{
    // v05.00 — enforce max loops limit
    if (!unlimitedLoops && (int)loopEngines.size() >= maxLoops)
        return;

    {
        const juce::ScopedLock sl(loopsLock);
        auto eng = std::make_unique<LoopEngine>();
        eng->prepare(getSampleRate(), getBlockSize(), getTotalNumInputChannels());
        eng->setSyncMode(globalSyncMode);
        eng->setMasterClock(&masterClock);
        loopEngines.push_back(std::move(eng));
    }

    updateSoloState();   // new loop is not muted/soloed; update cached state

    if (onLoopCardsChanged)
        onLoopCardsChanged();
}

//==============================================================================
// v02.04.01 - SYNC MODE
//==============================================================================
void OrbitalLooperAudioProcessor::setSyncMode(LoopSyncMode mode)
{
    globalSyncMode = mode;
    {
        const juce::ScopedLock sl(loopsLock);
        for (auto& eng : loopEngines)
            eng->setSyncMode(mode);
    }
    if (onSyncModeChanged)
        onSyncModeChanged();
}

//==============================================================================
// v05.00 - SETUP MODE
//==============================================================================
void OrbitalLooperAudioProcessor::setSingleTrackMode(bool single)
{
    isSingleTrack = single;
    if (onSingleTrackModeChanged)
        onSingleTrackModeChanged();
}

//==============================================================================
// v02.05.01/02 - MUTE and SOLO
//==============================================================================
void OrbitalLooperAudioProcessor::updateSoloState()
{
    anyLoopSoloed = false;
    {
        const juce::ScopedLock sl(loopsLock);
        for (const auto& eng : loopEngines)
        {
            if (eng->isSolo())
            {
                anyLoopSoloed = true;
                break;
            }
        }
    }
    if (onMuteSoloChanged)
        onMuteSoloChanged();
}

void OrbitalLooperAudioProcessor::deleteLoop(int index)
{
    {
        const juce::ScopedLock sl(loopsLock);
        const int n = (int)loopEngines.size();

        if (n > 1 && index >= 0 && index < n)
        {
            loopEngines.erase(loopEngines.begin() + index);
            if (currentLoopIndex >= (int)loopEngines.size())
                currentLoopIndex = (int)loopEngines.size() - 1;

            // v05.01: Handle master clock when deleting loops
            if (masterLoopIndex == index)
            {
                masterClock.reset();
                masterLoopIndex = -1;
            }
            else if (masterLoopIndex > index)
            {
                masterLoopIndex--;
                masterClock.setMasterLoop(masterLoopIndex);
            }
        }
        else if (n == 1 && index == 0)
        {
            // Only loop — reset instead of delete (GroundLoop pattern)
            loopEngines[0]->reset();
            masterClock.reset();
            masterLoopIndex = -1;
        }
    }

    updateSoloState();   // deleted loop may have been soloed

    if (onLoopCardsChanged)
        onLoopCardsChanged();
}

//==============================================================================
// v01.05.01 - MIDI MESSAGE HANDLING
// All transport actions target the currently selected loop engine.
//==============================================================================
void OrbitalLooperAudioProcessor::processMidiMessage(const juce::MidiMessage& message)
{
    if (midiMapping.channel == 0)
        return;

    if (message.getChannel() != midiMapping.channel)
        return;

    //==========================================================================
    // v02.06.01 - MIDI CC: volume and pan on the currently selected loop
    //==========================================================================
    if (message.isController())
    {
        const int cc  = message.getControllerNumber();
        const int val = message.getControllerValue();  // 0–127

        auto& eng = getLoopEngine();  // current loop

        if (midiMapping.volumeCC >= 0 && cc == midiMapping.volumeCC)
        {
            eng.setVolume(static_cast<float>(val) / 127.0f);
            if (onMuteSoloChanged) onMuteSoloChanged();
        }
        else if (midiMapping.panCC >= 0 && cc == midiMapping.panCC)
        {
            eng.setPan((static_cast<float>(val) - 64.0f) / 63.0f);
            if (onMuteSoloChanged) onMuteSoloChanged();
        }
        else if (midiMapping.clickVolumeCC >= 0 && cc == midiMapping.clickVolumeCC)
        {
            setClickVolume(static_cast<float>(val) / 127.0f);
        }
        return;
    }

    //==========================================================================
    // Note-On handlers
    //==========================================================================
    if (!message.isNoteOn())
        return;

    const int note = message.getNoteNumber();

    // Debounce — 50ms minimum between same note
    const juce::int64 now = juce::Time::currentTimeMillis();
    if (note == lastMidiNote && (now - lastMidiTime) < MIDI_DEBOUNCE_MS)
        return;
    lastMidiNote = note;
    lastMidiTime = now;

    // Operate on the selected loop engine
    auto& engine = getLoopEngine();  // uses currentLoopIndex
    const auto state = engine.getState();

    if (note == midiMapping.recordNote)
    {
        if (state == LoopState::Stopped)
        {
            // v05.01: First loop (no master cycle) records Free immediately
            // Subsequent loops + Sync mode: arm for bar boundary
            if (currentLoopIndex == 0 || !masterClock.hasMasterCycle())
            {
                engine.setSyncMode(LoopSyncMode::Free);
                engine.startRecording();
            }
            else if (globalSyncMode == LoopSyncMode::Sync)
            {
                engine.setSyncMode(LoopSyncMode::Sync);
                engine.armRecord();
            }
            else
            {
                engine.setSyncMode(LoopSyncMode::Free);
                engine.startRecording();
            }
        }
        else if (state == LoopState::RecordArmed)
        {
            engine.cancelArm();
        }
        else if (state == LoopState::Recording)
        {
            engine.stopRecording();

            // v05.01: Establish master cycle or quantize
            if (!masterClock.hasMasterCycle())
            {
                int loopLen = engine.getLoopLengthSamples();
                if (loopLen > 0)
                {
                    masterClock.setMasterCycleLength(loopLen);
                    masterLoopIndex = currentLoopIndex;
                    masterClock.setMasterLoop(currentLoopIndex);
                    engine.setMultiplier(MasterClock::LoopMultiplier::One);
                }
            }
            else if (engine.getSyncMode() == LoopSyncMode::Sync)
            {
                int recordedLen = engine.getLoopLengthSamples();
                auto mult = masterClock.findClosestMultiplier(recordedLen);
                int quantizedLen = masterClock.getLengthForMultiplier(mult);
                if (quantizedLen > 0 && quantizedLen != recordedLen)
                    engine.setLoopLength(quantizedLen);
                engine.setMultiplier(mult);
            }
        }
        else if ((state == LoopState::Playing ||
                  state == LoopState::Paused) && canAddLayer(engine))
            engine.startOverdub();
        else if (state == LoopState::Overdubbing) engine.stopOverdub();
    }
    else if (note == midiMapping.playNote)
    {
        if      (state == LoopState::Playing)                               engine.pausePlayback();
        else if (state == LoopState::Stopped || state == LoopState::Paused) engine.startPlayback();
    }
    else if (note == midiMapping.undoNote)    { engine.undo(); }
    else if (note == midiMapping.restartNote) { engine.restartPlayback(); }
    else if (note == midiMapping.clearNote)
    {
        const int idx = currentLoopIndex;
        if ((int)loopEngines.size() > 1)
            deleteLoop(idx);
        else
            engine.reset();
    }
    else if (note == midiMapping.multiplyNote) { engine.multiply(); }
    else if (note == midiMapping.redoNote)     { engine.redo(); }
    else if (note == midiMapping.addLoopNote)    { addNewLoop(); }
    else if (note == midiMapping.deleteLoopNote) { deleteLoop(currentLoopIndex); }
    // v02.06.01 - MUTE / SOLO toggle on current loop
    else if (note == midiMapping.muteNote)
    {
        engine.setMute(!engine.isMute());
        updateSoloState();   // fires onMuteSoloChanged → UI refresh
    }
    else if (note == midiMapping.soloNote)
    {
        engine.setSolo(!engine.isSolo());
        updateSoloState();
    }
    else if (note == midiMapping.loopAllNote)
    {
        if (onLoopAllToggled) onLoopAllToggled();
    }
    else if (note == midiMapping.loopUpNote)
    {
        const int n = (int)loopEngines.size();
        currentLoopIndex = (currentLoopIndex - 1 + n) % n;
        if (onLoopCardsChanged) onLoopCardsChanged();
    }
    else if (note == midiMapping.loopDownNote)
    {
        const int n = (int)loopEngines.size();
        currentLoopIndex = (currentLoopIndex + 1) % n;
        if (onLoopCardsChanged) onLoopCardsChanged();
    }
    // v03.06.02 — metronome + click track MIDI controls
    else if (note == midiMapping.metronomeToggleNote)
    {
        setMetronomeOn(!getMetronomeOn());
    }
    else if (note == midiMapping.tapTempoNote)
    {
        processMidiTapTempo();
    }
    else if (note == midiMapping.clickTrackToggleNote)
    {
        if (getMetronomeOn())
            setClickTrackOn(!getClickTrackOn());
    }
    // v05.00 — quantize/free + count-in MIDI toggles
    else if (note == midiMapping.quantizeFreeNote)
    {
        setSyncMode(globalSyncMode == LoopSyncMode::Free
                        ? LoopSyncMode::Sync : LoopSyncMode::Free);
    }
    else if (note == midiMapping.countInToggleNote)
    {
        setCountInEnabled(!getCountInEnabled());
    }
}

//==============================================================================
// v03.06.02 — MIDI tap tempo (same BPM calculation as UI tapButton.onClick)
//==============================================================================
void OrbitalLooperAudioProcessor::processMidiTapTempo()
{
    const juce::int64 now = juce::Time::currentTimeMillis();

    // Reset if gap > 2 seconds
    if (midiTapCountAudio > 0 &&
        (now - midiTapTimesAudio[midiTapCountAudio - 1]) > 2000)
        midiTapCountAudio = 0;

    // Shift when full (keep last 8)
    if (midiTapCountAudio >= 8)
    {
        for (int i = 0; i < 7; ++i)
            midiTapTimesAudio[i] = midiTapTimesAudio[i + 1];
        midiTapCountAudio = 7;
    }
    midiTapTimesAudio[midiTapCountAudio++] = now;

    if (midiTapCountAudio >= 2)
    {
        double total = 0.0;
        for (int i = 1; i < midiTapCountAudio; ++i)
            total += (double)(midiTapTimesAudio[i] - midiTapTimesAudio[i - 1]);
        const double avg = total / (midiTapCountAudio - 1);
        setBPM(juce::jlimit(40, 300, juce::roundToInt(60000.0 / avg)));
    }
}

//==============================================================================
// v04.00 — COUNT IN
//==============================================================================
void OrbitalLooperAudioProcessor::startCountIn(int action)
{
    pendingTriggerAction.store(action);
    countInJustCompleted.store(false);
    const int sr      = static_cast<int>(getSampleRate());
    const int samples = (sr > 0)
        ? static_cast<int>(countInSeconds * sr)
        : static_cast<int>(4.0 * 44100);   // safe fallback if sample rate not yet set
    countInSamplesRemaining.store(samples);
    // Do NOT call setBeatCounterActive(true) here —
    // beat counter starts only after count-in completes (fired in timerCallback)
}

void OrbitalLooperAudioProcessor::cancelCountIn()
{
    pendingTriggerAction.store(0);
    countInSamplesRemaining.store(0);
    countInJustCompleted.store(false);
}

//==============================================================================
bool OrbitalLooperAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* OrbitalLooperAudioProcessor::createEditor()
{
    return new OrbitalLooperAudioProcessorEditor(*this);
}

//==============================================================================
// v02.00.01 - DAW PROJECT STATE (lightweight settings only, no audio)
//==============================================================================
void OrbitalLooperAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto* obj = new juce::DynamicObject();
    juce::var state(obj);

    obj->setProperty("version",      "03.00.01");
    obj->setProperty("loopCount",    (int)loopEngines.size());
    obj->setProperty("selectedLoop", currentLoopIndex);
    obj->setProperty("bpm",           globalBPM.load());
    obj->setProperty("metronomeOn",  metronomeOn.load());
    // v03.04.01 - CLICK TRACK
    obj->setProperty("clickTrackOn", (bool)clickTrackOn.load());
    obj->setProperty("clickVolume",  clickPlayer.getVolume());
    obj->setProperty("accentMask",   (int64_t)accentMask.load());

    if (currentSessionFile.existsAsFile())
        obj->setProperty("sessionFile", currentSessionFile.getFullPathName());

    // Per-loop settings array
    juce::Array<juce::var> loopsArr;
    for (int i = 0; i < (int)loopEngines.size(); ++i)
    {
        auto* lobj = new juce::DynamicObject();
        lobj->setProperty("volume",       loopEngines[i]->getVolume());
        lobj->setProperty("pan",          loopEngines[i]->getPan());
        lobj->setProperty("loopSyncMode", (int)loopEngines[i]->getSyncMode());
        lobj->setProperty("multiplier",   (int)loopEngines[i]->getMultiplier());
        loopsArr.add(juce::var(lobj));
    }
    obj->setProperty("loops", loopsArr);

    // MIDI mapping
    auto* midiObj = new juce::DynamicObject();
    midiObj->setProperty("channel",        midiMapping.channel);
    midiObj->setProperty("recordNote",     midiMapping.recordNote);
    midiObj->setProperty("playNote",       midiMapping.playNote);
    midiObj->setProperty("undoNote",       midiMapping.undoNote);
    midiObj->setProperty("restartNote",    midiMapping.restartNote);
    midiObj->setProperty("clearNote",      midiMapping.clearNote);
    midiObj->setProperty("multiplyNote",   midiMapping.multiplyNote);
    midiObj->setProperty("redoNote",       midiMapping.redoNote);
    midiObj->setProperty("addLoopNote",    midiMapping.addLoopNote);
    midiObj->setProperty("loopUpNote",     midiMapping.loopUpNote);
    midiObj->setProperty("loopDownNote",   midiMapping.loopDownNote);
    midiObj->setProperty("deleteLoopNote", midiMapping.deleteLoopNote);
    midiObj->setProperty("muteNote",       midiMapping.muteNote);
    midiObj->setProperty("soloNote",       midiMapping.soloNote);
    midiObj->setProperty("loopAllNote",    midiMapping.loopAllNote);
    // v03.06.02
    midiObj->setProperty("metronomeToggleNote",  midiMapping.metronomeToggleNote);
    midiObj->setProperty("tapTempoNote",         midiMapping.tapTempoNote);
    midiObj->setProperty("clickTrackToggleNote", midiMapping.clickTrackToggleNote);
    midiObj->setProperty("volumeCC",             midiMapping.volumeCC);
    midiObj->setProperty("panCC",                midiMapping.panCC);
    // v05.00
    midiObj->setProperty("quantizeFreeNote",  midiMapping.quantizeFreeNote);
    midiObj->setProperty("clickVolumeCC",     midiMapping.clickVolumeCC);
    midiObj->setProperty("countInToggleNote", midiMapping.countInToggleNote);
    obj->setProperty("midi", juce::var(midiObj));

    // v04.00 — count-in enabled state + configurable duration
    obj->setProperty("countInEnabled", countInEnabled.load());
    obj->setProperty("countInSeconds",  countInSeconds);

    // v05.00 — setup mode + limits + sync mode
    obj->setProperty("isSingleTrack",   isSingleTrack);
    obj->setProperty("maxLoops",        maxLoops);
    obj->setProperty("unlimitedLoops",  unlimitedLoops);
    obj->setProperty("maxLayers",       maxLayers);
    obj->setProperty("unlimitedLayers", unlimitedLayers);
    obj->setProperty("syncMode",          (int)globalSyncMode);
    obj->setProperty("masterLoopIndex",   masterLoopIndex);
    obj->setProperty("masterCycleLength", masterClock.getMasterCycleLength());

    // v06.00 — editor width
    obj->setProperty("editorWidth", editorWidth);

    // v07.00 — dark mode
    obj->setProperty("darkMode", darkMode);

    juce::String json = juce::JSON::toString(state);
    destData.replaceAll(json.toRawUTF8(), json.getNumBytesAsUTF8());
}

void OrbitalLooperAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes == 0)
        return;

    juce::var state = juce::JSON::parse(
        juce::String::createStringFromData(data, sizeInBytes));

    if (!state.isObject())
        return;

    // Per-loop settings
    if (state.hasProperty("loops"))
    {
        const auto& loopsArr = *state["loops"].getArray();
        for (int i = 0; i < (int)loopsArr.size() && i < (int)loopEngines.size(); ++i)
        {
            if (loopsArr[i].hasProperty("volume"))
                loopEngines[i]->setVolume(static_cast<float>(loopsArr[i]["volume"]));
            if (loopsArr[i].hasProperty("pan"))
                loopEngines[i]->setPan(static_cast<float>(loopsArr[i]["pan"]));
            if (loopsArr[i].hasProperty("loopSyncMode"))
                loopEngines[i]->setSyncMode(static_cast<LoopSyncMode>(static_cast<int>(loopsArr[i]["loopSyncMode"])));
            if (loopsArr[i].hasProperty("multiplier"))
                loopEngines[i]->setMultiplier(static_cast<MasterClock::LoopMultiplier>(static_cast<int>(loopsArr[i]["multiplier"])));
            loopEngines[i]->setMasterClock(&masterClock);
        }
    }
    else
    {
        // v01 backward compat
        if (state.hasProperty("volume"))
            loopEngines[0]->setVolume(static_cast<float>(state["volume"]));
        if (state.hasProperty("pan"))
            loopEngines[0]->setPan(static_cast<float>(state["pan"]));
    }

    if (state.hasProperty("selectedLoop"))
        currentLoopIndex = juce::jlimit(0, juce::jmax(0, (int)loopEngines.size() - 1),
                                        static_cast<int>(state["selectedLoop"]));

    // v03.00.01 - BPM + metronome on/off
    if (state.hasProperty("bpm"))
        setBPM(static_cast<int>(state["bpm"]));
    if (state.hasProperty("metronomeOn"))
        metronomeOn.store(static_cast<bool>(state["metronomeOn"]));

    // v03.04.01 - CLICK TRACK state
    if (state.hasProperty("clickTrackOn"))
        clickTrackOn.store(static_cast<bool>(state["clickTrackOn"]));
    if (state.hasProperty("clickVolume"))
        clickPlayer.setVolume(static_cast<float>(state["clickVolume"]));
    if (state.hasProperty("accentMask"))
        accentMask.store(static_cast<uint32_t>(static_cast<int64_t>(state["accentMask"])));
    else
        resetAccentMaskToDownbeats();

    // MIDI mapping
    if (state.hasProperty("midi"))
    {
        auto& m = state["midi"];
        if (m.hasProperty("channel"))      midiMapping.channel      = static_cast<int>(m["channel"]);
        if (m.hasProperty("recordNote"))   midiMapping.recordNote   = static_cast<int>(m["recordNote"]);
        if (m.hasProperty("playNote"))     midiMapping.playNote     = static_cast<int>(m["playNote"]);
        if (m.hasProperty("undoNote"))     midiMapping.undoNote     = static_cast<int>(m["undoNote"]);
        if (m.hasProperty("restartNote"))  midiMapping.restartNote  = static_cast<int>(m["restartNote"]);
        if (m.hasProperty("clearNote"))    midiMapping.clearNote    = static_cast<int>(m["clearNote"]);
        if (m.hasProperty("multiplyNote")) midiMapping.multiplyNote = static_cast<int>(m["multiplyNote"]);
        if (m.hasProperty("redoNote"))     midiMapping.redoNote     = static_cast<int>(m["redoNote"]);
        if (m.hasProperty("addLoopNote"))  midiMapping.addLoopNote  = static_cast<int>(m["addLoopNote"]);
        if (m.hasProperty("loopUpNote"))     midiMapping.loopUpNote     = static_cast<int>(m["loopUpNote"]);
        if (m.hasProperty("loopDownNote"))   midiMapping.loopDownNote   = static_cast<int>(m["loopDownNote"]);
        if (m.hasProperty("deleteLoopNote")) midiMapping.deleteLoopNote = static_cast<int>(m["deleteLoopNote"]);
        if (m.hasProperty("muteNote"))       midiMapping.muteNote       = static_cast<int>(m["muteNote"]);
        if (m.hasProperty("soloNote"))       midiMapping.soloNote       = static_cast<int>(m["soloNote"]);
        if (m.hasProperty("loopAllNote"))    midiMapping.loopAllNote    = static_cast<int>(m["loopAllNote"]);
        // v03.06.02
        if (m.hasProperty("metronomeToggleNote"))
            midiMapping.metronomeToggleNote  = static_cast<int>(m["metronomeToggleNote"]);
        if (m.hasProperty("tapTempoNote"))
            midiMapping.tapTempoNote         = static_cast<int>(m["tapTempoNote"]);
        if (m.hasProperty("clickTrackToggleNote"))
            midiMapping.clickTrackToggleNote = static_cast<int>(m["clickTrackToggleNote"]);
        if (m.hasProperty("volumeCC"))       midiMapping.volumeCC       = static_cast<int>(m["volumeCC"]);
        if (m.hasProperty("panCC"))          midiMapping.panCC          = static_cast<int>(m["panCC"]);
        // v05.00
        if (m.hasProperty("quantizeFreeNote"))
            midiMapping.quantizeFreeNote  = static_cast<int>(m["quantizeFreeNote"]);
        if (m.hasProperty("clickVolumeCC"))
            midiMapping.clickVolumeCC     = static_cast<int>(m["clickVolumeCC"]);
        if (m.hasProperty("countInToggleNote"))
            midiMapping.countInToggleNote = static_cast<int>(m["countInToggleNote"]);
    }

    // v04.00 — count-in enabled + configurable duration
    if (state.hasProperty("countInEnabled"))
        countInEnabled.store(static_cast<bool>(state["countInEnabled"]));
    if (state.hasProperty("countInSeconds"))
        countInSeconds = juce::jlimit(0.5, 60.0, static_cast<double>(state["countInSeconds"]));

    // v05.00 — setup mode + limits + sync mode
    if (state.hasProperty("isSingleTrack"))
        isSingleTrack = static_cast<bool>(state["isSingleTrack"]);
    if (state.hasProperty("maxLoops"))
        maxLoops = juce::jlimit(1, 999, static_cast<int>(state["maxLoops"]));
    if (state.hasProperty("unlimitedLoops"))
        unlimitedLoops = static_cast<bool>(state["unlimitedLoops"]);
    if (state.hasProperty("maxLayers"))
        maxLayers = juce::jlimit(1, 999, static_cast<int>(state["maxLayers"]));
    if (state.hasProperty("unlimitedLayers"))
        unlimitedLayers = static_cast<bool>(state["unlimitedLayers"]);
    if (state.hasProperty("syncMode"))
        globalSyncMode = static_cast<LoopSyncMode>(static_cast<int>(state["syncMode"]));

    // v05.01 — restore master clock
    if (state.hasProperty("masterLoopIndex"))
        masterLoopIndex = static_cast<int>(state["masterLoopIndex"]);
    if (state.hasProperty("masterCycleLength"))
    {
        int mcl = static_cast<int>(state["masterCycleLength"]);
        if (mcl > 0)
        {
            masterClock.setMasterCycleLength(mcl);
            masterClock.setMasterLoop(masterLoopIndex);
        }
    }

    // v06.00 — editor width
    if (state.hasProperty("editorWidth"))
        editorWidth = juce::jlimit(600, 1536, static_cast<int>(state["editorWidth"]));

    // v07.00 — dark mode
    if (state.hasProperty("darkMode"))
        darkMode = static_cast<bool>(state["darkMode"]);

    if (state.hasProperty("sessionFile"))
        currentSessionFile = juce::File(state["sessionFile"].toString());
}

//==============================================================================
// v02.00.01 - FULL SESSION SAVE (multi-loop)
//==============================================================================
// v02.07.01: Internal helper — saves exactly the loops specified by indices.
// Indices are remapped to 0-based in the output file (loop_0.wav, loop_1.wav, …).
// The "selectedLoop" field in the JSON is set to the first index in the list.
bool OrbitalLooperAudioProcessor::saveSession(const juce::File& file,
                                               const std::vector<int>& loopIndices)
{
    const int numSaving = (int)loopIndices.size();
    if (numSaving == 0)
        return false;

    // Clamp and validate indices
    const int totalLoops = (int)loopEngines.size();
    std::vector<int> valid;
    valid.reserve((size_t)numSaving);
    for (int idx : loopIndices)
        if (idx >= 0 && idx < totalLoops)
            valid.push_back(idx);
    if (valid.empty())
        return false;

    // --- Build session.json metadata ---
    auto* obj = new juce::DynamicObject();
    juce::var meta(obj);

    obj->setProperty("version",      "02.07.01");
    obj->setProperty("sampleRate",   getSampleRate());
    obj->setProperty("loopCount",    (int)valid.size());
    // selectedLoop mapped to 0-based position within the saved subset
    const int savedSelectedPos = [&] {
        for (int p = 0; p < (int)valid.size(); ++p)
            if (valid[p] == currentLoopIndex) return p;
        return 0;
    }();
    obj->setProperty("selectedLoop", savedSelectedPos);

    juce::Array<juce::var> loopsArr;
    for (int idx : valid)
    {
        auto* lobj = new juce::DynamicObject();
        lobj->setProperty("volume",       loopEngines[idx]->getVolume());
        lobj->setProperty("pan",          loopEngines[idx]->getPan());
        lobj->setProperty("loopLength",   loopEngines[idx]->getLoopLengthSamples());
        lobj->setProperty("isMuted",      loopEngines[idx]->isMute());
        lobj->setProperty("isSoloed",     loopEngines[idx]->isSolo());
        lobj->setProperty("loopSyncMode", (int)loopEngines[idx]->getSyncMode());
        lobj->setProperty("multiplier",   (int)loopEngines[idx]->getMultiplier());
        loopsArr.add(juce::var(lobj));
    }
    obj->setProperty("loops", loopsArr);
    obj->setProperty("masterLoopIndex",   masterLoopIndex);
    obj->setProperty("masterCycleLength", masterClock.getMasterCycleLength());

    auto* midiObj = new juce::DynamicObject();
    midiObj->setProperty("channel",        midiMapping.channel);
    midiObj->setProperty("recordNote",     midiMapping.recordNote);
    midiObj->setProperty("playNote",       midiMapping.playNote);
    midiObj->setProperty("undoNote",       midiMapping.undoNote);
    midiObj->setProperty("restartNote",    midiMapping.restartNote);
    midiObj->setProperty("clearNote",      midiMapping.clearNote);
    midiObj->setProperty("multiplyNote",   midiMapping.multiplyNote);
    midiObj->setProperty("redoNote",       midiMapping.redoNote);
    midiObj->setProperty("addLoopNote",    midiMapping.addLoopNote);
    midiObj->setProperty("loopUpNote",     midiMapping.loopUpNote);
    midiObj->setProperty("loopDownNote",   midiMapping.loopDownNote);
    midiObj->setProperty("deleteLoopNote", midiMapping.deleteLoopNote);
    midiObj->setProperty("muteNote",       midiMapping.muteNote);
    midiObj->setProperty("soloNote",       midiMapping.soloNote);
    midiObj->setProperty("loopAllNote",    midiMapping.loopAllNote);
    midiObj->setProperty("volumeCC",       midiMapping.volumeCC);
    midiObj->setProperty("panCC",          midiMapping.panCC);
    obj->setProperty("midi", juce::var(midiObj));

    juce::String jsonString = juce::JSON::toString(meta, true);

    // --- Encode each loop's audio as 32-bit float WAV ---
    juce::WavAudioFormat wavFormat;
    std::vector<juce::MemoryBlock> wavBlocks(valid.size());

    for (int p = 0; p < (int)valid.size(); ++p)
    {
        const int   idx     = valid[p];
        const auto& loopBuf = loopEngines[idx]->getLoopBuffer();
        const int   loopLen = loopEngines[idx]->getLoopLengthSamples();

        if (loopLen > 0)
        {
            std::unique_ptr<juce::OutputStream> wavStream =
                std::make_unique<juce::MemoryOutputStream>(wavBlocks[(size_t)p], false);

            auto writerOptions = juce::AudioFormatWriterOptions{}
                                     .withSampleRate(getSampleRate())
                                     .withNumChannels(loopBuf.getNumChannels())
                                     .withBitsPerSample(32);

            auto writer = wavFormat.createWriterFor(wavStream, writerOptions);
            if (writer == nullptr)
                return false;

            writer->writeFromAudioSampleBuffer(loopBuf, 0, loopLen);
            writer->flush();
        }
    }

    // --- Write ZIP archive ---
    file.deleteFile();

    {
        juce::ZipFile::Builder zipBuilder;

        juce::MemoryBlock jsonBlock(jsonString.toRawUTF8(), jsonString.getNumBytesAsUTF8());
        zipBuilder.addEntry(new juce::MemoryInputStream(jsonBlock, true), 9,
                            "session.json", juce::Time::getCurrentTime());

        for (int p = 0; p < (int)valid.size(); ++p)
        {
            if (wavBlocks[(size_t)p].getSize() > 0)
            {
                juce::String wavName = "loop_" + juce::String(p) + ".wav";
                zipBuilder.addEntry(
                    new juce::MemoryInputStream(wavBlocks[(size_t)p], true), 9,
                    wavName, juce::Time::getCurrentTime());
            }
        }

        juce::FileOutputStream fileOut(file);
        if (!fileOut.openedOk())
            return false;

        double unused = 0.0;
        if (!zipBuilder.writeToStream(fileOut, &unused))
            return false;
    }

    currentSessionFile = file;
    return true;
}

// Convenience overload: save ALL loops (original behaviour)
bool OrbitalLooperAudioProcessor::saveSession(const juce::File& file)
{
    std::vector<int> allIndices;
    allIndices.reserve((size_t)loopEngines.size());
    for (int i = 0; i < (int)loopEngines.size(); ++i)
        allIndices.push_back(i);
    return saveSession(file, allIndices);
}

//==============================================================================
// v02.00.01 - FULL SESSION LOAD (multi-loop + v01 backward compat)
//==============================================================================
bool OrbitalLooperAudioProcessor::loadSession(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    juce::ZipFile zip(file);
    if (zip.getNumEntries() == 0)
        return false;

    // --- Read session.json ---
    const juce::ZipFile::ZipEntry* jsonEntry = zip.getEntry("session.json");
    if (jsonEntry == nullptr)
        return false;

    juce::var meta;
    {
        std::unique_ptr<juce::InputStream> jsonStream(zip.createStreamForEntry(*jsonEntry));
        if (jsonStream == nullptr)
            return false;

        juce::String jsonString = jsonStream->readEntireStreamAsString();
        meta = juce::JSON::parse(jsonString);
        if (!meta.isObject())
            return false;
    }

    // --- Determine loop count ---
    const bool isV2 = meta.hasProperty("loopCount");
    const int  loopCount = isV2 ? static_cast<int>(meta["loopCount"]) : 1;

    // --- Rebuild engine list ---
    {
        const juce::ScopedLock sl(loopsLock);

        while ((int)loopEngines.size() < loopCount)
        {
            auto eng = std::make_unique<LoopEngine>();
            eng->prepare(getSampleRate(), getBlockSize(), getTotalNumInputChannels());
            loopEngines.push_back(std::move(eng));
        }
        while ((int)loopEngines.size() > loopCount)
            loopEngines.pop_back();

        if (isV2 && meta.hasProperty("loops"))
        {
            const auto& loopsArr = *meta["loops"].getArray();
            for (int i = 0; i < (int)loopsArr.size() && i < (int)loopEngines.size(); ++i)
            {
                if (loopsArr[i].hasProperty("volume"))
                    loopEngines[i]->setVolume(static_cast<float>(loopsArr[i]["volume"]));
                if (loopsArr[i].hasProperty("pan"))
                    loopEngines[i]->setPan(static_cast<float>(loopsArr[i]["pan"]));
                // v02.05.01/02: mute/solo — default false for old sessions
                loopEngines[i]->setMute(loopsArr[i].hasProperty("isMuted")
                                         ? static_cast<bool>(loopsArr[i]["isMuted"]) : false);
                loopEngines[i]->setSolo(loopsArr[i].hasProperty("isSoloed")
                                         ? static_cast<bool>(loopsArr[i]["isSoloed"]) : false);
                // v05.01: sync mode + multiplier
                if (loopsArr[i].hasProperty("loopSyncMode"))
                    loopEngines[i]->setSyncMode(static_cast<LoopSyncMode>(static_cast<int>(loopsArr[i]["loopSyncMode"])));
                if (loopsArr[i].hasProperty("multiplier"))
                    loopEngines[i]->setMultiplier(static_cast<MasterClock::LoopMultiplier>(static_cast<int>(loopsArr[i]["multiplier"])));
                loopEngines[i]->setMasterClock(&masterClock);
            }
        }
        else
        {
            if (meta.hasProperty("volume"))
                loopEngines[0]->setVolume(static_cast<float>(meta["volume"]));
            if (meta.hasProperty("pan"))
                loopEngines[0]->setPan(static_cast<float>(meta["pan"]));
        }

        currentLoopIndex = (meta.hasProperty("selectedLoop"))
            ? juce::jlimit(0, juce::jmax(0, loopCount - 1), static_cast<int>(meta["selectedLoop"]))
            : 0;
    }

    // --- MIDI mapping ---
    if (meta.hasProperty("midi"))
    {
        auto& m = meta["midi"];
        if (m.hasProperty("channel"))      midiMapping.channel      = static_cast<int>(m["channel"]);
        if (m.hasProperty("recordNote"))   midiMapping.recordNote   = static_cast<int>(m["recordNote"]);
        if (m.hasProperty("playNote"))     midiMapping.playNote     = static_cast<int>(m["playNote"]);
        if (m.hasProperty("undoNote"))     midiMapping.undoNote     = static_cast<int>(m["undoNote"]);
        if (m.hasProperty("restartNote"))  midiMapping.restartNote  = static_cast<int>(m["restartNote"]);
        if (m.hasProperty("clearNote"))    midiMapping.clearNote    = static_cast<int>(m["clearNote"]);
        if (m.hasProperty("multiplyNote")) midiMapping.multiplyNote = static_cast<int>(m["multiplyNote"]);
        if (m.hasProperty("redoNote"))     midiMapping.redoNote     = static_cast<int>(m["redoNote"]);
        if (m.hasProperty("addLoopNote"))  midiMapping.addLoopNote  = static_cast<int>(m["addLoopNote"]);
        if (m.hasProperty("loopUpNote"))     midiMapping.loopUpNote     = static_cast<int>(m["loopUpNote"]);
        if (m.hasProperty("loopDownNote"))   midiMapping.loopDownNote   = static_cast<int>(m["loopDownNote"]);
        if (m.hasProperty("deleteLoopNote")) midiMapping.deleteLoopNote = static_cast<int>(m["deleteLoopNote"]);
        if (m.hasProperty("muteNote"))       midiMapping.muteNote       = static_cast<int>(m["muteNote"]);
        if (m.hasProperty("soloNote"))       midiMapping.soloNote       = static_cast<int>(m["soloNote"]);
        if (m.hasProperty("loopAllNote"))    midiMapping.loopAllNote    = static_cast<int>(m["loopAllNote"]);
        if (m.hasProperty("volumeCC"))       midiMapping.volumeCC       = static_cast<int>(m["volumeCC"]);
        if (m.hasProperty("panCC"))          midiMapping.panCC          = static_cast<int>(m["panCC"]);
    }

    // --- Load audio for each loop ---
    juce::WavAudioFormat wavFormat;
    for (int i = 0; i < loopCount; ++i)
    {
        // v02: "loop_0.wav"; v01 backward compat: "loop.wav" for index 0
        juce::String wavName = isV2 ? ("loop_" + juce::String(i) + ".wav") : "loop.wav";
        const juce::ZipFile::ZipEntry* wavEntry = zip.getEntry(wavName);
        if (wavEntry == nullptr)
            continue;

        std::unique_ptr<juce::InputStream> wavStream(zip.createStreamForEntry(*wavEntry));
        if (wavStream == nullptr)
            continue;

        std::unique_ptr<juce::AudioFormatReader> reader(
            wavFormat.createReaderFor(wavStream.release(), true));
        if (reader == nullptr)
            continue;

        const int numSamples  = static_cast<int>(reader->lengthInSamples);
        const int numChannels = static_cast<int>(reader->numChannels);

        if (numSamples > 0 && numChannels > 0)
            loopEngines[i]->loadBuffer(numChannels, numSamples, *reader);
    }

    // v05.01: restore master clock state
    if (meta.hasProperty("masterLoopIndex"))
    {
        masterLoopIndex = static_cast<int>(meta["masterLoopIndex"]);
        masterClock.setMasterLoop(masterLoopIndex);
    }
    if (meta.hasProperty("masterCycleLength"))
    {
        int cycleLen = static_cast<int>(meta["masterCycleLength"]);
        if (cycleLen > 0)
            masterClock.setMasterCycleLength(cycleLen);
    }

    updateSoloState();   // sync anyLoopSoloed from loaded state

    if (onLoopCardsChanged)
        onLoopCardsChanged();

    currentSessionFile = file;
    return true;
}

//==============================================================================
juce::File OrbitalLooperAudioProcessor::getDefaultSessionsFolder()
{
    juce::File folder = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                            .getChildFile("Orbital Looper Sessions");
    if (!folder.exists())
        folder.createDirectory();
    return folder;
}

//==============================================================================
// v05.00 — SETTINGS SAVE / LOAD
//==============================================================================
bool OrbitalLooperAudioProcessor::saveSettings(const juce::File& file)
{
    juce::MemoryBlock data;
    getStateInformation(data);
    auto dest = file.withFileExtension(".ols");
    dest.getParentDirectory().createDirectory();
    return dest.replaceWithData(data.getData(), data.getSize());
}

bool OrbitalLooperAudioProcessor::loadSettings(const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    juce::MemoryBlock data;
    if (!file.loadFileAsData(data)) return false;
    setStateInformation(data.getData(), (int)data.getSize());
    return true;
}

//==============================================================================
// v05.00 — GLOBAL DEFAULTS
//==============================================================================
juce::File OrbitalLooperAudioProcessor::getGlobalDefaultsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
               .getChildFile("Orbital Looper")
               .getChildFile("defaults.json");
}

void OrbitalLooperAudioProcessor::saveGlobalDefaults()
{
    auto* obj = new juce::DynamicObject();
    juce::var state(obj);

    obj->setProperty("bpm",            globalBPM.load());
    obj->setProperty("timeSigNum",     timeSigNumerator.load());
    obj->setProperty("timeSigDen",     timeSigDenominator.load());
    obj->setProperty("metronomeOn",    metronomeOn.load());
    obj->setProperty("clickTrackOn",   (bool)clickTrackOn.load());
    obj->setProperty("clickVolume",    clickPlayer.getVolume());
    obj->setProperty("accentMask",     (int64_t)accentMask.load());
    obj->setProperty("countInEnabled", countInEnabled.load());
    obj->setProperty("countInSeconds", countInSeconds);

    auto* midiObj = new juce::DynamicObject();
    midiObj->setProperty("channel",              midiMapping.channel);
    midiObj->setProperty("recordNote",           midiMapping.recordNote);
    midiObj->setProperty("playNote",             midiMapping.playNote);
    midiObj->setProperty("undoNote",             midiMapping.undoNote);
    midiObj->setProperty("restartNote",          midiMapping.restartNote);
    midiObj->setProperty("clearNote",            midiMapping.clearNote);
    midiObj->setProperty("multiplyNote",         midiMapping.multiplyNote);
    midiObj->setProperty("redoNote",             midiMapping.redoNote);
    midiObj->setProperty("addLoopNote",          midiMapping.addLoopNote);
    midiObj->setProperty("loopUpNote",           midiMapping.loopUpNote);
    midiObj->setProperty("loopDownNote",         midiMapping.loopDownNote);
    midiObj->setProperty("deleteLoopNote",       midiMapping.deleteLoopNote);
    midiObj->setProperty("muteNote",             midiMapping.muteNote);
    midiObj->setProperty("soloNote",             midiMapping.soloNote);
    midiObj->setProperty("loopAllNote",          midiMapping.loopAllNote);
    midiObj->setProperty("metronomeToggleNote",  midiMapping.metronomeToggleNote);
    midiObj->setProperty("tapTempoNote",         midiMapping.tapTempoNote);
    midiObj->setProperty("clickTrackToggleNote", midiMapping.clickTrackToggleNote);
    midiObj->setProperty("volumeCC",             midiMapping.volumeCC);
    midiObj->setProperty("panCC",                midiMapping.panCC);
    midiObj->setProperty("quantizeFreeNote",  midiMapping.quantizeFreeNote);
    midiObj->setProperty("clickVolumeCC",     midiMapping.clickVolumeCC);
    midiObj->setProperty("countInToggleNote", midiMapping.countInToggleNote);
    obj->setProperty("midi", juce::var(midiObj));

    // v05.00 — setup mode + limits + sync mode
    obj->setProperty("isSingleTrack",   isSingleTrack);
    obj->setProperty("maxLoops",        maxLoops);
    obj->setProperty("unlimitedLoops",  unlimitedLoops);
    obj->setProperty("maxLayers",       maxLayers);
    obj->setProperty("unlimitedLayers", unlimitedLayers);
    obj->setProperty("syncMode",        (int)globalSyncMode);
    obj->setProperty("darkMode",        darkMode);

    auto dest = getGlobalDefaultsFile();
    dest.getParentDirectory().createDirectory();
    dest.replaceWithText(juce::JSON::toString(state));
}

void OrbitalLooperAudioProcessor::loadGlobalDefaults()
{
    auto f = getGlobalDefaultsFile();
    if (!f.existsAsFile()) return;

    juce::var state = juce::JSON::parse(f.loadFileAsString());
    if (!state.isObject()) return;

    if (state.hasProperty("bpm"))
        setBPM(static_cast<int>(state["bpm"]));
    if (state.hasProperty("timeSigNum") && state.hasProperty("timeSigDen"))
        setTimeSig(static_cast<int>(state["timeSigNum"]),
                   static_cast<int>(state["timeSigDen"]));
    if (state.hasProperty("metronomeOn"))
        metronomeOn.store(static_cast<bool>(state["metronomeOn"]));
    if (state.hasProperty("clickTrackOn"))
        clickTrackOn.store(static_cast<bool>(state["clickTrackOn"]));
    if (state.hasProperty("clickVolume"))
        clickPlayer.setVolume(static_cast<float>(state["clickVolume"]));
    if (state.hasProperty("accentMask"))
        accentMask.store(static_cast<uint32_t>(static_cast<int64_t>(state["accentMask"])));
    if (state.hasProperty("countInEnabled"))
        countInEnabled.store(static_cast<bool>(state["countInEnabled"]));
    if (state.hasProperty("countInSeconds"))
        countInSeconds = juce::jlimit(0.5, 60.0, static_cast<double>(state["countInSeconds"]));

    if (state.hasProperty("midi"))
    {
        auto& m = state["midi"];
        if (m.hasProperty("channel"))              midiMapping.channel              = static_cast<int>(m["channel"]);
        if (m.hasProperty("recordNote"))           midiMapping.recordNote           = static_cast<int>(m["recordNote"]);
        if (m.hasProperty("playNote"))             midiMapping.playNote             = static_cast<int>(m["playNote"]);
        if (m.hasProperty("undoNote"))             midiMapping.undoNote             = static_cast<int>(m["undoNote"]);
        if (m.hasProperty("restartNote"))          midiMapping.restartNote          = static_cast<int>(m["restartNote"]);
        if (m.hasProperty("clearNote"))            midiMapping.clearNote            = static_cast<int>(m["clearNote"]);
        if (m.hasProperty("multiplyNote"))         midiMapping.multiplyNote         = static_cast<int>(m["multiplyNote"]);
        if (m.hasProperty("redoNote"))             midiMapping.redoNote             = static_cast<int>(m["redoNote"]);
        if (m.hasProperty("addLoopNote"))          midiMapping.addLoopNote          = static_cast<int>(m["addLoopNote"]);
        if (m.hasProperty("loopUpNote"))           midiMapping.loopUpNote           = static_cast<int>(m["loopUpNote"]);
        if (m.hasProperty("loopDownNote"))         midiMapping.loopDownNote         = static_cast<int>(m["loopDownNote"]);
        if (m.hasProperty("deleteLoopNote"))       midiMapping.deleteLoopNote       = static_cast<int>(m["deleteLoopNote"]);
        if (m.hasProperty("muteNote"))             midiMapping.muteNote             = static_cast<int>(m["muteNote"]);
        if (m.hasProperty("soloNote"))             midiMapping.soloNote             = static_cast<int>(m["soloNote"]);
        if (m.hasProperty("loopAllNote"))          midiMapping.loopAllNote          = static_cast<int>(m["loopAllNote"]);
        if (m.hasProperty("metronomeToggleNote"))  midiMapping.metronomeToggleNote  = static_cast<int>(m["metronomeToggleNote"]);
        if (m.hasProperty("tapTempoNote"))         midiMapping.tapTempoNote         = static_cast<int>(m["tapTempoNote"]);
        if (m.hasProperty("clickTrackToggleNote")) midiMapping.clickTrackToggleNote = static_cast<int>(m["clickTrackToggleNote"]);
        if (m.hasProperty("volumeCC"))             midiMapping.volumeCC             = static_cast<int>(m["volumeCC"]);
        if (m.hasProperty("panCC"))                midiMapping.panCC                = static_cast<int>(m["panCC"]);
        if (m.hasProperty("quantizeFreeNote"))     midiMapping.quantizeFreeNote     = static_cast<int>(m["quantizeFreeNote"]);
        if (m.hasProperty("clickVolumeCC"))        midiMapping.clickVolumeCC        = static_cast<int>(m["clickVolumeCC"]);
        if (m.hasProperty("countInToggleNote"))    midiMapping.countInToggleNote    = static_cast<int>(m["countInToggleNote"]);
    }

    // v05.00 — setup mode + limits + sync mode
    if (state.hasProperty("isSingleTrack"))
        isSingleTrack = static_cast<bool>(state["isSingleTrack"]);
    if (state.hasProperty("maxLoops"))
        maxLoops = juce::jlimit(1, 999, static_cast<int>(state["maxLoops"]));
    if (state.hasProperty("unlimitedLoops"))
        unlimitedLoops = static_cast<bool>(state["unlimitedLoops"]);
    if (state.hasProperty("maxLayers"))
        maxLayers = juce::jlimit(1, 999, static_cast<int>(state["maxLayers"]));
    if (state.hasProperty("unlimitedLayers"))
        unlimitedLayers = static_cast<bool>(state["unlimitedLayers"]);
    if (state.hasProperty("syncMode"))
        globalSyncMode = static_cast<LoopSyncMode>(static_cast<int>(state["syncMode"]));
    if (state.hasProperty("darkMode"))
        darkMode = static_cast<bool>(state["darkMode"]);
}

bool OrbitalLooperAudioProcessor::hasGlobalDefaults() const
{
    return getGlobalDefaultsFile().existsAsFile();
}

//==============================================================================
// v05.00 — MIDI LEARN
//==============================================================================
void OrbitalLooperAudioProcessor::startMidiLearn(int functionIndex)
{
    midiLearnResult.store(-1);
    midiLearnFunctionIndex.store(functionIndex);
}

void OrbitalLooperAudioProcessor::cancelMidiLearn()
{
    midiLearnFunctionIndex.store(-1);
}

int OrbitalLooperAudioProcessor::pollMidiLearnResult()
{
    return midiLearnResult.exchange(-1);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OrbitalLooperAudioProcessor();
}
