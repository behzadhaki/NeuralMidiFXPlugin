#include "Source/DeploymentThreads/ModelThread.h"

using namespace debugging_settings::ModelThread;

// ===================================================================================
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                       YOUR IMPLEMENTATION SHOULD BE DONE HERE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// ===================================================================================

bool ModelThread::deploy(bool new_model_input_received,
                         bool did_any_gui_params_change) {

    /*              IF YOU NEED TO PRINT TO CONSOLE FOR DEBUGGING,
    *                  YOU CAN USE THE FOLLOWING METHOD:
    *                      PrintMessage("YOUR MESSAGE HERE");
    */

    /* A flag like this one can be used to check whether || not the model input
        is ready to be sent to the model thread (MDL)*/

    // =================================================================================
    // ===         1. ACCESSING GUI PARAMETERS
    // =================================================================================

    /* **NOTE**
        If you need to access information from the GUI, you can do so by using the
        following methods:

          Rotary/Sliders: gui_params.getValueFor([slider/button name])
          Toggle Buttons: gui_params.isToggleButtonOn([button name])
          Trigger Buttons: gui_params.wasButtonClicked([button name])
    **NOTE**
       If you only need this data when the GUI parameters CHANGE, you can use the
          provided gui_params_changed_since_last_call flag */

    auto Slider1 = gui_params.getValueFor("Slider 1");
    auto ToggleButton1 = gui_params.isToggleButtonOn("ToggleButton 1");
    auto ButtonTrigger = gui_params.wasButtonClicked("TriggerButton 1");

    // =================================================================================
    // ===         1.b. ACCESSING REALTIME PLAYBACK INFORMATION
    // =================================================================================
    auto realtime_playback_info = realtimePlaybackInfo->get();
    auto sample_rate = realtime_playback_info.sample_rate;
    auto buffer_size_in_samples = realtime_playback_info.buffer_size_in_samples;
    auto qpm = realtime_playback_info.qpm;
    auto numerator = realtime_playback_info.numerator;
    auto denominator = realtime_playback_info.denominator;
    auto isPlaying = realtime_playback_info.isPlaying;
    auto isRecording = realtime_playback_info.isRecording;
    auto current_time_in_samples = realtime_playback_info.time_in_samples;
    auto current_time_in_seconds = realtime_playback_info.time_in_seconds;
    auto current_time_in_quarterNotes = realtime_playback_info.time_in_ppq;
    auto isLooping = realtime_playback_info.isLooping;
    auto loopStart_in_quarterNotes = realtime_playback_info.loop_start_in_ppq;
    auto loopEnd_in_quarterNotes = realtime_playback_info.loop_end_in_ppq;
    auto last_bar_pos_in_quarterNotes = realtime_playback_info.ppq_position_of_last_bar_start;
    // =================================================================================

    // =================================================================================
    // ===         Inference
    // =================================================================================
    // ---       the input is available in model_input
    // ---       the output MUST be placed in model_output
    // ---       if the output is ready for transmission to PPP, return true,
    // ---                                             otherwise return false
    // ---       The API of the model MUST be defined in DeploymentSettings/Model.h
    // =================================================================================
    if (new_model_input_received) {

        /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * Use DisplayTensor to display the data if debugging
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
        DisplayTensor(model_input.tensor1, "INPUT");

        // =================================================================================
        // ===        Run the Model
        //           (update definitions in DeploymentSettings/Model.h)
        // =================================================================================
        auto out = model.forwardPredict(model_input);
        if (out.has_value()) {
            model_output = *out; // NECESSARY!!  Must copy the data to the model_output
            DisplayTensor(out->tensor1, "OUTPUT");
            return true; // notify the wrapper to send the updated model_output
        }
    } else {
        return false;
    }
}