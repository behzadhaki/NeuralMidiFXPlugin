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

using slider_tuple = std::tuple<std::string, double, double, double, std::string, std::string>;
using rotary_tuple = std::tuple<std::string, double, double, double, std::string, std::string>;
using button_tuple = std::tuple<std::string, bool, std::string, std::string>;
using hslider_tuple = std::tuple<std::string, double, double, double, std::string, std::string>;
using comboBox_tuple = std::tuple<std::string, std::vector<std::string>, std::string, std::string>;

using slider_list = std::vector<slider_tuple>;
using rotary_list = std::vector<rotary_tuple>;
using button_list = std::vector<button_tuple>;
using hslider_list = std::vector<hslider_tuple>;
using comboBox_list = std::vector<comboBox_tuple>;

using tab_tuple = std::tuple<std::string, slider_list, rotary_list, button_list, hslider_list, comboBox_list>;

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

        // check if sliders exist
        if (tabJson.contains("sliders")) {
            for (const auto& sliderJson: tabJson["sliders"]) {
                // check if slider is vertical or horizontal
                // vertical if it has no "horizontal" key or if it is false
                if (!sliderJson.contains("horizontal") || !sliderJson["horizontal"].get<bool>()) {
                    std::string sliderLabel = sliderJson["label"].get<std::string>();
                    double sliderMin = sliderJson["min"];
                    double sliderMax = sliderJson["max"];
                    double sliderDefaultVal = sliderJson["defaultVal"];
                    std::string sliderTopLeftCorner = sliderJson["topLeftCorner"].get<std::string>();
                    std::string sliderBottomRightCorner = sliderJson["bottomRightCorner"].get<std::string>();
                    slider_tuple sliderTuple = {sliderLabel, sliderMin, sliderMax, sliderDefaultVal, sliderTopLeftCorner, sliderBottomRightCorner};
                    tabSliders.push_back(sliderTuple);
                } else {
                    std::string sliderLabel = sliderJson["label"].get<std::string>();
                    double sliderMin = sliderJson["min"];
                    double sliderMax = sliderJson["max"];
                    double sliderDefaultVal = sliderJson["defaultVal"];
                    std::string sliderTopLeftCorner = sliderJson["topLeftCorner"].get<std::string>();
                    std::string sliderBottomRightCorner = sliderJson["bottomRightCorner"].get<std::string>();
                    hslider_tuple sliderTuple = {sliderLabel, sliderMin, sliderMax, sliderDefaultVal, sliderTopLeftCorner, sliderBottomRightCorner};
                    tabhsliders.push_back(sliderTuple);
                }
            }
        }

        // check if rotaries exist
        if (tabJson.contains("rotaries")) {
            for (const auto& rotaryJson: tabJson["rotaries"]) {
                std::string rotaryLabel = rotaryJson["label"].get<std::string>();
                double rotaryMin = rotaryJson["min"];
                double rotaryMax = rotaryJson["max"];
                double rotaryDefaultVal = rotaryJson["defaultVal"];
                std::string rotaryTopLeftCorner = rotaryJson["topLeftCorner"].get<std::string>();
                std::string rotaryBottomRightCorner = rotaryJson["bottomRightCorner"].get<std::string>();

                rotary_tuple rotaryTuple = {rotaryLabel, rotaryMin, rotaryMax, rotaryDefaultVal, rotaryTopLeftCorner, rotaryBottomRightCorner};
                tabRotaries.push_back(rotaryTuple);
            }
        }

        // check if buttons exist
        if (tabJson.contains("buttons")) {
            for (const auto& buttonJson: tabJson["buttons"]) {
                std::string buttonLabel = buttonJson["label"].get<std::string>();
                bool buttonIsToggle = buttonJson["isToggle"];
                std::string buttonTopLeftCorner = buttonJson["topLeftCorner"].get<std::string>();
                std::string buttonBottomRightCorner =  buttonJson["bottomRightCorner"];
                button_tuple buttonTuple = {buttonLabel, buttonIsToggle, buttonTopLeftCorner, buttonBottomRightCorner};
                tabButtons.push_back(buttonTuple);
            }
        }

        // check if comboBoxes exist
        if (tabJson.contains("comboBoxes")) {
            for (const auto& comboBoxJson: tabJson["comboBoxes"]) {
                std::string comboBoxLabel = comboBoxJson["label"].get<std::string>();
                std::vector<std::string> comboBoxOptions = comboBoxJson["options"].get<std::vector<std::string>>();
                std::string comboBoxTopLeftCorner = comboBoxJson["topLeftCorner"].get<std::string>();
                std::string comboBoxBottomRightCorner =  comboBoxJson["bottomRightCorner"];
                comboBox_tuple comboBoxTuple = {comboBoxLabel, comboBoxOptions, comboBoxTopLeftCorner, comboBoxBottomRightCorner};
                tabComboBoxes.push_back(comboBoxTuple);
            }
        }

        tab_tuple tabTuple = {tabName, tabSliders, tabRotaries, tabButtons, tabhsliders, tabComboBoxes};
        tabList.push_back(tabTuple);

    }

    return tabList;
}

// GUI settings
namespace UIObjects {

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
        // MidiInVisualizer and also be provided to you in the InputTensorPreparatorThread
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

        // if you need to send midi out to a virtual midi cable
        // set following to true
        // NOTE: Only works on MacOs
        const bool NeedVirtualMidiOutCable = loaded_json["VirtualMidiOut"]["enable"];
    }


}

