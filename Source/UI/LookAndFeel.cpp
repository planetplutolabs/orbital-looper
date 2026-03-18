/*
  ==============================================================================
    LookAndFeel.cpp

    Orbital Looper v07.00 — LIGHT/DARK THEME

    Square icon+text buttons. Icons drawn procedurally with JUCE Graphics.
    Icon logic ported from GroundLoop LookAndFeel.cpp.

    Planet Pluto Effects
  ==============================================================================
*/

#include "LookAndFeel.h"

bool OrbitalLooperLookAndFeel::darkMode = true;

OrbitalLooperLookAndFeel::OrbitalLooperLookAndFeel()
{
    applyColourScheme();
}

void OrbitalLooperLookAndFeel::setDarkMode(bool dark)
{
    darkMode = dark;
    applyColourScheme();
}

void OrbitalLooperLookAndFeel::applyColourScheme()
{
    setColour(juce::ResizableWindow::backgroundColourId, getBackgroundColour());
    setColour(juce::TextButton::buttonColourId,          getPanelColour());
    setColour(juce::TextButton::textColourOffId,         getTextColour());
    setColour(juce::TextButton::textColourOnId,          getTextColour());
    setColour(juce::Label::textColourId,                 getTextColour());
    setColour(juce::Label::backgroundColourId,           juce::Colours::transparentBlack);
    setColour(juce::ComboBox::backgroundColourId,        getPanelColour());
    setColour(juce::ComboBox::textColourId,              getTextColour());
    setColour(juce::ComboBox::outlineColourId,           getBorderColour());
    setColour(juce::ComboBox::arrowColourId,             getSecondaryTextColour());
    setColour(juce::PopupMenu::backgroundColourId,                getPanelColour());
    setColour(juce::PopupMenu::textColourId,                      getTextColour());
    setColour(juce::PopupMenu::highlightedBackgroundColourId,     getPlayColour().withAlpha(0.2f));
    setColour(juce::PopupMenu::highlightedTextColourId,           getTextColour());
}

void OrbitalLooperLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                                     juce::Button& button,
                                                     const juce::Colour& /*backgroundColour*/,
                                                     bool shouldDrawButtonAsHighlighted,
                                                     bool shouldDrawButtonAsDown)
{
    // Toolbar icon buttons (SAVE, LOAD, SETTINGS) — identified by their label.
    // They get no background or border; just a subtle highlight/press effect.
    const auto label = button.getButtonText();
    const bool isToolbarIcon = (label == "SAVE" || label == "LOAD" || label == "SETTINGS"
                                || label == "DARK" || label == "LIGHT");

    if (isToolbarIcon)
    {
        if (shouldDrawButtonAsDown)
        {
            // Faint circular press indicator
            auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
            g.setColour(getTextColour().withAlpha(0.08f));
            g.fillRoundedRectangle(bounds, CORNER_SIZE);
        }
        // No background, no border — icon is the button
        return;
    }

    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    auto buttonColour = button.findColour(juce::TextButton::buttonColourId);

    if (shouldDrawButtonAsDown)
        buttonColour = buttonColour.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        buttonColour = buttonColour.brighter(0.1f);

    g.setColour(buttonColour);
    g.fillRoundedRectangle(bounds, CORNER_SIZE);

    g.setColour(darkMode ? buttonColour.brighter(0.2f) : getBorderColour());
    g.drawRoundedRectangle(bounds, CORNER_SIZE, 1.0f);
}

