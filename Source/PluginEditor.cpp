/*
  ==============================================================================
    PluginEditor.cpp

    Orbital Looper v03.00.01 - METRONOME: BPM

    Multi-loop UI. LoopCardComponent is a self-contained JUCE Component
    that draws one loop's card and owns its own 30 Hz timer.
    Each card has a ✕ delete button. Deleting the last loop resets it.
    LOOP ALL broadcasts every function button to all loops simultaneously.
    VARIABLE LOOP SELECTION: Cmd+click toggles, Shift+click range-selects.
    Function buttons target all selected loops via getTargetLoops().
    The main editor hosts a Viewport containing all cards stacked vertically.
    An [ADD LOOP] button sits below the list.

    Planet Pluto Effects
  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>   // std::ceil for count-in countdown display

//==============================================================================
//  LOOP CARD COMPONENT
//==============================================================================

LoopCardComponent::LoopCardComponent(OrbitalLooperAudioProcessor& proc, int idx)
    : audioProcessor(proc), loopIndex(idx)
{
    auto& engine = audioProcessor.getLoopEngine(loopIndex);

    //--- Loop name label ---
    loopNameLabel.setText("Loop " + juce::String(loopIndex + 1), juce::dontSendNotification);
    loopNameLabel.setEditable(true);
    loopNameLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(18.0f));
    loopNameLabel.setJustificationType(juce::Justification::centredLeft);
    loopNameLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    addAndMakeVisible(loopNameLabel);

    //--- State label ---
    stateLabel.setText("", juce::dontSendNotification);
    stateLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(12.0f));
    stateLabel.setJustificationType(juce::Justification::centredRight);
    stateLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    addAndMakeVisible(stateLabel);

    //--- Multiplier label (v05.01) ---
    multiplierLabel.setText("", juce::dontSendNotification);
    multiplierLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    multiplierLabel.setJustificationType(juce::Justification::centredLeft);
    multiplierLabel.setBorderSize(juce::BorderSize<int>(0));
    multiplierLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    addAndMakeVisible(multiplierLabel);

    //--- VOL ---
    volLabel.setText("VOL", juce::dontSendNotification);
    volLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    volLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    volLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(volLabel);

    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setRange(0.0, 100.0, 1.0);
    volumeSlider.setValue(engine.getVolume() * 100.0);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setDoubleClickReturnValue(true, 100.0);
    volumeSlider.onValueChange = [this]
    {
        float v = static_cast<float>(volumeSlider.getValue()) / 100.0f;
        audioProcessor.getLoopEngine(loopIndex).setVolume(v);
        volValueLabel.setText(juce::String(static_cast<int>(volumeSlider.getValue())),
                              juce::dontSendNotification);
    };
    addAndMakeVisible(volumeSlider);

    volValueLabel.setText("100", juce::dontSendNotification);
    volValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    volValueLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    volValueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(volValueLabel);

    //--- PAN ---
    panLabel.setText("PAN", juce::dontSendNotification);
    panLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    panLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    panLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(panLabel);

    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setRange(-50.0, 50.0, 1.0);
    panSlider.setValue(engine.getPan() * 50.0);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setDoubleClickReturnValue(true, 0.0);
    panSlider.onValueChange = [this]
    {
        float v = static_cast<float>(panSlider.getValue()) / 50.0f;
        audioProcessor.getLoopEngine(loopIndex).setPan(v);
        int val = static_cast<int>(panSlider.getValue());
        if      (val == 0) panValueLabel.setText("C",  juce::dontSendNotification);
        else if (val <  0) panValueLabel.setText("L" + juce::String(std::abs(val)),
                                                  juce::dontSendNotification);
        else               panValueLabel.setText("R" + juce::String(val),
                                                  juce::dontSendNotification);
    };
    addAndMakeVisible(panSlider);

    panValueLabel.setText("C", juce::dontSendNotification);
    panValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    panValueLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    panValueLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(panValueLabel);

    //--- Delete button (✕) — right edge of Row 1, always visible ---
    // 1 loop:  acts like CLEAR (resets the loop)
    // >1 loops: deletes this loop card
    deleteButton.setButtonText(juce::CharPointer_UTF8("\xE2\x9C\x95"));  // ✕
    deleteButton.setTooltip("Delete Loop");
    deleteButton.onClick = [this]
    {
        if (onDelete)
            onDelete(loopIndex);
    };
    addAndMakeVisible(deleteButton);

    //--- MUTE button (M) — far left of Row 1, visible only when >1 loop ---
    muteButton.setButtonText("M");
    muteButton.setTooltip("Mute Loop");
    muteButton.setClickingTogglesState(true);
    muteButton.onClick = [this]
    {
        audioProcessor.getLoopEngine(loopIndex).setMute(muteButton.getToggleState());
        audioProcessor.updateSoloState();
        updateMuteSoloColours();
    };
    addAndMakeVisible(muteButton);

    //--- SOLO button (S) — right of M, visible only when >1 loop ---
    soloButton.setButtonText("S");
    soloButton.setTooltip("Solo Loop");
    soloButton.setClickingTogglesState(true);
    soloButton.onClick = [this]
    {
        audioProcessor.getLoopEngine(loopIndex).setSolo(soloButton.getToggleState());
        audioProcessor.updateSoloState();
        updateMuteSoloColours();
    };
    addAndMakeVisible(soloButton);

    // Initialise M/S colours (both off by default)
    updateMuteSoloColours();

    startTimerHz(30);
}

LoopCardComponent::~LoopCardComponent()
{
    stopTimer();
}

//------------------------------------------------------------------------------
void LoopCardComponent::setSelected(bool selected)
{
    isSelected = selected;
    repaint();
}

//------------------------------------------------------------------------------
void LoopCardComponent::syncMuteSoloState()
{
    const bool m = audioProcessor.getLoopEngine(loopIndex).isMute();
    const bool s = audioProcessor.getLoopEngine(loopIndex).isSolo();
    cachedMute = m;
    cachedSolo = s;
    muteButton.setToggleState(m, juce::dontSendNotification);
    soloButton.setToggleState(s, juce::dontSendNotification);
    updateMuteSoloColours();
}

void LoopCardComponent::updateMuteSoloColours()
{
    const bool muted  = muteButton.getToggleState();
    const bool soloed = soloButton.getToggleState();

    // MUTE: amber when on, grey text when off
    muteButton.setColour(juce::TextButton::buttonColourId,
        muted ? juce::Colour(0xFFFF8C00) : OrbitalLooperLookAndFeel::getPanelColour());
    muteButton.setColour(juce::TextButton::textColourOffId,
        muted ? OrbitalLooperLookAndFeel::getTextColour() : OrbitalLooperLookAndFeel::getSecondaryTextColour());
    muteButton.setColour(juce::TextButton::textColourOnId, OrbitalLooperLookAndFeel::getTextColour());

    // SOLO: emerald green when on, grey text when off
    soloButton.setColour(juce::TextButton::buttonColourId,
        soloed ? OrbitalLooperLookAndFeel::getLoopAllColour()  // 0xFF10B981
               : OrbitalLooperLookAndFeel::getPanelColour());
    soloButton.setColour(juce::TextButton::textColourOffId,
        soloed ? OrbitalLooperLookAndFeel::getTextColour() : OrbitalLooperLookAndFeel::getSecondaryTextColour());
    soloButton.setColour(juce::TextButton::textColourOnId, OrbitalLooperLookAndFeel::getTextColour());

    muteButton.repaint();
    soloButton.repaint();
}

//------------------------------------------------------------------------------
void LoopCardComponent::refreshThemeColours()
{
    const auto tc = OrbitalLooperLookAndFeel::getTextColour();
    loopNameLabel.setColour(juce::Label::textColourId, tc);
    volLabel.setColour(juce::Label::textColourId, tc);
    volValueLabel.setColour(juce::Label::textColourId, tc);
    panLabel.setColour(juce::Label::textColourId, tc);
    panValueLabel.setColour(juce::Label::textColourId, tc);
    multiplierLabel.setColour(juce::Label::textColourId, tc);
    updateMuteSoloColours();
}

//------------------------------------------------------------------------------
void LoopCardComponent::mouseDown(const juce::MouseEvent& e)
{
    if (onSelected)
        onSelected(loopIndex, e.mods.isCommandDown(), e.mods.isShiftDown());
}

//------------------------------------------------------------------------------
void LoopCardComponent::timerCallback()
{
    auto& engine = audioProcessor.getLoopEngine(loopIndex);

    // Progress
    int loopLen = engine.getLoopLengthSamples();
    loopProgress = (loopLen > 0)
                   ? juce::jlimit(0.0f, 1.0f, (float)engine.getCurrentPosition() / (float)loopLen)
                   : 0.0f;

    // Layer count
    loopLayerCount = engine.getOverdubCount();

    // Time strings
    auto formatTime = [](double secs) -> juce::String {
        int    minutes = static_cast<int>(secs) / 60;
        double s       = secs - minutes * 60.0;
        return juce::String::formatted("%02d:%04.1f", minutes, s);
    };
    currentTime = formatTime(engine.getCurrentPositionSeconds());
    totalTime   = formatTime(engine.getLoopLengthSeconds());

    // State label
    auto state = engine.getState();
    juce::String stateText;
    juce::Colour stateColour = OrbitalLooperLookAndFeel::getSecondaryTextColour();

    switch (state)
    {
        case LoopState::RecordArmed:
            stateText   = "ARMED";
            stateColour = OrbitalLooperLookAndFeel::getRecordColour();
            break;
        case LoopState::Recording:
            stateText   = "RECORDING";
            stateColour = OrbitalLooperLookAndFeel::getRecordColour();
            break;
        case LoopState::Overdubbing:
            stateText   = "OVERDUBBING";
            stateColour = OrbitalLooperLookAndFeel::getOverdubColour();
            break;
        case LoopState::Playing:
            stateText   = "PLAYING";
            stateColour = OrbitalLooperLookAndFeel::getPlayColour();
            break;
        case LoopState::Paused:
            stateText   = "PAUSED";
            stateColour = OrbitalLooperLookAndFeel::getPauseColour();
            break;
        default:
            stateText   = "";
            stateColour = OrbitalLooperLookAndFeel::getSecondaryTextColour();
            break;
    }

    stateLabel.setText(stateText, juce::dontSendNotification);
    stateLabel.setColour(juce::Label::textColourId, stateColour);

    // v05.01 — Multiplier label (sync relationship display)
    {
        auto& mc = audioProcessor.getMasterClock();
        int numLoops = audioProcessor.getNumLoops();
        if (numLoops <= 1)
        {
            multiplierLabel.setText("", juce::dontSendNotification);
        }
        else if (audioProcessor.getMasterLoopIndex() == loopIndex)
        {
            multiplierLabel.setText("Master", juce::dontSendNotification);
            multiplierLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getPlayColour());
        }
        else if (mc.hasMasterCycle() && engine.getLoopLengthSamples() > 0)
        {
            auto mult = engine.getMultiplier();
            multiplierLabel.setText(mc.getMultiplierString(mult), juce::dontSendNotification);
            multiplierLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
        }
        else
        {
            multiplierLabel.setText("", juce::dontSendNotification);
        }
    }

    // Sync mute/solo toggle states (v02.05.01/02) — handles MIDI-triggered changes
    const bool newMute = audioProcessor.getLoopEngine(loopIndex).isMute();
    const bool newSolo = audioProcessor.getLoopEngine(loopIndex).isSolo();
    if (newMute != cachedMute || newSolo != cachedSolo)
    {
        cachedMute = newMute;
        cachedSolo = newSolo;
        muteButton.setToggleState(newMute, juce::dontSendNotification);
        soloButton.setToggleState(newSolo, juce::dontSendNotification);
        updateMuteSoloColours();
    }

    repaint();
}

//------------------------------------------------------------------------------
void LoopCardComponent::paint(juce::Graphics& g)
{
    auto cardF = getLocalBounds().toFloat();

    // Background fill
    g.setColour(OrbitalLooperLookAndFeel::getPanelColour());
    g.fillRoundedRectangle(cardF, OrbitalLooperLookAndFeel::CORNER_SIZE);

    // Border — drawn when multiple loops, or always in light mode
    const bool multiLoop = audioProcessor.getNumLoops() > 1;
    if (isSelected && multiLoop)
    {
        g.setColour(OrbitalLooperLookAndFeel::getPlayColour());
        g.drawRoundedRectangle(cardF.reduced(0.5f), OrbitalLooperLookAndFeel::CORNER_SIZE, 2.0f);
    }
    else if (multiLoop || !OrbitalLooperLookAndFeel::isDark())
    {
        g.setColour(OrbitalLooperLookAndFeel::getBorderColour());
        g.drawRoundedRectangle(cardF, OrbitalLooperLookAndFeel::CORNER_SIZE, 1.0f);
    }

    const int innerX = sc(CARD_PADDING_H);
    const int innerW = getWidth() - 2 * sc(CARD_PADDING_H);
    const int row2Top = sc(CARD_PADDING_V) + sc(CARD_ROW1_H) + sc(CARD_INNER_GAP);

    //--- Time counter (Row 2) ---
    g.setColour(OrbitalLooperLookAndFeel::getTextColour());
    g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(20.0f)));
    auto timeRow = juce::Rectangle<int>(innerX, row2Top, innerW, sc(CARD_ROW2_H));
    g.drawText(currentTime, timeRow, juce::Justification::centredLeft,  false);
    g.drawText(totalTime,   timeRow, juce::Justification::centredRight, false);

    //--- Progress bar (Row 3) ---
    const int row3Top = row2Top + sc(CARD_ROW2_H) + sc(CARD_INNER_GAP);
    auto barBounds = juce::Rectangle<float>((float)innerX, (float)row3Top,
                                             (float)innerW, (float)sc(CARD_ROW3_H));

    g.setColour(OrbitalLooperLookAndFeel::getBackgroundColour());
    g.fillRoundedRectangle(barBounds, scf(4.0f));

    if (loopProgress > 0.0f)
    {
        float fillW = barBounds.getWidth() * loopProgress;
        auto  fill  = juce::Rectangle<float>(barBounds.getX(), barBounds.getY(),
                                              fillW, barBounds.getHeight());
        g.setColour(OrbitalLooperLookAndFeel::getProgressGreen());
        g.fillRoundedRectangle(fill, scf(4.0f));

        float edgeX = fill.getRight();
        g.setColour(OrbitalLooperLookAndFeel::getTextColour());
        g.fillRect(edgeX - 2.0f, barBounds.getY(), 4.0f, barBounds.getHeight());
    }

    g.setColour(OrbitalLooperLookAndFeel::getBorderColour());
    g.drawRoundedRectangle(barBounds, scf(4.0f), 1.0f);

    //--- Layer indicator (right side of Row 1) — dots + text within LAYER_DOTS_WIDTH ---
    const int row1Top  = sc(CARD_PADDING_V);
    const int dotsAreaX = getWidth() - sc(CARD_PADDING_H) - sc(DELETE_BTN_SIZE) - sc(CONTROL_GAP) - sc(LAYER_DOTS_WIDTH);
    const int dotsAreaW = sc(LAYER_DOTS_WIDTH);

    const float dotSize    = scf(6.0f);
    const float dotSpacing = scf(3.0f);
    const int   maxDots    = 5;
    int dotsToShow = juce::jmin(loopLayerCount, maxDots);

    // Layer count text — right-aligned within the area
    g.setColour(OrbitalLooperLookAndFeel::getSecondaryTextColour());
    g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(11.0f)));
    juce::String layerText = juce::String(loopLayerCount)
                             + (loopLayerCount == 1 ? " Layer" : " Layers");
    auto textArea = juce::Rectangle<int>(dotsAreaX, row1Top, dotsAreaW, sc(CARD_ROW1_H));
    g.drawText(layerText, textArea, juce::Justification::centredRight, false);

    // Dots — drawn within the area, left-aligned (no leftward overflow)
    if (dotsToShow > 0)
    {
        float dotsStartX = (float)dotsAreaX + 2.0f;
        float dotY       = (float)row1Top + ((float)sc(CARD_ROW1_H) - dotSize) * 0.5f;
        g.setColour(OrbitalLooperLookAndFeel::getPlayColour());
        for (int i = 0; i < dotsToShow; ++i)
            g.fillEllipse(dotsStartX + i * (dotSize + dotSpacing), dotY, dotSize, dotSize);
    }
}

//------------------------------------------------------------------------------
void LoopCardComponent::resized()
{
    // v06.00: Compute card scale from width (base section width = 738)
    cardScale = (float)getWidth() / 738.0f;

    const bool multiLoop = audioProcessor.getNumLoops() > 1;
    const int  btnRowY   = sc(CARD_PADDING_V) + (sc(CARD_ROW1_H) - sc(MUTE_SOLO_BTN_SIZE)) / 2;

    // M / S buttons — far left of Row 1, only visible when >1 loop
    int muteSoloReserved = 0;
    if (multiLoop)
    {
        muteButton.setBounds(sc(CARD_PADDING_H),
                             btnRowY,
                             sc(MUTE_SOLO_BTN_SIZE), sc(MUTE_SOLO_BTN_SIZE));
        soloButton.setBounds(sc(CARD_PADDING_H) + sc(MUTE_SOLO_BTN_SIZE) + sc(MUTE_SOLO_GAP),
                             btnRowY,
                             sc(MUTE_SOLO_BTN_SIZE), sc(MUTE_SOLO_BTN_SIZE));
        muteButton.setVisible(true);
        soloButton.setVisible(true);
        muteSoloReserved = sc(MUTE_SOLO_BTN_SIZE) * 2 + sc(MUTE_SOLO_GAP) + sc(MUTE_SOLO_AFTER_GAP);
    }
    else
    {
        muteButton.setVisible(false);
        soloButton.setVisible(false);
    }

    // Delete button
    {
        const int btnX = getWidth() - sc(CARD_PADDING_H) - sc(DELETE_BTN_SIZE);
        const int btnY = sc(CARD_PADDING_V) + (sc(CARD_ROW1_H) - sc(DELETE_BTN_SIZE)) / 2;
        deleteButton.setBounds(btnX, btnY, sc(DELETE_BTN_SIZE), sc(DELETE_BTN_SIZE));
        deleteButton.setVisible(true);
    }

    // Row 1 controls
    const int rightReserved = sc(DELETE_BTN_SIZE) + sc(CONTROL_GAP) + sc(LAYER_DOTS_WIDTH);
    auto row1 = juce::Rectangle<int>(sc(CARD_PADDING_H) + muteSoloReserved,
                                      sc(CARD_PADDING_V),
                                      getWidth() - 2 * sc(CARD_PADDING_H) - muteSoloReserved - rightReserved,
                                      sc(CARD_ROW1_H));

    stateLabel.setBounds(row1.removeFromRight(sc(STATE_LABEL_WIDTH)));

    loopNameLabel.setBounds(row1.removeFromLeft(sc(90)));
    multiplierLabel.setBounds(row1.removeFromLeft(sc(40)));
    row1.removeFromLeft(sc(CONTROL_GAP));

    volLabel.setBounds     (row1.removeFromLeft(sc(VOL_LABEL_WIDTH)));
    volValueLabel.setBounds(row1.removeFromLeft(sc(VOL_VALUE_WIDTH)));
    volumeSlider.setBounds (row1.removeFromLeft(sc(SLIDER_VOL_WIDTH)));
    row1.removeFromLeft(sc(CONTROL_GAP));

    panLabel.setBounds     (row1.removeFromLeft(sc(VOL_LABEL_WIDTH)));
    panValueLabel.setBounds(row1.removeFromLeft(sc(VOL_VALUE_WIDTH)));
    panSlider.setBounds    (row1.removeFromLeft(sc(SLIDER_PAN_WIDTH)));

    // Update fonts for current scale
    loopNameLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(18.0f)));
    stateLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(12.0f)));
    multiplierLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(10.0f)));
    volLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(10.0f)));
    volValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(10.0f)));
    panLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(10.0f)));
    panValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(scf(10.0f)));
}


//==============================================================================
//  MAIN EDITOR
//==============================================================================

OrbitalLooperAudioProcessorEditor::OrbitalLooperAudioProcessorEditor(
    OrbitalLooperAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    setResizable(true, true);
    setResizeLimits(600, 300, 1536, 1200);
    setSize(BASE_WIDTH, 390);  // temporary height; resized() will snap it

    // v06.00: Restore saved width
    int savedW = audioProcessor.getEditorWidth();
    if (savedW > 0 && savedW != BASE_WIDTH)
        setSize(savedW, 390);

    //==========================================================================
    // TOOLBAR — version label
    //==========================================================================
    versionLabel.setText("Orbital Looper 1.0.0", juce::dontSendNotification);
    versionLabel.setFont(OrbitalLooperLookAndFeel::getLabelFont());
    versionLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    versionLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(versionLabel);

    //==========================================================================
    // TOOLBAR — THEME toggle (v07.00)
    //==========================================================================
    // Apply initial theme from processor
    customLookAndFeel.setDarkMode(audioProcessor.getDarkMode());

    themeToggleButton.setButtonText(audioProcessor.getDarkMode() ? "DARK" : "LIGHT");
    themeToggleButton.setTooltip("Toggle Light/Dark theme");
    themeToggleButton.setColour(juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    themeToggleButton.setColour(juce::TextButton::textColourOffId,  OrbitalLooperLookAndFeel::getSecondaryTextColour());
    themeToggleButton.onClick = [this]
    {
        const bool newDark = !audioProcessor.getDarkMode();
        audioProcessor.setDarkMode(newDark);
    };
    addAndMakeVisible(themeToggleButton);

    // Theme change callback — refreshes entire UI
    audioProcessor.onThemeChanged = [this]
    {
        const bool dark = audioProcessor.getDarkMode();
        customLookAndFeel.setDarkMode(dark);
        themeToggleButton.setButtonText(dark ? "DARK" : "LIGHT");

        // Refresh all component colours
        const auto tc = OrbitalLooperLookAndFeel::getTextColour();
        const auto sc = OrbitalLooperLookAndFeel::getSecondaryTextColour();

        versionLabel.setColour(juce::Label::textColourId, tc);
        metronomeLabel.setColour(juce::Label::textColourId, tc);
        bpmValueLabel.setColour(juce::Label::textColourId, tc);
        bpmUnitLabel.setColour(juce::Label::textColourId, tc);
        clickVolLabel.setColour(juce::Label::textColourId, tc);
        statusLabel.setColour(juce::Label::textColourId, tc);
        countInCountdownLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        themeToggleButton.setColour(juce::TextButton::textColourOffId, sc);
        timeSigCombo.setColour(juce::ComboBox::arrowColourId, tc.withAlpha(0.85f));

        // Refresh soundCombo colours
        soundCombo.setColour(juce::ComboBox::textColourId, tc);
        soundCombo.setColour(juce::ComboBox::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
        soundCombo.setColour(juce::ComboBox::outlineColourId, OrbitalLooperLookAndFeel::getBorderColour());

        for (auto* card : loopCards)
        {
            card->refreshThemeColours();
            card->repaint();
        }

        repaint();
    };

    //==========================================================================
    // TOOLBAR — SAVE button
    //==========================================================================
    saveButton.setButtonText("SAVE");
    saveButton.setTooltip("Save");
    saveButton.onClick = [this]
    {
        const int numLoops = audioProcessor.getNumLoops();

        if (numLoops > 1)
        {
            // v02.07.01: Multi-loop save — show choice dialog
            showSaveChoiceDialog();
        }
        else
        {
            // Single loop — save directly (original behaviour)
            showSaveChooser(SaveMode::All);
        }
    };
    addAndMakeVisible(saveButton);

    //==========================================================================
    // TOOLBAR — LOAD button
    //==========================================================================
    loadButton.setButtonText("LOAD");
    loadButton.setTooltip("Load");
    loadButton.onClick = [this]
    {
        bool hasAnyRecording = false;
        for (int i = 0; i < audioProcessor.getNumLoops(); ++i)
            if (audioProcessor.getLoopEngine(i).hasRecording()) { hasAnyRecording = true; break; }

        if (hasAnyRecording)
        {
            juce::AlertWindow::showOkCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Load Session",
                "Loading a session will replace all current loops. Continue?",
                "Load", "Cancel", nullptr,
                juce::ModalCallbackFunction::create([this](int result)
                {
                    if (result == 1) showLoadChooser();
                }));
        }
        else
        {
            showLoadChooser();
        }
    };
    addAndMakeVisible(loadButton);

    //==========================================================================
    // TOOLBAR — SETTINGS button (v05.00: enabled)
    //==========================================================================
    settingsButton.setButtonText("SETTINGS");
    settingsButton.setTooltip("Settings");
    settingsButton.onClick = [this] { openSettingsWindow(); };
    addAndMakeVisible(settingsButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 1: RECORD/OVERDUB
    //==========================================================================
    recordButton.setButtonText("REC/\nDUB");
    recordButton.onStateChange = [this]
    {
        if (!recordButton.isDown()) return;

        // v04.00 — if count-in is enabled and any target loop would start a fresh
        // recording (Stopped → Recording), trigger the count-in first.
        if (audioProcessor.getCountInEnabled())
        {
            bool anyStarting = false;
            for (int i : getTargetLoops())
                if (audioProcessor.getLoopEngine(i).getState() == LoopState::Stopped)
                    { anyStarting = true; break; }
            if (anyStarting)
            {
                audioProcessor.startCountIn(1);  // 1 = start recording after count-in
                updateButtonStates();
                return;
            }
        }

        bool startedRecording = false;
        auto& mc = audioProcessor.getMasterClock();
        for (int i : getTargetLoops())
        {
            auto& eng = audioProcessor.getLoopEngine(i);
            auto s = eng.getState();
            if (s == LoopState::RecordArmed)
            {
                eng.cancelArm();
            }
            else if (s == LoopState::Recording)
            {
                eng.stopRecording();
                // Establish master cycle or quantize
                if (!mc.hasMasterCycle())
                {
                    int loopLen = eng.getLoopLengthSamples();
                    if (loopLen > 0)
                    {
                        mc.setMasterCycleLength(loopLen);
                        audioProcessor.setMasterLoopIndex(i);
                        eng.setMultiplier(MasterClock::LoopMultiplier::One);
                    }
                }
                else if (eng.getSyncMode() == LoopSyncMode::Sync)
                {
                    int recordedLen = eng.getLoopLengthSamples();
                    auto mult = mc.findClosestMultiplier(recordedLen);
                    int quantizedLen = mc.getLengthForMultiplier(mult);
                    if (quantizedLen > 0 && quantizedLen != recordedLen)
                        eng.setLoopLength(quantizedLen);
                    eng.setMultiplier(mult);
                }
            }
            else if (s == LoopState::Overdubbing)
            {
                eng.stopOverdub();
            }
            else if ((s == LoopState::Playing || s == LoopState::Paused)
                     && audioProcessor.canAddLayer(eng))
            {
                eng.startOverdub();
            }
            else if (s == LoopState::Stopped)
            {
                // v05.01 — sync-aware recording
                if (!mc.hasMasterCycle())
                {
                    eng.setSyncMode(LoopSyncMode::Free);
                    eng.startRecording();
                }
                else if (audioProcessor.getSyncMode() == LoopSyncMode::Sync)
                {
                    eng.setSyncMode(LoopSyncMode::Sync);
                    eng.armRecord();
                }
                else
                {
                    eng.setSyncMode(LoopSyncMode::Free);
                    eng.startRecording();
                }
                startedRecording = true;
            }
        }
        // Beat viz: start from beat 0 whenever a fresh recording begins
        if (startedRecording && beatVisualization != nullptr)
        {
            audioProcessor.setBeatCounterActive(true);
            audioProcessor.resetBeatIndex();
            beatVisualization->startFromBeginning();
        }
        updateButtonStates();
    };
    addAndMakeVisible(recordButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 1: MULTIPLY
    //==========================================================================
    multiplyButton.setButtonText("MULTIPLY\n");
    multiplyButton.onClick = [this]
    {
        for (int i : getTargetLoops())
        {
            auto& eng = audioProcessor.getLoopEngine(i);
            if (eng.canMultiply()) eng.multiply();
        }

        auto original = multiplyButton.findColour(juce::TextButton::buttonColourId);
        multiplyButton.setColour(juce::TextButton::buttonColourId,
                                 OrbitalLooperLookAndFeel::getMultiplyColour());
        multiplyButton.repaint();
        juce::Timer::callAfterDelay(500, [this, original]()
        {
            multiplyButton.setColour(juce::TextButton::buttonColourId, original);
            multiplyButton.repaint();
        });
        updateButtonStates();
    };
    addAndMakeVisible(multiplyButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 2: PLAY/PAUSE
    //==========================================================================
    playButton.setButtonText("PLAY/\nPAUSE");
    playButton.setClickingTogglesState(true);
    playButton.onClick = [this]
    {
        auto targets = getTargetLoops();

        // Check if any loop is currently playing
        bool anyLoopPlaying = false;
        for (int i : targets)
            if (audioProcessor.getLoopEngine(i).getState() == LoopState::Playing)
                { anyLoopPlaying = true; break; }

        // Also treat as "playing" if the beat counter is running (metronome-only mode)
        const bool shouldPause = anyLoopPlaying
                                 || (audioProcessor.getMetronomeOn()
                                     && audioProcessor.getBeatCounterActive());

        // v04.00 — if count-in is enabled and we are STARTING (not pausing/resuming from Pause),
        // trigger a 1-bar count-in first. Resuming from Pause always fires immediately.
        if (!shouldPause && audioProcessor.getCountInEnabled())
        {
            bool anyPaused = false;
            for (int i : targets)
                if (audioProcessor.getLoopEngine(i).getState() == LoopState::Paused)
                    { anyPaused = true; break; }
            if (!anyPaused)
            {
                audioProcessor.startCountIn(2);  // 2 = start playing after count-in
                updateButtonStates();
                return;
            }
        }

        for (int i : targets)
        {
            auto& eng = audioProcessor.getLoopEngine(i);
            if (shouldPause) eng.pausePlayback();
            else             eng.startPlayback();
        }

        // Gate the audio-thread beat accumulator
        audioProcessor.setBeatCounterActive(!shouldPause);

        // Beat viz: mirror the play/pause state
        if (beatVisualization != nullptr)
        {
            if (shouldPause)
                beatVisualization->pauseAnimation();
            else
                beatVisualization->startAnimation();
        }
        updateButtonStates();
    };
    addAndMakeVisible(playButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 2: RESTART
    //==========================================================================
    restartButton.setButtonText("RESTART\n");
    restartButton.onClick = [this]
    {
        for (int i : getTargetLoops())
            audioProcessor.getLoopEngine(i).restartPlayback();
        // Beat viz: restart from beat 0 and resume counting
        if (beatVisualization != nullptr)
        {
            audioProcessor.setBeatCounterActive(true);
            audioProcessor.resetBeatIndex();
            beatVisualization->startFromBeginning();
        }
        updateButtonStates();
    };
    addAndMakeVisible(restartButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 3: UNDO
    //==========================================================================
    undoButton.setButtonText("UNDO\n");
    undoButton.onClick = [this]
    {
        for (int i : getTargetLoops())
        {
            auto& eng = audioProcessor.getLoopEngine(i);
            if (eng.canUndo()) eng.undo();
        }

        auto original = undoButton.findColour(juce::TextButton::buttonColourId);
        undoButton.setColour(juce::TextButton::buttonColourId,
                             OrbitalLooperLookAndFeel::getUndoColour());
        undoButton.repaint();
        juce::Timer::callAfterDelay(500, [this, original]()
        {
            undoButton.setColour(juce::TextButton::buttonColourId, original);
            undoButton.repaint();
        });
        updateButtonStates();
    };
    addAndMakeVisible(undoButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 3: REDO
    //==========================================================================
    redoButton.setButtonText("REDO\n");
    redoButton.onClick = [this]
    {
        for (int i : getTargetLoops())
        {
            auto& eng = audioProcessor.getLoopEngine(i);
            if (eng.canRedo()) eng.redo();
        }

        auto original = redoButton.findColour(juce::TextButton::buttonColourId);
        redoButton.setColour(juce::TextButton::buttonColourId,
                             OrbitalLooperLookAndFeel::getRedoColour());
        redoButton.repaint();
        juce::Timer::callAfterDelay(500, [this, original]()
        {
            redoButton.setColour(juce::TextButton::buttonColourId, original);
            redoButton.repaint();
        });
        updateButtonStates();
    };
    addAndMakeVisible(redoButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 3: CLEAR
    // When >1 loop: removes the selected loop.
    // When only 1 loop: resets it.
    //==========================================================================
    clearButton.setButtonText("CLEAR\n");
    clearButton.onClick = [this]
    {
        audioProcessor.cancelCountIn();  // v04.00 — CLEAR cancels any in-progress count-in

        auto targets = getTargetLoops();
        if (targets.size() > 1)
        {
            // Multi-selected: reset all selected loops (no deletion — window stays same size)
            for (int i : targets)
                audioProcessor.getLoopEngine(i).reset();
            updateButtonStates();
        }
        else
        {
            // Single selected: existing delete-or-reset logic
            int idx = targets.empty() ? audioProcessor.getCurrentLoopIndex() : targets[0];
            if (audioProcessor.getNumLoops() > 1)
            {
                audioProcessor.deleteLoop(idx);
                rebuildLoopCards();
                resized();
            }
            else
            {
                audioProcessor.getLoopEngine().reset();
            }
            updateButtonStates();
        }
        // Beat viz: reset to dark grid, stay stopped; freeze the audio-thread counter
        if (beatVisualization != nullptr)
        {
            audioProcessor.setBeatCounterActive(false);
            audioProcessor.resetBeatIndex();
            beatVisualization->resetAndStop();
        }
        // v03.05.01 — CLEAR also resets click track to defaults
        audioProcessor.setClickTrackOn(false);
        audioProcessor.resetClickTrack();
        clickTrackButton.setToggleState(false, juce::dontSendNotification);
        clickVolSlider.setValue(100.0, juce::dontSendNotification);
        { soundCombo.clear(juce::dontSendNotification); soundCombo.addItem("Woodblock", 1); soundCombo.addItem("Browse...", 2); }
        soundCombo.setSelectedId(1, juce::dontSendNotification);
        soundComboPreBrowseId = 1;
        if (beatVisualization != nullptr)
        {
            beatVisualization->setAccentMask(audioProcessor.getAccentMask());
            beatVisualization->setShowAccentBars(false);
        }
        // Reset master clock so next recording doesn't arm
        audioProcessor.getMasterClock().reset();
        audioProcessor.setMasterLoopIndex(-1);

        // Reset metronome to defaults (BPM=120, 4/4, ON)
        audioProcessor.setBPM(120);
        bpmValueLabel.setText("120", juce::dontSendNotification);
        applyTimeSig(4, 4);
        if (!audioProcessor.getMetronomeOn())
        {
            audioProcessor.setMetronomeOn(true);
            metronomeOnOffButton.setToggleState(true, juce::dontSendNotification);
            metronomeOnOffButton.setButtonText("ON");
            metronomeOnOffButton.setColour(juce::TextButton::buttonColourId,
                                           OrbitalLooperLookAndFeel::getPlayColour());
            metronomeLabel.setVisible(false);
            bpmValueLabel.setVisible(true);
            bpmUnitLabel.setVisible(true);
            tapButton.setVisible(true);
            timeSigCombo.setVisible(true);
        }
        if (beatVisualization != nullptr)
            beatVisualization->setVisible(true);
        resized();
    };
    addAndMakeVisible(clearButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 4: LOOP UP / DOWN / ALL
    // LOOP UP / DOWN now enabled (navigate selected loop).
    // LOOP ALL still disabled (future v02.00.02).
    //==========================================================================
    loopUpButton.setButtonText("LOOP\nUP");
    loopUpButton.onClick = [this]
    {
        int n   = audioProcessor.getNumLoops();
        int idx = (audioProcessor.getCurrentLoopIndex() - 1 + n) % n;
        selectLoop(idx);
    };
    addAndMakeVisible(loopUpButton);

    loopDownButton.setButtonText("LOOP\nDOWN");
    loopDownButton.onClick = [this]
    {
        int n   = audioProcessor.getNumLoops();
        int idx = (audioProcessor.getCurrentLoopIndex() + 1) % n;
        selectLoop(idx);
    };
    addAndMakeVisible(loopDownButton);

    loopAllButton.setButtonText("LOOP\nALL");
    loopAllButton.setClickingTogglesState(true);
    loopAllButton.onClick = [this]
    {
        loopAllActive = loopAllButton.getToggleState();
        if (loopAllActive)
        {
            preLoopAllSelection = selectedLoops;   // save full selection
            for (auto* card : loopCards)
                card->setSelected(true);
        }
        else
        {
            selectedLoops = preLoopAllSelection;   // restore
            if (selectedLoops.empty())
                selectedLoops.push_back(audioProcessor.getCurrentLoopIndex());
            preLoopAllSelection.clear();
            for (auto* card : loopCards)
                card->setSelected(isLoopSelected(card->getLoopIndex()));
            // Restore currentLoopIndex to last in restored selection
            if (!selectedLoops.empty())
                audioProcessor.setCurrentLoopIndex(selectedLoops.back());
        }
        updateButtonStates();
    };
    addAndMakeVisible(loopAllButton);

    //==========================================================================
    // FUNCTIONS BAR — Group 5: COUNT IN / CLICK TRACK
    //==========================================================================
    // v04.00 — GroundLoop-style COUNT IN: time-based, silent, visual countdown
    countInSeconds = audioProcessor.getCountInSeconds();
    countInButton.setButtonText("COUNT\nIN");   // 2 lines — seconds drawn as icon via property
    countInButton.getProperties().set("countInSeconds", countInSeconds);
    countInButton.setClickingTogglesState(true);
    countInButton.setToggleState(audioProcessor.getCountInEnabled(),
                                 juce::dontSendNotification);
    countInButton.onClick = [this]
    {
        const bool on = countInButton.getToggleState();
        audioProcessor.setCountInEnabled(on);
        if (!on) audioProcessor.cancelCountIn();
        updateButtonStates();
    };
    countInButton.onShowPopup = [this]
    {
        showCountInPopup();
    };
    addAndMakeVisible(countInButton);

    // v04.00 — large visual countdown overlay (GroundLoop style)
    countInCountdownLabel.setText("", juce::dontSendNotification);
    countInCountdownLabel.setJustificationType(juce::Justification::centred);
    countInCountdownLabel.setFont(juce::Font(120.0f, juce::Font::bold));
    countInCountdownLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    countInCountdownLabel.setColour(juce::Label::backgroundColourId,
                                     juce::Colours::black.withAlpha(0.7f));
    countInCountdownLabel.setVisible(false);
    countInCountdownLabel.setAlwaysOnTop(true);
    addAndMakeVisible(countInCountdownLabel);

    // v03.04.01 — CLICK TRACK button: toggles click audio on/off.
    // Only enabled when metronome is ON (enforced in updateButtonStates).
    clickTrackButton.setButtonText("CLICK\nTRACK");
    clickTrackButton.setClickingTogglesState(true);
    clickTrackButton.setToggleState(audioProcessor.getClickTrackOn(),
                                    juce::dontSendNotification);
    clickTrackButton.onClick = [this]
    {
        const bool on = clickTrackButton.getToggleState();
        audioProcessor.setClickTrackOn(on);
        if (!on)
        {
            // v03.05.01 — turning OFF resets all click track state to defaults
            audioProcessor.resetClickTrack();
            clickVolSlider.setValue(100.0, juce::dontSendNotification);
            { soundCombo.clear(juce::dontSendNotification); soundCombo.addItem("Woodblock", 1); soundCombo.addItem("Browse...", 2); }
            soundCombo.setSelectedId(1, juce::dontSendNotification);
            soundComboPreBrowseId = 1;
            if (beatVisualization != nullptr)
            {
                beatVisualization->setAccentMask(audioProcessor.getAccentMask());
                beatVisualization->setShowAccentBars(false);
            }
        }
        else
        {
            if (beatVisualization != nullptr)
                beatVisualization->setShowAccentBars(true);
        }
        resized();
        updateButtonStates();
    };
    addAndMakeVisible(clickTrackButton);

    //==========================================================================
    // v03.04.01 / v03.05.01 — CLICK TRACK controls (inline in metronome row)
    //==========================================================================

    // CLICK VOL label — same font as LOOP VOL (10pt)
    clickVolLabel.setText("CLICK VOL", juce::dontSendNotification);
    clickVolLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(10.0f));
    clickVolLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    clickVolLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(clickVolLabel);
    clickVolLabel.setVisible(false);  // hidden until click track is on

    // CLICK VOL slider — same width as LOOP VOL slider (90px)
    clickVolSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    clickVolSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    clickVolSlider.setRange(0.0, 100.0, 1.0);
    clickVolSlider.setValue(audioProcessor.getClickVolume() * 100.0,
                            juce::dontSendNotification);
    clickVolSlider.setDoubleClickReturnValue(true, 100.0);
    clickVolSlider.onValueChange = [this]
    {
        audioProcessor.setClickVolume(static_cast<float>(clickVolSlider.getValue())
                                      / 100.0f);
    };
    addAndMakeVisible(clickVolSlider);
    clickVolSlider.setVisible(false);

    // SOUND dropdown — replaces BROWSE + BUILT-IN buttons (v03.05.01)
    soundCombo.addItem("Woodblock", 1);
    soundCombo.addItem("Browse...",          2);
    soundCombo.setSelectedId(1, juce::dontSendNotification);
    soundComboPreBrowseId = 1;

    soundCombo.onChange = [this]
    {
        const int id = soundCombo.getSelectedId();
        if (id == 1)
        {
            audioProcessor.clearClickFile();
            { soundCombo.clear(juce::dontSendNotification); soundCombo.addItem("Woodblock", 1); soundCombo.addItem("Browse...", 2); }
            soundComboPreBrowseId = 1;
        }
        else if (id == 2)
        {
            // Revert immediately; launch async file chooser
            soundCombo.setSelectedId(soundComboPreBrowseId, juce::dontSendNotification);
            fileChooser = std::make_unique<juce::FileChooser>(
                "Load Click Sound", juce::File{}, "*.wav;*.aiff;*.aif");
            fileChooser->launchAsync(
                juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                [this](const juce::FileChooser& fc)
                {
                    const auto f = fc.getResult();
                    if (f == juce::File{}) return;
                    if (audioProcessor.loadClickFile(f))
                    {
                        soundCombo.clear(juce::dontSendNotification);
                        soundCombo.addItem("Woodblock", 1);
                        soundCombo.addItem("Browse...", 2);
                        soundCombo.addItem(audioProcessor.getClickFileName(), 3);
                        soundCombo.setSelectedId(3, juce::dontSendNotification);
                        soundComboPreBrowseId = 3;
                    }
                });
        }
        else if (id == 3)
        {
            soundComboPreBrowseId = 3;
        }
    };
    addAndMakeVisible(soundCombo);
    soundCombo.setVisible(false);

    //==========================================================================
    // METRONOME CARD — v03.00.01
    //==========================================================================

    // ON/OFF toggle — far left, default ON (blue)
    {
        const bool initOn = audioProcessor.getMetronomeOn();
        metronomeOnOffButton.setButtonText(initOn ? "ON" : "OFF");
        metronomeOnOffButton.setClickingTogglesState(true);
        metronomeOnOffButton.setToggleState(initOn, juce::dontSendNotification);
        metronomeOnOffButton.setColour(
            juce::TextButton::buttonColourId,
            initOn ? OrbitalLooperLookAndFeel::getPlayColour()
                   : OrbitalLooperLookAndFeel::getPanelColour());
        metronomeOnOffButton.onClick = [this]
        {
            const bool isOn = metronomeOnOffButton.getToggleState();
            audioProcessor.setMetronomeOn(isOn);
            metronomeOnOffButton.setButtonText(isOn ? "ON" : "OFF");
            metronomeOnOffButton.setColour(
                juce::TextButton::buttonColourId,
                isOn ? OrbitalLooperLookAndFeel::getPlayColour()
                     : OrbitalLooperLookAndFeel::getPanelColour());
            // Expanded (ON): show BPM + TAP + time sig, hide label
            // Collapsed (OFF): hide all controls, show "METRONOME" label
            metronomeLabel.setVisible(!isOn);
            bpmValueLabel.setVisible(isOn);
            bpmUnitLabel.setVisible(isOn);
            tapButton.setVisible(isOn);
            timeSigCombo.setVisible(isOn);
            // v03.04.01 / v03.05.01 — if metronome turns OFF, also reset+off click track
            if (!isOn && audioProcessor.getClickTrackOn())
            {
                audioProcessor.setClickTrackOn(false);
                clickTrackButton.setToggleState(false, juce::dontSendNotification);
                audioProcessor.resetClickTrack();
                clickVolSlider.setValue(100.0, juce::dontSendNotification);
                { soundCombo.clear(juce::dontSendNotification); soundCombo.addItem("Woodblock", 1); soundCombo.addItem("Browse...", 2); }
                soundCombo.setSelectedId(1, juce::dontSendNotification);
                soundComboPreBrowseId = 1;
                if (beatVisualization != nullptr)
                {
                    beatVisualization->setAccentMask(audioProcessor.getAccentMask());
                    beatVisualization->setShowAccentBars(false);
                }
            }
            // Beat visualization follows metronome ON/OFF
            if (beatVisualization != nullptr)
            {
                beatVisualization->setVisible(isOn);
                if (isOn)
                {
                    // Turning on: show the grid but don't auto-start — user presses PLAY
                    audioProcessor.resetBeatIndex();
                    audioProcessor.setBeatCounterActive(false);
                    beatVisualization->stopAnimation();  // show dark grid, wait for PLAY
                }
                else
                {
                    // Turning off: stop counter and reset
                    audioProcessor.setBeatCounterActive(false);
                    beatVisualization->stopAnimation();
                }
            }
            updateButtonStates();
            resized();
            repaint();
        };
    }
    addAndMakeVisible(metronomeOnOffButton);

    // "METRONOME" label — shown when collapsed (OFF state)
    metronomeLabel.setText("METRONOME", juce::dontSendNotification);
    metronomeLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(14.0f));
    metronomeLabel.setJustificationType(juce::Justification::centredLeft);
    metronomeLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    addAndMakeVisible(metronomeLabel);
    metronomeLabel.setVisible(!audioProcessor.getMetronomeOn());

    // BPM number — editable label, white text
    bpmValueLabel.setText(juce::String(audioProcessor.getBPM()),
                          juce::dontSendNotification);
    bpmValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(28.0f));
    bpmValueLabel.setJustificationType(juce::Justification::centred);
    bpmValueLabel.setColour(juce::Label::textColourId,       OrbitalLooperLookAndFeel::getTextColour());
    bpmValueLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    bpmValueLabel.setEditable(true, true);
    bpmValueLabel.onTextChange = [this]
    {
        const int bpm = bpmValueLabel.getText()
                            .retainCharacters("0123456789")
                            .getIntValue();
        if (bpm >= 40 && bpm <= 300)
        {
            audioProcessor.setBPM(bpm);
            bpmValueLabel.setText(juce::String(bpm), juce::dontSendNotification);
        }
        else
        {
            // Revert to current value if out of range
            bpmValueLabel.setText(juce::String(audioProcessor.getBPM()),
                                  juce::dontSendNotification);
        }
    };
    addAndMakeVisible(bpmValueLabel);
    bpmValueLabel.setVisible(audioProcessor.getMetronomeOn());

    // "BPM" unit label — static, shown when expanded
    bpmUnitLabel.setText("BPM", juce::dontSendNotification);
    bpmUnitLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(16.0f));
    bpmUnitLabel.setJustificationType(juce::Justification::centredLeft);
    bpmUnitLabel.setColour(juce::Label::textColourId, OrbitalLooperLookAndFeel::getTextColour());
    addAndMakeVisible(bpmUnitLabel);
    bpmUnitLabel.setVisible(audioProcessor.getMetronomeOn());

    // TAP button — far right, shown when expanded
    tapButton.setButtonText("TAP");
    tapButton.setColour(juce::TextButton::buttonColourId,
                        OrbitalLooperLookAndFeel::getLoopAllColour());  // always green
    tapButton.onClick = [this]
    {
        const juce::int64 now = juce::Time::currentTimeMillis();

        // Reset tap history if gap since last tap exceeds 2 seconds
        if (!metroTapTimes.empty() && (now - metroTapTimes.back()) > 2000)
            metroTapTimes.clear();

        metroTapTimes.push_back(now);

        // Keep the last 8 taps only
        if ((int)metroTapTimes.size() > 8)
            metroTapTimes.erase(metroTapTimes.begin());

        // Need at least 2 taps to calculate BPM
        if ((int)metroTapTimes.size() >= 2)
        {
            double totalMs = 0.0;
            for (int i = 1; i < (int)metroTapTimes.size(); ++i)
                totalMs += (double)(metroTapTimes[i] - metroTapTimes[i - 1]);
            const double avgMs = totalMs / (double)(metroTapTimes.size() - 1);
            const int newBPM   = juce::jlimit(40, 300,
                                              juce::roundToInt(60000.0 / avgMs));
            audioProcessor.setBPM(newBPM);
            bpmValueLabel.setText(juce::String(newBPM), juce::dontSendNotification);
        }
    };
    addAndMakeVisible(tapButton);
    tapButton.setVisible(audioProcessor.getMetronomeOn());

    //==========================================================================
    // TIME SIGNATURE DROPDOWN — v03.05.01 (replaces 5 TextButtons)
    //==========================================================================
    {
        timeSigCombo.addItem("4/4",       1);
        timeSigCombo.addItem("3/4",       2);
        timeSigCombo.addItem("6/8",       3);
        timeSigCombo.addItem("5/4",       4);
        timeSigCombo.addSeparator();
        timeSigCombo.addItem("Custom...", 5);

        // Set initial selection
        const int initN = audioProcessor.getTimeSigNumerator();
        const int initD = audioProcessor.getTimeSigDenominator();
        int initId = 5;
        if      (initN==4 && initD==4) initId=1;
        else if (initN==3 && initD==4) initId=2;
        else if (initN==6 && initD==8) initId=3;
        else if (initN==5 && initD==4) initId=4;
        timeSigCombo.setSelectedId(initId, juce::dontSendNotification);

        timeSigCombo.onChange = [this]
        {
            const int id = timeSigCombo.getSelectedId();
            if      (id==1) applyTimeSig(4,4);
            else if (id==2) applyTimeSig(3,4);
            else if (id==3) applyTimeSig(6,8);
            else if (id==4) applyTimeSig(5,4);
            else if (id==5)
            {
                // Revert combo immediately, then show AlertWindow
                const int n = audioProcessor.getTimeSigNumerator();
                const int d = audioProcessor.getTimeSigDenominator();
                int prevId = 5;
                if      (n==4&&d==4) prevId=1; else if (n==3&&d==4) prevId=2;
                else if (n==6&&d==8) prevId=3; else if (n==5&&d==4) prevId=4;
                timeSigCombo.setSelectedId(prevId, juce::dontSendNotification);

                auto* w = new juce::AlertWindow("Custom Time Signature",
                                                 "Enter numerator/denominator (e.g. 7/8):",
                                                 juce::AlertWindow::NoIcon);
                w->addTextEditor("sig", juce::String(n) + "/" + juce::String(d));
                w->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
                w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
                w->enterModalState(true, juce::ModalCallbackFunction::create(
                    [this, w](int result)
                    {
                        if (result == 1)
                        {
                            const auto txt   = w->getTextEditorContents("sig");
                            const int  slash = txt.indexOfChar('/');
                            if (slash > 0)
                            {
                                const int num = txt.substring(0, slash).getIntValue();
                                const int den = txt.substring(slash + 1).getIntValue();
                                if (num > 0 && den > 0) applyTimeSig(num, den);
                            }
                        }
                        delete w;
                    }), true);
            }
        };

        addAndMakeVisible(timeSigCombo);
        timeSigCombo.setVisible(audioProcessor.getMetronomeOn());
        // v03.06.01 — blue background to match the old time sig button highlight
        timeSigCombo.setColour(juce::ComboBox::backgroundColourId,
                               OrbitalLooperLookAndFeel::getPlayColour());
        timeSigCombo.setColour(juce::ComboBox::outlineColourId,
                               OrbitalLooperLookAndFeel::getPlayColour().darker(0.35f));
        timeSigCombo.setColour(juce::ComboBox::arrowColourId,
                               OrbitalLooperLookAndFeel::getTextColour().withAlpha(0.85f));

        // Apply initial time sig (syncs beat viz + accent mask)
        applyTimeSig(initN, initD);
    }

    //==========================================================================
    // ADD LOOP BUTTON
    //==========================================================================
    addLoopButton.setButtonText("ADD LOOP");
    addLoopButton.setTooltip("Add Loop");
    addLoopButton.onClick = [this]
    {
        audioProcessor.addNewLoop();
        rebuildLoopCards();
        resized();
    };
    addAndMakeVisible(addLoopButton);

    //==========================================================================
    // VIEWPORT for loop cards
    //==========================================================================
    loopCardsViewport.setScrollBarsShown(true, false);  // vertical scroll only
    loopCardsViewport.setScrollBarThickness(8);
    addAndMakeVisible(loopCardsViewport);

    //==========================================================================
    // STATUS LABEL
    //==========================================================================
    statusLabel.setVisible(false);

    //==========================================================================
    // Wire MIDI-triggered add/delete to rebuild UI on message thread
    //==========================================================================
    audioProcessor.onLoopCardsChanged = [this, alive = editorAlive]
    {
        juce::MessageManager::callAsync([this, alive]
        {
            if (!*alive) return;
            rebuildLoopCards();
            resized();
            updateButtonStates();
        });
    };

    //==========================================================================
    // Wire MIDI-triggered LOOP ALL toggle (v02.02.01)
    //==========================================================================
    audioProcessor.onLoopAllToggled = [this, alive = editorAlive]
    {
        juce::MessageManager::callAsync([this, alive]
        {
            if (!*alive) return;
            loopAllActive = !loopAllActive;
            loopAllButton.setToggleState(loopAllActive, juce::dontSendNotification);
            if (loopAllActive)
            {
                preLoopAllSelection = selectedLoops;   // save full selection
                for (auto* card : loopCards)
                    card->setSelected(true);
            }
            else
            {
                selectedLoops = preLoopAllSelection;   // restore
                if (selectedLoops.empty())
                    selectedLoops.push_back(audioProcessor.getCurrentLoopIndex());
                preLoopAllSelection.clear();
                for (auto* card : loopCards)
                    card->setSelected(isLoopSelected(card->getLoopIndex()));
                if (!selectedLoops.empty())
                    audioProcessor.setCurrentLoopIndex(selectedLoops.back());
            }
            updateButtonStates();
        });
    };

    //==========================================================================
    // Wire MIDI-triggered SYNC MODE change (v02.04.01)
    //==========================================================================
    audioProcessor.onSyncModeChanged = [this, alive = editorAlive]
    {
        juce::MessageManager::callAsync([this, alive] { if (*alive) updateButtonStates(); });
    };

    //==========================================================================
    // Wire mute/solo state changes (v02.05.01/02)
    //==========================================================================
    audioProcessor.onMuteSoloChanged = [this, alive = editorAlive]
    {
        juce::MessageManager::callAsync([this, alive] { if (*alive) updateButtonStates(); });
    };

    //==========================================================================
    // BEAT VISUALIZATION — v03.03.01
    //==========================================================================
    beatVisualization = std::make_unique<BeatVisualizationComponent>(audioProcessor);
    beatVisualization->setTimeSignature(audioProcessor.getTimeSigNumerator(),
                                        audioProcessor.getTimeSigDenominator());
    // v03.04.01 — accent mask and toggle callback
    beatVisualization->setAccentMask(audioProcessor.getAccentMask());
    beatVisualization->onAccentToggled = [this](int beatIdx, bool nowAccented)
    {
        uint32_t mask = audioProcessor.getAccentMask();
        if (nowAccented) mask |=  (1u << beatIdx);
        else             mask &= ~(1u << beatIdx);
        audioProcessor.setAccentMask(mask);
    };
    addAndMakeVisible(beatVisualization.get());
    // Visible when metronome is ON, but NOT animating until RECORD or PLAY is pressed
    beatVisualization->setVisible(audioProcessor.getMetronomeOn());
    // Accent toggling only active when click track is on (set in updateButtonStates)
    beatVisualization->setInterceptsMouseClicks(
        audioProcessor.getMetronomeOn() && audioProcessor.getClickTrackOn(), false);

    //==========================================================================
    // Build initial card list and start timer
    //==========================================================================
    rebuildLoopCards();
    // Initialise selection to the current loop (rebuildLoopCards may have set this
    // already, but be explicit here so the invariant is guaranteed)
    selectedLoops = { audioProcessor.getCurrentLoopIndex() };
    startTimerHz(30);
    updateButtonStates();

    // Force layout + repaint on the message thread so cards are visible immediately
    // without needing a user interaction to trigger the first draw.
    juce::MessageManager::callAsync([this, alive = editorAlive]
    {
        if (*alive) { resized(); repaint(); }
    });
}

OrbitalLooperAudioProcessorEditor::~OrbitalLooperAudioProcessorEditor()
{
    // Signal all queued callAsync lambdas that the editor is gone.
    // They hold a shared_ptr copy of editorAlive, so they can check before
    // touching 'this'. This prevents crashes when audio thread fires a callback
    // after the editor window has been closed.
    *editorAlive = false;

    audioProcessor.onLoopCardsChanged  = nullptr;
    audioProcessor.onLoopAllToggled    = nullptr;
    audioProcessor.onSyncModeChanged   = nullptr;
    audioProcessor.onMuteSoloChanged   = nullptr;
    audioProcessor.onThemeChanged      = nullptr;
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
// v02.07.01 - SAVE CHOICE DIALOG (multi-loop only)
//==============================================================================
void OrbitalLooperAudioProcessorEditor::showSaveChoiceDialog()
{
    // Build a description of what "Selected" will save
    const int numSelected = (int)selectedLoops.size();
    const int numTotal    = audioProcessor.getNumLoops();

    juce::String selectedDesc;
    if (numSelected == numTotal)
        selectedDesc = "Save Selected (" + juce::String(numSelected) + " — same as All)";
    else
        selectedDesc = "Save Selected (" + juce::String(numSelected) + " of "
                       + juce::String(numTotal) + " loops)";

    auto* dialog = new juce::AlertWindow(
        "Save Session",
        "How would you like to save " + juce::String(numTotal) + " loops?",
        juce::MessageBoxIconType::QuestionIcon);

    dialog->addButton("Save All",        1);
    dialog->addButton(selectedDesc,      2);
    dialog->addButton("Save Separately", 3);
    dialog->addButton("Cancel",          0);

    dialog->enterModalState(true,
        juce::ModalCallbackFunction::create([this](int result)
        {
            switch (result)
            {
                case 1: showSaveChooser(SaveMode::All);        break;
                case 2: showSaveChooser(SaveMode::Selected);   break;
                case 3: showSaveChooser(SaveMode::Separately); break;
                default: break;  // Cancel — do nothing
            }
        }), true);
}

//==============================================================================
// v02.07.01 - SAVE FILE CHOOSER (handles All / Selected / Separately)
//==============================================================================
void OrbitalLooperAudioProcessorEditor::showSaveChooser(SaveMode mode)
{
    auto defaultFolder = OrbitalLooperAudioProcessor::getDefaultSessionsFolder();

    juce::String dialogTitle;
    switch (mode)
    {
        case SaveMode::All:        dialogTitle = "Save All Loops";          break;
        case SaveMode::Selected:   dialogTitle = "Save Selected Loops";     break;
        case SaveMode::Separately: dialogTitle = "Save Loops Separately — choose base name"; break;
    }

    fileChooser = std::make_unique<juce::FileChooser>(
        dialogTitle,
        defaultFolder.getChildFile("Session.orbl"),
        "*.orbl");

    const auto flags = juce::FileBrowserComponent::saveMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(flags, [this, mode](const juce::FileChooser& fc)
    {
        auto result = fc.getResult();
        if (result == juce::File{})
            return;

        auto baseFile = result.withFileExtension(".orbl");

        auto showError = [](const juce::String& path)
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Save Failed",
                "Could not save session to:\n" + path);
        };

        if (mode == SaveMode::All)
        {
            // Save all loops into one .orbl
            if (!audioProcessor.saveSession(baseFile))
                showError(baseFile.getFullPathName());
        }
        else if (mode == SaveMode::Selected)
        {
            // Save only the currently selected loops into one .orbl
            if (!audioProcessor.saveSession(baseFile, selectedLoops))
                showError(baseFile.getFullPathName());
        }
        else // SaveMode::Separately
        {
            // Save each loop as its own single-loop .orbl file.
            // Files are named: BaseName_Loop1.orbl, BaseName_Loop2.orbl, …
            const juce::String stem    = baseFile.getFileNameWithoutExtension();
            const juce::File   folder  = baseFile.getParentDirectory();
            const int numLoops = audioProcessor.getNumLoops();
            bool anyFailed = false;

            for (int i = 0; i < numLoops; ++i)
            {
                juce::String loopName = stem + "_Loop" + juce::String(i + 1) + ".orbl";
                juce::File   loopFile = folder.getChildFile(loopName);
                if (!audioProcessor.saveSession(loopFile, { i }))
                {
                    anyFailed = true;
                    showError(loopFile.getFullPathName());
                }
            }

            if (!anyFailed)
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::InfoIcon,
                    "Save Complete",
                    juce::String(numLoops) + " loop files saved to:\n"
                    + folder.getFullPathName());
            }
        }
    });
}

//==============================================================================
// SESSION LOAD CHOOSER
//==============================================================================
void OrbitalLooperAudioProcessorEditor::showLoadChooser()
{
    auto defaultFolder = OrbitalLooperAudioProcessor::getDefaultSessionsFolder();
    fileChooser = std::make_unique<juce::FileChooser>(
        "Load Orbital Looper Session", defaultFolder, "*.orbl");

    auto flags = juce::FileBrowserComponent::openMode
               | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File{}) return;

        if (!audioProcessor.loadSession(file))
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon, "Load Failed",
                "Could not load session from:\n" + file.getFullPathName());
        }
        else
        {
            // onLoopCardsChanged will trigger rebuildLoopCards + resized
            // Also update sliders on all existing cards
            for (auto* card : loopCards)
            {
                // Sliders are owned by the card; rebuildLoopCards (triggered by
                // onLoopCardsChanged) will recreate them with fresh values
                juce::ignoreUnused(card);
            }
            rebuildLoopCards();
            resized();
            updateButtonStates();
        }
    });
}

//==============================================================================
// MULTI-LOOP HELPERS
//==============================================================================
void OrbitalLooperAudioProcessorEditor::rebuildLoopCards()
{
    loopCards.clear();               // OwnedArray deletes all cards
    loopCardsContainer.removeAllChildren();

    // Validate selectedLoops — remove stale indices after deletions/loads
    const int n = audioProcessor.getNumLoops();
    selectedLoops.erase(
        std::remove_if(selectedLoops.begin(), selectedLoops.end(),
                       [n](int i){ return i < 0 || i >= n; }),
        selectedLoops.end());
    if (selectedLoops.empty())
        selectedLoops.push_back(audioProcessor.getCurrentLoopIndex());

    for (int i = 0; i < audioProcessor.getNumLoops(); ++i)
    {
        auto* card = new LoopCardComponent(audioProcessor, i);
        card->setSelected(loopAllActive ? true : isLoopSelected(i));
        card->onSelected = [this](int loopIndex, bool cmdHeld, bool shiftHeld)
        {
            selectLoop(loopIndex, cmdHeld, shiftHeld);
        };
        card->onDelete   = [this](int loopIndex)
        {
            audioProcessor.deleteLoop(loopIndex);
            rebuildLoopCards();
            resized();
            updateButtonStates();
        };
        // Sync initial mute/solo toggle states from engine (v02.05.01/02)
        card->syncMuteSoloState();

        loopCards.add(card);
        loopCardsContainer.addAndMakeVisible(card);
    }
    loopCardsViewport.setViewedComponent(&loopCardsContainer, false);
}

void OrbitalLooperAudioProcessorEditor::applyToAllLoops(
    std::function<void(LoopEngine&)> fn)
{
    for (int i = 0; i < audioProcessor.getNumLoops(); ++i)
        fn(audioProcessor.getLoopEngine(i));
}

bool OrbitalLooperAudioProcessorEditor::isLoopSelected(int index) const
{
    return std::find(selectedLoops.begin(), selectedLoops.end(), index)
           != selectedLoops.end();
}

std::vector<int> OrbitalLooperAudioProcessorEditor::getTargetLoops() const
{
    // LOOP ALL overrides: target every loop
    if (loopAllActive)
    {
        std::vector<int> all;
        for (int i = 0; i < audioProcessor.getNumLoops(); ++i)
            all.push_back(i);
        return all;
    }
    return selectedLoops;
}

void OrbitalLooperAudioProcessorEditor::selectLoop(int index, bool cmdHeld, bool shiftHeld)
{
    // Any card click cancels LOOP ALL
    if (loopAllActive)
    {
        loopAllActive = false;
        loopAllButton.setToggleState(false, juce::dontSendNotification);
    }

    if (shiftHeld && !selectedLoops.empty())
    {
        // Range-select from anchor (currentLoopIndex) to clicked index
        int anchor = audioProcessor.getCurrentLoopIndex();
        int lo = juce::jmin(anchor, index);
        int hi = juce::jmax(anchor, index);
        selectedLoops.clear();
        for (int i = lo; i <= hi; ++i)
            selectedLoops.push_back(i);
    }
    else if (cmdHeld)
    {
        // Toggle: add or remove from selection
        auto it = std::find(selectedLoops.begin(), selectedLoops.end(), index);
        if (it != selectedLoops.end())
        {
            selectedLoops.erase(it);
            if (selectedLoops.empty())        // never leave empty
                selectedLoops.push_back(index);
        }
        else
        {
            selectedLoops.push_back(index);
        }
    }
    else
    {
        // Plain click: single-select
        selectedLoops.clear();
        selectedLoops.push_back(index);
    }

    // The "current" loop (for status label + LOOP UP/DOWN) is the most-recently-clicked
    audioProcessor.setCurrentLoopIndex(index);

    // Sync card highlights
    for (auto* card : loopCards)
        card->setSelected(isLoopSelected(card->getLoopIndex()));

    updateButtonStates();
}

//==============================================================================
// SHARED GEOMETRY HELPER
//==============================================================================
juce::Rectangle<int> OrbitalLooperAudioProcessorEditor::computeFunctionsGeometry(
    int /*windowWidth*/, int /*windowHeight*/,
    int& btnSizeOut, int& paddingVOut, int& withinGapOut, int& betweenGapOut)
{
    const int margin = s(OrbitalLooperLookAndFeel::MARGIN);
    btnSizeOut    = s(BASE_BTN_SIZE);
    paddingVOut   = s(FUNCTIONS_PADDING_V);
    withinGapOut  = s(BTN_WITHIN_GROUP);
    betweenGapOut = s(BTN_BETWEEN_GROUP);
    return { margin, s(TOOLBAR_HEIGHT), getWidth() - 2 * margin, s(BASE_BTN_SIZE) };
}

