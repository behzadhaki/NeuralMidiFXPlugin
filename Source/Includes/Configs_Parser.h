//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "json.hpp"

using json = nlohmann::json;

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// -----------------  DO NOT CHANGE THE FOLLOWING -----------------
#if defined(_WIN32) || defined(_WIN64)
inline std::string settings_json_path = TOSTRING(DEFAULT_SETTINGS_FILE_PATH);
#else
inline std::string settings_json_path = TOSTRING(DEFAULT_SETTINGS_FILE_PATH);
#endif

// ======================================================================================
// ==================     UI Settings                      ==============================
// ======================================================================================

/*using slider_json = json;
using rotary_json = json;
using button_json = json;
using hslider_json = json;
using comboBox_json = json;*/

// label, allowToDragInMidi, allowToDragOutAsMidi, needs_playhead, topLeftCorner, bottomRightCorner
//using midiDisplay_json = std::tuple<std::string, bool, bool, bool, std::string, std::string>;

using slider_list = std::vector<json>;
using rotary_list = std::vector<json>;
using button_list = std::vector<json>;
using hslider_list = std::vector<json>;
using comboBox_list = std::vector<json>;
using midiDisplay_list = std::vector<json>;
using audioDisplay_list = std::vector<json>;
using labels_list = std::vector<json>;
using lines_list = std::vector<json>;
using triangleSliders_list = std::vector<json>;

using tab_tuple = std::tuple<
    std::string, slider_list, rotary_list, button_list, hslider_list,
    comboBox_list, midiDisplay_list, audioDisplay_list, labels_list,
    lines_list, triangleSliders_list>;

inline json load_settings_json() {

    std::string filePath = DEFAULT_SETTINGS_FILE_PATH;  // Use the predefined macro

     std::ifstream file(filePath);
     if (!file.is_open()) {
         std::cerr << "Error opening JSON file: " << filePath << std::endl;
         throw std::runtime_error("Failed to open JSON file");
     }

     json j;
     try {
         file >> j;
     } catch (const std::exception& e) {
         std::cerr << "Error parsing JSON file: " << e.what() << std::endl;
         throw;
     }

     return j;
}

inline json loaded_json = load_settings_json();

// ---------------------------------------------------------------------------------
inline std::vector<tab_tuple> parse_to_tabList() {

    std::vector<tab_tuple> tabList;

    // check if tabList exists
    auto tabs_json = loaded_json["UI"]["Tabs"];


    for (const auto& tabJson: tabs_json["tabList"]) {
         std::string tabName = tabJson["name"].get<std::string>();
        slider_list tabSliders;
        rotary_list tabRotaries;
        button_list tabButtons;
        hslider_list tabhsliders;
        comboBox_list tabComboBoxes;
        midiDisplay_list tabMidiDisplays;
        audioDisplay_list tabAudioDisplays;
        labels_list tabLabels;
        lines_list tabLines;
        triangleSliders_list tabtriangleSliders;

        // check if sliders exist
        if (tabJson.contains("sliders")) {
            for (const auto& sliderJson: tabJson["sliders"]) {
                // check if slider is vertical or horizontal
                // vertical if it has no "horizontal" key or if it is false
                if (!sliderJson.contains("horizontal") || !sliderJson["horizontal"].get<bool>()) {
                    tabSliders.push_back(sliderJson);
                } else {
                    tabhsliders.push_back(sliderJson);
                }
            }
        }

        // check if rotaries exist
        if (tabJson.contains("rotaries")) {
            for (const auto& rotaryJson: tabJson["rotaries"]) {
                tabRotaries.push_back(rotaryJson);
            }
        }

        // check if buttons exist
        if (tabJson.contains("buttons")) {
            for (const auto& buttonJson: tabJson["buttons"]) {
                tabButtons.push_back(buttonJson);
            }
        }

        // check if comboBoxes exist
        if (tabJson.contains("comboBoxes")) {
            for (const auto& comboBoxJson: tabJson["comboBoxes"]) {
                tabComboBoxes.push_back(comboBoxJson);
            }
        }

        if (tabJson.contains("MidiDisplays")) {
            for (const auto& midiDisplayJson: tabJson["MidiDisplays"]) {
                tabMidiDisplays.push_back(midiDisplayJson);
            }
        }

        if (tabJson.contains("AudioDisplays")) {
            for (const auto& audioDisplayJson: tabJson["AudioDisplays"]) {
                tabAudioDisplays.push_back(audioDisplayJson);
            }
        }

        if (tabJson.contains("labels")) {
            for (const auto& labelJson: tabJson["labels"]) {
                tabLabels.push_back(labelJson);
            }
        }

        if (tabJson.contains("lines")) {
            for (const auto& lineJson: tabJson["lines"]) {
                tabLines.push_back(lineJson);
            }
        }

        if (tabJson.contains("triangleSliders")) {
            for (const auto& triangleSlidersJson: tabJson["triangleSliders"]) {
                tabtriangleSliders.push_back(triangleSlidersJson);
            }
        }
        tab_tuple tabTuple = {tabName, tabSliders, tabRotaries, tabButtons, tabhsliders, tabComboBoxes, tabMidiDisplays, tabAudioDisplays, tabLabels, tabLines, tabtriangleSliders};
        tabList.push_back(tabTuple);

    }

    return tabList;
}

