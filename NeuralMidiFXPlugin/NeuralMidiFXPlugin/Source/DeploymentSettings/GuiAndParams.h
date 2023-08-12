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
    using slider_tuple = std::tuple<const char *, double, double, double, const char *, const char *>;
    using rotary_tuple = std::tuple<const char *, double, double, double, const char *, const char *>;
    using button_tuple = std::tuple<const char *, bool, const char *, const char *>;

    using slider_list = std::vector<slider_tuple>;
    using rotary_list = std::vector<rotary_tuple>;
    using button_list = std::vector<button_tuple>;

    using tab_tuple = std::tuple<const char *, slider_list, rotary_list, button_list>;

    using tab_tuple = std::tuple<const char *, slider_list, rotary_list, button_list>;

    namespace Tabs {
        const bool show_grid = true;
        const bool draw_borders_for_components = true;
        const std::vector<tab_tuple> tabList{
            tab_tuple
            {
                "Tab 1", // use any name you want for the tab
                slider_list
                {
                    slider_tuple{"Slider 1", 0.0, 1.0, 0.0, "Cc", "Ek"}
                },
                rotary_list
                {
                    rotary_tuple{"Rotary 1", 0.0, 1.0, 0.5, "Vc", "Xk"},
                    rotary_tuple{"Rotary 2", 0.0, 4.0, 1.5, "Ij", "Qr"}
                },
                button_list
                {
                    button_tuple{"ClickButton 1", false, "Wv", "Zz"}
                }
            },
            tab_tuple{
                "Tab 2",
                slider_list
                {
                    slider_tuple{"Test 1", 0.0, 1.0, 0.0, "Aa", "Ff"},
                    slider_tuple{"Gibberish", 0.0, 25.0, 11.0, "Ff", "Ll"}
                },
                rotary_list
                {
                    rotary_tuple{"Rotary 1B", 0.0, 1.0, 0.5, "Ll", "Pp"},
                    rotary_tuple{"Test 2B", 0.0, 4.0, 1.5, "Pp", "Tt"}
                },
                button_list
                {
                    button_tuple{"ToggleButton 2", true, "Tt", "Zz"}
                }
            },

            tab_tuple{
                "Tab 3",
                slider_list
                {
                    slider_tuple{"Generation Playback Delay", 0.0, 10.0, 0.0, "Aa", "Zz"}
                },
                rotary_list
                {

                },
                button_list
                {

                }
            }
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

    namespace PlaybackSequenceVisualizer {
        // if you need the widget used for visualizing generated midi notes
        // set following to true
        // The content here visualizes the playbackSequence as it is at any given time
        // (remember that playbackSequence can be changed by the user within the
        // PlaybackPreparatorThread)
        const bool enable = true;

        // if you want to allow the user to drag out the visualized content,
        // set following to true
        const bool allowToDragOutAsMidi = true;
    }


}