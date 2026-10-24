cmake_minimum_required(VERSION 3.22)

# LUMELODY plugin: the LUMI engine (engine/*.h) as a VST3 instrument (+ AU on macOS, + Standalone app).
# It plays notes from the host's piano roll / MIDI input; every effect control is a host-automatable parameter.
#
#   cmake -S plugin -B build-plugin -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-plugin --config Release
#
# Options: -DLUMI_JUCE_DIR=<path to a JUCE checkout> to build offline; -DLUMI_WEB_UI=OFF for the plain fallback editor.

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary (Apple silicon + Intel)" FORCE)
    endif()
endif()
if(MSVC)
    # static runtime: the plugin does not need the Visual C++ redistributable installed
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(LUMELODY VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(LUMI_WEB_UI "Use the LUMELODY web interface as the plugin window" ON)
option(LUMI_TESTS "Build the headless plugin tests" ON)
set(LUMI_JUCE_DIR "" CACHE PATH "Optional local JUCE checkout")

if(LUMI_JUCE_DIR)
    add_subdirectory(${LUMI_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.8 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(LUMI_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND LUMI_FORMATS AU)
endif()

set(LUMI_NEEDS_WEBVIEW2 FALSE)
if(WIN32 AND LUMI_WEB_UI)
    set(LUMI_NEEDS_WEBVIEW2 TRUE)
endif()

juce_add_plugin(LUMELODY
    COMPANY_NAME "Lunamiverse"
    COMPANY_WEBSITE "https://github.com/Lunamiverse/LUMELODY"
    BUNDLE_ID "com.lunamiverse.lumelody"
    PLUGIN_MANUFACTURER_CODE Lmvs
    PLUGIN_CODE Lmld
    PRODUCT_NAME "LUMELODY"
    DESCRIPTION "Dark melodic instrument with 20 effects"
    FORMATS ${LUMI_FORMATS}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    NEEDS_WEBVIEW2 ${LUMI_NEEDS_WEBVIEW2}
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_add_binary_data(LumiUiData SOURCES
    ui/index.html
    fonts/PixelifySans.ttf
    fonts/Silkscreen-Regular.ttf)
set_target_properties(LumiUiData PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_sources(LUMELODY PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)
target_include_directories(LUMELODY PRIVATE Source ${CMAKE_CURRENT_SOURCE_DIR}/../engine)

target_compile_definitions(LUMELODY PUBLIC
    JUCE_WEB_BROWSER=$<IF:$<BOOL:${LUMI_WEB_UI}>,1,0>
    LUMI_WEB_UI=$<IF:$<BOOL:${LUMI_WEB_UI}>,1,0>
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=$<IF:$<BOOL:${LUMI_NEEDS_WEBVIEW2}>,1,0>)

target_link_libraries(LUMELODY
    PRIVATE
        LumiUiData
        juce::juce_audio_utils
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(LUMI_TESTS)
    # the same processor sources compiled into a console app (plain editor, no web view), driven headlessly
    juce_add_console_app(LumiTests PRODUCT_NAME "LumiTests")
    target_sources(LumiTests PRIVATE Tests/PluginTests.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(LumiTests PRIVATE Source ${CMAKE_CURRENT_SOURCE_DIR}/../engine)
    target_compile_definitions(LumiTests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 LUMI_WEB_UI=0 LUMI_TESTING=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(LumiTests PRIVATE juce::juce_audio_utils juce::juce_gui_extra juce::juce_recommended_config_flags)
endif()
