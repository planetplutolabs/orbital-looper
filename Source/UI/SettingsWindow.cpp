/*
  ==============================================================================
    SettingsWindow.cpp

    v05.00 — SESSION MANAGEMENT
    Closely follows GroundLoop MidiSettingsComponent patterns.

    Planet Pluto Effects
  ==============================================================================
*/

#include "SettingsWindow.h"

// ─── static helpers ───────────────────────────────────────────────────────────

static const char* kFunctionNames[22] =
{
    // FUNCTIONS (0-6)
    "RECORD/OVERDUB",    // 0
    "MULTIPLY",          // 1
    "PLAY/PAUSE",        // 2
    "RESTART",           // 3
    "UNDO",              // 4
    "REDO",              // 5
    "CLEAR",             // 6
    // LOOPS (7-16)
    "LOOP VOLUME",       // 7
    "LOOP PAN",          // 8
    "ADD LOOP",          // 9
    "MUTE",              // 10
    "SOLO",              // 11
    "QUANTIZE/FREE",     // 12
    "LOOP UP",           // 13
    "LOOP DOWN",         // 14
    "LOOP ALL",          // 15
    "DELETE LOOP(S)",    // 16
    // METRONOME (17-18)
    "ON/OFF",            // 17
    "TAP TEMPO",         // 18
    // CLICK TRACK (19-20)
    "ON/OFF",            // 19
    "CLICK VOLUME",      // 20
    // COUNT IN (21)
    "ON/OFF"             // 21
};

static int getNoteForIndex(const OrbitalLooperAudioProcessor::MidiMapping& m, int i)
{
    switch (i)
    {
        case  0: return m.recordNote;
        case  1: return m.multiplyNote;
        case  2: return m.playNote;
        case  3: return m.restartNote;
        case  4: return m.undoNote;
        case  5: return m.redoNote;
        case  6: return m.clearNote;
        case  7: return m.volumeCC;
        case  8: return m.panCC;
        case  9: return m.addLoopNote;
        case 10: return m.muteNote;
        case 11: return m.soloNote;
        case 12: return m.quantizeFreeNote;
        case 13: return m.loopUpNote;
        case 14: return m.loopDownNote;
        case 15: return m.loopAllNote;
        case 16: return m.deleteLoopNote;
        case 17: return m.metronomeToggleNote;
        case 18: return m.tapTempoNote;
        case 19: return m.clickTrackToggleNote;
        case 20: return m.clickVolumeCC;
        case 21: return m.countInToggleNote;
        default: return 0;
    }
}

static void setNoteForIndex(OrbitalLooperAudioProcessor::MidiMapping& m, int i, int val)
{
    switch (i)
    {
        case  0: m.recordNote           = val; break;
        case  1: m.multiplyNote         = val; break;
        case  2: m.playNote             = val; break;
        case  3: m.restartNote          = val; break;
        case  4: m.undoNote             = val; break;
        case  5: m.redoNote             = val; break;
        case  6: m.clearNote            = val; break;
        case  7: m.volumeCC             = val; break;
        case  8: m.panCC                = val; break;
        case  9: m.addLoopNote          = val; break;
        case 10: m.muteNote             = val; break;
        case 11: m.soloNote             = val; break;
        case 12: m.quantizeFreeNote     = val; break;
        case 13: m.loopUpNote           = val; break;
        case 14: m.loopDownNote         = val; break;
        case 15: m.loopAllNote          = val; break;
        case 16: m.deleteLoopNote       = val; break;
        case 17: m.metronomeToggleNote  = val; break;
        case 18: m.tapTempoNote         = val; break;
        case 19: m.clickTrackToggleNote = val; break;
        case 20: m.clickVolumeCC        = val; break;
        case 21: m.countInToggleNote    = val; break;
    }
}

static bool isCCTypeByDefault(int index)
{
    return (index == 7 || index == 8 || index == 20);
}

// ─── constructor ──────────────────────────────────────────────────────────────

