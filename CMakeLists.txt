cmake_minimum_required(VERSION 3.22)
project(EpsilonFET VERSION 0.2.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "")

# JUCE: -DJUCE_DIR=/path/to/a full JUCE 9.0.5 checkout, otherwise it is downloaded.
# (The JUCE/ folder in this project holds only the modules, for Projucer.)
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.5 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(EF_FORMATS AU VST3 Standalone)
else()
    set(EF_FORMATS VST3 Standalone)
endif()

juce_add_plugin(EpsilonFET
    COMPANY_NAME "Epsilon Sounds"
    PRODUCT_NAME "Epsilon FET"
    VERSION 0.2.0
    PLUGIN_MANUFACTURER_CODE Epsl
    PLUGIN_CODE EpFt
    FORMATS ${EF_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect)

target_sources(EpsilonFET PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(EpsilonFET PRIVATE Source)
target_compile_definitions(EpsilonFET PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(EpsilonFET
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Tests and the manual's pictures (not part of the plugin):  -DEF_BUILD_TESTS=ON
option(EF_BUILD_TESTS "Build plugin tests and the screenshot tool" OFF)
if(EF_BUILD_TESTS)
    foreach(tool EpsilonPluginTest EpsilonShots)
        juce_add_console_app(${tool} PRODUCT_NAME "${tool}")
        target_include_directories(${tool} PRIVATE Source)
        target_compile_definitions(${tool} PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
            JucePlugin_Name="Epsilon FET" JucePlugin_VersionString="0.2.0" JucePlugin_Build_Standalone=0)
        target_link_libraries(${tool} PRIVATE juce::juce_dsp juce::juce_audio_processors juce::juce_audio_formats juce::juce_gui_basics
            PUBLIC juce::juce_recommended_config_flags)
    endforeach()
    target_sources(EpsilonPluginTest PRIVATE tests/PluginTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_sources(EpsilonShots PRIVATE tests/snapshot/Shots.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
endif()
