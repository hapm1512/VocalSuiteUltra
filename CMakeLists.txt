cmake_minimum_required(VERSION 3.22)

project(VocalSuiteUltraPro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(JUCE_DIR "D:/JUCE8" CACHE PATH "Path to JUCE 8")

add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(VocalSuiteUltraPro
    COMPANY_NAME "Hai Pham"
    BUNDLE_ID "com.haipham.vocalsuiteultrapro"
    PLUGIN_MANUFACTURER_CODE Haip
    PLUGIN_CODE Vsup
    FORMATS VST3 Standalone
    PRODUCT_NAME "VocalSuiteUltraPro"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(VocalSuiteUltraPro
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp

        Source/Parameters/ParameterLayout.h
        Source/Parameters/ParameterLayout.cpp

        Source/GUI/Components/GlassPanel.h
        Source/GUI/Components/GlassPanel.cpp

        Source/GUI/Buttons/LedButton.h
        Source/GUI/Buttons/LedButton.cpp

        Source/GUI/Knobs/ProKnob.h
        Source/GUI/Knobs/ProKnob.cpp

        Source/GUI/LookAndFeel/ProLookAndFeel.h
        Source/GUI/LookAndFeel/ProLookAndFeel.cpp
)

target_include_directories(VocalSuiteUltraPro
    PRIVATE
        Source
        Source/Parameters
        Source/GUI/Components
        Source/GUI/Buttons
        Source/GUI/Knobs
        Source/GUI/LookAndFeel
)

target_compile_definitions(VocalSuiteUltraPro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(VocalSuiteUltraPro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(MSVC)
    target_compile_options(VocalSuiteUltraPro PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
endif()