//==============================================================================
// drawButtonText - draws icon in top half, label text in bottom half
// Icon is chosen by button text label.
//==============================================================================
void OrbitalLooperLookAndFeel::drawButtonText(juce::Graphics& g,
                                               juce::TextButton& button,
                                               bool /*shouldDrawButtonAsHighlighted*/,
                                               bool /*shouldDrawButtonAsDown*/)
{
    const float alpha = button.isEnabled() ? 1.0f : 0.4f;
    g.setColour(button.findColour(juce::TextButton::textColourOffId).withMultipliedAlpha(alpha));

    const auto label = button.getButtonText();
    const auto bounds = button.getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Split button vertically: top 55% for icon, bottom 45% for text
    const float iconAreaH = h * 0.55f;
    const float textAreaY = iconAreaH;
    const float textAreaH = h - iconAreaH;

    // Icon centre is centred horizontally, vertically centred in icon area
    const juce::Point<float> centre(w * 0.5f, iconAreaH * 0.5f);

    // Radius scales with the smaller of width or icon area height
    const float radius = juce::jmin(w, iconAreaH) * 0.38f;

    //==========================================================================
    // ICON DRAWING
    //==========================================================================
    if (label == "REC/\nDUB" || label == "OVERDUB")
    {
        // Outline circle with centred filled dot
        float circleRadius = radius * 0.85f;
        float dotRadius    = circleRadius * 0.35f;

        g.drawEllipse(centre.x - circleRadius, centre.y - circleRadius,
                      circleRadius * 2.0f, circleRadius * 2.0f, 2.5f);
        g.fillEllipse(centre.x - dotRadius, centre.y - dotRadius,
                      dotRadius * 2.0f, dotRadius * 2.0f);
    }
    else if (label == "MULTIPLY\n")
    {
        // 3×3 grid of dots
        float dotRadius = radius * 0.14f;
        float spacing   = radius * 0.55f;

        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
            {
                float x = centre.x + (col - 1) * spacing;
                float y = centre.y + (row - 1) * spacing;
                g.fillEllipse(x - dotRadius, y - dotRadius,
                              dotRadius * 2.0f, dotRadius * 2.0f);
            }
    }
    else if (label == "PLAY/\nPAUSE")
    {
        // Left: play triangle  |  Right: pause bars
        float iconW = radius * 1.4f;

        // Play triangle
        juce::Path tri;
        float triCX = centre.x - iconW * 0.3f;
        tri.addTriangle(triCX - radius * 0.3f, centre.y - radius * 0.45f,
                        triCX - radius * 0.3f, centre.y + radius * 0.45f,
                        triCX + radius * 0.4f, centre.y);
        g.fillPath(tri);

        // Pause bars
        float pCX    = centre.x + iconW * 0.3f;
        float barW   = radius * 0.16f;
        float barH   = radius * 0.9f;
        float barGap = radius * 0.28f;
        g.fillRoundedRectangle(pCX - barGap,          centre.y - barH * 0.5f,
                               barW, barH, radius * 0.08f);
        g.fillRoundedRectangle(pCX + barGap - barW,   centre.y - barH * 0.5f,
                               barW, barH, radius * 0.08f);
    }
    else if (label == "RESTART\n")
    {
        // Vertical line + left-pointing triangle
        juce::Path line;
        float lineX = centre.x - radius * 0.6f;
        line.startNewSubPath(lineX, centre.y - radius * 0.75f);
        line.lineTo        (lineX, centre.y + radius * 0.75f);
        g.strokePath(line, juce::PathStrokeType(2.5f));

        juce::Path tri;
        tri.addTriangle(centre.x - radius * 0.6f,  centre.y,
                        centre.x + radius * 0.45f, centre.y - radius * 0.65f,
                        centre.x + radius * 0.45f, centre.y + radius * 0.65f);
        g.fillPath(tri);
    }
    else if (label == "UNDO\n")
    {
        // Counterclockwise arc (0° → 270°) with left-pointing arrowhead
        juce::Path arc;
        float arcR = radius * 0.6f;
        arc.addCentredArc(centre.x, centre.y, arcR, arcR,
                          0.0f, 0.0f,
                          juce::MathConstants<float>::pi * 1.5f,
                          true);
        g.strokePath(arc, juce::PathStrokeType(2.5f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));

        // Arrowhead at 270° — larger (was 0.35f)
        float arrowX = centre.x + arcR * std::cos(juce::MathConstants<float>::pi * 1.5f);
        float arrowY = centre.y + arcR * std::sin(juce::MathConstants<float>::pi * 1.5f);
        float as = radius * 0.55f;
        juce::Path head;
        head.addTriangle(arrowX - as * 0.7f, arrowY,
                         arrowX + as * 0.3f, arrowY - as * 0.6f,
                         arrowX + as * 0.3f, arrowY + as * 0.6f);
        g.fillPath(head);
    }
    else if (label == "REDO\n")
    {
        // Clockwise arc (180° → 450°) with right-pointing arrowhead
        juce::Path arc;
        float arcR = radius * 0.6f;
        arc.addCentredArc(centre.x, centre.y, arcR, arcR,
                          0.0f,
                          juce::MathConstants<float>::pi,
                          juce::MathConstants<float>::pi * 2.5f,
                          true);
        g.strokePath(arc, juce::PathStrokeType(2.5f,
                          juce::PathStrokeType::curved,
                          juce::PathStrokeType::rounded));

        // Arrowhead at 450° = -90° — larger (was 0.35f)
        float arrowX = centre.x + arcR * std::cos(juce::MathConstants<float>::pi * 2.5f);
        float arrowY = centre.y + arcR * std::sin(juce::MathConstants<float>::pi * 2.5f);
        float as = radius * 0.55f;
        juce::Path head;
        head.addTriangle(arrowX + as * 0.7f, arrowY,
                         arrowX - as * 0.3f, arrowY - as * 0.6f,
                         arrowX - as * 0.3f, arrowY + as * 0.6f);
        g.fillPath(head);
    }
    else if (label == "CLEAR\n")
    {
        // Circle outline with X inside
        float cr = radius * 0.7f;
        g.drawEllipse(centre.x - cr, centre.y - cr, cr * 2.0f, cr * 2.0f, 2.0f);

        float xr = cr * 0.55f;
        juce::Path x;
        x.startNewSubPath(centre.x - xr, centre.y - xr);
        x.lineTo         (centre.x + xr, centre.y + xr);
        x.startNewSubPath(centre.x + xr, centre.y - xr);
        x.lineTo         (centre.x - xr, centre.y + xr);
        g.strokePath(x, juce::PathStrokeType(2.0f,
                        juce::PathStrokeType::mitered,
                        juce::PathStrokeType::rounded));
    }
    else if (label == "LOOP\nUP")
    {
        // Up arrow
        float ar = radius * 0.75f;
        juce::Path arrow;
        arrow.addTriangle(centre.x,          centre.y - ar,
                          centre.x - ar * 0.65f, centre.y,
                          centre.x + ar * 0.65f, centre.y);
        g.fillPath(arrow);

        juce::Path stem;
        stem.startNewSubPath(centre.x, centre.y);
        stem.lineTo(centre.x, centre.y + ar * 0.55f);
        g.strokePath(stem, juce::PathStrokeType(2.5f));
    }
    else if (label == "LOOP\nDOWN")
    {
        // Down arrow
        float ar = radius * 0.75f;
        juce::Path arrow;
        arrow.addTriangle(centre.x,              centre.y + ar,
                          centre.x - ar * 0.65f, centre.y,
                          centre.x + ar * 0.65f, centre.y);
        g.fillPath(arrow);

        juce::Path stem;
        stem.startNewSubPath(centre.x, centre.y);
        stem.lineTo(centre.x, centre.y - ar * 0.55f);
        g.strokePath(stem, juce::PathStrokeType(2.5f));
    }
    else if (label == "LOOP\nALL")
    {
        // Checkmark
        float s = radius * 0.7f;
        juce::Path check;
        check.startNewSubPath(centre.x - s,        centre.y);
        check.lineTo         (centre.x - s * 0.2f, centre.y + s * 0.7f);
        check.lineTo         (centre.x + s,         centre.y - s * 0.65f);
        g.strokePath(check, juce::PathStrokeType(2.5f,
                            juce::PathStrokeType::curved,
                            juce::PathStrokeType::rounded));
    }
    else if (label == "COUNT\nIN")
    {
        // Read seconds from component property (set by editor when value changes)
        auto& props = button.getProperties();
        const double secs = props.contains("countInSeconds")
            ? static_cast<double>(props["countInSeconds"]) : 4.0;
        const juce::String secStr = juce::String(static_cast<int>(secs)) + "s";
        g.setFont(juce::Font(juce::FontOptions(radius * 1.4f, juce::Font::bold)));
        g.drawText(secStr,
                   (int)(centre.x - radius), (int)(centre.y - radius),
                   (int)(radius * 2.0f), (int)(radius * 2.0f),
                   juce::Justification::centred, false);
    }
    else if (label == "CLICK\nTRACK")
    {
        // Musical note: filled oval note head + vertical stem + flag
        float noteHeadRX = radius * 0.38f;
        float noteHeadRY = radius * 0.28f;
        float stemH      = radius * 1.1f;

        // Note head — slightly tilted oval, lower-left of centre
        float headCX = centre.x - radius * 0.1f;
        float headCY = centre.y + radius * 0.45f;

        juce::Path noteHead;
        juce::AffineTransform tilt = juce::AffineTransform::rotation(-0.4f, headCX, headCY);
        juce::Path ovalPath;
        ovalPath.addEllipse(headCX - noteHeadRX, headCY - noteHeadRY,
                            noteHeadRX * 2.0f, noteHeadRY * 2.0f);
        g.fillPath(ovalPath, tilt);

        // Stem — vertical line up from right edge of note head
        float stemX = headCX + noteHeadRX * 0.85f;
        float stemTop = headCY - stemH;
        juce::Path stem;
        stem.startNewSubPath(stemX, headCY);
        stem.lineTo         (stemX, stemTop);
        g.strokePath(stem, juce::PathStrokeType(2.0f));

        // Flag — curved stroke from stem top going right and down
        juce::Path flag;
        flag.startNewSubPath(stemX, stemTop);
        flag.cubicTo(stemX + radius * 0.55f, stemTop + radius * 0.2f,
                     stemX + radius * 0.6f,  stemTop + radius * 0.5f,
                     stemX + radius * 0.2f,  stemTop + radius * 0.75f);
        g.strokePath(flag, juce::PathStrokeType(2.0f,
                           juce::PathStrokeType::curved,
                           juce::PathStrokeType::rounded));
    }
    else if (label == "DARK")
    {
        // Toolbar icon: crescent moon for dark mode
        const juce::Point<float> ic(w * 0.5f, h * 0.5f);
        const float ir = juce::jmin(w, h) * 0.36f;

        // Fill full circle, then erase a shifted circle to leave a crescent
        g.setColour(getSecondaryTextColour());
        g.fillEllipse(ic.x - ir, ic.y - ir, ir * 2.0f, ir * 2.0f);

        g.setColour(getBackgroundColour());
        g.fillEllipse(ic.x - ir * 0.3f, ic.y - ir * 1.1f, ir * 1.8f, ir * 1.8f);
        return;
    }
    else if (label == "LIGHT")
    {
        // Toolbar icon: sun for light mode
        const juce::Point<float> ic(w * 0.5f, h * 0.5f);
        const float ir = juce::jmin(w, h) * 0.36f;
        g.setColour(getSecondaryTextColour());

        // Sun: central circle + rays
        const float coreR = ir * 0.45f;
        g.fillEllipse(ic.x - coreR, ic.y - coreR, coreR * 2.0f, coreR * 2.0f);

        const float rayInner = ir * 0.65f;
        const float rayOuter = ir * 0.95f;
        for (int i = 0; i < 8; ++i)
        {
            float angle = (float)i * juce::MathConstants<float>::twoPi / 8.0f;
            float x0 = ic.x + rayInner * std::cos(angle);
            float y0 = ic.y + rayInner * std::sin(angle);
            float x1 = ic.x + rayOuter * std::cos(angle);
            float y1 = ic.y + rayOuter * std::sin(angle);
            juce::Path ray;
            ray.startNewSubPath(x0, y0);
            ray.lineTo(x1, y1);
            g.strokePath(ray, juce::PathStrokeType(1.5f, juce::PathStrokeType::mitered,
                                                    juce::PathStrokeType::rounded));
        }
        return;
    }
    else if (label == "SAVE")
    {
        // Toolbar icon: draw centred in full button, use blue colour
        const juce::Point<float> ic(w * 0.5f, h * 0.5f);
        const float ir = juce::jmin(w, h) * 0.36f;
        g.setColour(getPlayColour());  // #4A9EFF blue

        float s  = ir * 0.85f;
        float x0 = ic.x - s, y0 = ic.y - s;
        float sz = s * 2.0f;

        g.drawRoundedRectangle(x0, y0, sz, sz, 2.0f, 2.0f);

        float notchW = sz * 0.35f, notchH = sz * 0.3f;
        juce::Path notch;
        notch.startNewSubPath(x0 + sz - notchW, y0);
        notch.lineTo(x0 + sz - notchW, y0 + notchH);
        notch.lineTo(x0 + sz,           y0 + notchH);
        g.strokePath(notch, juce::PathStrokeType(1.5f));

        float labelY = y0 + sz * 0.55f, labelH = sz * 0.35f;
        g.drawRect(x0 + sz * 0.15f, labelY, sz * 0.7f, labelH, 1.5f);
        return;  // no text for toolbar icons
    }
    else if (label == "LOAD")
    {
        // Toolbar icon: folder, use green colour
        const juce::Point<float> ic(w * 0.5f, h * 0.5f);
        const float ir = juce::jmin(w, h) * 0.36f;
        g.setColour(getProgressGreen());  // #10B981 green

        float s  = ir * 0.75f;
        float bx = ic.x - s,        by = ic.y - s * 0.3f;
        float bw = s * 2.0f,         bh = s * 1.3f;
        float tx = ic.x - s * 0.5f, ty = ic.y - s;
        float tw = s * 1.2f,         th = s * 0.55f;

        g.drawRoundedRectangle(tx, ty, tw, th, 2.0f, 1.5f);
        g.drawRoundedRectangle(bx, by, bw, bh, 2.0f, 1.5f);
        return;
    }
    else if (label == "SETTINGS")
    {
        // Toolbar icon: hamburger (v05.00: white when enabled)
        const juce::Point<float> ic(w * 0.5f, h * 0.5f);
        const float ir = juce::jmin(w, h) * 0.36f;
        g.setColour(getTextColour());

        float lw = ir * 1.1f, lh = ir * 0.12f, gap = ir * 0.4f;
        for (int i = -1; i <= 1; ++i)
            g.fillRoundedRectangle(ic.x - lw * 0.5f,
                                   ic.y + i * gap - lh * 0.5f,
                                   lw, lh, lh * 0.5f);
        return;
    }
    else if (label == "M" || label == "S")
    {
        // Mute / Solo buttons — large centred single letter fills the whole button.
        // Font size ≈ 85% of button height for a comfortable fit at 24×24px.
        const float fontSize = juce::jmin(w, h) * 0.85f;
        g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(fontSize));
        g.setColour(button.findColour(juce::TextButton::textColourOffId)
                         .withMultipliedAlpha(alpha));
        g.drawFittedText(label,
                         0, 0, (int)w, (int)h,
                         juce::Justification::centred, 1);
        return;
    }
    else if (label == "ON" || label == "OFF" || label == "TAP")
    {
        // Metronome card small buttons — proportional to button height
        g.setFont(OrbitalLooperLookAndFeel::getSanFranciscoFont(juce::jmax(8.0f, h * 0.54f)));
        g.drawFittedText(label,
                         2, 0, (int)w - 4, (int)h,
                         juce::Justification::centred, 1);
        return;
    }
    else if (label == juce::CharPointer_UTF8("\xE2\x9C\x95"))  // ✕ delete button
    {
        // Full-button ✕ — drawn to fill the whole button, no icon/text split.
        // Colour: danger red when enabled, dimmed when disabled.
        const juce::Colour xColour = button.isEnabled()
                                     ? getClearColour().brighter(0.4f)
                                     : getSecondaryTextColour();
        g.setColour(xColour);

        // Draw two diagonal lines forming an ✕, inset slightly from the button edge
        const float inset  = juce::jmin(w, h) * 0.22f;
        const float x0     = inset;
        const float y0     = inset;
        const float x1     = w - inset;
        const float y1     = h - inset;
        const float stroke = juce::jmax(1.5f, juce::jmin(w, h) * 0.12f);

        juce::Path cross;
        cross.startNewSubPath(x0, y0);
        cross.lineTo(x1, y1);
        cross.startNewSubPath(x1, y0);
        cross.lineTo(x0, y1);
        g.strokePath(cross, juce::PathStrokeType(stroke,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        return;
    }
    else if (label == "ADD LOOP")
    {
        // Full-width button: '+' icon and "ADD LOOP" text centred together as a group
        const float vertCentre = h * 0.5f;
        const float iconR      = h * 0.22f;
        const float barW       = iconR * 2.0f;
        const float barH       = iconR * 0.4f;
        const float gap        = iconR * 0.9f;

        // Use the same font helper as the rest of the plugin
        auto textFont = OrbitalLooperLookAndFeel::getSanFranciscoFont(juce::jmax(10.0f, h * 0.42f));
        const float textW  = textFont.getStringWidthFloat("ADD LOOP");
        const float groupW = barW + gap + textW;
        const float iconCX = (w - groupW) * 0.5f + barW * 0.5f;  // centre of '+' within group

        g.setColour(getPlayColour().withMultipliedAlpha(alpha));
        // Horizontal bar of '+'
        g.fillRoundedRectangle(iconCX - barW * 0.5f, vertCentre - barH * 0.5f,
                               barW, barH, barH * 0.5f);
        // Vertical bar of '+'
        g.fillRoundedRectangle(iconCX - barH * 0.5f, vertCentre - barW * 0.5f,
                               barH, barW, barH * 0.5f);

        // "ADD LOOP" text immediately to the right of the icon
        g.setFont(textFont);
        g.drawText("ADD LOOP",
                   (int)(iconCX + barW * 0.5f + gap), 0,
                   (int)(textW + 2.0f), (int)h,
                   juce::Justification::centredLeft, false);
        return;
    }
    // else: unknown label — no icon drawn

    //==========================================================================
    // LABEL TEXT (bottom 45% of button) — always exactly 2 lines
    //==========================================================================
    if (textAreaH > 8.0f)
    {
        // Reset colour (icon drawing may have changed font)
        g.setColour(button.findColour(juce::TextButton::textColourOffId)
                         .withMultipliedAlpha(alpha));

        // Split label on \n into line1 / line2.
        // Labels without \n render on line 1, line 2 is empty.
        // This guarantees exact placement — no reliance on drawFittedText word-wrap.
        juce::String line1, line2;
        int newlinePos = label.indexOfChar('\n');
        if (newlinePos >= 0)
        {
            line1 = label.substring(0, newlinePos);
            line2 = label.substring(newlinePos + 1);
        }
        else
        {
            line1 = label;
            line2 = {};
        }

        // Each line gets half the text area height
        const float lineH    = textAreaH * 0.5f;
        const float fontSize = juce::jmax(8.0f, lineH * 0.72f);
        g.setFont(juce::Font(juce::FontOptions(fontSize)));

        const int tx = 2;
        const int tw = (int)w - 4;

        // Line 1 — top half of text area
        if (line1.isNotEmpty())
            g.drawFittedText(line1,
                             tx, (int)textAreaY,
                             tw, (int)lineH,
                             juce::Justification::centred, 1);

        // Line 2 — bottom half of text area
        if (line2.isNotEmpty())
            g.drawFittedText(line2,
                             tx, (int)(textAreaY + lineH),
                             tw, (int)lineH,
                             juce::Justification::centred, 1);
    }
}

//==============================================================================
// v03.05.01 — ComboBox / popup menu styling
//==============================================================================

void OrbitalLooperLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                             bool isButtonDown,
                                             int, int, int, int, juce::ComboBox& box)
{
    // v03.06.01 — read per-component colours so timeSigCombo can be blue
    //             while soundCombo stays dark (panel colour default)
    const auto bgCol     = box.findColour(juce::ComboBox::backgroundColourId);
    const auto borderCol = box.findColour(juce::ComboBox::outlineColourId);

    const auto b = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height);
    g.setColour(isButtonDown ? bgCol.darker(0.2f) : bgCol);
    g.fillRoundedRectangle(b, CORNER_SIZE);
    g.setColour(borderCol);
    g.drawRoundedRectangle(b.reduced(0.5f), CORNER_SIZE, 1.0f);

    // Inverted triangle ▼ — right-aligned, vertically centred
    const float triSz = 5.0f;
    const float tx    = (float)width - 14.0f;
    const float ty    = ((float)height - triSz * 0.65f) * 0.5f;
    juce::Path tri;
    tri.addTriangle(tx, ty,  tx + triSz, ty,  tx + triSz * 0.5f, ty + triSz * 0.65f);
    g.setColour(box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(tri);
}