//==============================================================================
// PAINT
//==============================================================================
void OrbitalLooperAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(OrbitalLooperLookAndFeel::getBackgroundColour());

    // METRONOME CARD background panel — v03.00.01
    if (!metronomeCardBounds.isEmpty())
    {
        g.setColour(OrbitalLooperLookAndFeel::getPanelColour());
        g.fillRoundedRectangle(metronomeCardBounds.toFloat(),
                               OrbitalLooperLookAndFeel::CORNER_SIZE);

        // Light mode: thin border around card
        if (!OrbitalLooperLookAndFeel::isDark())
        {
            g.setColour(OrbitalLooperLookAndFeel::getBorderColour());
            g.drawRoundedRectangle(metronomeCardBounds.toFloat(),
                                   OrbitalLooperLookAndFeel::CORNER_SIZE, 1.0f);
        }
    }

    // Loop cards draw themselves.
    // No FUNCTIONS BAR border (removed in v01.07.02).
}

//==============================================================================
// BEAT VISUALIZATION COMPONENT — v03.03.01
//==============================================================================

BeatVisualizationComponent::BeatVisualizationComponent(OrbitalLooperAudioProcessor& proc)
    : audioProcessor(proc)
{
    visitedBeats.resize(NUM_BARS * beatsPerMeasure, false);
    startTimerHz(60);
}

