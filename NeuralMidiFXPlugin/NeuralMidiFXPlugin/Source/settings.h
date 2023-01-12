//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.

// ======================================================================================
// ==================       Thread  Settings                  ============================
// ======================================================================================
namespace thread_configurations::InputTensorPreparator {
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
    constexpr int NMP2ITP_que_size{4096};
    constexpr int ITP2MDL_que_size{128};
    constexpr int MDL2PPP_que_size{128};
    constexpr int PPP2NMP_que_size{4096};
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



