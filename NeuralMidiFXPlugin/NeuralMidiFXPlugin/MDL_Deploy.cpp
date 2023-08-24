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

    bool should_interpolate = false;   // flag to check if we should interpolate

    // =================================================================================
    // ===         1. ACCESSING GUI PARAMETERS
    // Refer to:
    // https://neuralmidifx.github.io/datatypes/GuiParams#accessing-the-ui-parameters
    // =================================================================================
    // check if the buttons have been clicked, if so, update the MDLdata
    auto ButtonATriggered = gui_params.wasButtonClicked("Random A");
    if (ButtonATriggered) {
        should_interpolate = true;
        MDLdata.latent_A = torch::randn({ 1, 128 });
    }
    auto ButtonBTriggered = gui_params.wasButtonClicked("Random B");
    if (ButtonBTriggered) {
        should_interpolate = true;
        MDLdata.latent_B = torch::randn({ 1, 128 });
    }

    // check if the interpolate slider has changed, if so, update the MDLdata
    auto sliderValue = gui_params.getValueFor("Interpolate");
    bool sliderChanged = (sliderValue != MDLdata.interpolate_slider_value);
    if (sliderChanged) {
        should_interpolate = true;
        MDLdata.interpolate_slider_value = sliderValue;
    }
    // =================================================================================

    // =================================================================================
    // ===         2. initialize latent vectors on the first call
    // =================================================================================
    if (MDLdata.latent_A.size(0) == 0) {
        MDLdata.latent_A = torch::randn({ 1, 128 });
    }
    if (MDLdata.latent_B.size(0) == 0) {
        MDLdata.latent_B = torch::randn({ 1, 128 });
    }

    // =================================================================================
    // ===         Inference
    // =================================================================================
    // ---       the input is available in model_input
    // ---       the output MUST be placed in model_output
    // ---       if the output is ready for transmission to PPP, return true,
    // ---                                             otherwise return false
    // =================================================================================

    // flag to indicate if a new pattern has been generated and is ready for transmission
    // to the PPP thread
    bool newPatternGenerated = false;

    if (should_interpolate || did_any_gui_params_change) {

        if (isModelLoaded)
        {
            // calculate interpolated latent vector
            auto slider_value = MDLdata.interpolate_slider_value;
            auto latent_A = MDLdata.latent_A;
            auto latent_B = MDLdata.latent_B;
            auto latentVector = (1 - slider_value) * latent_A + slider_value * latent_B;

            // Prepare other inputs
            auto voice_thresholds = torch::ones({9 }, torch::kFloat32) * 0.5f;
            auto max_counts_allowed = torch::ones({9 }, torch::kFloat32) * 32;
            int sampling_mode = 0;
            float temperature = 1.0f;

            // Prepare above for inference
            std::vector<torch::jit::IValue> inputs;
            inputs.emplace_back(latentVector);
            inputs.emplace_back(voice_thresholds);
            inputs.emplace_back(max_counts_allowed);
            inputs.emplace_back(sampling_mode);
            inputs.emplace_back(temperature);

            // Get the scripted method
            auto sample_method = model.get_method("sample");

            // Run inference
            auto output = sample_method(inputs);

            // Extract the generated tensors from the output
            auto hits = output.toTuple()->elements()[0].toTensor();
            auto velocities = output.toTuple()->elements()[1].toTensor();
            auto offsets = output.toTuple()->elements()[2].toTensor();

            // wrap the generated tensors into a ModelOutput struct
            model_output.hits = hits;
            model_output.velocities = velocities;
            model_output.offsets = offsets;

            // Set the flag to true
            newPatternGenerated = true;
        }
    }

    return newPatternGenerated;
}