juce::Font OrbitalLooperLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    return getSanFranciscoFont(juce::jmax(8.0f, box.getHeight() * 0.46f));
}

void OrbitalLooperLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int w, int h)
{
    const auto b = juce::Rectangle<float>(0.0f, 0.0f, (float)w, (float)h);
    g.setColour(getPanelColour());
    g.fillRoundedRectangle(b, CORNER_SIZE);
    g.setColour(getBorderColour());
    g.drawRoundedRectangle(b.reduced(0.5f), CORNER_SIZE, 1.0f);
}

void OrbitalLooperLookAndFeel::drawPopupMenuItem(
    juce::Graphics& g, const juce::Rectangle<int>& area,
    bool isSeparator, bool, bool isHighlighted, bool isTicked, bool,
    const juce::String& text, const juce::String&,
    const juce::Drawable*, const juce::Colour*)
{
    if (isSeparator)
    {
        g.setColour(getBorderColour());
        g.drawLine((float)area.getX() + 4.0f, (float)area.getCentreY(),
                   (float)area.getRight() - 4.0f, (float)area.getCentreY(), 1.0f);
        return;
    }

    if (isHighlighted)
    {
        g.setColour(getPlayColour().withAlpha(0.18f));
        g.fillRoundedRectangle(area.toFloat().reduced(2.0f, 1.0f), 3.0f);
    }

    if (isTicked)
    {
        // Small check mark to the left of the text
        const float cx = (float)area.getX() + 8.0f;
        const float cy = (float)area.getCentreY();
        juce::Path tick;
        tick.startNewSubPath(cx, cy);
        tick.lineTo(cx + 3.0f, cy + 3.0f);
        tick.lineTo(cx + 8.0f, cy - 4.0f);
        g.setColour(getPlayColour());
        g.strokePath(tick, juce::PathStrokeType(1.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setFont(getSanFranciscoFont(12.0f));
    g.setColour(isHighlighted ? getTextColour()
                              : getSecondaryTextColour().brighter(0.3f));
    const int textX = area.getX() + (isTicked ? 20 : 8);
    g.drawFittedText(text, textX, area.getY(),
                     area.getWidth() - textX - 4, area.getHeight(),
                     juce::Justification::centredLeft, 1);
}
