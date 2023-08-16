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

    // =================================================================================
    // ===         0. LOADING THE MODEL
    // =================================================================================
    // Try loading the model if it hasn't been loaded yet
    if (!isModelLoaded) {
        load("drumLoopVAE.pt");
    }

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
    if (new_model_input_received || did_any_gui_params_change) {

        /* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
         * Use DisplayTensor to display the data if debugging
         * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
        // DisplayTensor(model_input.tensor1, "INPUT");

        if (isModelLoaded)
        { // =================================================================================
            // ===       Prepare your input
            // =================================================================================
            // Prepare The Input To Your Model
            // example: let's assume the pytorch model has some method that you want to
            // use for inference, e.g.:
            //          def SomeMethod(self, x: float, y: torch.Tensor)
            //             ....
            //             return a, b

            // Step A: To Prepare the inputs, first create a vector of torch::jit::IValue
            // e.g.:
            // >> std::vector<torch::jit::IValue> inputs;

            // Step B: Push the input parameters
            // Remember that if you need data passed from the ITP thread,
            // you can access it from the model_input struct
            // e.g.:
            // >> inputs.emplace_back(float(Slider1)); // this is the x parameter
            // >> inputs.emplace_back(model_input.tensor1); // this is the y parameter
            // =================================================================================

            // =================================================================================
            // ===        Run Inference
            // =================================================================================
            // Get the method for inference or use the forward method
            // auto SomeMethod = model.get_method("SomeMethod");
            // auto outs = SomeMethod(inputs);
            // auto outs = model.forward(inputs);

            // =================================================================================
            // ===        Prepare the model_output struct and return true to signal
            // ===        that the output is ready to be sent to PPP
            // =================================================================================
            // auto outs_as_tuple = outs.toTuple()->elements();
            // model_output.tensor1 = outs_as_tuple[0].toTensor();
            // model_output.tensor2 = outs_as_tuple[1].toTensor();

            return true;
        }
    } else {
        return false;
    }
}