BeatVisualizationComponent::~BeatVisualizationComponent()
{
    stopTimer();
}

void BeatVisualizationComponent::setTimeSignature(int numerator, int denominator)
{
    beatsPerMeasure = numerator;
    beatDenominator = denominator;
    visitedBeats.assign(NUM_BARS * beatsPerMeasure, false);
    currentBeat = 0;
    lastBeat    = -1;
    repaint();
}

void BeatVisualizationComponent::startFromBeginning()
{
    // RECORD: always restart the grid from beat 0
    std::fill(visitedBeats.begin(), visitedBeats.end(), false);
    currentBeat = 0;
    lastBeat    = -1;
    isAnimating = true;
    repaint();
}

void BeatVisualizationComponent::startAnimation()
{
    // PLAY: resume from wherever we paused, or from 0 if never started
    isAnimating = true;
    repaint();
}

void BeatVisualizationComponent::pauseAnimation()
{
    // PAUSE: freeze position, keep visited-beats intact for visual context
    isAnimating = false;
    repaint();
}

void BeatVisualizationComponent::resetAndStop()
{
    // CLEAR: wipe the grid back to all-dark, do NOT start
    isAnimating = false;
    std::fill(visitedBeats.begin(), visitedBeats.end(), false);
    currentBeat = 0;
    lastBeat    = -1;
    repaint();
}

