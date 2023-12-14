cmake_minimum_required(VERSION 3.16)
project(JUCECMakeRepo)

#including CPM.cmake, a package manager:
#https://github.com/TheLartians/CPM.cmake
include(CMake/CPM.cmake)

#list(APPEND CMAKE_PREFIX_PATH "~/libtorch/share/cmake")

#Compile commands, useful for some IDEs like VS-Code
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

#Minimum MacOS target, set globally
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.10" CACHE STRING "Minimum OS X deployment version" FORCE)

# if ON, bults a universal binary for mac (x86_64 and arm64)
# if OFF, builds a native binary for the current architecture
option(UniversalBinary "Build universal binary for mac" OFF)

if (UniversalBinary)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE INTERNAL "")
endif()

#static linking in Windows
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

#Adds all the module sources so they appear correctly in the IDE
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Enable Module Source Groups" ON)


#set any of these to "ON" if you want to build one of the juce examples
#or extras (Projucer/AudioPluginHost, etc):
option(JUCE_BUILD_EXTRAS "Build JUCE Extras" OFF)
option(JUCE_BUILD_EXAMPLES "Build JUCE Examples" OFF)

#Fetching JUCE from git
#IF you want to instead point it to a local version, you can invoke CMake with
#-DCPM_JUCE_SOURCE="Path_To_JUCE"
# use following for Latest JUCE version
CPMAddPackage("gh:juce-framework/JUCE#develop")
# rn I'm using version 7.0.1 on commit # b08520c
# CPMAddPackage("gh:juce-framework/JUCE#b08520c")

#By default, we are building the provided plugin/app examples:
option(BUILD_JUCE_PROTOTYPE_EXAMPLES "Build JUCE prototype examples" ON)

juce_add_modules(shared_plugin_helpers)

if (${BUILD_JUCE_PROTOTYPE_EXAMPLES})
    #adding project folders:
    add_subdirectory(Demos/UnconditionalMidiGeneration)
endif()

add_subdirectory(Deployment/PluginCode)

option(BUILD_UNIT_TESTS "Build JUCE prototype examples" ON)

