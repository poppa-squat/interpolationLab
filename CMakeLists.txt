cmake_minimum_required(VERSION 3.22)
cmake_policy(SET CMP0135 NEW)

project(InterpolationLab VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CTest)
include("${CMAKE_SOURCE_DIR}/cmake/Dependencies.cmake")

add_library(interpolation_lab_warnings INTERFACE)
target_compile_options(interpolation_lab_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>)

add_library(interpolation_lab_dsp STATIC
    src/dsp/InterpolatorCatalog.cpp
    src/dsp/methods/FadeInterpolator.cpp)
target_include_directories(interpolation_lab_dsp PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(interpolation_lab_dsp
    PUBLIC
        juce::juce_dsp
        juce::juce_recommended_config_flags
        interpolation_lab_warnings)
target_compile_definitions(interpolation_lab_dsp
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

juce_add_plugin(InterpolationLab
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "interpolationLab"
    BUNDLE_ID "dev.interpolationlab.plugin"
    PLUGIN_MANUFACTURER_CODE InLb
    PLUGIN_CODE Iplb
    FORMATS VST3 Standalone
    PRODUCT_NAME "Interpolation Lab"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES Fx)

target_sources(InterpolationLab PRIVATE
    src/plugin/ParameterIds.h
    src/plugin/PluginEditor.cpp
    src/plugin/PluginEditor.h
    src/plugin/PluginProcessor.cpp
    src/plugin/PluginProcessor.h)

target_include_directories(InterpolationLab PRIVATE "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(InterpolationLab
    PRIVATE
        interpolation_lab_dsp
        juce::juce_audio_utils
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        interpolation_lab_warnings)
target_compile_definitions(InterpolationLab
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