void BeatVisualizationComponent::stopAnimation()
{
    // Metronome OFF: full stop + reset
    isAnimating = false;
    std::fill(visitedBeats.begin(), visitedBeats.end(), false);
    currentBeat = 0;
    lastBeat    = -1;
    repaint();
}

void BeatVisualizationComponent::timerCallback()
{
    if (!isAnimating)
        return;

    const int total = NUM_BARS * beatsPerMeasure;
    if (total <= 0)
        return;

    const int absIdx  = audioProcessor.getCurrentBeatIndex();
    // v03.06.01 — fix off-by-one: the processor stores beatIdx AFTER incrementing
    // (patternBeat = beatIdx - 1 in processBlock), so subtract 1 to sync visual + audio.
    const int newBeat = absIdx > 0 ? (absIdx - 1) % total : 0;

    if (newBeat == lastBeat)
        return;

    if (lastBeat >= 0 && newBeat < lastBeat)
    {
        // Wrapped around — reset all visited, mark beat 0 if we're on it
        std::fill(visitedBeats.begin(), visitedBeats.end(), false);
    }
    else if (lastBeat >= 0)
    {
        // Mark any intermediate beats we might have jumped over
        for (int i = lastBeat + 1; i < newBeat && i < (int)visitedBeats.size(); ++i)
            visitedBeats[i] = true;
    }

    currentBeat = newBeat;
    lastBeat    = newBeat;

    if (newBeat < (int)visitedBeats.size())
        visitedBeats[newBeat] = true;

    repaint();
}

void BeatVisualizationComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // No background drawn here — the editor's paint() extends metronomeCardBounds
    // to cover this component, so we share the same panel seamlessly.

    const int total = NUM_BARS * beatsPerMeasure;

    // Reset cell registries for mouseDown hit-testing (rebuilt every paint)
    cells.clear();
    accentBarCells.clear();

    const juce::Colour amberColour = OrbitalLooperLookAndFeel::getClickTrackColour();

    // v06.00: scale relative to base section width (738)
    const float vizScale = bounds.getWidth() / 738.0f;

    // v03.05.01: reserve top acBarPx pixels for accent bar strip
    const int  acBarPx    = showAccentBars ? (int)(12.0f * vizScale) : 0;
    const auto accentArea = bounds.withHeight((float)acBarPx);
    const auto beatArea   = bounds.withTrimmedTop((float)acBarPx);
    const auto grid       = beatArea.reduced(8.0f * vizScale);

    const float  barSpacing    = 12.0f * vizScale;
    const float  cellSpacing   = 4.0f * vizScale;
    const float  totalBarSpace = barSpacing * (NUM_BARS - 1);
    const float  barWidth      = (grid.getWidth() - totalBarSpace) / NUM_BARS;

    if (total <= 0 || barWidth <= 0)
        return;

    // ── Pre-compute layout (shared by accent bar + beat cell drawing) ─────────
    const bool twoRows   = (beatsPerMeasure == 5 || beatsPerMeasure == 6);
    const int  row1Beats = twoRows ? 4 : beatsPerMeasure;
    const int  row2Beats = twoRows ? beatsPerMeasure - 4 : 0;

    float cellSize;
    if (!twoRows)
        cellSize = juce::jmin(
            (barWidth - cellSpacing * (beatsPerMeasure - 1)) / beatsPerMeasure,
            grid.getHeight() - 8.0f);
    else
        cellSize = juce::jmin(
            (barWidth - cellSpacing * (row1Beats - 1)) / row1Beats,
            (grid.getHeight() - cellSpacing) * 0.5f);

    // ── Accent bar strip (v03.05.01) — thin rectangles above beat cells ───────
    if (showAccentBars && acBarPx > 0)
    {
        const float barH = (float)acBarPx - 4.0f;   // 2px padding top + bottom
        const float barY = accentArea.getY() + 2.0f;

        for (int bar = 0; bar < NUM_BARS; ++bar)
        {
            const float barX = grid.getX() + bar * (barWidth + barSpacing);
            for (int b = 0; b < beatsPerMeasure; ++b)
            {
                const int   bIdx  = bar * beatsPerMeasure + b;
                const bool  isAcc = (bIdx < 32) && ((localAccentMask >> bIdx) & 1u);
                const float bx    = barX + (b % row1Beats) * (cellSize + cellSpacing);
                const juce::Rectangle<float> r(bx, barY, cellSize, barH);
                accentBarCells.add({ r, bIdx });
                g.setColour(isAcc ? amberColour
                                  : OrbitalLooperLookAndFeel::getStoppedColour().darker(0.5f));
                g.fillRoundedRectangle(r, 2.0f);
            }
        }
    }

    // Helper: draw a single beat cell with play colour (current/visited) or stopped colour (dark).
    // v03.06.01 — accent state no longer affects beat cell drawing; accent bars handle that.
    const auto drawCell = [&](float cx, float cy, float sz, int beatIndex)
    {
        const bool isCurrent = isAnimating && (beatIndex == currentBeat % total);
        const bool isVisited = (beatIndex < (int)visitedBeats.size() && visitedBeats[beatIndex]);

        const juce::Rectangle<float> cellRect(cx, cy, sz, sz);
        cells.add({ cellRect, beatIndex });  // register for legacy hit-testing

        if (isCurrent)
        {
            // Blue glow halo + bright blue fill
            const juce::Colour glowCol = OrbitalLooperLookAndFeel::getPlayColour().withAlpha(0.8f);
            juce::DropShadow glow(glowCol, 12, { 0, 0 });
            juce::Path p;
            p.addRoundedRectangle(cx, cy, sz, sz, 3.0f);
            glow.drawForPath(g, p);
            g.setColour(OrbitalLooperLookAndFeel::getPlayColour().brighter(0.5f));
            g.fillRoundedRectangle(cx, cy, sz, sz, 3.0f);
        }
        else if (isVisited)
        {
            g.setColour(OrbitalLooperLookAndFeel::getPlayColour());
            g.fillRoundedRectangle(cx, cy, sz, sz, 3.0f);
        }
        else
        {
            // Dark unvisited cell — no amber dot; accent bars show accent state above
            g.setColour(OrbitalLooperLookAndFeel::getStoppedColour().darker(0.5f));
            g.fillRoundedRectangle(cx, cy, sz, sz, 3.0f);
        }
    };

    if (!twoRows)
    {
        // ── Single-row layout: 4/4, 3/4, and any other straight meter ──────
        for (int bar = 0; bar < NUM_BARS; ++bar)
        {
            const float barX = grid.getX() + bar * (barWidth + barSpacing);
            const float cy   = grid.getY() + (grid.getHeight() - cellSize) * 0.5f;

            for (int beat = 0; beat < beatsPerMeasure; ++beat)
                drawCell(barX + beat * (cellSize + cellSpacing), cy,
                         cellSize, bar * beatsPerMeasure + beat);
        }
    }
    else
    {
        // ── Two-row layout: 6/8 (4+2) and 5/4 (4+1) ────────────────────────
        const float rowSpacing = cellSpacing;

        for (int bar = 0; bar < NUM_BARS; ++bar)
        {
            const float barX  = grid.getX() + bar * (barWidth + barSpacing);
            const float row1Y = grid.getY();
            const float row2Y = row1Y + cellSize + rowSpacing;

            for (int b = 0; b < row1Beats; ++b)
                drawCell(barX + b * (cellSize + cellSpacing), row1Y,
                         cellSize, bar * beatsPerMeasure + b);

            for (int b = 0; b < row2Beats; ++b)
                drawCell(barX + b * (cellSize + cellSpacing), row2Y,
                         cellSize, bar * beatsPerMeasure + row1Beats + b);
        }
    }
}

