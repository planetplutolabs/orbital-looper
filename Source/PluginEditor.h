/*
  ==============================================================================
    PluginEditor.h

    Orbital Looper v03.00.01 - METRONOME: BPM

    Multi-loop UI. Each loop gets a full-width LoopCardComponent in a
    vertical stack inside a Viewport. An [ADD LOOP] button sits below
    the list. LOOP UP / LOOP DOWN navigate the selected loop.
    Each card has a ✕ delete button; deleting the last loop resets it.
    LOOP ALL broadcasts all function buttons to every loop simultaneously.
    VARIABLE LOOP SELECTION: Cmd+click toggles a loop, Shift+click
    range-selects. Function buttons target all selected loops.

    Planet Pluto Effects
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/SettingsWindow.h"

// Forward declaration
class OrbitalLooperAudioProcessorEditor;

//==============================================================================
// LOOP CARD COMPONENT
// One instance per loop. Owns its own 30 Hz timer for live updates.
// Draws background, time counter, progress bar, and layer dots in paint().
// Row 1 controls (name, VOL, PAN, state) are real JUCE components.
//==============================================================================
class LoopCardComponent : public juce::Component,
                          private juce::Timer
{
public:
    LoopCardComponent(OrbitalLooperAudioProcessor& processor, int loopIndex);
    ~LoopCardComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setSelected(bool selected);
    int  getLoopIndex() const { return loopIndex; }

    // Set by editor — called when the card is clicked
    // Signature: (loopIndex, cmdHeld, shiftHeld)
    std::function<void(int, bool, bool)> onSelected;

    // Set by editor — called when the ✕ delete button is clicked
    std::function<void(int)> onDelete;

    // v02.05.01/02 — called from editor to sync M/S state + colours
    void updateMuteSoloColours();
    void syncMuteSoloState();   // syncs toggle states from engine then updates colours

    // v07.00 — refresh label colours after theme change
    void refreshThemeColours();

    //==========================================================================
    // Layout constants shared with PluginEditor (must match CARD_* below)
    //==========================================================================
    static constexpr int CARD_PADDING_H  = 12;
    static constexpr int CARD_PADDING_V  = 4;
    static constexpr int CARD_ROW1_H     = 30;
    static constexpr int CARD_ROW2_H     = 22;
    static constexpr int CARD_ROW3_H     = 28;
    static constexpr int CARD_INNER_GAP  = 4;
    // Total: 2*4 + 30 + 4 + 22 + 4 + 28 = 96
    static constexpr int CARD_HEIGHT     = 2 * CARD_PADDING_V + CARD_ROW1_H
                                           + CARD_INNER_GAP + CARD_ROW2_H
                                           + CARD_INNER_GAP + CARD_ROW3_H;

    static constexpr int VOL_LABEL_WIDTH   = 28;
    static constexpr int VOL_VALUE_WIDTH   = 32;
    static constexpr int SLIDER_VOL_WIDTH  = 90;
    static constexpr int SLIDER_PAN_WIDTH  = 90;
    static constexpr int CONTROL_GAP       = 8;
    static constexpr int STATE_LABEL_WIDTH = 90;
    static constexpr int LAYER_DOTS_WIDTH  = 80;
    static constexpr int DELETE_BTN_SIZE      = 22;  // ✕ button on right edge of Row 1
    static constexpr int MUTE_SOLO_BTN_SIZE   = 24;  // M / S buttons at left edge of Row 1
    static constexpr int MUTE_SOLO_GAP        = 6;   // gap between M and S
    static constexpr int MUTE_SOLO_AFTER_GAP  = 10;  // gap after S, before loop name

private:
    //==========================================================================
    void timerCallback() override;  // 30 Hz

    OrbitalLooperAudioProcessor& audioProcessor;
    int  loopIndex;
    bool isSelected = false;

    //==========================================================================
    // Row 1 sub-components
    //==========================================================================
    juce::Label      loopNameLabel;
    juce::Label      volLabel, volValueLabel;
    juce::Slider     volumeSlider;
    juce::Label      panLabel, panValueLabel;
    juce::Slider     panSlider;
    juce::Label      stateLabel;
    juce::Label      multiplierLabel;  // v05.01 — "Master", "1x", "2x" etc.
    juce::TextButton deleteButton;  // ✕ — always visible
    juce::TextButton muteButton;    // M — visible only when >1 loop (v02.05.01)
    juce::TextButton soloButton;    // S — visible only when >1 loop (v02.05.02)
    bool cachedMute  = false;
    bool cachedSolo  = false;

    //==========================================================================
    // Cached display values — updated by timer, used by paint()
    //==========================================================================
    float        loopProgress   = 0.0f;
    int          loopLayerCount = 0;
    juce::String currentTime  { "00:00.0" };
    juce::String totalTime    { "00:00.0" };

    // v06.00 — scale helpers for resizable UI
    float cardScale = 1.0f;
    int sc(int val) const { return static_cast<int>(val * cardScale); }
    float scf(float val) const { return val * cardScale; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoopCardComponent)
};

//==============================================================================
// BEAT VISUALIZATION COMPONENT — v03.03.01
// Displays a 4-bar beat grid that animates in sync with the metronome BPM.
// Polls currentBeatIndex from the processor at 60 Hz (zero-latency pattern,
// identical to GroundLoop's BeatGridComponent).
//==============================================================================
class BeatVisualizationComponent : public juce::Component,
                                   private juce::Timer
{
public:
    explicit BeatVisualizationComponent(OrbitalLooperAudioProcessor& proc);
    ~BeatVisualizationComponent() override;

    void paint(juce::Graphics& g) override;

    // Called when the user selects a different time signature
    void setTimeSignature(int numerator, int denominator);

    // State machine — driven by RECORD / PLAY / PAUSE / CLEAR / metronome ON/OFF
    void startFromBeginning(); // RECORD: reset beat index + start
    void startAnimation();     // PLAY:   resume from current position (or from 0)
    void pauseAnimation();     // PAUSE:  freeze position, keep visited-beats state
    void resetAndStop();       // CLEAR:  reset to beat 0, stop — do NOT start
    void stopAnimation();      // Metronome OFF: stop + full reset

    // v03.04.01 - Accent toggle
    void mouseDown(const juce::MouseEvent& e) override;
    void setAccentMask(uint32_t mask);
    uint32_t getAccentMask() const { return localAccentMask; }

    // v03.05.01 - Accent bar strip (thin rectangles above beat cells)
    void setShowAccentBars(bool show);
    bool getShowAccentBars() const { return showAccentBars; }

    // Fired when user toggles an accent (beat cell click or accent bar click)
    // Signature: (beatIndex, isNowAccented)
    std::function<void(int, bool)> onAccentToggled;

private:
    void timerCallback() override;

    static constexpr int NUM_BARS = 4;

    OrbitalLooperAudioProcessor& audioProcessor;

    int  beatsPerMeasure = 4;
    int  beatDenominator = 4;
    bool isAnimating     = false;
    int  currentBeat     = 0;
    int  lastBeat        = -1;

    std::vector<bool> visitedBeats;

    // v03.04.01 - Accent support
    uint32_t localAccentMask = 0x00001111u;  // UI-side mirror of processor accentMask

    // Beat cell registry — populated during paint(), used in mouseDown() when no accent bars
    struct CellInfo
    {
        juce::Rectangle<float> bounds;
        int beatIndex;
    };
    juce::Array<CellInfo> cells;  // up to 32 entries (4 bars × 8 beats max)

    // v03.05.01 - Accent bar registry (separate from beat cells)
    bool showAccentBars = false;
    struct AccentBarInfo
    {
        juce::Rectangle<float> bounds;
        int beatIndex;
    };
    juce::Array<AccentBarInfo> accentBarCells;  // populated in paint()

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BeatVisualizationComponent)
};

//==============================================================================
// v04.00 — TextButton subclass that exposes a right-click callback
//==============================================================================
class CountInMenuButton : public juce::TextButton
{
public:
    std::function<void()> onShowPopup;   // triggered by icon-area left-click OR right-click

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
        { if (onShowPopup) onShowPopup(); return; }

        // Left-click in top 55% (icon area) → open seconds popup, not a toggle
        if (e.getPosition().getY() < getHeight() * 0.55f)
        { if (onShowPopup) onShowPopup(); return; }

        // Left-click in bottom 45% (text area) → normal toggle behaviour
        juce::TextButton::mouseDown(e);
    }
};

//==============================================================================
// MAIN EDITOR
//==============================================================================
class OrbitalLooperAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    OrbitalLooperAudioProcessorEditor(OrbitalLooperAudioProcessor&);
    ~OrbitalLooperAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    //==========================================================================
    // Lifetime guard — set to false in destructor so queued callAsync lambdas
    // can safely bail out instead of calling through a dangling this pointer.
    std::shared_ptr<std::atomic<bool>> editorAlive { std::make_shared<std::atomic<bool>>(true) };

    void timerCallback() override;
    void updateButtonStates();

    // Shared geometry helper
    juce::Rectangle<int> computeFunctionsGeometry(int windowWidth, int windowHeight,
                                                   int& btnSizeOut, int& paddingVOut,
                                                   int& withinGapOut, int& betweenGapOut);

    // Session helpers
    void showLoadChooser();
    void openSettingsWindow();   // v05.00

    // v02.07.01 - Save mode for multi-loop save dialog
    enum class SaveMode { All, Selected, Separately };
    void showSaveChoiceDialog();
    void showSaveChooser(SaveMode mode);

    std::unique_ptr<juce::FileChooser> fileChooser;

    // Multi-loop helpers
    void rebuildLoopCards();
    void selectLoop(int index, bool cmdHeld = false, bool shiftHeld = false);

    // Selection helpers (v02.03.01)
    void applyToAllLoops(std::function<void(LoopEngine&)> fn);  // keep for LOOP ALL
    std::vector<int> getTargetLoops() const;
    bool isLoopSelected(int index) const;

    bool loopAllActive = false;
    std::vector<int> selectedLoops;            // current selection (always ≥ 1)
    std::vector<int> preLoopAllSelection;      // saved before LOOP ALL engages

    //==========================================================================
    OrbitalLooperAudioProcessor& audioProcessor;
    OrbitalLooperLookAndFeel     customLookAndFeel;

    //==========================================================================
    // TOOLBAR STRIP
    //==========================================================================
    juce::Label      versionLabel;
    juce::TextButton themeToggleButton;   // v07.00 — LIGHT/DARK toggle
    juce::TextButton saveButton;
    juce::TextButton loadButton;
    juce::TextButton settingsButton;

    //==========================================================================
    // FUNCTIONS BAR — Group 1
    //==========================================================================
    juce::TextButton recordButton;
    juce::TextButton multiplyButton;

    // Group 2
    juce::TextButton playButton;
    juce::TextButton restartButton;

    // Group 3
    juce::TextButton undoButton;
    juce::TextButton redoButton;
    juce::TextButton clearButton;

    // Group 4: LOOP UP / DOWN / ALL
    juce::TextButton loopUpButton;
    juce::TextButton loopDownButton;
    juce::TextButton loopAllButton;

    // Group 5: COUNT IN / CLICK TRACK
    CountInMenuButton countInButton;     // v04.00 — right-click opens seconds popup
    juce::TextButton  clickTrackButton;

    //==========================================================================
    // MULTI-LOOP CARD LIST
    //==========================================================================
    juce::Viewport               loopCardsViewport;
    juce::Component              loopCardsContainer;
    juce::OwnedArray<LoopCardComponent> loopCards;
    juce::TextButton             addLoopButton;

    //==========================================================================
    // METRONOME CARD — v03.00.01 (BPM + TAP)
    //                  v03.01.01 (TIME SIGNATURES)
    //==========================================================================
    juce::TextButton metronomeOnOffButton;  // ON / OFF toggle — far left
    juce::Label      metronomeLabel;        // "METRONOME" text — shown when collapsed (OFF)
    juce::Label      bpmValueLabel;         // Editable number (e.g. "120")
    juce::Label      bpmUnitLabel;          // Static "BPM" text
    juce::TextButton tapButton;             // TAP tempo — far right

    // Time signature dropdown — v03.05.01 (replaces 5 TextButtons)
    juce::ComboBox   timeSigCombo;

    // Helper to select a time sig and update button highlight colours
    void applyTimeSig(int numerator, int denominator);

    // Tap tempo state
    std::vector<juce::int64> metroTapTimes; // Timestamps of recent taps
    int lastDisplayedBPM = 120;  // v03.06.02 — detect MIDI tap BPM changes

    // v04.00 — count-in countdown overlay + settings
    juce::Label countInCountdownLabel;   // large visual "4,3,2,1,GO" overlay
    double      countInSeconds = 4.0;    // mirrors audioProcessor.getCountInSeconds()
    void showCountInPopup();             // right-click popup for configuring seconds

    // Panel bounds — set in resized(), used by paint() to draw background
    juce::Rectangle<int> metronomeCardBounds;

    // Beat visualization — v03.03.01
    std::unique_ptr<BeatVisualizationComponent> beatVisualization;

    // v03.04.01 / v03.05.01 - CLICK TRACK controls (visible when metronomeOn AND clickTrackOn)
    juce::Label      clickVolLabel;
    juce::Slider     clickVolSlider;
    juce::ComboBox   soundCombo;              // replaces BROWSE/BUILT-IN buttons
    int              soundComboPreBrowseId = 1;  // id to revert to if Browse is cancelled

    //==========================================================================
    // STATUS LABEL
    //==========================================================================
    juce::Label statusLabel;

    //==========================================================================
    // TOOLTIP WINDOW
    //==========================================================================
    juce::TooltipWindow tooltipWindow { this, 400 };

    //==========================================================================
    // LAYOUT CONSTANTS
    //==========================================================================
    static constexpr int SECTION_WIDTH   = 738;
    static constexpr int BASE_WIDTH      = 2 * 15 + SECTION_WIDTH;   // 768px
    static constexpr int BASE_BTN_SIZE   = 54;

    static constexpr int TOOLBAR_HEIGHT      = 32;
    static constexpr int TOOLBAR_ICON_SIZE   = 28;
    static constexpr int TOOLBAR_ICON_GAP    = 6;

    static constexpr int FUNCTIONS_PADDING_V = 0;
    static constexpr int BTN_WITHIN_GROUP    = 6;
    static constexpr int BTN_BETWEEN_GROUP   = 12;

    // Metronome card (v03.00.01)
    static constexpr int METRONOME_CARD_H    = 44;

    // Beat visualization card (v03.03.01) — shown below metronome card when ON
    static constexpr int BEAT_VIZ_H          = 70;

    // Accent bar strip (v03.05.01) — thin row above beat viz when click track is ON
    static constexpr int ACCENT_BAR_H        = 12;

    // Card gap (below FUNCTIONS BAR and between cards)
    static constexpr int CARD_GAP            = 8;

    // ADD LOOP button (below card list)
    static constexpr int ADD_LOOP_BTN_H      = 32;
    static constexpr int ADD_LOOP_GAP        = 2;   // reduced gap between last card and Add button

    // Status bar
    static constexpr int STATUS_HEIGHT       = 26;

    // Maximum loops visible without scrolling (shows 3 cards; more requires scroll)
    static constexpr int VIEWPORT_CARDS_MAX  = 3;

    // Derived card height — mirrors LoopCardComponent::CARD_HEIGHT
    static constexpr int CARD_HEIGHT         = LoopCardComponent::CARD_HEIGHT;

    // v06.00 — scale helpers for resizable UI
    float currentScale = 1.0f;
    int s(int val) const { return static_cast<int>(val * currentScale); }
    float sf(float val) const { return val * currentScale; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrbitalLooperAudioProcessorEditor)
};
