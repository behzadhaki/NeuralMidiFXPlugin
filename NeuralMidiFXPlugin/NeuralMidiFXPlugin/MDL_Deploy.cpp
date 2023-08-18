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
    // Refer to:
    // https://neuralmidifx.github.io/datatypes/GuiParams#accessing-the-ui-parameters
    // =================================================================================


    // =================================================================================


    // =================================================================================
    // ===         1.b. ACCESSING REALTIME PLAYBACK INFORMATION
    // Refer to:
    // https://neuralmidifx.github.io/datatypes/RealtimePlaybackInfo#accessing-the-realtimeplaybackinfo
    // =================================================================================


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