// GUI settings
namespace UIObjects {

    const bool user_resizable = loaded_json["UI"].contains("resizable") ? loaded_json["UI"]["resizable"].get<bool>() : false;
    const bool user_maintain_aspect_ratio = loaded_json["UI"].contains("maintain_aspect_ratio") ? loaded_json["UI"]["maintain_aspect_ratio"].get<bool>() : false;
    const int user_width = loaded_json["UI"].contains("width") ? loaded_json["UI"]["width"].get<int>() : 800;
    const int user_height = loaded_json["UI"].contains("height") ? loaded_json["UI"]["height"].get<int>() : 600;

    namespace Tabs {
        const bool show_grid = loaded_json["UI"]["Tabs"]["show_grid"];
        const bool draw_borders_for_components = loaded_json["UI"]["Tabs"]["draw_borders_for_components"];
        const std::vector<tab_tuple> tabList = parse_to_tabList();
    }

    namespace MidiInVisualizer {
        // if you need the widget used for visualizing midi notes coming from host
        // set following to true
        const bool enable = loaded_json["UI"]["MidiInVisualizer"]["enable"];

        // if you want to allow user to drag/drop midi files into the plugin
        // set following to true
        // If active, the content of the midi file will be visualized in the
        // MidiInVisualizer and also be provided to you in the DeploymentThread
        const bool allowToDragInMidi = loaded_json["UI"]["MidiInVisualizer"]["allowToDragInMidi"];

        // if you want to visualize notes received in real-time from host
        // set following to true
        const bool visualizeIncomingMidiFromHost = loaded_json["UI"]["MidiInVisualizer"]["visualizeIncomingMidiFromHost"];
        // if playhead is manually moved backward, do you want to delete all the
        // previously visualized notes received from host?
        const bool deletePreviousIncomingMidiMessagesOnBackwardPlayhead = loaded_json["UI"]["MidiInVisualizer"]["deletePreviousIncomingMidiMessagesOnBackwardPlayhead"];
        // if playback is stopped, do you want to delete all the previously
        // visualized notes received from host?
        const bool deletePreviousIncomingMidiMessagesOnRestart = loaded_json["UI"]["MidiInVisualizer"]["deletePreviousIncomingMidiMessagesOnRestart"];
    }

    namespace GeneratedContentVisualizer
    {
        // if you need the widget used for visualizing generated midi notes
        // set following to true
        // The content here visualizes the playbackSequence as it is at any given time
        // (remember that playbackSequence can be changed by the user within the
        // PlaybackPreparatorThread)
        const bool enable = loaded_json["UI"]["GeneratedContentVisualizer"]["enable"];

        // if you want to allow the user to drag out the visualized content,
        // set following to true
        const bool allowToDragOutAsMidi = loaded_json["UI"]["GeneratedContentVisualizer"]["allowToDragOutAsMidi"];
    }

    namespace StandaloneTransportPanel
    {
        // if you need the widget used for controlling the standalone transport
        // set following to true
        const bool enable = loaded_json["StandaloneTransportPanel"]["enable"];
        const bool disableInPluginMode = loaded_json["StandaloneTransportPanel"]["disableInPluginMode"];
        // if you need to send midi out to a virtual midi cable
        // set following to true
        // NOTE: Only works on MacOs
        const bool NeedVirtualMidiOutCable = loaded_json["VirtualMidiOut"]["enable"];
    }


}

// ======================================================================================
// ==================        EventFromHost Communication Settings        ================
// ======================================================================================
/*
 * You can send Events to DPL at different frequencies with different
 *      intentions, depending on the use case intended. The type and frequency of providing
 *      these events is determined below.
 *
 *      Examples:
 *      1. If you need these information at every buffer, then set
 *
 *      >> constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
 *      >> constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{false};
 *
 *      2. Alternatively, you may want to access these information,
 *      only when the metadata changes. In this case:
 *
 *      >> constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
 *      >> constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{false};
 *
 *      3. If you need these information at every bar, then:
 *
 *      >> constexpr bool SendNewBarEvents_FLAG{true};
 *
 *      4. If you need these information at every specific time shift periods,
 *      you can specify the time shift as a ratio of a quarter note. For instance,
 *      if you want to access these information at every 8th note, then:
 *
 *      >>  constexpr bool SendTimeShiftEvents_FLAG{false};
 *      >>  constexpr double delta_TimeShiftEventRatioOfQuarterNote{0.5};
 *
 *      5. If you only need this information, whenever a new midi message
 *      (note on/off or cc) is received, you don't need to set any of the above,
 *      since the metadata is embedded in any midi EventFromHost. Just remember to not
 *      filter out the midi events.
 *
 *      >>  constexpr bool FilterNoteOnEvents_FLAG{false};
 *      and/or
 *      >>  constexpr bool FilterNoteOffEvents_FLAG{false};
 *      and/orzX
 *      >>  constexpr bool FilterCCEvents_FLAG{false};
 *
 */