//==============================================================================
// BEAT VIZ — mouseDown (v03.05.01)
// v03.05.01: accent bars take priority when showAccentBars is true.
//   Clicking a bar toggles that beat's accent; beat cells are not clickable.
// Legacy (showAccentBars == false): hit-tests beat cells directly.
//==============================================================================
void BeatVisualizationComponent::mouseDown(const juce::MouseEvent& e)
{
    if (showAccentBars)
    {
        for (const auto& bar : accentBarCells)
        {
            if (bar.bounds.contains(e.position))
            {
                const bool wasAccented = (bar.beatIndex < 32)
                                      && ((localAccentMask >> bar.beatIndex) & 1u);
                if (wasAccented)
                    localAccentMask &= ~(1u << bar.beatIndex);
                else
                    localAccentMask |=  (1u << bar.beatIndex);

                if (onAccentToggled)
                    onAccentToggled(bar.beatIndex, !wasAccented);

                repaint();
                return;
            }
        }
        return;  // click not on any accent bar — ignore (beat cells not clickable)
    }

    // Legacy: beat-cell clicking when accent bars are hidden
    for (const auto& cell : cells)
    {
        if (cell.bounds.contains(e.position))
        {
            const bool wasAccented = (cell.beatIndex < 32)
                                  && ((localAccentMask & (1u << cell.beatIndex)) != 0u);

            if (wasAccented)
                localAccentMask &= ~(1u << cell.beatIndex);
            else
                localAccentMask |=  (1u << cell.beatIndex);

            if (onAccentToggled)
                onAccentToggled(cell.beatIndex, !wasAccented);

            repaint();
            return;
        }
    }
}

