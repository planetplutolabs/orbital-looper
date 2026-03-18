/*
  ==============================================================================
    SettingsWindow.h

    v05.00 — SESSION MANAGEMENT
    Floating settings window following GroundLoop MidiSettingsDialog pattern.

    Sections: SESSION, SETUP, METRONOME, CLICK TRACK, COUNT IN,
              SYNC MODE, MIDI (collapsible w/ LEARN), LIMITS,
              GLOBAL DEFAULTS

    Planet Pluto Effects
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "../PluginProcessor.h"
#include "LookAndFeel.h"

//==============================================================================
// SettingsComponent — scrollable content panel
//==============================================================================
class SettingsComponent : public juce::Component,
                          public juce::Timer
{
public:
    explicit SettingsComponent(OrbitalLooperAudioProcessor& processor);
    ~SettingsComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    OrbitalLooperAudioProcessor& audioProcessor;

    //==========================================================================
    // Section: THEME (v07.00)
    //==========================================================================
    juce::Label      themeSectionHeader;
    juce::TextButton themeDarkButton;
    juce::TextButton themeLightButton;
    bool             cachedDarkMode = true;   // tracks last-known state for timer sync
    void updateThemeButtons();
    void refreshThemeColours();

    //==========================================================================
    // Section: SESSION
    //==========================================================================
    juce::Label      sessionSectionHeader;
    juce::TextButton saveSettingsButton;
    juce::TextButton loadSettingsButton;
    std::unique_ptr<juce::FileChooser> fileChooser;

    //==========================================================================
    // Section: SETUP
    //==========================================================================
    juce::Label      setupSectionHeader;
    juce::TextButton singleTrackButton;
    juce::TextButton multiTrackButton;

    //==========================================================================
    // Section: METRONOME
    //==========================================================================
    juce::Label      metronomeSectionHeader;
    juce::TextButton metronomeOnOffButton;
    juce::Label      metronomeBPMLabel;        // editable grey box
    juce::ComboBox   timeSignatureCombo;
    int              customTimeSigNum = 4;
    int              customTimeSigDen = 4;

    //==========================================================================
    // Section: CLICK TRACK
    //==========================================================================
    juce::Label      clickSectionHeader;
    juce::TextButton clickOnOffButton;
    juce::Label      clickVolLabel;
    juce::Slider     clickVolSlider;
    juce::ComboBox   clickTypeCombo;
    std::unique_ptr<juce::FileChooser> clickFileChooser;

    //==========================================================================
    // Section: COUNT IN
    //==========================================================================
    juce::Label      countInSectionHeader;
    juce::TextButton countInOnOffButton;
    juce::Label      countInSecsLabel;          // editable grey box

    //==========================================================================
    // Section: SYNC MODE
    //==========================================================================
    juce::Label      syncSectionHeader;
    juce::TextButton syncOnOffButton;

    //==========================================================================
    // Section: MIDI (collapsible)
    //==========================================================================
    juce::Label      midiSectionHeader;         // hit-test for expand/collapse
    bool             isMidiExpanded = false;

    static constexpr int NUM_MIDI_FUNCTIONS      = 22;
    static constexpr int FUNCTIONS_GROUP_START    = 0;
    static constexpr int LOOPS_GROUP_START        = 7;
    static constexpr int METRONOME_GROUP_START    = 17;
    static constexpr int CLICK_TRACK_GROUP_START  = 19;
    static constexpr int COUNT_IN_GROUP_START     = 21;

    juce::OwnedArray<juce::Label>       functionLabels;
    juce::OwnedArray<juce::ComboBox>    typeComboBoxes;     // Note / CC
    juce::OwnedArray<juce::ComboBox>    channelComboBoxes;  // per-row channel
    juce::OwnedArray<juce::Label>       noteDataLabels;     // editable grey box
    juce::OwnedArray<juce::TextButton>  learnButtons;
    int learningIndex = -1;

    //==========================================================================
    // Section: LIMITS
    //==========================================================================
    juce::Label      limitsSectionHeader;
    juce::Label      maxLoopsLabel;
    juce::Label      maxLoopsValueLabel;        // editable grey box
    juce::TextButton unlimitedLoopsButton;      // ON/OFF toggle
    juce::Label      maxLayersLabel;
    juce::Label      maxLayersValueLabel;       // editable grey box
    juce::TextButton unlimitedLayersButton;     // ON/OFF toggle

    //==========================================================================
    // Section: GLOBAL DEFAULTS
    //==========================================================================
    juce::Label      globalSectionHeader;
    juce::TextButton setDefaultButton;
    juce::TextButton restoreDefaultButton;

    //==========================================================================
    // Bottom row
    //==========================================================================
    juce::TextButton resetToDefaultsButton;

    //==========================================================================
    // Helpers
    //==========================================================================
    void updateMetronomeButton();
    void updateClickButton();
    void updateCountInButton();
    void updateSetupButtons();
    void updateSyncButton();
    void updateUnlimitedLoopsButton();
    void updateUnlimitedLayersButton();
    void syncTimeSigComboToMasterClock();
    void cancelLearn();
    void setMidiRowsVisible(bool visible);
    void resizeParentWindow(int newHeight);
    int  computeExpandedHeight() const;
    void refreshAllControls();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};

//==============================================================================
// SettingsViewport — wraps SettingsComponent in a juce::Viewport
//==============================================================================
class SettingsViewport : public juce::Component
{
public:
    explicit SettingsViewport(OrbitalLooperAudioProcessor& processor);
    void resized() override;
    SettingsComponent* getContent() { return content.get(); }

private:
    juce::Viewport viewport;
    std::unique_ptr<SettingsComponent> content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsViewport)
};

//==============================================================================
// SettingsDialog — floating DocumentWindow; self-deletes on close
//==============================================================================
class SettingsDialog : public juce::DocumentWindow
{
public:
    explicit SettingsDialog(OrbitalLooperAudioProcessor& processor);
    void closeButtonPressed() override { delete this; }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsDialog)
};
