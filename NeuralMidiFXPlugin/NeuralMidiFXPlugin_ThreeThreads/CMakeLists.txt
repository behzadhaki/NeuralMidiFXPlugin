project(NMFx_ThreeThreads VERSION 0.0.1)

set (BaseTargetName NMFx_ThreeThreads)

add_definitions(-DPROJECT_NAME="${BaseTargetName}")

# --------------------------------------------
#---- Torch Installation ---------------------

set(TORCH_VERSION 2.0.0)

# Libtorch Installation Path for Windows and MacOS
if(MSVC)
    set(LIBTORCH_INSTALL_DIR "C:/libtorch-${TORCH_VERSION}-${CMAKE_BUILD_TYPE}")
    find_package(Torch HINTS ${LIBTORCH_INSTALL_DIR})
else()
    set(LIBTORCH_INSTALL_DIR "$ENV{HOME}/Library/libtorch-${TORCH_VERSION}-${CMAKE_BUILD_TYPE}")
    find_package(Torch HINTS "${LIBTORCH_INSTALL_DIR}/libtorch")
endif()

if(NOT Torch_FOUND)

    # Check if we're running on Windows
    if(MSVC)

        # if directory exists, delete it
        if(EXISTS ${LIBTORCH_INSTALL_DIR})
            message(STATUS "Deleting existing libtorch directory...")
            file(REMOVE_RECURSE ${LIBTORCH_INSTALL_DIR})
        endif()

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Downloading libtorch (DEBUG) for Windows...")
            set(TORCH_FILE libtorch-win-shared-with-deps-debug-${TORCH_VERSION}%2Bcpu.zip)
        else()
            message(STATUS "Downloading libtorch (RELEASE) for Windows...")
            set(TORCH_FILE libtorch-win-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip)
        endif()

        file(DOWNLOAD
                https://download.pytorch.org/libtorch/cpu/${TORCH_FILE}
                ${CMAKE_BINARY_DIR}/${TORCH_FILE}
                SHOW_PROGRESS
                )

        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${CMAKE_BINARY_DIR}/${TORCH_FILE}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                )

        # Move the extracted libtorch directory to the desired location
        file(RENAME ${CMAKE_BINARY_DIR}/libtorch ${LIBTORCH_INSTALL_DIR})

            set(Torch_DIR ${LIBTORCH_INSTALL_DIR}/libtorch/share/cmake/Torch)
        find_package(Torch REQUIRED)

        # Add Torch to the include directories
        include_directories(${TORCH_INCLUDE_DIRS})

        # Add Torch libraries to the linker flags
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${TORCH_LINKER_FLAGS}")

    else()
        message(STATUS "Downloading libtorch for MacOS...")
        set(TORCH_FILE libtorch-cxx11-abi-shared-with-deps-${TORCH_VERSION}%2Bcpu.zip)
        file(DOWNLOAD
                https://download.pytorch.org/libtorch/cpu/libtorch-macos-${TORCH_VERSION}.zip
                ${CMAKE_BINARY_DIR}/${TORCH_FILE}
                SHOW_PROGRESS
                )



        # Create the directory if it does not exist
        if(NOT EXISTS "${LIBTORCH_INSTALL_DIR}")
            file(MAKE_DIRECTORY "${LIBTORCH_INSTALL_DIR}")
        endif()

        # Extract the zip file
        execute_process(COMMAND unzip -q ${TORCH_FILE} -d "${LIBTORCH_INSTALL_DIR}"
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                )

        # Add Torch to the include directories
        include_directories(${TORCH_INCLUDE_DIRS})

    endif()

    set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/libtorch)
endif()

# ----------------------------------------------

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
        PRODUCT_NAME "NMFx_ThreeThreads")           # Replace with your plugin title

# Include the libtorch setup file
#include(libtorch_setup.cmake)

target_sources(${BaseTargetName} PRIVATE
        ../../Source/NeuralMidiFXPlugin/PluginProcessor.cpp
        ../../Source/NeuralMidiFXPlugin/PluginEditor.cpp
        ../../Source/DeploymentThreads/InputTensorPreparatorThread.cpp
        ITP_Deploy.cpp
        ../../Source/DeploymentThreads/ModelThread.cpp
        MDL_Deploy.cpp
        ../../Source/DeploymentThreads/PlaybackPreparatorThread.cpp
        PPP_Deploy.cpp
        ../../Source/Includes/colored_cout.cpp
        )

target_compile_definitions(${BaseTargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

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


# Copying the TorchScripts directory and all its contents
if(WIN32)
    file(COPY "${CMAKE_SOURCE_DIR}/TorchScripts" DESTINATION "C:/${BaseTargetName}")
    add_definitions(-DDEFAULT_MODEL_DIR="C:\\${BaseTargetName}\\TorchScripts\\Models")
    add_definitions(-DDEFAULT_PROCESSING_SCRIPTS_DIR="C:\\${BaseTargetName}\\TorchScripts\\ProcessingScripts")
elseif(APPLE)
    set(TARGET_DIR "$ENV{HOME}/Library/${BaseTargetName}")
    file(COPY "${CMAKE_SOURCE_DIR}/TorchScripts" DESTINATION "${TARGET_DIR}")
    add_definitions(-DDEFAULT_MODEL_DIR="${TARGET_DIR}/TorchScripts/Models")
    add_definitions(-DDEFAULT_PROCESSING_SCRIPTS_DIR="${TARGET_DIR}/TorchScripts/ProcessingScripts")
endif()

# Provide a path to settings.json file
# Define DEFAULT_SETTINGS_FILE_PATH
set(DEFAULT_SETTINGS_PATH "${CMAKE_SOURCE_DIR}/NeuralMidiFxPlugin/NeuralMidiFxPlugin_ThreeThreads/settings.json")

# No need for platform-specific path formatting
add_definitions(-DDEFAULT_SETTINGS_FILE_PATH="${DEFAULT_SETTINGS_PATH}")