//==============================================================================
// BEAT VIZ — setAccentMask (v03.04.01)
// Syncs the UI-side accent mirror from the processor (called on time sig change).
//==============================================================================
void BeatVisualizationComponent::setAccentMask(uint32_t mask)
{
    localAccentMask = mask;
    repaint();
}

//==============================================================================
// BEAT VIZ — setShowAccentBars (v03.05.01)
// Shows/hides the thin accent bar strip drawn above the beat cells.
// When true, a 12px strip of amber/dark rectangles replaces direct cell clicking.
//==============================================================================
void BeatVisualizationComponent::setShowAccentBars(bool show)
{
    if (showAccentBars == show)
        return;
    showAccentBars = show;
    repaint();
}

//==============================================================================
// applyTimeSig — v03.05.01 (simplified: no more button highlighting)
// Sets the time signature in the processor and syncs the ComboBox selection.
//==============================================================================
void OrbitalLooperAudioProcessorEditor::applyTimeSig(int numerator, int denominator)
{
    audioProcessor.setTimeSig(numerator, denominator);

    // Sync ComboBox to preset (or leave as Custom for non-preset values)
    int id = 5;
    if      (numerator == 4 && denominator == 4) id = 1;
    else if (numerator == 3 && denominator == 4) id = 2;
    else if (numerator == 6 && denominator == 8) id = 3;
    else if (numerator == 5 && denominator == 4) id = 4;
    timeSigCombo.setSelectedId(id, juce::dontSendNotification);

    // Sync beat visualization to new time signature + reset accent mask to downbeats
    if (beatVisualization != nullptr)
    {
        audioProcessor.resetBeatIndex();
        beatVisualization->setTimeSignature(numerator, denominator);
        beatVisualization->setAccentMask(audioProcessor.getAccentMask());
    }
}

//==============================================================================
// RESIZED
//==============================================================================
void OrbitalLooperAudioProcessorEditor::resized()
{
    // v06.00: Compute scale from width
    currentScale = (float)getWidth() / (float)BASE_WIDTH;

    const int margin = s(OrbitalLooperLookAndFeel::MARGIN);
    const int sectionW = getWidth() - 2 * margin;

    int btnSize, paddingV, withinGap, betweenGap;
    auto funcRect = computeFunctionsGeometry(getWidth(), getHeight(),
                                             btnSize, paddingV, withinGap, betweenGap);

    //--- Compute viewport height ---
    const int numLoops      = audioProcessor.getNumLoops();
    const int scaledCardH   = s(CARD_HEIGHT);
    const int scaledCardGap = s(CARD_GAP);
    const int contentH      = numLoops * (scaledCardH + scaledCardGap);
    const int maxViewportH  = VIEWPORT_CARDS_MAX * (scaledCardH + scaledCardGap);
    const int viewportH     = juce::jmin(maxViewportH, contentH);

    //--- Compute total window height ---
    const bool metroOn          = audioProcessor.getMetronomeOn();
    const bool clickOn          = audioProcessor.getClickTrackOn();
    const bool showAccentBar    = metroOn && clickOn;
    const int  accentBarContrib = showAccentBar ? s(ACCENT_BAR_H) : 0;
    const int  beatVizContrib   = metroOn ? s(BEAT_VIZ_H) : 0;

    const int totalH = s(TOOLBAR_HEIGHT) + s(BASE_BTN_SIZE) + scaledCardGap
                     + s(METRONOME_CARD_H) + accentBarContrib + beatVizContrib + scaledCardGap
                     + viewportH + s(ADD_LOOP_GAP) + s(ADD_LOOP_BTN_H);

    // Snap height
    if (getHeight() != totalH)
    {
        setSize(getWidth(), totalH);
        return;
    }

    // Persist width
    audioProcessor.setEditorWidth(getWidth());

    //==========================================================================
    // TOOLBAR
    //==========================================================================
    const int toolbarH = s(TOOLBAR_HEIGHT);
    auto toolbar = juce::Rectangle<int>(0, 0, getWidth(), toolbarH);
    versionLabel.setBounds(toolbar.withTrimmedRight(getWidth() - funcRect.getX() - 1)
                                  .withTrimmedLeft(margin));
    versionLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(12.0f)));

    const int iconSz  = s(TOOLBAR_ICON_SIZE);
    const int iconGap = s(TOOLBAR_ICON_GAP);
    const int iconY   = (toolbarH - iconSz) / 2;
    int iconRight = funcRect.getRight();

    settingsButton.setBounds(iconRight - iconSz, iconY, iconSz, iconSz);
    iconRight -= iconSz + iconGap;
    loadButton.setBounds    (iconRight - iconSz, iconY, iconSz, iconSz);
    iconRight -= iconSz + iconGap;
    saveButton.setBounds    (iconRight - iconSz, iconY, iconSz, iconSz);
    iconRight -= iconSz + iconGap;
    // v07.00 — theme toggle (same size as toolbar icons)
    themeToggleButton.setBounds(iconRight - iconSz, iconY, iconSz, iconSz);
    iconRight -= iconSz + iconGap;

    //==========================================================================
    // FUNCTIONS BAR
    //==========================================================================
    auto functionsArea = funcRect;

    auto placeBtn = [&](juce::TextButton& btn, int gap = 0)
    {
        btn.setBounds(functionsArea.removeFromLeft(btnSize));
        if (gap > 0) functionsArea.removeFromLeft(gap);
    };

    placeBtn(recordButton,    withinGap);
    placeBtn(multiplyButton,  betweenGap);
    placeBtn(playButton,      withinGap);
    placeBtn(restartButton,   betweenGap);
    placeBtn(undoButton,      withinGap);
    placeBtn(redoButton,      withinGap);
    placeBtn(clearButton,     betweenGap);
    placeBtn(loopUpButton,    withinGap);
    placeBtn(loopDownButton,  withinGap);
    placeBtn(loopAllButton,   betweenGap);
    placeBtn(countInButton,   withinGap);
    placeBtn(clickTrackButton);

    //==========================================================================
    // METRONOME CARD — v03.00.01
    //==========================================================================
    {
        const int scaledMetroH = s(METRONOME_CARD_H);
        const int cardTop = funcRect.getBottom() + scaledCardGap;
        const int cardX   = margin;
        const int cardW   = sectionW;

        metronomeCardBounds = { cardX, cardTop, cardW, scaledMetroH + accentBarContrib + beatVizContrib };

        const int PANEL_PAD  = s(8);
        const int TOP_PAD    = s(5);
        const int rowH       = scaledMetroH - TOP_PAD * 2;

        const int METRO_BTN_H = s(26);
        const int btnY = cardTop + (scaledMetroH - METRO_BTN_H) / 2;

        const int ON_BTN_W      = METRO_BTN_H;
        const int BPM_VAL_W     = s(64);
        const int BPM_UNIT_W    = s(42);
        const int TAP_BTN_W     = s(52);
        const int INNER_GAP     = s(6);

        const int rowY       = cardTop + TOP_PAD;
        const int innerLeft  = cardX + PANEL_PAD;
        const int innerRight = cardX + cardW - PANEL_PAD;

        metronomeOnOffButton.setBounds(innerLeft, btnY, ON_BTN_W, METRO_BTN_H);

        const int afterBtn = innerLeft + ON_BTN_W + INNER_GAP;

        metronomeLabel.setBounds(afterBtn, rowY, innerRight - afterBtn, rowH);
        metronomeLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(14.0f)));

        bpmValueLabel.setBounds(afterBtn, rowY, BPM_VAL_W, rowH);
        bpmValueLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(28.0f)));
        const int unitH = s(18);
        bpmUnitLabel.setBounds (afterBtn + BPM_VAL_W + s(3),
                                rowY + (rowH - unitH) / 2, BPM_UNIT_W, unitH);
        bpmUnitLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(16.0f)));

        {
            const int TSIG_COMBO_W   = s(72);
            const int CLICK_LABEL_W  = s(52);
            const int CLICK_SLIDER_W = s(90);
            const int SOUND_COMBO_W  = s(110);
            const int CK_GAP         = s(4);

            int rx = innerRight;

            const int TSIG_H = s(32);
            timeSigCombo.setBounds(rx - TSIG_COMBO_W, btnY - (TSIG_H - METRO_BTN_H) / 2, TSIG_COMBO_W, TSIG_H);
            rx -= TSIG_COMBO_W + INNER_GAP;

            tapButton.setBounds(rx - TAP_BTN_W, btnY, TAP_BTN_W, METRO_BTN_H);
            rx -= TAP_BTN_W + INNER_GAP * 2;

            if (showAccentBar)
            {
                soundCombo.setBounds(rx - SOUND_COMBO_W, btnY, SOUND_COMBO_W, METRO_BTN_H);
                rx -= SOUND_COMBO_W + CK_GAP;

                clickVolSlider.setBounds(rx - CLICK_SLIDER_W, btnY, CLICK_SLIDER_W, METRO_BTN_H);
                rx -= CLICK_SLIDER_W + CK_GAP;
            }
            const int clickLblH = s(16);
            clickVolLabel.setBounds(rx - CLICK_LABEL_W,
                                    cardTop + (scaledMetroH - clickLblH) / 2,
                                    CLICK_LABEL_W, clickLblH);
            clickVolLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(10.0f)));
        }
    }

    //==========================================================================
    // BEAT VISUALIZATION CARD — v03.03.01
    //==========================================================================
    if (beatVisualization != nullptr)
    {
        const int beatVizTop = funcRect.getBottom() + scaledCardGap
                             + s(METRONOME_CARD_H) + accentBarContrib;
        beatVisualization->setBounds(margin, beatVizTop, sectionW, s(BEAT_VIZ_H));
        beatVisualization->setShowAccentBars(showAccentBar);
    }

    //==========================================================================
    // LOOP CARDS VIEWPORT
    //==========================================================================
    const int viewportTop = funcRect.getBottom() + scaledCardGap
                          + s(METRONOME_CARD_H) + accentBarContrib + beatVizContrib + scaledCardGap;
    loopCardsViewport.setBounds(margin, viewportTop, sectionW, viewportH);

    loopCardsContainer.setSize(sectionW, contentH);

    for (int i = 0; i < loopCards.size(); ++i)
    {
        int cardTop = i * (scaledCardH + scaledCardGap);
        loopCards[i]->setBounds(0, cardTop, sectionW, scaledCardH);
    }

    //==========================================================================
    // ADD LOOP BUTTON
    //==========================================================================
    const int addBtnTop = viewportTop + viewportH + s(ADD_LOOP_GAP);
    addLoopButton.setBounds(margin, addBtnTop, sectionW, s(ADD_LOOP_BTN_H));

    //==========================================================================
    // STATUS LABEL
    //==========================================================================
    const int statusTop = addBtnTop + s(ADD_LOOP_BTN_H);
    statusLabel.setBounds(0, statusTop, getWidth(), s(STATUS_HEIGHT));
    statusLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(11.0f)));

    //==========================================================================
    // v04.00 — COUNT IN COUNTDOWN OVERLAY
    //==========================================================================
    countInCountdownLabel.setBounds(metronomeCardBounds);
    countInCountdownLabel.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(sf(90.0f)));
    countInCountdownLabel.toFront(true);
}

