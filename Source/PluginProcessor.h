/*
  ==============================================================================
    PluginProcessor.h

    Orbital Looper v03.00.01 - METRONOME: BPM

    Multi-loop audio processor. Holds a vector of LoopEngine instances.
    All loops play simultaneously (mixed to output). Buttons target the
    currently selected loop or variable selection.
    v02.04.01: Adds global LoopSyncMode (Free/Sync) — Sync is a no-op
    until MasterClock/Metronome integration is added in a future version.

    Planet Pluto Effects
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Core/LoopEngine.h"
#include "Core/MasterClock.h"
#include "Core/ClickPlayer.h"

//==============================================================================
class OrbitalLooperAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    OrbitalLooperAudioProcessor();
    ~OrbitalLooperAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
   #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // v02.00.01 - MULTI-LOOP ENGINE ACCESS
    //==============================================================================

    // Returns the selected loop engine (backward-compatible with v01 callers)
    LoopEngine& getLoopEngine()
    {
        const juce::ScopedLock sl(loopsLock);
        int safe = juce::jlimit(0, juce::jmax(0, (int)loopEngines.size() - 1), currentLoopIndex);
        return *loopEngines[safe];
    }

    // Returns loop engine by index (bounds-clamped)
    LoopEngine& getLoopEngine(int index)
    {
        const juce::ScopedLock sl(loopsLock);
        int safe = juce::jlimit(0, juce::jmax(0, (int)loopEngines.size() - 1), index);
        return *loopEngines[safe];
    }

    int  getCurrentLoopIndex() const { return currentLoopIndex; }
    void setCurrentLoopIndex(int index)
    {
        currentLoopIndex = juce::jlimit(0, juce::jmax(0, (int)loopEngines.size() - 1), index);
    }
    int  getNumLoops() const { return (int)loopEngines.size(); }

    void addNewLoop();      // Creates new LoopEngine, prepares, appends to vector
    void deleteLoop(int index); // Removes loop (keeps ≥1; resets if only loop)

    // UI callbacks — set by editor to respond to MIDI-triggered changes
    std::function<void()> onLoopCardsChanged;   // MIDI add/delete loop
    std::function<void()> onLoopAllToggled;     // MIDI LOOP ALL toggle (v02.02.01)
    std::function<void()> onSyncModeChanged;    // Sync mode changed (v02.04.01)
    std::function<void()> onMuteSoloChanged;    // Mute/solo state changed (v02.05.01/02)
    std::function<void()> onSingleTrackModeChanged;  // v05.00 — setup mode changed

    // v05.00 — SETUP MODE (single track vs multitrack)
    bool isSingleTrackMode() const { return isSingleTrack; }
    void setSingleTrackMode(bool single);

    // v05.00 — LIMITS
    int  getMaxLoops()        const { return maxLoops; }
    void setMaxLoops(int n)         { maxLoops = juce::jlimit(1, 999, n); }
    bool getUnlimitedLoops()  const { return unlimitedLoops; }
    void setUnlimitedLoops(bool u)  { unlimitedLoops = u; }
    int  getMaxLayers()       const { return maxLayers; }
    void setMaxLayers(int n)        { maxLayers = juce::jlimit(1, 999, n); }
    bool getUnlimitedLayers() const { return unlimitedLayers; }
    void setUnlimitedLayers(bool u) { unlimitedLayers = u; }
    bool canAddLayer(const LoopEngine& eng) const { return unlimitedLayers || eng.getOverdubCount() < maxLayers; }

    // v02.04.01 - SYNC MODE
    LoopSyncMode getSyncMode() const { return globalSyncMode; }
    void setSyncMode(LoopSyncMode mode);

    // v05.01 - MASTER CLOCK
    MasterClock& getMasterClock() { return masterClock; }
    const MasterClock& getMasterClock() const { return masterClock; }
    int getMasterLoopIndex() const { return masterLoopIndex; }
    void setMasterLoopIndex(int idx) { masterLoopIndex = idx; masterClock.setMasterLoop(idx); }

    //==============================================================================
    // v03.00.01 - METRONOME: BPM
    // v03.01.01 - METRONOME: TIME SIGNATURES
    //==============================================================================
    int  getBPM() const  { return globalBPM.load(); }
    void setBPM(int bpm) {
        globalBPM.store(juce::jlimit(40, 300, bpm));
        masterClock.setBPM(juce::jlimit(40, 300, bpm));
    }

    bool getMetronomeOn() const  { return metronomeOn.load(); }
    void setMetronomeOn(bool on) { metronomeOn.store(on); }

    int  getTimeSigNumerator()   const { return timeSigNumerator.load(); }
    int  getTimeSigDenominator() const { return timeSigDenominator.load(); }
    void setTimeSig(int num, int den)
    {
        timeSigNumerator.store(juce::jlimit(1, 32, num));
        timeSigDenominator.store(den);
        masterClock.setTimeSignature(juce::jlimit(1, 32, num), den);
        resetAccentMaskToDownbeats();
    }

    // v03.03.01 - BEAT VISUALIZATION
    // currentBeatIndex is an ever-increasing beat counter written by the audio
    // thread and read (modulo) by the UI thread — zero-latency, lock-free.
    int  getCurrentBeatIndex() const { return currentBeatIndex.load(); }
    void resetBeatIndex();   // call from UI; audio thread services the request

    // beatCounterActive: UI sets true to run the beat accumulator, false to freeze it.
    // Prevents "catch-up" when PAUSE is pressed — the accumulator stops advancing.
    bool getBeatCounterActive() const  { return beatCounterActive.load(); }
    void setBeatCounterActive(bool on) { beatCounterActive.store(on); }

    //==============================================================================
    // v03.04.01 - CLICK TRACK
    //==============================================================================
    bool     getClickTrackOn() const    { return clickTrackOn.load(); }
    void     setClickTrackOn(bool on)   { clickTrackOn.store(on); }
    float    getClickVolume()  const    { return clickPlayer.getVolume(); }
    void     setClickVolume(float v)    { clickPlayer.setVolume(v); }
    uint32_t getAccentMask()   const    { return accentMask.load(); }
    void     setAccentMask(uint32_t m)  { accentMask.store(m); }
    bool     loadClickFile(const juce::File& f) { return clickPlayer.loadUserFile(f); }
    void     clearClickFile()           { clickPlayer.clearUserFile(); }
    juce::String getClickFileName()     { return clickPlayer.getUserFileName(); }
    // v03.05.01 — reset all click track state to defaults (called from UI on OFF / CLEAR)
    void resetClickTrack();
    void resetAccentMaskToDownbeats();  // promoted to public — also called from UI on time sig change

    // v04.00 — COUNT IN (GroundLoop-style: time-based, silent, visual countdown)
    void startCountIn(int action);   // action: 1=record, 2=play
    void cancelCountIn();
    bool getCountInEnabled() const       { return countInEnabled.load(); }
    void setCountInEnabled(bool enabled) { countInEnabled.store(enabled); }
    bool isCountInActive() const         { return countInSamplesRemaining.load() > 0; }
    double getCountInSeconds() const     { return countInSeconds; }
    void   setCountInSeconds(double s)   { countInSeconds = juce::jlimit(0.5, 60.0, s); }
    double getCountInSecondsRemaining() const
    {
        const int sr = static_cast<int>(getSampleRate());
        if (sr <= 0) return 0.0;
        return countInSamplesRemaining.load() / static_cast<double>(sr);
    }
    // Poll from UI timer: returns true (and resets flag) when count-in just completed
    bool pollCountInCompleted()          { return countInJustCompleted.exchange(false); }
    // Poll from UI timer: returns and resets the pending action (0=none, 1=record, 2=play)
    int  pollPendingTriggerAction()      { return pendingTriggerAction.exchange(0); }

    // v02.05.01/02 - MUTE and SOLO
    void updateSoloState();

    //==============================================================================
    // v01.05.01 - MIDI MAPPING
    //==============================================================================
    struct MidiMapping
    {
        int channel      = 0;   // MIDI channel (0 = unset/disabled, 1–16 = active)
        int recordNote   = -1;  // RECORD / OVERDUB
        int playNote     = -1;  // PLAY / PAUSE
        int undoNote     = -1;  // UNDO
        int restartNote  = -1;  // RESTART
        int clearNote    = -1;  // CLEAR
        int multiplyNote = -1;  // MULTIPLY
        int redoNote     = -1;  // REDO
        int addLoopNote    = -1;  // ADD LOOP
        int loopDownNote   = -1;  // LOOP DOWN
        int deleteLoopNote = -1;  // DELETE LOOP
        int muteNote       = -1;  // MUTE toggle (current loop)
        int soloNote       = -1;  // SOLO toggle (current loop)
        int loopUpNote     = -1;  // LOOP UP
        int loopAllNote    = -1;  // LOOP ALL

        int metronomeToggleNote  = -1;   // METRONOME ON/OFF
        int tapTempoNote         = -1;   // TAP TEMPO
        int clickTrackToggleNote = -1;   // CLICK TRACK ON/OFF

        // MIDI CC assignments (-1 = unset; reassignable in Settings)
        int volumeCC = -1;   // CC for channel volume
        int panCC    = -1;   // CC for pan position

        int quantizeFreeNote  = -1;   // QUANTIZE/FREE toggle
        int clickVolumeCC     = -1;   // CLICK VOLUME via MIDI CC
        int countInToggleNote = -1;   // COUNT IN ON/OFF toggle
    };

    MidiMapping& getMidiMapping() { return midiMapping; }

    //==============================================================================
    // v01.06.01 - SAVE / LOAD SESSION
    //==============================================================================
    bool saveSession(const juce::File& file);
    // v02.07.01: Save only the loops at the given indices (subset save)
    bool saveSession(const juce::File& file, const std::vector<int>& loopIndices);
    bool loadSession(const juce::File& file);
    static juce::File getDefaultSessionsFolder();

    //==============================================================================
    // v05.00 — SETTINGS SAVE / LOAD (.ols = full state minus audio)
    //==============================================================================
    bool saveSettings(const juce::File& file);
    bool loadSettings(const juce::File& file);

    // v05.00 — GLOBAL DEFAULTS
    // Stored at: ~/Library/Application Support/Orbital Looper/defaults.json
    void saveGlobalDefaults();
    void loadGlobalDefaults();
    bool hasGlobalDefaults() const;
    static juce::File getGlobalDefaultsFile();

    // v05.00 — MIDI LEARN
    void startMidiLearn(int functionIndex);   // 0=record,1=play,...,18=panCC
    void cancelMidiLearn();
    int  pollMidiLearnResult();               // returns learned note/CC or -1; resets on read

    // v06.00 — EDITOR WIDTH PERSISTENCE
    int  getEditorWidth() const { return editorWidth; }
    void setEditorWidth(int w)  { editorWidth = w; }

    // v07.00 — LIGHT/DARK THEME
    bool getDarkMode() const     { return darkMode; }
    void setDarkMode(bool dark)  { darkMode = dark; if (onThemeChanged) onThemeChanged(); }
    std::function<void()> onThemeChanged;

private:
    //==============================================================================
    // v02.00.01 - LOOP ENGINE VECTOR
    // Guarded by loopsLock for audio-thread safety.
    //==============================================================================
    std::vector<std::unique_ptr<LoopEngine>> loopEngines;
    int currentLoopIndex = 0;
    mutable juce::CriticalSection loopsLock;

    //==============================================================================
    // v01.05.01 - MIDI
    //==============================================================================
    MidiMapping midiMapping;
    void processMidiMessage(const juce::MidiMessage& message);
    void processMidiTapTempo();   // v03.06.02 — tap tempo via MIDI

    int            lastMidiNote = -1;
    juce::int64    lastMidiTime = 0;
    static constexpr juce::int64 MIDI_DEBOUNCE_MS = 50;

    // v03.06.02 — MIDI tap tempo state (audio thread only, no sync needed)
    juce::int64 midiTapTimesAudio[8] = {};
    int         midiTapCountAudio    = 0;

    //==============================================================================
    // v02.04.01 - SYNC MODE
    //==============================================================================
    LoopSyncMode globalSyncMode = LoopSyncMode::Free;  // default Free
    MasterClock masterClock;
    int masterLoopIndex = -1;

    //==============================================================================
    // v02.05.01/02 - MUTE and SOLO
    //==============================================================================
    bool anyLoopSoloed = false;  // cached: true if any engine is soloed

    //==============================================================================
    // v03.00.01 - METRONOME: BPM
    // v03.01.01 - METRONOME: TIME SIGNATURES
    //==============================================================================
    std::atomic<int>  globalBPM           { 120 };  // 40–300 BPM
    std::atomic<bool> metronomeOn         { false };  // ON/OFF toggle — default OFF
    std::atomic<int>  timeSigNumerator    { 4 };     // default 4/4
    std::atomic<int>  timeSigDenominator  { 4 };

    // v03.03.01 - BEAT VISUALIZATION
    std::atomic<int>  currentBeatIndex       { 0 };    // audio writes, UI reads
    std::atomic<bool> resetBeatPhaseRequested{ false }; // UI sets, audio services
    std::atomic<bool> beatCounterActive      { false }; // UI sets; audio gates accumulator
    int               beatPhaseAccumulator   { 0 };    // audio thread only (plain int)
    int               lastBPMCache           { 120 };  // detect BPM change in processBlock

    // v03.04.01 - CLICK TRACK
    std::atomic<bool>     clickTrackOn  { false };           // UI toggle
    std::atomic<uint32_t> accentMask    { 0x00001111u };     // recalculated in prepareToPlay
    ClickPlayer           clickPlayer;
    juce::AudioBuffer<float> clickBuffer;

    //==============================================================================
    // v04.00 - COUNT IN (GroundLoop-style: silent, time-based sample countdown)
    //==============================================================================
    std::atomic<int>  countInSamplesRemaining { 0 };  // audio: decremented per block
    std::atomic<int>  pendingTriggerAction    { 0 };  // 0=none, 1=record, 2=play
    std::atomic<bool> countInJustCompleted    { false }; // audio → UI signal
    std::atomic<bool> countInEnabled          { false }; // user setting (persisted)
    double            countInSeconds          = 4.0;     // configurable duration (persisted)

    // v05.00 — MIDI LEARN (audio thread writes, UI polls)
    std::atomic<int>  midiLearnFunctionIndex { -1 };
    std::atomic<int>  midiLearnResult        { -1 };

    // v05.00 — SETUP MODE
    bool isSingleTrack = false;

    // v05.00 — LIMITS
    int  maxLoops       = 16;
    bool unlimitedLoops = false;
    int  maxLayers      = 100;
    bool unlimitedLayers = false;

    //==============================================================================
    // v01.06.01 - SESSION STATE
    //==============================================================================
    juce::File currentSessionFile;

    //==============================================================================
    // v06.00 - EDITOR WIDTH PERSISTENCE
    //==============================================================================
    int editorWidth = 768;

    //==============================================================================
    // v07.00 - LIGHT/DARK THEME
    //==============================================================================
    bool darkMode = true;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitalLooperAudioProcessor)
};
