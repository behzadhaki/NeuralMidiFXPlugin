//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
// ======================================================================================
// ==================     UI Settings                      ==============================
// ======================================================================================

// GUI settings
namespace UIObjects {
    using slider_tuple = std::tuple<const char *, double, double, double>;
    using rotary_tuple = std::tuple<const char *, double, double, double>;
    using button_tuple = std::tuple<const char *, bool>;

    using slider_list = std::vector<slider_tuple>;
    using rotary_list = std::vector<rotary_tuple>;
    using button_list = std::vector<button_tuple>;

    using tab_tuple = std::tuple<const char *, slider_list, rotary_list, button_list>;

    namespace Tabs {
        const std::vector<tab_tuple> tabList{

                tab_tuple{
                        "Model",
                        slider_list{
                                slider_tuple{"Slider 1", 0.0, 1.0, 0.0},
                                slider_tuple{"Slider 2", 0.0, 25.0, 11.0}},
                        rotary_list{
                                rotary_tuple{"Rotary 1", 0.0, 1.0, 0.5},
                                rotary_tuple{"Rotary 2", 0.0, 4.0, 1.5},
                                rotary_tuple{"Rotary 3", 0.0, 1.0, 0.0}},
                        button_list{
                                button_tuple{"ToggleButton 1", true},
                                button_tuple{"TriggerButton 1", false}}
                },

                tab_tuple{
                        "Settings",
                        slider_list{
                                slider_tuple{"Test 1", 0.0, 1.0, 0.0},
                                slider_tuple{"Gibberish", 0.0, 25.0, 11.0}},
                        rotary_list{
                                rotary_tuple{"Rotary 1B", 0.0, 1.0, 0.5},
                                rotary_tuple{"Test 2B", 0.0, 4.0, 1.5}},
                        button_list{
                                button_tuple{"ToggleButton 2", true},
                                button_tuple{"ToggleButton 3", true},
                                button_tuple{"Dynamics", false}}
                },

                tab_tuple{
                        "Generation",
                        slider_list{
                                slider_tuple{"Generation Playback Delay", 0.0, 10.0, 0.0}},
                        rotary_list{},
                        button_list{}
                },
        };
    }

    namespace MidiInVisualizer {
        // if you need the widget used for visualizing midi notes coming from host
        // set following to true
        const bool enable = true;

        // if you want to allow user to drag/drop midi files into the plugin
        // set following to true
        // If active, the content of the midi file will be visualized in the
        // MidiInVisualizer and also be provided to you in the InputTensorPreparatorThread
        const bool allowToDragInMidi = true;

        // if you want to visualize notes received in real-time from host
        // set following to true
        const bool visualizeIncomingMidiFromHost = true;
        // if playhead is manually moved backward, do you want to delete all the
        // previously visualized notes received from host?
        const bool deletePreviousIncomingMidiMessagesOnBackwardPlayhead = true;
        // if playback is stopped, do you want to delete all the previously
        // visualized notes received from host?
        const bool deletePreviousIncomingMidiMessagesOnRestart = true;
    }

    namespace GenerationVisualizer {
        // if you need the widget used for visualizing generated midi notes
        // set following to true
        // The content here visualizes the playbackSequence in the PlaybackPreparatorThread
        const bool enable = true;
    }

}