project(NMFx_SingleThread VERSION 0.0.1)

set (BaseTargetName NMFx_SingleThread)

add_definitions(-DPROJECT_NAME="${BaseTargetName}")

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
        PRODUCT_NAME "NMFx_SingleThread")           # Replace with your plugin title




# ==============================================================================
# DO NOT EDIT BELOW THIS LINE
# ==============================================================================
target_sources(${BaseTargetName} PRIVATE
        ../../shared_plugin_helpers/shared_plugin_helpers.cpp
        ../../Source/NeuralMidiFXPlugin/PluginProcessor.cpp
        ../../Source/NeuralMidiFXPlugin/processBlock.cpp
        ../../Source/NeuralMidiFXPlugin/PluginEditor.cpp
        ../../Source/DeploymentThreads/DeploymentThread.cpp
        ../../Source/Includes/colored_cout.cpp
        Deploy.cpp
        settings.json
        )

# Provide a path to settings.json file
# Define DEFAULT_SETTINGS_FILE_PATH
set(DEFAULT_SETTINGS_PATH "${CMAKE_SOURCE_DIR}/Deployment/PluginCode/settings.json")
message(STATUS "DEFAULT_SETTINGS_PATH: ${DEFAULT_SETTINGS_PATH}")

# No need for platform-specific path formatting
add_definitions(-DDEFAULT_SETTINGS_FILE_PATH="${DEFAULT_SETTINGS_PATH}")

include("../../CMakeLists.wrapper.txt")
