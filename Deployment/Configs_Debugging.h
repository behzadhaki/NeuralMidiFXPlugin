//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

// ==============================================================================================
// ==================       Debugging  Settings                  ================================
// ==============================================================================================
namespace debugging_settings::DeploymentThread {
    constexpr bool print_received_gui_params{true};                // print the received gui parameters
    constexpr bool print_manually_dropped_midi_messages{false};     // print the midi messages received from manually drag-dropped midi file
    constexpr bool print_input_events{false};                       // print the input tensor
    constexpr bool print_deploy_method_time{false};                 // print the time taken to deploy the model
    constexpr bool disable_user_print_requests{false};              // disable all user requested prints
}

namespace debugging_settings::ProcessorThread {
    constexpr bool print_start_stop_times{false};                    // print start and stop times of the thread
    constexpr bool print_new_buffer_started{false};                 // print the new buffer started
    constexpr bool print_generation_policy_reception{false};         // print generation policy received from DPL
    constexpr bool print_generation_stream_reception{false};         // print generation stream received from DPL
    constexpr bool disableAllPrints{false};                          // disable all prints
}
