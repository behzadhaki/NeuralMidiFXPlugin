//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

// ==============================================================================================
// ==================       Debugging  Settings                  ================================
// ==============================================================================================
namespace debugging_settings::InputTensorPreparatorThread {
    constexpr bool print_received_gui_params{false};                // print the received gui parameters
    constexpr bool print_input_events{false};                       // print the input tensor
    constexpr bool print_deploy_method_time{false};                 // print the time taken to deploy the model
    constexpr bool print_timed_consecutive_ModelInputs_pushed{false};   // print the output tensor
    constexpr bool disable_user_print_requests{false};              // disable all user requested prints
}

namespace debugging_settings::ModelThread {
    constexpr bool print_received_gui_params{false};                 // print the received gui parameters
    constexpr bool print_model_inference_time{false};                // print the output tensor
    constexpr bool disable_user_print_requests{false};              // disable all user requested prints
    constexpr bool disable_user_tensor_display_requests{true};     // disable all user requested prints of tensors
    constexpr bool disable_printing_tensor_content{false};           // disable printing of the tensor content
    constexpr bool disable_printing_tensor_info{false};             // disable printing of the tensor content
}

namespace debugging_settings::PlaybackPreparatorThread {
    constexpr bool print_received_gui_params{true};                   // print the received gui parameters
    constexpr bool print_ModelOutput_transfer_delay{true};            // print the output tensor
    constexpr bool print_playback_sequence{false};                    // print the output tensor
    constexpr bool disable_user_print_requests{false};               // disable all user requested prints
}

namespace debugging_settings::ProcessorThread {
    constexpr bool print_start_stop_times{true};                    // print start and stop times of the thread
    constexpr bool print_new_buffer_started{false};                 // print the new buffer started
    constexpr bool print_generation_policy_reception{true};         // print generation policy received from PPP
    constexpr bool print_generation_stream_reception{true};         // print generation stream received from PPP
    constexpr bool print_events_sent_to_host{true};
}
