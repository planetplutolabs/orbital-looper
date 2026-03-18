/*
  ==============================================================================
    LookAndFeel.h

    Orbital Looper v07.00 — LIGHT/DARK THEME

    Square icon+text buttons, resizable window, toolbar strip.
    Supports dark (default) and light colour modes.

    Planet Pluto Effects
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class OrbitalLooperLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrbitalLooperLookAndFeel();

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    // v03.05.01 — ComboBox / popup menu styling
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                           bool isSeparator, bool isActive, bool isHighlighted,
                           bool isTicked, bool hasSubMenu,
                           const juce::String& text,
                           const juce::String& shortcutKeyText,
                           const juce::Drawable* icon,
                           const juce::Colour* textColour) override;

    //==============================================================================
    // THEME MODE — dark (default) or light
    //==============================================================================
    void setDarkMode(bool dark);
    static bool isDark() { return darkMode; }

    //==============================================================================
    // COLOUR SCHEME — routes through darkMode flag
    //==============================================================================
    static juce::Colour getBackgroundColour()    { return darkMode ? juce::Colour(0xff1a1a1a) : juce::Colour(0xFFFFFFFF); }
    static juce::Colour getContainerColour()     { return darkMode ? juce::Colour(0xff252525) : juce::Colour(0xFFF0F0F0); }
    static juce::Colour getPanelColour()         { return darkMode ? juce::Colour(0xff2a2a2a) : juce::Colour(0xFFF0F0F0); }
    static juce::Colour getTextColour()          { return darkMode ? juce::Colours::white     : juce::Colour(0xFF1A1A1A); }
    static juce::Colour getSecondaryTextColour() { return juce::Colour(0xff888888); }
    static juce::Colour getBorderColour()        { return darkMode ? juce::Colour(0xff555555) : juce::Colours::black; }

    // Button state colours
    static juce::Colour getRecordColour()        { return juce::Colour(0xffff005c); }
    static juce::Colour getOverdubColour()       { return juce::Colour(0xffd97706); }
    static juce::Colour getPlayColour()          { return juce::Colour(0xff4a9eff); }
    static juce::Colour getPauseColour()         { return juce::Colour(0xff7c3aed); }
    static juce::Colour getStoppedColour()       { return darkMode ? juce::Colour(0xff2a2a2a) : juce::Colour(0xFFD5D5D5); }

    // Momentary flash colours
    static juce::Colour getMultiplyColour()      { return juce::Colour(0xff8b5cf6); }
    static juce::Colour getUndoColour()          { return juce::Colour(0xff14b8a6); }
    static juce::Colour getRedoColour()          { return juce::Colour(0xff22d3ee); }
    static juce::Colour getClearColour()         { return juce::Colour(0xffb91c1c); }

    // Toggle ON colours
    static juce::Colour getLoopAllColour()       { return juce::Colour(0xff10b981); }
    static juce::Colour getCountInColour()       { return juce::Colour(0xfff59e0b); }
    static juce::Colour getClickTrackColour()    { return juce::Colour(0xffff8c00); }

    // Progress bar / layer dots
    static juce::Colour getProgressGreen()       { return juce::Colour(0xff10b981); }

    // Disabled button colour
    static juce::Colour getDisabledColour()      { return darkMode ? juce::Colour(0xff222222) : juce::Colour(0xFFE0E0E0); }

    //==============================================================================
    // FONTS  — SF Pro Display on macOS
    //==============================================================================
    static juce::Font getSanFranciscoFont(float size)
    {
        #if JUCE_MAC
            return juce::FontOptions("SF Pro Display", size, juce::Font::plain);
        #else
            return juce::Font(juce::FontOptions(size));
        #endif
    }

    // Convenience alias used by toolbar labels etc.
    static juce::Font getLabelFont() { return getSanFranciscoFont(14.0f); }

    //==============================================================================
    // UI METRICS
    //==============================================================================
    static constexpr float CORNER_SIZE    = 6.0f;
    static constexpr int   BUTTON_HEIGHT  = 40;
    static constexpr int   BUTTON_SPACING = 8;
    static constexpr int   MARGIN         = 15;

private:
    static bool darkMode;
    void applyColourScheme();
};
