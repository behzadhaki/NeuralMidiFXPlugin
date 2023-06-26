project(NeuralMidiFXPlugin VERSION 0.0.1)

set (BaseTargetName NeuralMidiFXPlugin)

# Include the libtorch setup file
include(libtorch_setup.cmake)


juce_add_plugin("${BaseTargetName}"
        # VERSION ...                               # Set this if the plugin version is different to the project version
        # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
        # ICON_SMALL ...
        COMPANY_NAME "MyCompanyTBD"                 # Replace with a tag identifying your name
        IS_SYNTH TRUE                               # There is no MIDI vst3 plugin format, so we are going to assume a midi instrument plugin
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT TRUE
        AU_MAIN_TYPE kAudioUnitType_MIDIProcessor
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE                 # copies the plugin to user plugins folder so as to easily load in DAW
        PLUGIN_MANUFACTURER_CODE Juce                #
        PLUGIN_CODE NMFx                             # MUST BE UNIQUE!! If similar to other plugins, conflicts will occur
        FORMATS AU VST3 Standalone
        PRODUCT_NAME "NeuralMidiFXPlugin")           # Replace with your plugin title

# Include the libtorch setup file
#include(libtorch_setup.cmake)

target_sources(${BaseTargetName} PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DeploymentThreads/InputTensorPreparatorThread.cpp
        Source/DeploymentThreads/ModelThread.cpp
        Source/DeploymentThreads/PlaybackPreparatorThread.cpp
        Source/Includes/colored_cout.cpp
        )

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

find_package(Torch REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

if (MSVC)
    file(GLOB TORCH_DLLS "${TORCH_INSTALL_PREFIX}/lib/*.dll")
    add_custom_command(TARGET ${BaseTargetName}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TORCH_DLLS}
            $<TARGET_FILE_DIR:${BaseTargetName}>)
endif()

target_link_libraries(${BaseTargetName} PRIVATE
        shared_plugin_helpers
        juce_recommended_config_flags
        juce_recommended_lto_flags
        juce_recommended_warning_flags
        ${TORCH_LIBRARIES}
        )

# Add libtorch headers to the include directories
target_include_directories(${BaseTargetName} PRIVATE "${TORCH_INCLUDE_DIRS}")