SettingsComponent::SettingsComponent(OrbitalLooperAudioProcessor& processor)
    : audioProcessor(processor)
{
    const auto panelColour  = OrbitalLooperLookAndFeel::getPanelColour();
    const auto textColour   = OrbitalLooperLookAndFeel::getTextColour();
    const auto borderColour = OrbitalLooperLookAndFeel::getBorderColour();

    // ═══════════════════════════════════════════════════════════════════════════
    // THEME section (v07.00)
    // ═══════════════════════════════════════════════════════════════════════════
    themeSectionHeader.setText("THEME", juce::dontSendNotification);
    themeSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    themeSectionHeader.setColour(juce::Label::textColourId, textColour);
    themeSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(themeSectionHeader);

    themeDarkButton.setButtonText("DARK");
    themeDarkButton.setClickingTogglesState(false);
    themeDarkButton.setColour(juce::TextButton::textColourOffId, textColour);
    themeDarkButton.setColour(juce::TextButton::textColourOnId,  textColour);
    themeDarkButton.onClick = [this]
    {
        audioProcessor.setDarkMode(true);
        updateThemeButtons();
    };
    addAndMakeVisible(themeDarkButton);

    themeLightButton.setButtonText("LIGHT");
    themeLightButton.setClickingTogglesState(false);
    themeLightButton.setColour(juce::TextButton::textColourOffId, textColour);
    themeLightButton.setColour(juce::TextButton::textColourOnId,  textColour);
    themeLightButton.onClick = [this]
    {
        audioProcessor.setDarkMode(false);
        updateThemeButtons();
    };
    addAndMakeVisible(themeLightButton);

    updateThemeButtons();

    // ═══════════════════════════════════════════════════════════════════════════
    // SESSION section
    // ═══════════════════════════════════════════════════════════════════════════
    sessionSectionHeader.setText("SESSION", juce::dontSendNotification);
    sessionSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    sessionSectionHeader.setColour(juce::Label::textColourId, textColour);
    sessionSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sessionSectionHeader);

    saveSettingsButton.setButtonText("Save Settings");
    saveSettingsButton.setColour(juce::TextButton::buttonColourId,   panelColour);
    saveSettingsButton.setColour(juce::TextButton::buttonOnColourId, panelColour);
    saveSettingsButton.setColour(juce::TextButton::textColourOffId,  textColour);
    saveSettingsButton.setColour(juce::TextButton::textColourOnId,   textColour);
    saveSettingsButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Settings",
            audioProcessor.getDefaultSessionsFolder().getChildFile("MySettings.ols"),
            "*.ols");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode |
            juce::FileBrowserComponent::canSelectFiles |
            juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result != juce::File{})
                    audioProcessor.saveSettings(result);
            });
    };
    addAndMakeVisible(saveSettingsButton);

    loadSettingsButton.setButtonText("Load Settings");
    loadSettingsButton.setColour(juce::TextButton::buttonColourId,   panelColour);
    loadSettingsButton.setColour(juce::TextButton::buttonOnColourId, panelColour);
    loadSettingsButton.setColour(juce::TextButton::textColourOffId,  textColour);
    loadSettingsButton.setColour(juce::TextButton::textColourOnId,   textColour);
    loadSettingsButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Settings",
            audioProcessor.getDefaultSessionsFolder(),
            "*.ols");
        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result != juce::File{})
                {
                    if (audioProcessor.loadSettings(result))
                        refreshAllControls();
                }
            });
    };
    addAndMakeVisible(loadSettingsButton);

    // ═══════════════════════════════════════════════════════════════════════════
    // SETUP section (Single Track / Multitrack)
    // ═══════════════════════════════════════════════════════════════════════════
    setupSectionHeader.setText("SETUP", juce::dontSendNotification);
    setupSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    setupSectionHeader.setColour(juce::Label::textColourId, textColour);
    setupSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(setupSectionHeader);

    singleTrackButton.setButtonText("SINGLE TRACK");
    singleTrackButton.setClickingTogglesState(false);
    singleTrackButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    singleTrackButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    singleTrackButton.onClick = [this]
    {
        audioProcessor.setSingleTrackMode(true);
        updateSetupButtons();
    };
    addAndMakeVisible(singleTrackButton);

    multiTrackButton.setButtonText("MULTITRACK");
    multiTrackButton.setClickingTogglesState(false);
    multiTrackButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    multiTrackButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    multiTrackButton.onClick = [this]
    {
        audioProcessor.setSingleTrackMode(false);
        updateSetupButtons();
    };
    addAndMakeVisible(multiTrackButton);

    updateSetupButtons();

    // ═══════════════════════════════════════════════════════════════════════════
    // METRONOME section
    // ═══════════════════════════════════════════════════════════════════════════
    metronomeSectionHeader.setText("METRONOME", juce::dontSendNotification);
    metronomeSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    metronomeSectionHeader.setColour(juce::Label::textColourId, textColour);
    metronomeSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(metronomeSectionHeader);

    metronomeOnOffButton.setClickingTogglesState(true);
    metronomeOnOffButton.setToggleState(audioProcessor.getMetronomeOn(), juce::dontSendNotification);
    metronomeOnOffButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    metronomeOnOffButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    updateMetronomeButton();
    metronomeOnOffButton.onClick = [this]
    {
        audioProcessor.setMetronomeOn(metronomeOnOffButton.getToggleState());
        updateMetronomeButton();
    };
    addAndMakeVisible(metronomeOnOffButton);

    metronomeBPMLabel.setText(juce::String(audioProcessor.getBPM()), juce::dontSendNotification);
    metronomeBPMLabel.setEditable(true, true);
    metronomeBPMLabel.setJustificationType(juce::Justification::centred);
    metronomeBPMLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    metronomeBPMLabel.setColour(juce::Label::textColourId,       textColour);
    metronomeBPMLabel.setColour(juce::Label::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
    metronomeBPMLabel.setColour(juce::Label::outlineColourId,    OrbitalLooperLookAndFeel::getBorderColour());
    metronomeBPMLabel.onTextChange = [this]
    {
        int val = metronomeBPMLabel.getText().retainCharacters("0123456789").getIntValue();
        val = juce::jlimit(40, 300, val);
        audioProcessor.setBPM(val);
        metronomeBPMLabel.setText(juce::String(val), juce::dontSendNotification);
    };
    addAndMakeVisible(metronomeBPMLabel);

    timeSignatureCombo.addItem("4/4",      1);
    timeSignatureCombo.addItem("3/4",      2);
    timeSignatureCombo.addItem("2/4",      3);
    timeSignatureCombo.addItem("6/8",      4);
    timeSignatureCombo.addItem("7/8",      5);
    timeSignatureCombo.addItem("5/4",      6);
    timeSignatureCombo.addItem("Custom...", 7);
    timeSignatureCombo.setColour(juce::ComboBox::backgroundColourId, panelColour);
    timeSignatureCombo.setColour(juce::ComboBox::textColourId,       textColour);
    timeSignatureCombo.setColour(juce::ComboBox::outlineColourId,    borderColour);
    syncTimeSigComboToMasterClock();
    timeSignatureCombo.onChange = [this]
    {
        int id = timeSignatureCombo.getSelectedId();
        switch (id)
        {
            case 1: audioProcessor.setTimeSig(4, 4); break;
            case 2: audioProcessor.setTimeSig(3, 4); break;
            case 3: audioProcessor.setTimeSig(2, 4); break;
            case 4: audioProcessor.setTimeSig(6, 8); break;
            case 5: audioProcessor.setTimeSig(7, 8); break;
            case 6: audioProcessor.setTimeSig(5, 4); break;
            case 7:
            {
                auto* window = new juce::AlertWindow("Custom Time Signature",
                                                      "Enter time signature as num/den (e.g. 7/8):",
                                                      juce::AlertWindow::NoIcon);
                auto* editor = new juce::TextEditor();
                editor->setText(juce::String(customTimeSigNum) + "/" + juce::String(customTimeSigDen));
                editor->setInputRestrictions(5, "0123456789/");
                editor->setJustification(juce::Justification::centred);
                editor->setSize(120, 28);
                window->addCustomComponent(editor);
                window->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
                window->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                window->enterModalState(true, juce::ModalCallbackFunction::create([this, window, editor](int result)
                {
                    if (result == 1)
                    {
                        juce::String text = editor->getText();
                        int slashPos = text.indexOf("/");
                        if (slashPos > 0)
                        {
                            int num = text.substring(0, slashPos).getIntValue();
                            int den = text.substring(slashPos + 1).getIntValue();
                            if (num >= 1 && num <= 32 && den >= 1 && den <= 32)
                            {
                                customTimeSigNum = num;
                                customTimeSigDen = den;
                                audioProcessor.setTimeSig(num, den);
                            }
                        }
                    }
                    else
                    {
                        syncTimeSigComboToMasterClock();
                    }
                    delete window;
                }), true);
                break;
            }
            default: break;
        }
    };
    addAndMakeVisible(timeSignatureCombo);

    // ═══════════════════════════════════════════════════════════════════════════
    // CLICK TRACK section
    // ═══════════════════════════════════════════════════════════════════════════
    clickSectionHeader.setText("CLICK TRACK", juce::dontSendNotification);
    clickSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    clickSectionHeader.setColour(juce::Label::textColourId, textColour);
    clickSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(clickSectionHeader);

    clickOnOffButton.setClickingTogglesState(true);
    clickOnOffButton.setToggleState(audioProcessor.getClickTrackOn(), juce::dontSendNotification);
    clickOnOffButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    clickOnOffButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    updateClickButton();
    clickOnOffButton.onClick = [this]
    {
        audioProcessor.setClickTrackOn(clickOnOffButton.getToggleState());
        updateClickButton();
    };
    addAndMakeVisible(clickOnOffButton);

    clickVolSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    clickVolSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    clickVolSlider.setRange(0.0, 1.0, 0.01);
    clickVolSlider.setValue(audioProcessor.getClickVolume(), juce::dontSendNotification);
    clickVolSlider.setColour(juce::Slider::thumbColourId, OrbitalLooperLookAndFeel::getTextColour().brighter(0.2f));
    clickVolSlider.onValueChange = [this]
    {
        audioProcessor.setClickVolume(static_cast<float>(clickVolSlider.getValue()));
    };
    addAndMakeVisible(clickVolSlider);

    clickVolLabel.setText("VOLUME", juce::dontSendNotification);
    clickVolLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    clickVolLabel.setColour(juce::Label::textColourId, textColour);
    clickVolLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(clickVolLabel);

    // Click type combo
    clickTypeCombo.addItem("Woodblock (Built-in)", 1);
    clickTypeCombo.addItem("Load Custom File...",  2);
    if (audioProcessor.getClickFileName().isNotEmpty())
        clickTypeCombo.setText(audioProcessor.getClickFileName(), juce::dontSendNotification);
    else
        clickTypeCombo.setSelectedId(1, juce::dontSendNotification);
    clickTypeCombo.setColour(juce::ComboBox::backgroundColourId, panelColour);
    clickTypeCombo.setColour(juce::ComboBox::textColourId,       textColour);
    clickTypeCombo.setColour(juce::ComboBox::outlineColourId,    borderColour);
    clickTypeCombo.onChange = [this]
    {
        int id = clickTypeCombo.getSelectedId();
        if (id == 1)
        {
            audioProcessor.clearClickFile();
        }
        else if (id == 2)
        {
            clickFileChooser = std::make_unique<juce::FileChooser>(
                "Load Click Sound", juce::File{}, "*.wav;*.aif;*.aiff");
            clickFileChooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& fc)
                {
                    auto result = fc.getResult();
                    if (result != juce::File{})
                    {
                        if (audioProcessor.loadClickFile(result))
                            clickTypeCombo.setText(result.getFileName(), juce::dontSendNotification);
                        else
                            clickTypeCombo.setSelectedId(1, juce::dontSendNotification);
                    }
                    else
                    {
                        if (audioProcessor.getClickFileName().isNotEmpty())
                            clickTypeCombo.setText(audioProcessor.getClickFileName(), juce::dontSendNotification);
                        else
                            clickTypeCombo.setSelectedId(1, juce::dontSendNotification);
                    }
                });
        }
    };
    addAndMakeVisible(clickTypeCombo);

    // ═══════════════════════════════════════════════════════════════════════════
    // COUNT IN section
    // ═══════════════════════════════════════════════════════════════════════════
    countInSectionHeader.setText("COUNT IN", juce::dontSendNotification);
    countInSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    countInSectionHeader.setColour(juce::Label::textColourId, textColour);
    countInSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(countInSectionHeader);

    countInOnOffButton.setClickingTogglesState(true);
    countInOnOffButton.setToggleState(audioProcessor.getCountInEnabled(), juce::dontSendNotification);
    countInOnOffButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    countInOnOffButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    updateCountInButton();
    countInOnOffButton.onClick = [this]
    {
        audioProcessor.setCountInEnabled(countInOnOffButton.getToggleState());
        updateCountInButton();
    };
    addAndMakeVisible(countInOnOffButton);

    {
        double secs = audioProcessor.getCountInSeconds();
        countInSecsLabel.setText(juce::String(secs, 1), juce::dontSendNotification);
    }
    countInSecsLabel.setEditable(true, true);
    countInSecsLabel.setJustificationType(juce::Justification::centred);
    countInSecsLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    countInSecsLabel.setColour(juce::Label::textColourId,       textColour);
    countInSecsLabel.setColour(juce::Label::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
    countInSecsLabel.setColour(juce::Label::outlineColourId,    OrbitalLooperLookAndFeel::getBorderColour());
    countInSecsLabel.onTextChange = [this]
    {
        double val = countInSecsLabel.getText().retainCharacters("0123456789.").getDoubleValue();
        val = juce::jlimit(0.5, 60.0, val);
        audioProcessor.setCountInSeconds(val);
        countInSecsLabel.setText(juce::String(val, 1), juce::dontSendNotification);
    };
    addAndMakeVisible(countInSecsLabel);

    // ═══════════════════════════════════════════════════════════════════════════
    // SYNC MODE section
    // ═══════════════════════════════════════════════════════════════════════════
    syncSectionHeader.setText("SYNC MODE", juce::dontSendNotification);
    syncSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    syncSectionHeader.setColour(juce::Label::textColourId, textColour);
    syncSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(syncSectionHeader);

    syncOnOffButton.setClickingTogglesState(true);
    syncOnOffButton.setToggleState(audioProcessor.getSyncMode() == LoopSyncMode::Sync, juce::dontSendNotification);
    syncOnOffButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    syncOnOffButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    updateSyncButton();
    syncOnOffButton.onClick = [this]
    {
        bool on = syncOnOffButton.getToggleState();
        audioProcessor.setSyncMode(on ? LoopSyncMode::Sync : LoopSyncMode::Free);
        updateSyncButton();
    };
    addAndMakeVisible(syncOnOffButton);

    // ═══════════════════════════════════════════════════════════════════════════
    // MIDI section (collapsible)
    // ═══════════════════════════════════════════════════════════════════════════
    midiSectionHeader.setText("", juce::dontSendNotification);
    midiSectionHeader.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(midiSectionHeader);

    {
        auto mapping = audioProcessor.getMidiMapping();

        for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
        {
            auto* nameLabel = functionLabels.add(new juce::Label());
            nameLabel->setText(kFunctionNames[i], juce::dontSendNotification);
            nameLabel->setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(11.0f));
            nameLabel->setColour(juce::Label::textColourId, textColour);
            nameLabel->setJustificationType(juce::Justification::centredLeft);
            addAndMakeVisible(nameLabel);

            auto* typeCombo = typeComboBoxes.add(new juce::ComboBox());
            typeCombo->setTextWhenNothingSelected("");
            typeCombo->addItem("Note", 1);
            typeCombo->addItem("CC",   2);
            if (getNoteForIndex(mapping, i) >= 0)
                typeCombo->setSelectedId(isCCTypeByDefault(i) ? 2 : 1, juce::dontSendNotification);
            typeCombo->setColour(juce::ComboBox::backgroundColourId, panelColour);
            typeCombo->setColour(juce::ComboBox::textColourId,       textColour);
            typeCombo->setColour(juce::ComboBox::outlineColourId,    borderColour);
            addAndMakeVisible(typeCombo);

            auto* chanCombo = channelComboBoxes.add(new juce::ComboBox());
            chanCombo->setTextWhenNothingSelected("None");
            for (int ch = 1; ch <= 16; ++ch)
                chanCombo->addItem(juce::String(ch), ch);
            if (mapping.channel >= 1 && mapping.channel <= 16)
                chanCombo->setSelectedId(mapping.channel, juce::dontSendNotification);
            chanCombo->setColour(juce::ComboBox::backgroundColourId, panelColour);
            chanCombo->setColour(juce::ComboBox::textColourId,       textColour);
            chanCombo->setColour(juce::ComboBox::outlineColourId,    borderColour);
            addAndMakeVisible(chanCombo);

            auto* dataLabel = noteDataLabels.add(new juce::Label());
            {
                int noteVal = getNoteForIndex(mapping, i);
                dataLabel->setText(noteVal < 0 ? juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")) : juce::String(noteVal), juce::dontSendNotification);
            }
            dataLabel->setEditable(true, true);
            dataLabel->setJustificationType(juce::Justification::centred);
            dataLabel->setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(11.0f));
            dataLabel->setColour(juce::Label::textColourId,       textColour);
            dataLabel->setColour(juce::Label::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
            dataLabel->setColour(juce::Label::outlineColourId,    OrbitalLooperLookAndFeel::getBorderColour());
            dataLabel->onTextChange = [dataLabel]
            {
                auto text = dataLabel->getText().trim();
                if (text.isEmpty() || text == juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")))
                {
                    dataLabel->setText(juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")), juce::dontSendNotification);
                    return;
                }
                int val = text.retainCharacters("0123456789").getIntValue();
                val = juce::jlimit(0, 127, val);
                dataLabel->setText(juce::String(val), juce::dontSendNotification);
            };
            addAndMakeVisible(dataLabel);

            auto* learnBtn = learnButtons.add(new juce::TextButton("Learn"));
            learnBtn->setColour(juce::TextButton::buttonColourId,   OrbitalLooperLookAndFeel::getPanelColour());
            learnBtn->setColour(juce::TextButton::textColourOffId,  textColour);
            learnBtn->setColour(juce::TextButton::textColourOnId,   textColour);
            learnBtn->onClick = [this, i]
            {
                if (learningIndex == i)
                {
                    cancelLearn();
                }
                else
                {
                    cancelLearn();
                    learningIndex = i;
                    audioProcessor.startMidiLearn(i);
                    learnButtons[i]->setButtonText("Listening");
                    learnButtons[i]->setColour(juce::TextButton::buttonColourId, juce::Colour(0xffef4444));
                    learnButtons[i]->repaint();
                }
            };
            addAndMakeVisible(learnBtn);
        }
    }

    // ═══════════════════════════════════════════════════════════════════════════
    // LIMITS section
    // ═══════════════════════════════════════════════════════════════════════════
    limitsSectionHeader.setText("LIMITS", juce::dontSendNotification);
    limitsSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    limitsSectionHeader.setColour(juce::Label::textColourId, textColour);
    limitsSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(limitsSectionHeader);

    // Max Loops
    maxLoopsLabel.setText("Max Loops", juce::dontSendNotification);
    maxLoopsLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    maxLoopsLabel.setColour(juce::Label::textColourId, textColour);
    addAndMakeVisible(maxLoopsLabel);

    maxLoopsValueLabel.setText(juce::String(audioProcessor.getMaxLoops()), juce::dontSendNotification);
    maxLoopsValueLabel.setEditable(true, true);
    maxLoopsValueLabel.setJustificationType(juce::Justification::centred);
    maxLoopsValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    maxLoopsValueLabel.setColour(juce::Label::textColourId,       textColour);
    maxLoopsValueLabel.setColour(juce::Label::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
    maxLoopsValueLabel.setColour(juce::Label::outlineColourId,    OrbitalLooperLookAndFeel::getBorderColour());
    maxLoopsValueLabel.setEnabled(!audioProcessor.getUnlimitedLoops());
    maxLoopsValueLabel.onTextChange = [this]
    {
        int val = maxLoopsValueLabel.getText().retainCharacters("0123456789").getIntValue();
        val = juce::jlimit(1, 999, val);
        audioProcessor.setMaxLoops(val);
        maxLoopsValueLabel.setText(juce::String(val), juce::dontSendNotification);
    };
    addAndMakeVisible(maxLoopsValueLabel);

    unlimitedLoopsButton.setClickingTogglesState(true);
    unlimitedLoopsButton.setToggleState(audioProcessor.getUnlimitedLoops(), juce::dontSendNotification);
    unlimitedLoopsButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    unlimitedLoopsButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    updateUnlimitedLoopsButton();
    unlimitedLoopsButton.onClick = [this]
    {
        bool on = unlimitedLoopsButton.getToggleState();
        audioProcessor.setUnlimitedLoops(on);
        maxLoopsValueLabel.setEnabled(!on);
        updateUnlimitedLoopsButton();
    };
    addAndMakeVisible(unlimitedLoopsButton);

    // Max Layers
    maxLayersLabel.setText("Max Layers", juce::dontSendNotification);
    maxLayersLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    maxLayersLabel.setColour(juce::Label::textColourId, textColour);
    addAndMakeVisible(maxLayersLabel);

    maxLayersValueLabel.setText(juce::String(audioProcessor.getMaxLayers()), juce::dontSendNotification);
    maxLayersValueLabel.setEditable(true, true);
    maxLayersValueLabel.setJustificationType(juce::Justification::centred);
    maxLayersValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    maxLayersValueLabel.setColour(juce::Label::textColourId,       textColour);
    maxLayersValueLabel.setColour(juce::Label::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
    maxLayersValueLabel.setColour(juce::Label::outlineColourId,    OrbitalLooperLookAndFeel::getBorderColour());
    maxLayersValueLabel.setEnabled(!audioProcessor.getUnlimitedLayers());
    maxLayersValueLabel.onTextChange = [this]
    {
        int val = maxLayersValueLabel.getText().retainCharacters("0123456789").getIntValue();
        val = juce::jlimit(1, 999, val);
        audioProcessor.setMaxLayers(val);
        maxLayersValueLabel.setText(juce::String(val), juce::dontSendNotification);
    };
    addAndMakeVisible(maxLayersValueLabel);

    unlimitedLayersButton.setClickingTogglesState(true);
    unlimitedLayersButton.setToggleState(audioProcessor.getUnlimitedLayers(), juce::dontSendNotification);
    unlimitedLayersButton.setColour(juce::TextButton::textColourOffId, OrbitalLooperLookAndFeel::getTextColour());
    unlimitedLayersButton.setColour(juce::TextButton::textColourOnId,  OrbitalLooperLookAndFeel::getTextColour());
    updateUnlimitedLayersButton();
    unlimitedLayersButton.onClick = [this]
    {
        bool on = unlimitedLayersButton.getToggleState();
        audioProcessor.setUnlimitedLayers(on);
        maxLayersValueLabel.setEnabled(!on);
        updateUnlimitedLayersButton();
    };
    addAndMakeVisible(unlimitedLayersButton);

    // ═══════════════════════════════════════════════════════════════════════════
    // GLOBAL DEFAULTS section
    // ═══════════════════════════════════════════════════════════════════════════
    globalSectionHeader.setText("GLOBAL DEFAULTS", juce::dontSendNotification);
    globalSectionHeader.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
    globalSectionHeader.setColour(juce::Label::textColourId, textColour);
    globalSectionHeader.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(globalSectionHeader);

    setDefaultButton.setButtonText("Set as Global Default");
    setDefaultButton.setColour(juce::TextButton::buttonColourId,   panelColour);
    setDefaultButton.setColour(juce::TextButton::buttonOnColourId, panelColour);
    setDefaultButton.setColour(juce::TextButton::textColourOffId,  textColour);
    setDefaultButton.setColour(juce::TextButton::textColourOnId,   textColour);
    setDefaultButton.onClick = [this]
    {
        auto& mapping = audioProcessor.getMidiMapping();
        if (channelComboBoxes.size() > 0)
            mapping.channel = channelComboBoxes[0]->getSelectedId();
        for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
        {
            auto text = noteDataLabels[i]->getText().trim();
            int val = (text.isEmpty() || text == juce::String(juce::CharPointer_UTF8("\xe2\x80\x94"))) ? -1 : text.getIntValue();
            setNoteForIndex(mapping, i, val);
        }
        audioProcessor.saveGlobalDefaults();
    };
    addAndMakeVisible(setDefaultButton);

    restoreDefaultButton.setButtonText("Restore Global Default");
    restoreDefaultButton.setColour(juce::TextButton::buttonColourId,   panelColour);
    restoreDefaultButton.setColour(juce::TextButton::buttonOnColourId, panelColour);
    restoreDefaultButton.setColour(juce::TextButton::textColourOffId,  textColour);
    restoreDefaultButton.setColour(juce::TextButton::textColourOnId,   textColour);
    restoreDefaultButton.onClick = [this]
    {
        if (!audioProcessor.hasGlobalDefaults())
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon,
                "No Global Default",
                "No global default has been saved yet.");
            return;
        }
        audioProcessor.loadGlobalDefaults();
        refreshAllControls();
    };
    addAndMakeVisible(restoreDefaultButton);

    // ═══════════════════════════════════════════════════════════════════════════
    // Bottom row: Reset to Defaults
    // ═══════════════════════════════════════════════════════════════════════════
    resetToDefaultsButton.setButtonText("Reset to Defaults");
    resetToDefaultsButton.setColour(juce::TextButton::buttonColourId,   panelColour);
    resetToDefaultsButton.setColour(juce::TextButton::buttonOnColourId, panelColour);
    resetToDefaultsButton.setColour(juce::TextButton::textColourOffId,  textColour);
    resetToDefaultsButton.setColour(juce::TextButton::textColourOnId,   textColour);
    resetToDefaultsButton.onClick = [this]
    {
        cancelLearn();

        OrbitalLooperAudioProcessor::MidiMapping defaultMapping;

        // Reset MIDI rows
        for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
        {
            {
                int val = getNoteForIndex(defaultMapping, i);
                noteDataLabels[i]->setText(val < 0 ? juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")) : juce::String(val), juce::dontSendNotification);
            }
            channelComboBoxes[i]->setSelectedId(defaultMapping.channel, juce::dontSendNotification);
            if (getNoteForIndex(defaultMapping, i) >= 0)
                typeComboBoxes[i]->setSelectedId(isCCTypeByDefault(i) ? 2 : 1, juce::dontSendNotification);
            else
                typeComboBoxes[i]->setSelectedId(0, juce::dontSendNotification);
        }

        // Reset setup
        audioProcessor.setSingleTrackMode(false);
        updateSetupButtons();

        // Reset metronome
        metronomeOnOffButton.setToggleState(true, juce::dontSendNotification);
        audioProcessor.setMetronomeOn(true);
        updateMetronomeButton();
        audioProcessor.setBPM(120);
        metronomeBPMLabel.setText("120", juce::dontSendNotification);
        audioProcessor.setTimeSig(4, 4);
        syncTimeSigComboToMasterClock();
        customTimeSigNum = 4;
        customTimeSigDen = 4;

        // Reset click track
        clickOnOffButton.setToggleState(false, juce::dontSendNotification);
        audioProcessor.setClickTrackOn(false);
        updateClickButton();
        audioProcessor.setClickVolume(0.5f);
        clickVolSlider.setValue(0.5, juce::dontSendNotification);
        audioProcessor.clearClickFile();
        clickTypeCombo.setSelectedId(1, juce::dontSendNotification);

        // Reset count in
        countInOnOffButton.setToggleState(false, juce::dontSendNotification);
        audioProcessor.setCountInEnabled(false);
        updateCountInButton();
        audioProcessor.setCountInSeconds(4.0);
        countInSecsLabel.setText("4.0", juce::dontSendNotification);

        // Reset sync mode (default: Sync ON)
        syncOnOffButton.setToggleState(true, juce::dontSendNotification);
        audioProcessor.setSyncMode(LoopSyncMode::Sync);
        updateSyncButton();

        // Reset limits
        audioProcessor.setMaxLoops(16);
        audioProcessor.setUnlimitedLoops(false);
        maxLoopsValueLabel.setText("16", juce::dontSendNotification);
        maxLoopsValueLabel.setEnabled(true);
        unlimitedLoopsButton.setToggleState(false, juce::dontSendNotification);
        updateUnlimitedLoopsButton();

        audioProcessor.setMaxLayers(100);
        audioProcessor.setUnlimitedLayers(false);
        maxLayersValueLabel.setText("100", juce::dontSendNotification);
        maxLayersValueLabel.setEnabled(true);
        unlimitedLayersButton.setToggleState(false, juce::dontSendNotification);
        updateUnlimitedLayersButton();

        // Apply MIDI defaults
        auto& mapping = audioProcessor.getMidiMapping();
        mapping = defaultMapping;
    };
    addAndMakeVisible(resetToDefaultsButton);

    // MIDI rows start hidden
    for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
    {
        functionLabels[i]->setVisible(false);
        typeComboBoxes[i]->setVisible(false);
        channelComboBoxes[i]->setVisible(false);
        noteDataLabels[i]->setVisible(false);
        learnButtons[i]->setVisible(false);
    }

    setSize(560, 760);
    startTimerHz(15);
}

// ─── destructor ───────────────────────────────────────────────────────────────

SettingsComponent::~SettingsComponent()
{
    stopTimer();
    if (learningIndex >= 0)
        audioProcessor.cancelMidiLearn();
}

// ─── timer (MIDI learn polling) ───────────────────────────────────────────────

void SettingsComponent::timerCallback()
{
    // Sync theme when changed externally (e.g. toolbar toggle)
    const bool dark = audioProcessor.getDarkMode();
    if (dark != cachedDarkMode)
    {
        cachedDarkMode = dark;
        refreshThemeColours();
    }

    if (learningIndex >= 0)
    {
        int result = audioProcessor.pollMidiLearnResult();
        if (result >= 0)
        {
            noteDataLabels[learningIndex]->setText(juce::String(result), juce::dontSendNotification);
            setNoteForIndex(audioProcessor.getMidiMapping(), learningIndex, result);
            if (typeComboBoxes[learningIndex]->getSelectedId() == 0)
                typeComboBoxes[learningIndex]->setSelectedId(isCCTypeByDefault(learningIndex) ? 2 : 1, juce::dontSendNotification);
            learnButtons[learningIndex]->setButtonText("Learn");
            learnButtons[learningIndex]->setColour(juce::TextButton::buttonColourId, OrbitalLooperLookAndFeel::getPanelColour());
            learnButtons[learningIndex]->repaint();
            learningIndex = -1;
        }
    }
}

// ─── mouse events (MIDI header bar click) ─────────────────────────────────────

void SettingsComponent::mouseDown(const juce::MouseEvent& e)
{
    if (midiSectionHeader.getBounds().contains(e.position.toInt()))
    {
        isMidiExpanded = !isMidiExpanded;
        setMidiRowsVisible(isMidiExpanded);

        int newHeight = isMidiExpanded ? computeExpandedHeight() : 836;
        resizeParentWindow(newHeight);

        resized();
        repaint();
    }
}

// ─── paint ────────────────────────────────────────────────────────────────────

void SettingsComponent::paint(juce::Graphics& g)
{
    const int w = getWidth();
    const int h = getHeight();

    const auto bgColour        = OrbitalLooperLookAndFeel::getBackgroundColour();
    const auto containerColour = OrbitalLooperLookAndFeel::getContainerColour();
    const auto textColour      = OrbitalLooperLookAndFeel::getTextColour();
    const auto borderColour    = OrbitalLooperLookAndFeel::getBorderColour();

    g.fillAll(bgColour);

    // ── Title: "Settings" ───────────────────────────────────────────────────
    g.setColour(OrbitalLooperLookAndFeel::getTextColour());
    g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(18.0f).withStyle(juce::Font::bold));
    g.drawText("Settings", 0, 8, w, 26, juce::Justification::centred);

    // ── Section separator lines ──────────────────────────────────────────────
    auto drawSeparator = [&](const juce::Rectangle<int>& headerBounds)
    {
        g.setColour(borderColour);
        g.drawHorizontalLine(headerBounds.getBottom() + 2,
                             (float)headerBounds.getX(),
                             (float)headerBounds.getRight());
    };

    drawSeparator(themeSectionHeader.getBounds());
    drawSeparator(sessionSectionHeader.getBounds());
    drawSeparator(setupSectionHeader.getBounds());
    drawSeparator(metronomeSectionHeader.getBounds());
    drawSeparator(clickSectionHeader.getBounds());
    drawSeparator(countInSectionHeader.getBounds());
    drawSeparator(syncSectionHeader.getBounds());
    drawSeparator(limitsSectionHeader.getBounds());
    drawSeparator(globalSectionHeader.getBounds());

    // ── SETUP descriptions ──────────────────────────────────────────────────
    {
        g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
        g.setColour(OrbitalLooperLookAndFeel::getTextColour());

        int stX = singleTrackButton.getBounds().getX();
        int stW = singleTrackButton.getBounds().getWidth();
        int stBottom = singleTrackButton.getBounds().getBottom();

        int mtX = multiTrackButton.getBounds().getX();
        int mtW = multiTrackButton.getBounds().getWidth();

        g.drawFittedText("Individual Loop w/ multiple layers.\nRecord and Overdub like a looper pedal",
                         stX, stBottom + 2, stW, 28, juce::Justification::centredLeft, 2);
        g.drawFittedText("Multiple Loops w/ multiple layers each.\nRecord and Overdub like a looper station",
                         mtX, stBottom + 2, mtW, 28, juce::Justification::centredLeft, 2);
    }

    // ── METRONOME column descriptions ────────────────────────────────────────
    {
        int descY = metronomeOnOffButton.getBounds().getY() - 18;
        g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
        g.setColour(OrbitalLooperLookAndFeel::getTextColour());

        int onOffX = metronomeOnOffButton.getBounds().getX();
        int onOffW = metronomeOnOffButton.getBounds().getWidth();
        int bpmX   = metronomeBPMLabel.getBounds().getX();
        int bpmW   = metronomeBPMLabel.getBounds().getWidth();
        int tsX    = timeSignatureCombo.getBounds().getX();
        int tsW    = timeSignatureCombo.getBounds().getWidth();

        g.drawText("ON/OFF",         onOffX, descY, onOffW, 16, juce::Justification::centred);
        g.drawText("BPM",            bpmX,   descY, bpmW,   16, juce::Justification::centred);
        g.drawText("Time Signature", tsX,    descY, tsW,    16, juce::Justification::centred);
    }

    // ── MIDI header bar (custom-painted) ─────────────────────────────────────
    {
        auto midiBarBounds = midiSectionHeader.getBounds();
        g.setColour(containerColour);
        g.fillRoundedRectangle(midiBarBounds.toFloat(), 4.0f);

        g.setColour(textColour);
        g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f).withStyle(juce::Font::bold));
        g.drawText("MIDI", midiBarBounds.getX() + 12, midiBarBounds.getY(),
                   80, midiBarBounds.getHeight(), juce::Justification::centredLeft);

        g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f));
        g.drawText(isMidiExpanded ? "\u25BC" : "\u25B6",
                   midiBarBounds.getRight() - 24, midiBarBounds.getY(),
                   20, midiBarBounds.getHeight(), juce::Justification::centred);
    }

    // ── MIDI sub-group headers + column descriptions (when expanded) ─────────
    if (isMidiExpanded)
    {
        auto drawSubGroup = [&](int rowIndex, const char* headerText)
        {
            if (rowIndex >= functionLabels.size()) return;

            int firstRowY = functionLabels[rowIndex]->getBounds().getY();

            int headerY = firstRowY - 38;
            int headerX = typeComboBoxes[rowIndex]->getBounds().getX();
            g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(13.0f).withStyle(juce::Font::bold));
            g.setColour(OrbitalLooperLookAndFeel::getTextColour());
            g.drawText(headerText, headerX, headerY, 120, 18, juce::Justification::centredLeft);

            int descY = firstRowY - 18;
            g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
            g.setColour(OrbitalLooperLookAndFeel::getTextColour());

            int typeX    = typeComboBoxes[rowIndex]->getBounds().getX();
            int typeW    = typeComboBoxes[rowIndex]->getBounds().getWidth();
            int chanX    = channelComboBoxes[rowIndex]->getBounds().getX();
            int chanW    = channelComboBoxes[rowIndex]->getBounds().getWidth();
            int dataX    = noteDataLabels[rowIndex]->getBounds().getX();
            int dataW    = noteDataLabels[rowIndex]->getBounds().getWidth();
            int learnX   = learnButtons[rowIndex]->getBounds().getX();
            int learnW   = learnButtons[rowIndex]->getBounds().getWidth();

            g.drawText("Type",    typeX,  descY, typeW,  16, juce::Justification::centred);
            g.drawText("Channel", chanX,  descY, chanW,  16, juce::Justification::centred);
            g.drawText("Value",   dataX,  descY, dataW,  16, juce::Justification::centred);
            g.drawText("Learn",   learnX, descY, learnW, 16, juce::Justification::centred);
        };

        drawSubGroup(FUNCTIONS_GROUP_START,   "Functions");
        drawSubGroup(LOOPS_GROUP_START,       "Loops");
        drawSubGroup(METRONOME_GROUP_START,   "Metronome");
        drawSubGroup(CLICK_TRACK_GROUP_START, "Click Track");
        drawSubGroup(COUNT_IN_GROUP_START,    "Count In");
    }

    // ── LIMITS "Unlimited" column description ────────────────────────────────
    {
        if (unlimitedLoopsButton.isVisible())
        {
            int descY = unlimitedLoopsButton.getBounds().getY() - 18;
            int ulX   = unlimitedLoopsButton.getBounds().getX();
            int ulW   = unlimitedLoopsButton.getBounds().getWidth();
            g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
            g.setColour(OrbitalLooperLookAndFeel::getTextColour());
            g.drawText("Unlimited", ulX, descY, ulW, 16, juce::Justification::centred);
        }
    }

    // ── Branding footer ─────────────────────────────────────────────────────
    g.setColour(OrbitalLooperLookAndFeel::getTextColour());
    g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(20.0f));
    g.drawText("Orbital Looper by Planet Pluto Effects", 0, h - 22, w, 18,
               juce::Justification::centred);
}

