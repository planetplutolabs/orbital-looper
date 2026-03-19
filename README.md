# Orbital Looper

A multi-loop audio looper plugin with MIDI foot controller integration.

Orbital Looper is a DAW plugin (AU/VST3) that turns your recording workflow into a live looping instrument. Record, overdub, multiply, and arrange multiple loops in real time, controlled from your DAW or a MIDI foot controller.

Built by [Planet Pluto Effects](https://planetplutolabs.github.io/planet-pluto-effects/).

---

## Download

Download the latest release from the [Releases](https://github.com/planetplutolabs/orbital-looper/releases) page.

---

## Features

- **Multi-loop engine** with unlimited loops and layers
- **Per-loop volume and pan** with real-time sliders
- **Overdub** layering with multi-level undo/redo
- **Multiply** to extend loop length on the fly
- **Mute / Solo** per loop
- **Loop All** mode to broadcast commands to every loop at once
- **Variable loop selection** via Cmd+click and Shift+click
- **Built-in metronome** with configurable BPM (40-300) and time signatures
- **Click track** with built-in woodblock or custom WAV/AIFF sounds
- **Beat visualization** with accent bar editing
- **Count-in** with visual countdown overlay
- **Sync mode** with master clock and loop multipliers (1/4x to 4x)
- **Full MIDI mapping** with 22 assignable functions and MIDI learn
- **Session save/load** (.ols format)
- **Light and dark themes**
- **Resizable UI** that scales proportionally
- **AU and VST3** formats

---

## System Requirements

- A DAW that supports AU or VST3 plugins (macOS, Windows, Linux)
- Built and tested on macOS with Reaper. Windows and Linux builds are provided but community-tested.

---

## Installation

Copy the plugin to the appropriate system directory for your platform:

**macOS:**

| Format | Install Path |
|--------|-------------|
| AU | `~/Library/Audio/Plug-Ins/Components/` |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/` |


**Windows:**

| Format | Install Path |
|--------|-------------|
| VST3 | `C:\Program Files\Common Files\VST3\` |

**Linux:**

| Format | Install Path |
|--------|-------------|
| VST3 | `~/.vst3/` |

*Note: AU format is macOS only. Windows and Linux use VST3.*

Rescan plugins in your DAW, then add Orbital Looper to an audio track.

**AU Validation (macOS only):**

```bash
auval -a | grep "Planet Pluto"
auval -v aufx Orb1 PPfx
```

---

## Quick Start

1. Add the plugin to an audio track in your DAW.
2. Press **Record** to capture your first loop. Press Record again to stop and begin playback.
3. Press **Record while playing** to overdub a new layer.
4. Press **Add Loop** to create additional loops. Use **Loop Up / Loop Down** to navigate.
5. Open **Settings** (gear icon) to configure MIDI mapping, metronome, sync mode, and more.

---

## Interface Overview

The plugin interface is divided into five areas from top to bottom.

### Toolbar

| Element | Description |
|---------|-------------|
| Version label | Current version |
| Light/Dark | Toggle between themes |
| Save | Save current session |
| Load | Load a saved session |
| Settings | Open the settings panel |

### Function Buttons

Two rows of buttons that control loop operations. These operate on the selected loop, or all loops when Loop All is active.

**Transport:** Record, Multiply, Play/Pause, Restart

**Edit:** Undo, Redo, Clear

**Navigation:** Loop Up, Loop Down, Loop All

**Utility:** Count In, Click Track

### Metronome Card

A horizontal bar with metronome controls (visible when metronome is on). Shows the on/off toggle, BPM value, tap tempo button, and time signature selector. When the metronome is off, the card collapses to a simple label.

### Beat Visualization

A 4-bar grid that animates in time with the metronome. The current beat glows, visited beats fill with the accent color, and unvisited beats remain dark. When the click track is on, an accent bar strip appears above the grid where you can toggle which beats receive an accent click.

### Loop Cards

Each loop displays as a card in a scrollable viewport containing:

- Loop name (editable), Mute/Solo buttons, layer dots, layer count, delete button
- Current position and total loop length
- Progress bar showing playback position
- Volume slider (0-100) and pan slider (L50 to R50, center = C)

### Status Bar

A narrow bar at the bottom showing the current loop state.

### Resizable Window

Drag any edge or corner to resize. Minimum 600x300, maximum 1536x1200. All elements scale proportionally.

---

## Core Operations

### Recording

Press **Record** to begin capturing audio. Audio passes through to the output during recording. Press Record again to stop. Playback begins automatically. The first recording on a loop sets the loop length. In Sync mode, recording is quantized to bar boundaries.

### Overdubbing

Press **Record** while a loop is playing to overdub. New audio mixes on top of existing content. Each overdub creates a new layer that can be independently undone.

### Playback and Pause

**Play/Pause** toggles playback. When paused, the position is preserved. Press again to resume.

### Restart

**Restart** jumps playback back to position zero while continuing to play.

### Multiply

**Multiply** extends the loop length while continuing to record into the extended region.

### Undo / Redo

**Undo** removes the most recent overdub layer. **Redo** restores a layer that was undone. Both support multiple levels. Stacks are independent per loop.

### Clear

**Clear** erases the current loop entirely, returning it to the stopped state. This cannot be undone.

---

## Multi-Loop

### Adding and Deleting

Press **Add Loop** to create a new empty loop. Click the **x** button on a card to delete it. If only one loop remains, deleting resets it to an empty state.

### Selecting Loops

- **Click** a loop card to select it
- **Cmd+click** to toggle individual selections
- **Shift+click** to select a range
- **Loop Up / Loop Down** to move selection by one

Function buttons operate on all selected loops.

### Loop All

Press **Loop All** to toggle broadcast mode. When active (green), button presses affect every loop simultaneously.

### Mute and Solo

Each card has M (Mute) and S (Solo) buttons, visible when multiple loops exist. Mute silences a loop without stopping it. Solo silences all other loops. Multiple loops can be soloed at once.

---

## Volume and Pan

Each loop has independent volume and pan on its card.

**Volume:** 0 to 100 (maps to 0.0 to 1.0 gain). Default 100. Double-click to reset. Controllable via MIDI CC.

**Pan:** -50 (full left) to +50 (full right). Center displays "C". Default center. Double-click to reset. Controllable via MIDI CC.

---

## Metronome and Click Track

### Metronome

Provides a visual and optional audible tempo reference. BPM range is 40 to 300. Click the BPM value to type a new number, or use the TAP button to set tempo by tapping. Time signature is selectable from a dropdown (numerator range 1 to 32).

### Click Track

When on, you hear an audible click on each beat. The built-in sound is a synthesized woodblock (~50ms decay). Accented beats play at 1200 Hz, normal beats at 800 Hz. You can load a custom WAV or AIFF file to replace the built-in click. Volume is adjustable in Settings or via MIDI CC.

### Accent Bars

When the click track is on, a row of bars appears above the beat grid. Click any bar to toggle its accent state. Accented beats play the louder click. By default, the downbeat of each measure is accented. Supports up to 32 beats.

---

## Count-In

Count-in provides a countdown before recording or playback begins.

1. Enable in Settings (Count In section)
2. Set the duration (0.5 to 60 seconds, default 4 seconds)
3. Press Record or Play
4. A large countdown appears ("4", "3", "2", "1", "GO")
5. After the countdown, recording or playback begins

---

## Sync Mode

### Free Mode (default)

Loops can be any length. Recording starts and stops immediately. No quantization.

### Sync Mode

The first loop establishes the master cycle. Subsequent loops are quantized to the master cycle length. Recording is armed when you press Record and begins at the next bar boundary. Loop lengths snap to multipliers: 1/4x, 1/2x, 1x, 2x, 4x.

### Master Clock

The master clock tracks the global tempo and provides beat/bar positions for quantization, set by the metronome BPM and time signature.

---

## MIDI Control

Orbital Looper supports full MIDI control for hands-free operation with a foot controller or any MIDI device.

**No MIDI channel is configured by default.** Open Settings to set your MIDI channel and assignments before MIDI will respond. All 22 functions support Note or CC mapping, and every assignment is configurable via the settings panel or MIDI learn.

### Available Functions

| # | Function | Description |
|---|----------|-------------|
| 1 | Record / Overdub | Toggle recording or overdub |
| 2 | Multiply | Extend loop length |
| 3 | Play / Pause | Toggle playback |
| 4 | Restart | Jump to loop start |
| 5 | Undo | Remove last overdub layer |
| 6 | Redo | Restore undone layer |
| 7 | Clear | Erase current loop |
| 8 | Loop Volume | Current loop volume (CC) |
| 9 | Loop Pan | Current loop pan (CC) |
| 10 | Add Loop | Create new loop |
| 11 | Mute | Toggle mute on current loop |
| 12 | Solo | Toggle solo on current loop |
| 13 | Quantize/Free | Toggle sync mode |
| 14 | Loop Up | Select next loop |
| 15 | Loop Down | Select previous loop |
| 16 | Loop All | Toggle broadcast mode |
| 17 | Delete Loop | Delete current loop |
| 18 | Metronome On/Off | Toggle metronome |
| 19 | Tap Tempo | Tap to set BPM |
| 20 | Click Track On/Off | Toggle click track |
| 21 | Click Volume | Click track volume (CC) |
| 22 | Count In On/Off | Toggle count-in |

### MIDI Learn

1. Open Settings and scroll to the MIDI section
2. Click **Learn** next to the function you want to assign
3. Send a MIDI note or CC from your controller
4. The assignment updates automatically

A 50ms debounce window prevents duplicate triggers from mechanical switches.

---

## Settings Reference

Open Settings by clicking the gear icon in the toolbar. The panel is scrollable with collapsible sections.

**Theme:** Dark/Light toggle. Persists across sessions. Also available from the toolbar.

**Session:** Save/Load all settings (MIDI mapping, BPM, mode, limits) to a .ols file.

**Setup:** Single-Track/Multi-Track toggle.

**Metronome:** BPM (40-300), time signature selector.

**Click Track:** On/Off, volume slider (0-100), sound selection (built-in woodblock or custom WAV/AIFF).

**Count In:** On/Off, duration (0.5-60 seconds).

**Sync Mode:** Free/Sync toggle.

**MIDI:** All 22 functions listed with type (Note/CC), channel, value, and Learn button.

**Limits:** Max loops and max layers (or unlimited for each).

**Global Defaults:** Set as Default (save current settings for new instances), Restore Defaults (load saved defaults), Reset to Factory (reset everything to factory values).

---

## Default Settings

| Setting | Default |
|---------|---------|
| BPM | 120 |
| Time Signature | 4/4 |
| Metronome | Off |
| Click Track | Off |
| Count-In | Off |
| Sync Mode | Free |
| Loop Volume | 100% |
| Loop Pan | Center |
| Theme | Dark |
| MIDI Channel | None (configure in Settings) |

---

## Session Management

### Saving and Loading

Click **Save** in the toolbar to save the current session as a .ols file containing all loop audio, states, MIDI mapping, metronome settings, and configuration. Click **Load** to restore a saved session completely.

### Global Defaults

Global defaults are stored locally and applied when the plugin first loads, before DAW project state is restored. Set your preferred settings via Settings > Global Defaults > Set as Default.

### DAW Project State

The plugin automatically saves and restores its complete state (including audio) with the DAW project. No manual save/load is needed for normal DAW workflow.

---

## Troubleshooting

**Plugin not showing in DAW:** Verify the plugin file is in the correct directory. Rescan plugins in your DAW preferences. On macOS, run `auval -v aufx Orb1 PPfx` to check AU validation.

**No audio input:** Check your track's input routing, verify your audio interface is selected, and ensure the track is record-armed.

**MIDI not responding:** Open Settings and verify a MIDI channel is configured. Check that your MIDI device is connected and recognized. Use MIDI Learn to match your controller's actual values.

**Clicks or pops:** Increase your DAW's audio buffer size. Close CPU-intensive applications.

**Loop timing drifts:** Enable Sync mode for quantized loop boundaries. Set the metronome BPM before recording.

**Settings not persisting:** Use Settings > Global Defaults > Set as Default for cross-project preferences. DAW project state saves automatically.

---

## Building from Source

Requires JUCE 7 and Xcode.

```bash
# Build AU
xcodebuild -project "Builds/MacOSX/OrbitalLooper.xcodeproj" \
  -scheme "Orbital Looper - AU" \
  -configuration Release build

# Build VST3
xcodebuild -project "Builds/MacOSX/OrbitalLooper.xcodeproj" \
  -scheme "Orbital Looper - VST3" \
  -configuration Release build
```

---

## Technical Reference

### Architecture

```
Source/
  PluginProcessor.h/cpp    Audio processing, state management, MIDI handling
  PluginEditor.h/cpp       UI components, layout, user interaction
  Core/
    LoopEngine.h/cpp       Per-loop audio engine, recording, playback, undo/redo
    MasterClock.h/cpp      Global tempo, beat tracking, quantization
    ClickPlayer.h/cpp      Click track synthesis and playback
  UI/
    LookAndFeel.h/cpp      Visual styling, custom component drawing
    SettingsWindow.h/cpp   Settings panel UI
```

### Loop States

| State | Description |
|-------|-------------|
| Stopped | No audio content, or loop at position zero |
| RecordArmed | Waiting for bar boundary to begin recording (Sync mode) |
| Recording | Actively capturing audio input |
| Playing | Looping playback, wrapping seamlessly |
| Paused | Playback frozen, position preserved |
| Overdubbing | Recording new audio while playing existing content |

### Audio Processing

- Sample rate and buffer size set by the DAW
- Stereo processing (2 channels)
- All loops mixed to a single stereo output
- Volume applied as linear gain (0.0 to 1.0)
- Pan uses constant-power panning
- Audio input passes through during recording

### File Format

Session files (.ols) use JUCE binary serialization containing all loop audio buffers, states, positions, MIDI mapping, metronome settings, and configuration. Global defaults use JSON format.

### Plugin Identity

| Property | Value |
|----------|-------|
| Name | Orbital Looper |
| Manufacturer | Planet Pluto Effects |
| Manufacturer Code | PPfx |
| Plugin Code | Orb1 |
| Bundle ID | com.planetplutoeffects.OrbitalLooper |
| Type | Audio Effect (aufx) |
| Formats | AU, VST3 |

---

## License

Copyright 2026 Planet Pluto Labs. All rights reserved. See [LICENSE](LICENSE) for details.

---

## Support

[![Buy Me a Coffee](https://img.shields.io/badge/Buy%20Me%20a%20Coffee-support-yellow?logo=buymeacoffee)](https://buymeacoffee.com/planetplutolabs)

Your support keeps the Quantum Gamma Fusion Drive running.

---

## Contact

- Email: planetplutolabs@gmail.com
- GitHub: [github.com/planetplutolabs](https://github.com/planetplutolabs)

---

Planet Pluto Effects — The Sound Lab of Tomorrow.
