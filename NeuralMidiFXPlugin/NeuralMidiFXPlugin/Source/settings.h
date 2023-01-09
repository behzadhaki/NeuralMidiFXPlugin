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

namespace queue_settings {
    constexpr int NMP2ITP_que_size{4096};
    constexpr int ITP2MDL_que_size{128};
    constexpr int MDL2PPP_que_size{4096};
}
// ======================================================================================
// ==================     General Settings                 ==============================
// ======================================================================================
//namespace GeneralSettings {
//    // model_settings
//    // FIXME add to readme.me for setup ==> model should be placed in root
//    //  (/Library/NeuralMidiFXPlugin/trained_models) folder
//    char constexpr *default_model_path{(char*)"/Library/NeuralMidiFXPlugin/trained_models/model_1.pt"};
//    char constexpr *default_model_folder{(char*)"/Library/NeuralMidiFXPlugin/trained_models"};
//}


// ======================================================================================
// ==================        Event Communication Settings                ================
// ======================================================================================

namespace event_communication_settings {
    // set to true, if you need to send the metadata for a new buffer to the ITP thread
    constexpr bool SendEventAtBeginningOfNewBuffers_FLAG{true};
    constexpr bool SendEventForNewBufferIfMetadataChanged_FLAG{true};     // only sends if metadata changes

    // set to true if you need to notify the beginning of a new bar
    constexpr bool SendNewBarEvents_FLAG{true};

    // set to true Event for every time_shift_event ratio of quarter notes
    constexpr bool SendTimeShiftEvents_FLAG{true};
    constexpr double delta_TimeShiftEventRatioOfQuarterNote{0.5}; // sends a time shift event every 8th note

    // Filter Note On Events if you don't need them
    constexpr bool FilterNoteOnEvents_FLAG{false};

    // Filter Note Off Events if you don't need them
    constexpr bool FilterNoteOffEvents_FLAG{false};

    // Filter CC Events if you don't need them
    constexpr bool FilterCCEvents_FLAG{false};
}