// ─── resized ──────────────────────────────────────────────────────────────────

void SettingsComponent::resized()
{
    const int margin = 20;
    auto bounds = getLocalBounds().reduced(margin);

    bounds.removeFromTop(36);  // Title

    // ═════════════════════════════════════════════════════════════════════════
    // THEME (v07.00)
    // ═════════════════════════════════════════════════════════════════════════
    themeSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto row = bounds.removeFromTop(30);
        int halfW = (row.getWidth() - 8) / 2;
        themeDarkButton.setBounds(row.removeFromLeft(halfW));
        row.removeFromLeft(8);
        themeLightButton.setBounds(row);
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // SESSION
    // ═════════════════════════════════════════════════════════════════════════
    sessionSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto btnRow = bounds.removeFromTop(28);
        int btnW = (btnRow.getWidth() - 8) / 2;
        saveSettingsButton.setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(8);
        loadSettingsButton.setBounds(btnRow);
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // SETUP
    // ═════════════════════════════════════════════════════════════════════════
    setupSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto row = bounds.removeFromTop(30);
        int halfW = (row.getWidth() - 8) / 2;
        singleTrackButton.setBounds(row.removeFromLeft(halfW));
        row.removeFromLeft(8);
        multiTrackButton.setBounds(row);
    }
    bounds.removeFromTop(32);  // space for description text (painted below buttons)
    bounds.removeFromTop(12);

    // ═════════════════════════════════════════════════════════════════════════
    // METRONOME
    // ═════════════════════════════════════════════════════════════════════════
    metronomeSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    bounds.removeFromTop(18);  // column descriptions
    {
        auto row = bounds.removeFromTop(30);
        metronomeOnOffButton.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(8);
        metronomeBPMLabel.setBounds(row.removeFromLeft(70));
        row.removeFromLeft(8);
        timeSignatureCombo.setBounds(row.removeFromLeft(110));
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // CLICK TRACK
    // ═════════════════════════════════════════════════════════════════════════
    clickSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto row = bounds.removeFromTop(28);
        clickOnOffButton.setBounds(row.removeFromLeft(60));
    }
    bounds.removeFromTop(6);
    {
        auto row = bounds.removeFromTop(24);
        clickVolLabel.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(4);
        clickVolSlider.setBounds(row);
    }
    bounds.removeFromTop(6);
    {
        auto row = bounds.removeFromTop(28);
        clickTypeCombo.setBounds(row.removeFromLeft(200));
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // COUNT IN
    // ═════════════════════════════════════════════════════════════════════════
    countInSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto row = bounds.removeFromTop(28);
        countInOnOffButton.setBounds(row.removeFromLeft(60));
        row.removeFromLeft(8);
        countInSecsLabel.setBounds(row.removeFromLeft(40));
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // SYNC MODE
    // ═════════════════════════════════════════════════════════════════════════
    syncSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto row = bounds.removeFromTop(28);
        syncOnOffButton.setBounds(row.removeFromLeft(60));
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // MIDI (collapsible header + rows)
    // ═════════════════════════════════════════════════════════════════════════
    midiSectionHeader.setBounds(bounds.removeFromTop(30));

    if (isMidiExpanded)
    {
        bounds.removeFromTop(6);

        for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
        {
            if (i == FUNCTIONS_GROUP_START || i == LOOPS_GROUP_START ||
                i == METRONOME_GROUP_START || i == CLICK_TRACK_GROUP_START ||
                i == COUNT_IN_GROUP_START)
            {
                bounds.removeFromTop(20);   // sub-header
                bounds.removeFromTop(18);   // column descriptions
            }

            auto row = bounds.removeFromTop(26);

            functionLabels[i]->setBounds(row.removeFromLeft(120));
            row.removeFromLeft(4);
            typeComboBoxes[i]->setBounds(row.removeFromLeft(72));
            row.removeFromLeft(4);
            channelComboBoxes[i]->setBounds(row.removeFromLeft(60));
            row.removeFromLeft(4);
            noteDataLabels[i]->setBounds(row.removeFromLeft(50));
            row.removeFromLeft(4);
            learnButtons[i]->setBounds(row.removeFromLeft(70));
        }

        bounds.removeFromTop(10);
    }

    bounds.removeFromTop(10);

    // ═════════════════════════════════════════════════════════════════════════
    // LIMITS
    // ═════════════════════════════════════════════════════════════════════════
    limitsSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    bounds.removeFromTop(18);  // "Unlimited" description
    {
        auto row = bounds.removeFromTop(28);
        maxLoopsLabel.setBounds(row.removeFromLeft(80));
        row.removeFromLeft(8);
        maxLoopsValueLabel.setBounds(row.removeFromLeft(50));
        row.removeFromLeft(8);
        unlimitedLoopsButton.setBounds(row.removeFromLeft(60));
    }
    bounds.removeFromTop(6);
    {
        auto row = bounds.removeFromTop(28);
        maxLayersLabel.setBounds(row.removeFromLeft(80));
        row.removeFromLeft(8);
        maxLayersValueLabel.setBounds(row.removeFromLeft(50));
        row.removeFromLeft(8);
        unlimitedLayersButton.setBounds(row.removeFromLeft(60));
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // GLOBAL DEFAULTS
    // ═════════════════════════════════════════════════════════════════════════
    globalSectionHeader.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(10);
    {
        auto btnRow = bounds.removeFromTop(28);
        int btnW = (btnRow.getWidth() - 8) / 2;
        setDefaultButton.setBounds(btnRow.removeFromLeft(btnW));
        btnRow.removeFromLeft(8);
        restoreDefaultButton.setBounds(btnRow);
    }
    bounds.removeFromTop(16);

    // ═════════════════════════════════════════════════════════════════════════
    // Bottom row: Reset to Defaults
    // ═════════════════════════════════════════════════════════════════════════
    {
        auto row = bounds.removeFromTop(30);
        resetToDefaultsButton.setBounds(row.removeFromLeft(140));
    }

    // Tell viewport how tall content actually is
    int contentHeight = getLocalBounds().getHeight() - bounds.getHeight() + 40;
    contentHeight = juce::jmax(contentHeight, getHeight());
    setSize(getWidth(), contentHeight);
}

// ─── helper implementations ──────────────────────────────────────────────────

void SettingsComponent::updateThemeButtons()
{
    const auto activeColour = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour  = OrbitalLooperLookAndFeel::getPanelColour();
    bool dark = audioProcessor.getDarkMode();
    themeDarkButton.setColour(juce::TextButton::buttonColourId,    dark ? activeColour : panelColour);
    themeDarkButton.setColour(juce::TextButton::buttonOnColourId,  dark ? activeColour : panelColour);
    themeLightButton.setColour(juce::TextButton::buttonColourId,   dark ? panelColour : activeColour);
    themeLightButton.setColour(juce::TextButton::buttonOnColourId, dark ? panelColour : activeColour);
    themeDarkButton.repaint();
    themeLightButton.repaint();
}

void SettingsComponent::refreshThemeColours()
{
    const auto tc = OrbitalLooperLookAndFeel::getTextColour();
    const auto pc = OrbitalLooperLookAndFeel::getPanelColour();
    const auto bc = OrbitalLooperLookAndFeel::getBorderColour();

    updateThemeButtons();

    // Section headers
    themeSectionHeader.setColour(juce::Label::textColourId, tc);
    sessionSectionHeader.setColour(juce::Label::textColourId, tc);
    setupSectionHeader.setColour(juce::Label::textColourId, tc);
    metronomeSectionHeader.setColour(juce::Label::textColourId, tc);
    clickSectionHeader.setColour(juce::Label::textColourId, tc);
    countInSectionHeader.setColour(juce::Label::textColourId, tc);
    syncSectionHeader.setColour(juce::Label::textColourId, tc);
    limitsSectionHeader.setColour(juce::Label::textColourId, tc);
    globalSectionHeader.setColour(juce::Label::textColourId, tc);

    // Buttons
    saveSettingsButton.setColour(juce::TextButton::buttonColourId, pc);
    saveSettingsButton.setColour(juce::TextButton::textColourOffId, tc);
    loadSettingsButton.setColour(juce::TextButton::buttonColourId, pc);
    loadSettingsButton.setColour(juce::TextButton::textColourOffId, tc);
    setDefaultButton.setColour(juce::TextButton::buttonColourId, pc);
    setDefaultButton.setColour(juce::TextButton::textColourOffId, tc);
    restoreDefaultButton.setColour(juce::TextButton::buttonColourId, pc);
    restoreDefaultButton.setColour(juce::TextButton::textColourOffId, tc);
    resetToDefaultsButton.setColour(juce::TextButton::buttonColourId, pc);
    resetToDefaultsButton.setColour(juce::TextButton::textColourOffId, tc);

    singleTrackButton.setColour(juce::TextButton::textColourOffId, tc);
    singleTrackButton.setColour(juce::TextButton::textColourOnId, tc);
    multiTrackButton.setColour(juce::TextButton::textColourOffId, tc);
    multiTrackButton.setColour(juce::TextButton::textColourOnId, tc);
    metronomeOnOffButton.setColour(juce::TextButton::textColourOffId, tc);
    metronomeOnOffButton.setColour(juce::TextButton::textColourOnId, tc);
    clickOnOffButton.setColour(juce::TextButton::textColourOffId, tc);
    clickOnOffButton.setColour(juce::TextButton::textColourOnId, tc);
    countInOnOffButton.setColour(juce::TextButton::textColourOffId, tc);
    countInOnOffButton.setColour(juce::TextButton::textColourOnId, tc);
    syncOnOffButton.setColour(juce::TextButton::textColourOffId, tc);
    syncOnOffButton.setColour(juce::TextButton::textColourOnId, tc);
    unlimitedLoopsButton.setColour(juce::TextButton::textColourOffId, tc);
    unlimitedLoopsButton.setColour(juce::TextButton::textColourOnId, tc);
    unlimitedLayersButton.setColour(juce::TextButton::textColourOffId, tc);
    unlimitedLayersButton.setColour(juce::TextButton::textColourOnId, tc);
    themeDarkButton.setColour(juce::TextButton::textColourOffId, tc);
    themeDarkButton.setColour(juce::TextButton::textColourOnId, tc);
    themeLightButton.setColour(juce::TextButton::textColourOffId, tc);
    themeLightButton.setColour(juce::TextButton::textColourOnId, tc);

    // Editable labels
    metronomeBPMLabel.setColour(juce::Label::textColourId, tc);
    metronomeBPMLabel.setColour(juce::Label::backgroundColourId, pc);
    metronomeBPMLabel.setColour(juce::Label::outlineColourId, bc);
    countInSecsLabel.setColour(juce::Label::textColourId, tc);
    countInSecsLabel.setColour(juce::Label::backgroundColourId, pc);
    countInSecsLabel.setColour(juce::Label::outlineColourId, bc);
    clickVolLabel.setColour(juce::Label::textColourId, tc);
    maxLoopsLabel.setColour(juce::Label::textColourId, tc);
    maxLayersLabel.setColour(juce::Label::textColourId, tc);
    maxLoopsValueLabel.setColour(juce::Label::textColourId, tc);
    maxLoopsValueLabel.setColour(juce::Label::backgroundColourId, pc);
    maxLoopsValueLabel.setColour(juce::Label::outlineColourId, bc);
    maxLayersValueLabel.setColour(juce::Label::textColourId, tc);
    maxLayersValueLabel.setColour(juce::Label::backgroundColourId, pc);
    maxLayersValueLabel.setColour(juce::Label::outlineColourId, bc);

    // Combos
    timeSignatureCombo.setColour(juce::ComboBox::backgroundColourId, pc);
    timeSignatureCombo.setColour(juce::ComboBox::textColourId, tc);
    timeSignatureCombo.setColour(juce::ComboBox::outlineColourId, bc);
    clickTypeCombo.setColour(juce::ComboBox::backgroundColourId, pc);
    clickTypeCombo.setColour(juce::ComboBox::textColourId, tc);
    clickTypeCombo.setColour(juce::ComboBox::outlineColourId, bc);

    // MIDI rows
    for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
    {
        functionLabels[i]->setColour(juce::Label::textColourId, tc);
        typeComboBoxes[i]->setColour(juce::ComboBox::backgroundColourId, pc);
        typeComboBoxes[i]->setColour(juce::ComboBox::textColourId, tc);
        typeComboBoxes[i]->setColour(juce::ComboBox::outlineColourId, bc);
        channelComboBoxes[i]->setColour(juce::ComboBox::backgroundColourId, pc);
        channelComboBoxes[i]->setColour(juce::ComboBox::textColourId, tc);
        channelComboBoxes[i]->setColour(juce::ComboBox::outlineColourId, bc);
        noteDataLabels[i]->setColour(juce::Label::textColourId, tc);
        noteDataLabels[i]->setColour(juce::Label::backgroundColourId, pc);
        noteDataLabels[i]->setColour(juce::Label::outlineColourId, bc);
        learnButtons[i]->setColour(juce::TextButton::textColourOffId, tc);
        learnButtons[i]->setColour(juce::TextButton::textColourOnId, tc);
        if (learningIndex != i)
            learnButtons[i]->setColour(juce::TextButton::buttonColourId, pc);
    }

    // Re-apply toggle button highlight colours
    updateSetupButtons();
    updateMetronomeButton();
    updateClickButton();
    updateCountInButton();
    updateSyncButton();
    updateUnlimitedLoopsButton();
    updateUnlimitedLayersButton();

    repaint();
}

void SettingsComponent::updateMetronomeButton()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool on = metronomeOnOffButton.getToggleState();
    metronomeOnOffButton.setButtonText(on ? "ON" : "OFF");
    metronomeOnOffButton.setColour(juce::TextButton::buttonColourId,   on ? blueColour : panelColour);
    metronomeOnOffButton.setColour(juce::TextButton::buttonOnColourId, on ? blueColour : panelColour);
    metronomeOnOffButton.repaint();
}

