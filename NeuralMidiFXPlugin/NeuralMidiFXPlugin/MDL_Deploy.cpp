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

    // flag to indicate if a new pattern has been generated and is ready for transmission
    // to the PPP thread
    bool newPatternGenerated = false;

    // =================================================================================
    // ===         0. LOADING THE MODEL
    // =================================================================================
    // Try loading the model if it hasn't been loaded yet
    if (!isModelLoaded) {
        load("drumLoopVAE.pt");
    }

    if (isModelLoaded) {
        // get the density value
        auto density = gui_params.getValueFor("Density");

        // get latest groove received from ITP
        // ignore the operations (I'm just changing the formation of the tensor)
        auto hvo = torch::zeros({1, 32, 27});
        hvo.index_put_({torch::indexing::Ellipsis, 2}, model_input.hvo.index({torch::indexing::Ellipsis, 0}));
        hvo.index_put_({torch::indexing::Ellipsis, 11}, model_input.hvo.index({torch::indexing::Ellipsis, 1}));
        hvo.index_put_({torch::indexing::Ellipsis, 20}, model_input.hvo.index({torch::indexing::Ellipsis, 2}));


        // preparing the input to encode() method
        std::vector<torch::jit::IValue> enc_inputs;
        enc_inputs.emplace_back(hvo);
        enc_inputs.emplace_back(torch::tensor(density, torch::kFloat32).unsqueeze_(0));

        // get the encode method
        auto encode = model.get_method("encode");

        // encode the input
        auto encoder_output = encode(enc_inputs);

        // get latent vector from encoder output
        auto latentVector = encoder_output.toTuple()->elements()[2].toTensor();

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
    return newPatternGenerated;
}