namespace event_communication_settings {
// set to true, if you need to send the metadata for a new buffer to the DPL thread
const bool SendEventAtBeginningOfNewBuffers_FLAG{
    loaded_json["event_communication_settings"]["SendEventAtBeginningOfNewBuffers_FLAG"]};
const bool SendEventForNewBufferIfMetadataChanged_FLAG{
    loaded_json["event_communication_settings"]["SendEventForNewBufferIfMetadataChanged_FLAG"]
};     // only sends if metadata changes

// set to true if you need to notify the beginning of a new bar
const bool SendNewBarEvents_FLAG{
    loaded_json["event_communication_settings"]["SendNewBarEvents_FLAG"]};

// set to true EventFromHost for every time_shift_event ratio of quarter notes
const bool SendTimeShiftEvents_FLAG{
    loaded_json["event_communication_settings"]["SendTimeShiftEvents_FLAG"]};
const double delta_TimeShiftEventRatioOfQuarterNote{
    loaded_json["event_communication_settings"]["delta_TimeShiftEventRatioOfQuarterNote"]
}; // sends a time shift event every 8th note

// Filter Note On Events if you don't need them
const bool FilterNoteOnEvents_FLAG{
    loaded_json["event_communication_settings"]["FilterNoteOnEvents_FLAG"]};

// Filter Note Off Events if you don't need them
const bool FilterNoteOffEvents_FLAG{
    loaded_json["event_communication_settings"]["FilterNoteOffEvents_FLAG"]};

// Filter CC Events if you don't need them
const bool FilterCCEvents_FLAG{
    loaded_json["event_communication_settings"]["FilterCCEvents_FLAG"]};
};


// ======================================================================================
// ==================       Thread  Settings                  ============================
// ======================================================================================
namespace thread_configurations::SingleMidiThread {
// wait time between iterations in ms
const double waitTimeBtnIters{
    loaded_json["deploy_method_min_wait_time_between_iterations"]};
}

namespace thread_configurations::APVTSMediatorThread {
// wait time between iterations in ms
const double waitTimeBtnIters{1};
}
// ======================================================================================
// ==================       QUEUE  Settings                  ============================
// ======================================================================================
/* specifies the max number of elements that can be stored in the queue
 *  if the queue is full, the producer thread will overwrite the oldest element
 */
namespace queue_settings {
constexpr int NMP2DPL_que_size{512};    // same as NMP2DPL que size
    // same as NMP2DPL que size
constexpr int DPL2NMP_que_size{512};    // same as DPL2NMP que size
    // same as DPL2NMP que size
constexpr int APVM_que_size{4};    // same as APVM que size
    // same as APVM que size
};


// ==============================================================================================
// ==================       Debugging  Settings                  ================================
// ==============================================================================================
namespace debugging_settings::DeploymentThread {
const bool print_received_gui_params{
    loaded_json["debugging_settings"]["DeploymentThread"]["print_received_gui_params"]};                // print the received gui parameters
const bool print_manually_dropped_midi_messages{
    loaded_json["debugging_settings"]["DeploymentThread"]["print_manually_dropped_midi_messages"]
};     // print the midi messages received from manually drag-dropped midi file
const bool print_input_events{
    loaded_json["debugging_settings"]["DeploymentThread"]["print_input_events"]};                        // print the input events
const bool print_deploy_method_time{
    loaded_json["debugging_settings"]["DeploymentThread"]["print_deploy_method_time"]};                    // print the time taken to deploy the model
const bool disable_user_print_requests{
    loaded_json["debugging_settings"]["DeploymentThread"]["disable_user_print_requests"]};                // disable all user requested prints
}

namespace debugging_settings::ProcessorThread {
const bool print_start_stop_times{
    loaded_json["debugging_settings"]["ProcessorThread"]["print_start_stop_times"]};                    // print start and stop times of the thread
const bool print_new_buffer_started{
    loaded_json["debugging_settings"]["ProcessorThread"]["print_new_buffer_started"]};                    // print the new buffer started
const bool print_generation_policy_reception{
    loaded_json["debugging_settings"]["ProcessorThread"]["print_generation_policy_reception"]};            // print generation policy received from DPL
const bool print_generation_stream_reception{
    loaded_json["debugging_settings"]["ProcessorThread"]["print_generation_stream_reception"]};            // print generation stream received from DPL
const bool disableAllPrints{
    loaded_json["debugging_settings"]["ProcessorThread"]["disableAllPrints"]};                            // disable all prints
};