void SettingsComponent::updateClickButton()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool on = clickOnOffButton.getToggleState();
    clickOnOffButton.setButtonText(on ? "ON" : "OFF");
    clickOnOffButton.setColour(juce::TextButton::buttonColourId,   on ? blueColour : panelColour);
    clickOnOffButton.setColour(juce::TextButton::buttonOnColourId, on ? blueColour : panelColour);
    clickOnOffButton.repaint();
}

void SettingsComponent::updateCountInButton()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool on = countInOnOffButton.getToggleState();
    countInOnOffButton.setButtonText(on ? "ON" : "OFF");
    countInOnOffButton.setColour(juce::TextButton::buttonColourId,   on ? blueColour : panelColour);
    countInOnOffButton.setColour(juce::TextButton::buttonOnColourId, on ? blueColour : panelColour);
    countInOnOffButton.repaint();
}

void SettingsComponent::updateSetupButtons()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool single = audioProcessor.isSingleTrackMode();
    singleTrackButton.setColour(juce::TextButton::buttonColourId,   single ? blueColour : panelColour);
    singleTrackButton.setColour(juce::TextButton::buttonOnColourId, single ? blueColour : panelColour);
    multiTrackButton.setColour(juce::TextButton::buttonColourId,    single ? panelColour : blueColour);
    multiTrackButton.setColour(juce::TextButton::buttonOnColourId,  single ? panelColour : blueColour);
    singleTrackButton.repaint();
    multiTrackButton.repaint();
}

