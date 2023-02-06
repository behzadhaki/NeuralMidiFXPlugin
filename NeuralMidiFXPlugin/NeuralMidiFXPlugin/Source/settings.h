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
        // To access:
//      std::string tabName = std::get<0>(tabList[0]);
//      slider_list sliders = std::get<1>(tabList[0]);
//      rotary_list rotaries = std::get<2>(tabList[0]);
//      button_list buttons = std::get<3>(tabList[0]);
//
//      slider_tuple firstSlider = std::get<0>(sliders);
//      double sliderValue = std::get<1>(firstSlider);


        const std::vector<std::vector<int>> myVec = {{0, 0},
                                                     {1, 1}};
        const std::vector<tab_tuple> tabList{

                tab_tuple{
                        "Model",
                        slider_list{
                                slider_tuple{"Slider 1", 0.0, 1.0, 0.0},
                                slider_tuple{"Slider 2", 0.0, 25.0, 11.0}},
                        rotary_list{
                                rotary_tuple{"Rotary 1A", 0.0, 1.0, 0.5},
                                rotary_tuple{"Test 2A", 0.0, 4.0, 1.5},
                                rotary_tuple{"Rotary 3", 0.0, 1.0, 0.0}},
                        button_list{
                                button_tuple{"Button 1", true},
                                button_tuple{"Button 2", false}}
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
                                button_tuple{"Test Button", true},
                                button_tuple{"Velocity", true},
                                button_tuple{"Dynamics", false}}
                }
        };
    }
}

// ======================================================================================
// ==================       Thread  Settings                  ============================
// ======================================================================================
namespace thread_configurations::InputTensorPreparator {
    // waittime between iterations in ms
    constexpr int waitTimeBtnIters{5};
}
namespace thread_configurations::Model {
    // waittime between iterations in ms
    constexpr int waitTimeBtnIters{5};
}
namespace thread_configurations::PlaybackPreparator {
    // waittime between iterations in ms
    constexpr int waitTimeBtnIters{5};
}

namespace thread_configurations::APVTSMediatorThread {
    // waittime between iterations in ms
    constexpr int waitTimeBtnIters{5};
}
// ======================================================================================
// ==================       QUEUE  Settings                  ============================
// ======================================================================================
/* specifies the max number of elements that can be stored in the queue
 *  if the queue is full, the producer thread will overwrite the oldest element
 */
namespace queue_settings {
    constexpr int NMP2ITP_que_size{512};
    constexpr int ITP2MDL_que_size{512};
    constexpr int MDL2PPP_que_size{512};
    constexpr int PPP2NMP_que_size{512};
    constexpr int APVM_que_size{32};
}

// ======================================================================================
// ==================        Event Communication Settings                ================
// ======================================================================================
/*
 * You can send Events to ITP at different frequencies with different
 *      intentions, depending on the usecase intended. The type and frequency of providing
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
 *      since the metadata is embedded in any midi Event. Just remember to not
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
    // set to true, if you need to send the metadata for a new buffer to the ITP thread
    constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
    constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{true};     // only sends if metadata changes

    // set to true if you need to notify the beginning of a new bar
    constexpr bool SendNewBarEvents_FLAG{true};

    // set to true Event for every time_shift_event ratio of quarter notes
    constexpr bool SendTimeShiftEvents_FLAG{false};
    constexpr double delta_TimeShiftEventRatioOfQuarterNote{0.5}; // sends a time shift event every 8th note

    // Filter Note On Events if you don't need them
    constexpr bool FilterNoteOnEvents_FLAG{false};

    // Filter Note Off Events if you don't need them
    constexpr bool FilterNoteOffEvents_FLAG{false};

    // Filter CC Events if you don't need them
    constexpr bool FilterCCEvents_FLAG{false};
}
