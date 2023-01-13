//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "Includes/CustomStructsAndLockFreeQueue.h"
#include "includes/EventTracker.h"

// ======================================================================================
// ==================       MODEL  IO Structures             ============================
// ======================================================================================

/* All necessary data to be sent from ITP to MDL thread need
 to be wrapped in ModelInput structure.
 Make sure you modify the following to reflect the actual data you need to send.
   If for whatever reason you need to send also the buffer metadata, you can
      use the BufferMetaData dtype as shown below.
 update the following structure to hold the data that you want to pass to the model

 ||| DO NOT CHANGE THE NAME OF THE STRUCTURE |||

 */
struct ModelInput {
    torch::Tensor tensor1{};
    // torch::Tensor tensor2{};
    // torch::Tensor tensor3{};
    double someDouble{};
    BufferMetaData metadata{};
};

/* Similarly, all necessary data to be sent from MDL to PPP thread (i.e. model output)
    need to be wrapped in ModelOutput structure.

            ||| DO NOT CHANGE THE NAME OF THE STRUCTURE |||

 */
struct ModelOutput {
    torch::Tensor tensor1{};
    torch::Tensor tensor2{};
    torch::Tensor tensor3{};
    // std::vector<int> intVector{};
};


// ======================================================================================
// ==================       Model Params                     ============================
// ======================================================================================

namespace model_settings {
    char constexpr *default_model_path{(char *) "/Library/NeuralMidiFXPlugin/trained_models/ModelA.pt"};
}

struct Model {
    string path;            // "/Library/NeuralMidiFXPlugin/trained_models/ModelA.pt"
    std::optional<torch::jit::script::Module> model;

    /*
     * constructor for Model to be loaded from a path
     */
    explicit Model(const string& path_) {
        ifstream myFile;
        myFile.open(path_);
        if (myFile.is_open()) {
            myFile.close();
            model = torch::jit::load(path_);
            myFile.close();
            this->path = path_;
        } else {
            DBG("Model file not found at: " + path_);
            model = std::nullopt;
            this->path = "";
        }
    }

    std::optional<ModelOutput> forwardPredict(const ModelInput& inputStruct) {

        std::optional<ModelOutput> outputStruct{};

        if (model.has_value()) {
            // ---------------------------------  Step 1 -----------------------------------------------------
            // ==================   prepare the actual input of forward methods   ============================
            /* e.g. , we process the input tensors (if needed) and pass them to the model */
            // auto in = torch::concat({inputStruct.tensor1, inputStruct.tensor2, inputStruct.tensor3}, 1);

            /* forward pass through the model */
            // auto outs = model->forward({in}); // in this case, out is a tuple of 3 tensors

            auto outs = model->forward({inputStruct.tensor1}); // Example

            // ---------------------------------  Step 2 -----------------------------------------------------
            // ==================       process the output of model here          ============================
            /* process the output of model here */


            /* e.g. this specific model, returns a tuple of 3 tensors,
             where the first tensor is the probilities of some notes
             extract these tensors
            auto probs = outs.toTuple()->elements()[0].toTensor();

             // the double value in the inputStruct is used for thresholding the probs
            auto hits = torch::zeros_like(probs);
            auto idx = probs >= inputStruct.someDouble;
            hits.index_put_({idx}, 1);
            ....
            */

            auto probs = outs.toTuple()->elements()[0].toTensor();
            auto hits = torch::zeros_like(probs);
            auto idx = probs >= inputStruct.someDouble;
            hits.index_put_({idx}, 1);
            auto vels = outs.toTuple()->elements()[1].toTensor();
            auto offsets = outs.toTuple()->elements()[2].toTensor();

            // ---------------------------------  Step 3 -----------------------------------------------------
            // ==================       wrap the output in ModelOutput struct     ============================
            /*
            output->tensor1 = hits;
            ...
            */

            // comment out following line (just for demoing)
            ModelOutput output;
            output.tensor1 = hits;
            output.tensor2 = vels;
            output.tensor3 = offsets;
            outputStruct = std::optional<ModelOutput>(output);      // don't delete this line!!
        } else {
            /* if model is not loaded, return an empty optional */
            outputStruct = std::nullopt;                            // don't delete this line!!
        }

        /* return the output */
        return outputStruct;                                        // don't delete this line!!
    }
};