void SettingsComponent::updateSyncButton()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool on = syncOnOffButton.getToggleState();
    syncOnOffButton.setButtonText(on ? "ON" : "OFF");
    syncOnOffButton.setColour(juce::TextButton::buttonColourId,   on ? blueColour : panelColour);
    syncOnOffButton.setColour(juce::TextButton::buttonOnColourId, on ? blueColour : panelColour);
    syncOnOffButton.repaint();
}

void SettingsComponent::updateUnlimitedLoopsButton()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool on = unlimitedLoopsButton.getToggleState();
    unlimitedLoopsButton.setButtonText(on ? "ON" : "OFF");
    unlimitedLoopsButton.setColour(juce::TextButton::buttonColourId,   on ? blueColour : panelColour);
    unlimitedLoopsButton.setColour(juce::TextButton::buttonOnColourId, on ? blueColour : panelColour);
    unlimitedLoopsButton.repaint();
}

void SettingsComponent::updateUnlimitedLayersButton()
{
    const auto blueColour  = OrbitalLooperLookAndFeel::getPlayColour();
    const auto panelColour = OrbitalLooperLookAndFeel::getPanelColour();
    bool on = unlimitedLayersButton.getToggleState();
    unlimitedLayersButton.setButtonText(on ? "ON" : "OFF");
    unlimitedLayersButton.setColour(juce::TextButton::buttonColourId,   on ? blueColour : panelColour);
    unlimitedLayersButton.setColour(juce::TextButton::buttonOnColourId, on ? blueColour : panelColour);
    unlimitedLayersButton.repaint();
}