//==============================================================================
// v04.00 — COUNT IN POPUP (right-click on COUNT IN button)
//==============================================================================
void OrbitalLooperAudioProcessorEditor::showCountInPopup()
{
    auto* content = new juce::Component();
    content->setSize(s(250), s(80));

    auto* title = new juce::Label("", "COUNT IN TIME");
    title->setBounds(s(10), s(10), s(230), s(20));
    title->setFont(juce::Font(sf(14.0f), juce::Font::bold));
    title->setColour(juce::Label::textColourId, juce::Colours::white);
    title->setJustificationType(juce::Justification::centred);
    content->addAndMakeVisible(title);

    auto* field = new juce::Label("", juce::String(static_cast<int>(countInSeconds)) + "s");
    field->setBounds(s(10), s(40), s(230), s(30));
    field->setFont(juce::Font(sf(16.0f)));
    field->setColour(juce::Label::textColourId, juce::Colours::white);
    field->setColour(juce::Label::backgroundColourId, OrbitalLooperLookAndFeel::getPanelColour());
    field->setColour(juce::Label::outlineColourId, juce::Colours::grey);
    field->setJustificationType(juce::Justification::centred);
    field->setEditable(true);
    field->onTextChange = [this, field]
    {
        const int value = juce::jlimit(1, 60,
            field->getText().trim().replace("s", "").getIntValue());
        if (value > 0)
        {
            countInSeconds = static_cast<double>(value);
            audioProcessor.setCountInSeconds(countInSeconds);
            field->setText(juce::String(value) + "s", juce::dontSendNotification);
            countInButton.getProperties().set("countInSeconds", countInSeconds);
            countInButton.repaint();
        }
    };
    content->addAndMakeVisible(field);

    juce::CallOutBox::launchAsynchronously(
        std::unique_ptr<juce::Component>(content),
        countInButton.getScreenBounds(),
        nullptr);
}

//==============================================================================
// TIMER
//==============================================================================
void OrbitalLooperAudioProcessorEditor::timerCallback()
{
    updateButtonStates();
    // Each LoopCardComponent has its own 30 Hz timer for live updates.

    // v03.06.02 — refresh BPM label if MIDI tap changed the BPM externally
    const int currentBPM = audioProcessor.getBPM();
    if (currentBPM != lastDisplayedBPM)
    {
        lastDisplayedBPM = currentBPM;
        bpmValueLabel.setText(juce::String(currentBPM), juce::dontSendNotification);
    }

    // v04.00 — update countdown display (GroundLoop style: silent, visual only)
    if (audioProcessor.isCountInActive())
    {
        const double secsLeft = audioProcessor.getCountInSecondsRemaining();
        const int whole = static_cast<int>(std::ceil(secsLeft));
        const juce::String txt = (whole > 0) ? juce::String(whole) : "GO";
        countInCountdownLabel.setText(txt, juce::dontSendNotification);
        countInCountdownLabel.setVisible(true);
    }
    else if (countInCountdownLabel.isVisible())
    {
        countInCountdownLabel.setText("", juce::dontSendNotification);
        countInCountdownLabel.setVisible(false);
    }

    // v04.00 — fire deferred action when count-in completes
    if (audioProcessor.pollCountInCompleted())
    {
        const int action = audioProcessor.pollPendingTriggerAction();

        if (action == 1)  // start recording
        {
            auto& mc = audioProcessor.getMasterClock();
            for (int i : getTargetLoops())
            {
                auto& eng = audioProcessor.getLoopEngine(i);
                if (!mc.hasMasterCycle())
                {
                    eng.setSyncMode(LoopSyncMode::Free);
                    eng.startRecording();
                }
                else if (audioProcessor.getSyncMode() == LoopSyncMode::Sync)
                {
                    eng.setSyncMode(LoopSyncMode::Sync);
                    eng.armRecord();
                }
                else
                {
                    eng.setSyncMode(LoopSyncMode::Free);
                    eng.startRecording();
                }
            }
            audioProcessor.setBeatCounterActive(true);
            audioProcessor.resetBeatIndex();
            if (beatVisualization != nullptr)
            {
                beatVisualization->startFromBeginning();
                beatVisualization->startAnimation();
            }
        }
        else if (action == 2)  // start playing
        {
            for (int i : getTargetLoops())
                audioProcessor.getLoopEngine(i).startPlayback();
            audioProcessor.setBeatCounterActive(true);
            if (beatVisualization != nullptr)
                beatVisualization->startAnimation();
        }
        updateButtonStates();
    }
}

//==============================================================================
// UPDATE BUTTON STATES
//==============================================================================
void OrbitalLooperAudioProcessorEditor::updateButtonStates()
{
    auto& engine = audioProcessor.getLoopEngine();  // selected loop
    auto  state  = engine.getState();
    const int numLoops = audioProcessor.getNumLoops();

    //--- RECORD/OVERDUB ---
    if (state == LoopState::Recording || state == LoopState::Overdubbing || state == LoopState::RecordArmed)
        recordButton.setColour(juce::TextButton::buttonColourId,
                               OrbitalLooperLookAndFeel::getRecordColour());
    else
        recordButton.setColour(juce::TextButton::buttonColourId,
                               OrbitalLooperLookAndFeel::getStoppedColour());

    //--- PLAY/PAUSE ---
    // "Active" if a loop is playing OR if the beat counter is running (metronome-only mode)
    const bool beatRunning = audioProcessor.getBeatCounterActive();
    const bool showAsPlaying = (state == LoopState::Playing) || beatRunning;
    if (showAsPlaying)
    {
        playButton.setToggleState(true, juce::dontSendNotification);
        playButton.setColour(juce::TextButton::buttonColourId,
                             OrbitalLooperLookAndFeel::getPlayColour());
    }
    else
    {
        playButton.setToggleState(false, juce::dontSendNotification);
        playButton.setColour(juce::TextButton::buttonColourId,
                             OrbitalLooperLookAndFeel::getStoppedColour());
    }

    // Enabled when loop has a recording OR when the metronome is on (metronome-only PLAY)
    bool canPlay = (engine.hasRecording() && (state != LoopState::Recording))
                   || audioProcessor.getMetronomeOn();
    playButton.setEnabled(canPlay);

    //--- RESTART ---
    // Enabled when a loop is active OR when the metronome is on
    restartButton.setEnabled(state == LoopState::Playing
                             || state == LoopState::Paused
                             || audioProcessor.getMetronomeOn());

    //--- MULTIPLY ---
    multiplyButton.setEnabled(engine.canMultiply());

    //--- UNDO ---
    undoButton.setEnabled(engine.canUndo() &&
                          state != LoopState::Recording &&
                          state != LoopState::Overdubbing);

    //--- REDO ---
    redoButton.setEnabled(engine.canRedo() &&
                          state != LoopState::Recording &&
                          state != LoopState::Overdubbing);

    //--- CLEAR ---
    // v03.06.01 — also enabled when metronome or click track is ON (CLEAR resets both)
    clearButton.setEnabled(state == LoopState::Recording    ||
                           state == LoopState::Playing      ||
                           engine.hasRecording()             ||
                           numLoops > 1                      ||
                           audioProcessor.getMetronomeOn()  ||
                           audioProcessor.getClickTrackOn());

    //--- LOOP UP / DOWN — enabled when >1 loop AND LOOP ALL is not active ---
    loopUpButton.setEnabled(numLoops > 1 && !loopAllActive);
    loopDownButton.setEnabled(numLoops > 1 && !loopAllActive);

    //--- LOOP ALL — enabled when >1 loop; lit green when active ---
    loopAllButton.setEnabled(numLoops > 1);
    loopAllButton.setToggleState(loopAllActive, juce::dontSendNotification);
    loopAllButton.setColour(juce::TextButton::buttonColourId,
        loopAllActive ? OrbitalLooperLookAndFeel::getLoopAllColour()
                      : OrbitalLooperLookAndFeel::getStoppedColour());

    //--- ADD LOOP — always enabled (no loop limit) ---
    addLoopButton.setEnabled(true);

    //--- STATUS LABEL ---
    if (state == LoopState::Recording)
    {
        statusLabel.setText("Recording...", juce::dontSendNotification);
    }
    else if (state == LoopState::Overdubbing)
    {
        double secs = engine.getLoopLengthSamples() / audioProcessor.getSampleRate();
        statusLabel.setText("Overdubbing: " + juce::String(secs, 2) + "s loop",
                            juce::dontSendNotification);
    }
    else if (state == LoopState::Playing)
    {
        double secs = engine.getLoopLengthSamples() / audioProcessor.getSampleRate();
        statusLabel.setText("Playing: " + juce::String(secs, 2) + "s loop",
                            juce::dontSendNotification);
    }
    else if (state == LoopState::Paused)
    {
        double secs = engine.getLoopLengthSamples() / audioProcessor.getSampleRate();
        statusLabel.setText("Paused: " + juce::String(secs, 2) + "s loop",
                            juce::dontSendNotification);
    }
    else if (engine.hasRecording())
    {
        double secs = engine.getLoopLengthSamples() / audioProcessor.getSampleRate();
        statusLabel.setText("Stopped: " + juce::String(secs, 2) + "s loop",
                            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("", juce::dontSendNotification);
    }


    // v02.05.01/02 — Refresh all mute/solo button colours
    for (auto* card : loopCards)
        card->updateMuteSoloColours();

    // v03.04.01 — CLICK TRACK button state
    const bool metroOn = audioProcessor.getMetronomeOn();
    const bool clickOn = audioProcessor.getClickTrackOn();

    clickTrackButton.setEnabled(metroOn);
    clickTrackButton.setToggleState(clickOn, juce::dontSendNotification);
    clickTrackButton.setColour(juce::TextButton::buttonColourId,
        !metroOn ? OrbitalLooperLookAndFeel::getDisabledColour()
        : clickOn ? OrbitalLooperLookAndFeel::getClickTrackColour()
                  : OrbitalLooperLookAndFeel::getStoppedColour());

    const bool showClickControls = metroOn && clickOn;
    clickVolLabel.setVisible(showClickControls);
    clickVolSlider.setVisible(showClickControls);
    soundCombo.setVisible(showClickControls);

    // Accent bar state + beat viz mouse interception
    if (beatVisualization != nullptr)
    {
        beatVisualization->setShowAccentBars(showClickControls);
        // Intercept mouse when metronome ON so accent bars can receive clicks
        beatVisualization->setInterceptsMouseClicks(metroOn, false);
    }

    // v04.00 — COUNT IN button: icon shows current seconds (via property), blue when enabled
    countInButton.setEnabled(true);
    countInButton.setButtonText("COUNT\nIN");   // always 2 lines; seconds drawn in icon area
    countInButton.getProperties().set("countInSeconds", countInSeconds);
    countInButton.setColour(juce::TextButton::buttonColourId,
        audioProcessor.getCountInEnabled()
            ? OrbitalLooperLookAndFeel::getPlayColour()
            : OrbitalLooperLookAndFeel::getStoppedColour());
}

//==============================================================================
// v05.00 — SETTINGS WINDOW
//==============================================================================
void OrbitalLooperAudioProcessorEditor::openSettingsWindow()
{
    new SettingsDialog(audioProcessor);   // self-deletes on close
}
