cmake_minimum_required(VERSION 3.15)
project(track VERSION 0.0.1)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(JUCE)

juce_add_plugin(track
    COMPANY_NAME johnmanjohnston
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE JMJN
    PLUGIN_CODE TRCK
    FORMATS Standalone VST3
    PRODUCT_NAME "track"
)
                                #add_subdirectory(assets)
                                #juce_add_binary_data(jelodyneBinaryData SOURCES 
                                #assets/Inter_24pt-BoldItalic.ttf)

juce_generate_juce_header(track)
target_sources(track PRIVATE
    src/processor.cpp
    src/editor.cpp)

target_compile_definitions(track PRIVATE
    PIP_JUCE_EXAMPLES_DIRECTORY_STRING="${JUCE_SOURCE_DIR}/examples"
    JUCE_ALLOW_STATIC_NULL_VARIABLES=0
    JUCE_CONTENT_SHARING=0
    JUCE_DEMO_RUNNER=0
    JUCE_PLUGINHOST_LV2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_STRICT_REFCOUNTEDPOINTER=0
    JUCE_UNIT_TESTS=0
    JUCE_USE_CAMERA=0
    JUCE_USE_CURL=0
    JUCE_USE_MP3AUDIOFORMAT=0
    JUCE_WEB_BROWSER=0
    # This is a temporary workaround to allow builds to complete on Xcode 15.
    # Add -Wl,-ld_classic to the OTHER_LDFLAGS build setting if you need to
    # deploy to older versions of macOS/iOS.
    JUCE_SILENCE_XCODE_15_LINKER_WARNING=1)

target_link_libraries(track PRIVATE
    #jelodyneBinaryData
    juce::juce_analytics
    juce::juce_animation
    juce::juce_audio_utils
    juce::juce_box2d
    juce::juce_dsp
    juce::juce_opengl
    juce::juce_osc
    juce::juce_product_unlocking
    juce::juce_video
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)