void SettingsComponent::syncTimeSigComboToMasterClock()
{
    int num = audioProcessor.getTimeSigNumerator();
    int den = audioProcessor.getTimeSigDenominator();

    if      (num == 4 && den == 4) timeSignatureCombo.setSelectedId(1, juce::dontSendNotification);
    else if (num == 3 && den == 4) timeSignatureCombo.setSelectedId(2, juce::dontSendNotification);
    else if (num == 2 && den == 4) timeSignatureCombo.setSelectedId(3, juce::dontSendNotification);
    else if (num == 6 && den == 8) timeSignatureCombo.setSelectedId(4, juce::dontSendNotification);
    else if (num == 7 && den == 8) timeSignatureCombo.setSelectedId(5, juce::dontSendNotification);
    else if (num == 5 && den == 4) timeSignatureCombo.setSelectedId(6, juce::dontSendNotification);
    else
    {
        customTimeSigNum = num;
        customTimeSigDen = den;
        timeSignatureCombo.setSelectedId(7, juce::dontSendNotification);
    }
}

void SettingsComponent::cancelLearn()
{
    if (learningIndex >= 0 && learningIndex < NUM_MIDI_FUNCTIONS)
    {
        learnButtons[learningIndex]->setButtonText("Learn");
        learnButtons[learningIndex]->setColour(juce::TextButton::buttonColourId, OrbitalLooperLookAndFeel::getPanelColour());
        learnButtons[learningIndex]->repaint();
    }
    learningIndex = -1;
    audioProcessor.cancelMidiLearn();
}

