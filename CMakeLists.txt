cmake_minimum_required(VERSION 3.22)
project(OrbitalLooper VERSION 1.0.0)

if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} juce_build)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_plugin(OrbitalLooper
    COMPANY_NAME "Planet Pluto Effects"
    PLUGIN_MANUFACTURER_CODE PPfx
    PLUGIN_CODE Orb1
    FORMATS AU VST3
    PRODUCT_NAME "Orbital Looper"
    BUNDLE_ID com.planetplutoeffects.OrbitalLooper
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

# Generate JuceHeader.h so existing source files with #include "JuceHeader.h" compile
juce_generate_juce_header(OrbitalLooper)

target_sources(OrbitalLooper
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Core/LoopEngine.cpp
        Source/Core/MasterClock.cpp
        Source/Core/ClickPlayer.cpp
        Source/UI/LookAndFeel.cpp
        Source/UI/SettingsWindow.cpp)

target_include_directories(OrbitalLooper
    PRIVATE
        Source
        Source/Core
        Source/UI
        JuceLibraryCode)

target_compile_definitions(OrbitalLooper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(OrbitalLooper
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
