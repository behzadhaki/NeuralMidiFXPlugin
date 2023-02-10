//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

// ==============================================================================================
// ==================       Debugging  Settings                  ================================
// ==============================================================================================
namespace debugging_settings::InputTensorPreparatorThread {
    constexpr bool print_received_gui_params{false};        // print the received gui parameters
    constexpr bool print_input_events{false};               // print the input tensor
    constexpr bool print_deploy_method_time{false};         // print the time taken to deploy the model
    constexpr bool print_timed_consecutive_ModelInputs_pushed{true};   // print the output tensor
    constexpr bool print_ModelInput_after_push{false};      // print the output tensor
    constexpr bool disable_user_print_requests{false};      // disable all user requested prints
}

namespace debugging_settings::ModelThread {
    constexpr bool print_gui_params{false};                 // print the received gui parameters
    constexpr bool print_ModelInput_after_pop{false};       // print the output tensor
    constexpr bool print_ModelOutput_before_push{false};    // print the output tensor
    constexpr bool print_ModelOutput_after_push{false};     // print the output tensor
    constexpr bool disable_user_print_requests{false};      // disable all user requested prints
}

namespace debugging_settings::PlaybackPreparatorThread {
    constexpr bool print_gui_params{false};                 // print the received gui parameters
    constexpr bool print_ModelOutput_after_pop{false};      // print the output tensor
    constexpr bool print_playback_events{false};            // print the output tensor
    constexpr bool disable_user_print_requests{false};      // disable all user requested prints
}

namespace debugging_settings::playhead {
    constexpr bool print_playhead_as_arrived{false};                 // print the received gui parameters
}