void SettingsComponent::setMidiRowsVisible(bool visible)
{
    for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
    {
        functionLabels[i]->setVisible(visible);
        typeComboBoxes[i]->setVisible(visible);
        channelComboBoxes[i]->setVisible(visible);
        noteDataLabels[i]->setVisible(visible);
        learnButtons[i]->setVisible(visible);
    }
}

void SettingsComponent::resizeParentWindow(int newHeight)
{
    if (auto* viewport = getParentComponent())
    {
        if (auto* docWin = dynamic_cast<juce::DocumentWindow*>(viewport->getParentComponent()))
        {
            int cappedHeight = juce::jmin(newHeight, 900);
            docWin->setSize(docWin->getWidth(), cappedHeight);
        }
    }
}

int SettingsComponent::computeExpandedHeight() const
{
    int h = 836;   // collapsed baseline (760 + 76 for THEME section)

    // MIDI expanded content:
    //   22 data rows     x 26 px = 572
    //    5 sub-headers   x 20 px = 100
    //    5 col-desc rows x 18 px =  90
    //    top gap after header    =   6
    //    bottom gap              =  10
    //                              ─────
    //    total extra             = 778 px
    h += 22 * 26 + 5 * 20 + 5 * 18 + 6 + 10;
    return h;
}

void SettingsComponent::refreshAllControls()
{
    // Theme
    updateThemeButtons();

    // Metronome
    metronomeBPMLabel.setText(juce::String(audioProcessor.getBPM()), juce::dontSendNotification);
    metronomeOnOffButton.setToggleState(audioProcessor.getMetronomeOn(), juce::dontSendNotification);
    updateMetronomeButton();
    syncTimeSigComboToMasterClock();

    // Click track
    clickOnOffButton.setToggleState(audioProcessor.getClickTrackOn(), juce::dontSendNotification);
    updateClickButton();
    clickVolSlider.setValue(audioProcessor.getClickVolume(), juce::dontSendNotification);
    if (audioProcessor.getClickFileName().isNotEmpty())
        clickTypeCombo.setText(audioProcessor.getClickFileName(), juce::dontSendNotification);
    else
        clickTypeCombo.setSelectedId(1, juce::dontSendNotification);

    // Count in
    countInOnOffButton.setToggleState(audioProcessor.getCountInEnabled(), juce::dontSendNotification);
    updateCountInButton();
    countInSecsLabel.setText(juce::String(audioProcessor.getCountInSeconds(), 1), juce::dontSendNotification);

    // Setup
    updateSetupButtons();

    // Sync mode
    syncOnOffButton.setToggleState(audioProcessor.getSyncMode() == LoopSyncMode::Sync, juce::dontSendNotification);
    updateSyncButton();

    // Limits
    maxLoopsValueLabel.setText(juce::String(audioProcessor.getMaxLoops()), juce::dontSendNotification);
    unlimitedLoopsButton.setToggleState(audioProcessor.getUnlimitedLoops(), juce::dontSendNotification);
    maxLoopsValueLabel.setEnabled(!audioProcessor.getUnlimitedLoops());
    updateUnlimitedLoopsButton();
    maxLayersValueLabel.setText(juce::String(audioProcessor.getMaxLayers()), juce::dontSendNotification);
    unlimitedLayersButton.setToggleState(audioProcessor.getUnlimitedLayers(), juce::dontSendNotification);
    maxLayersValueLabel.setEnabled(!audioProcessor.getUnlimitedLayers());
    updateUnlimitedLayersButton();

    // MIDI
    auto mapping = audioProcessor.getMidiMapping();
    for (int i = 0; i < NUM_MIDI_FUNCTIONS; ++i)
    {
        {
            int val = getNoteForIndex(mapping, i);
            noteDataLabels[i]->setText(val < 0 ? juce::String(juce::CharPointer_UTF8("\xe2\x80\x94")) : juce::String(val), juce::dontSendNotification);
        }
        channelComboBoxes[i]->setSelectedId(mapping.channel, juce::dontSendNotification);
    }
}

//==============================================================================
// SettingsViewport
//==============================================================================
SettingsViewport::SettingsViewport(OrbitalLooperAudioProcessor& processor)
{
    content = std::make_unique<SettingsComponent>(processor);
    viewport.setViewedComponent(content.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);
}

void SettingsViewport::resized()
{
    viewport.setBounds(getLocalBounds());
    content->setSize(viewport.getMaximumVisibleWidth(), content->getHeight());
}

//==============================================================================
// SettingsDialog
//==============================================================================
SettingsDialog::SettingsDialog(OrbitalLooperAudioProcessor& processor)
    : DocumentWindow("Settings",
                     OrbitalLooperLookAndFeel::getBackgroundColour(),
                     DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new SettingsViewport(processor), true);
    setSize(560, 836);
    centreWithSize(560, 836);
    setVisible(true);
    setResizable(true, true);
}
