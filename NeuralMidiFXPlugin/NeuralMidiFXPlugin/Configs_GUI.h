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

    // ---------------------------------------------------------------------------------
    // Don't Modify the following
    // ---------------------------------------------------------------------------------

    // slider_tuple = {label, min, max, defaultVal, topLeftCorner, bottomRightCorner}
    // topLeftCorner and bottomRightCorner are coordinates on the grid ("Aa" to "Zz")
    using slider_tuple = std::tuple<const char *, double, double, double, const char *, const char *>;
    using rotary_tuple = std::tuple<const char *, double, double, double, const char *, const char *>;
    using button_tuple = std::tuple<const char *, bool, const char *, const char *>;

    using slider_list = std::vector<slider_tuple>;
    using rotary_list = std::vector<rotary_tuple>;
    using button_list = std::vector<button_tuple>;

    using tab_tuple = std::tuple<const char *, slider_list, rotary_list, button_list>;
    // ---------------------------------------------------------------------------------

    // ---------------------------------------------------------------------------------
    // Feel Free to Modify the following
    // ---------------------------------------------------------------------------------
    namespace Tabs {
        const bool show_grid = false;
        const bool draw_borders_for_components = false;
        const std::vector<tab_tuple> tabList{
            tab_tuple
            {
                "RandomGeneration",
                slider_list
                {
                },
                rotary_list
                {
                },
                button_list
                {
                    button_tuple{"Randomize", false, "Kh", "Pm"}
                }
            },
            tab_tuple
            {
                "Midi Mappings",
                slider_list
                {
                },
                rotary_list
                {
                    rotary_tuple{"Kick", 0, 127, 36, "Cc", "Gi"},
                    rotary_tuple{"Snare", 0, 127, 38, "Hc", "Li"},
                    rotary_tuple{"ClosedHat", 0, 127, 42, "Pc", "Ti"},
                    rotary_tuple{"OpenHat", 0, 127, 46, "Uc", "Yi"},

                    rotary_tuple{"LowTom", 0, 127, 41, "Cm", "Gs"},
                    rotary_tuple{"MidTom", 0, 127, 48, "Hm", "Ls"},
                    rotary_tuple{"HighTom", 0, 127, 45, "Pm", "Ts"},
                    rotary_tuple{"Crash", 0, 127, 49, "Um", "Ys"},

                    rotary_tuple{"Ride", 0, 127, 51, "Lt", "Pz"},

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
        const bool enable = false;

        // if you want to allow user to drag/drop midi files into the plugin
        // set following to true
        // If active, the content of the midi file will be visualized in the
        // MidiInVisualizer and also be provided to you in the InputTensorPreparatorThread
        const bool allowToDragInMidi = false;

        // if you want to visualize notes received in real-time from host
        // set following to true
        const bool visualizeIncomingMidiFromHost = false;
        // if playhead is manually moved backward, do you want to delete all the
        // previously visualized notes received from host?
        const bool deletePreviousIncomingMidiMessagesOnBackwardPlayhead = false;
        // if playback is stopped, do you want to delete all the previously
        // visualized notes received from host?
        const bool deletePreviousIncomingMidiMessagesOnRestart = false;
    }

    namespace GeneratedContentVisualizer
    {
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