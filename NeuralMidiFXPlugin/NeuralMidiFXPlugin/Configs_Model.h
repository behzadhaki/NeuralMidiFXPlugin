//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "Source/Includes/GuiParameters.h"
#include "Source/Includes/InputEvent.h"
#include "Source/Includes/colored_cout.h"

#define DEFAULT_MODEL_DIR_STR(x) #x
#define DEFAULT_MODEL_DIR_EXPAND(x) DEFAULT_MODEL_DIR_STR(x)

namespace MDL_path {
    // -----------------  DO NOT CHANGE THE FOLLOWING -----------------
    #if defined(_WIN32) || defined(_WIN64)
        inline const char* default_model_path = R"(C:\NeuralMidiFx\TorchScripts\MDL)";
        inline const char* path_separator = R"(\)";
    #else
        inline const char* default_model_path = "/Library/NeuralMidiFx/TorchScripts/MDL";
        inline const char* path_separator = "/";
    #endif
}

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

    // ==============================================
    // Don't Change Anything in the following section
    // ==============================================
    // used to measure the time it takes
    // for a prepared instance to be
    // sent to the next thread
    chrono_timer timer{};
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

    // ==============================================
    // Don't Change Anything in the following section
    // ==============================================
    // used to measure the time it takes
    // for a prepared instance to be
    // sent to the next thread
    chrono_timer timer{};
};


// ======================================================================================
// ==================       Model Params                     ============================
// ======================================================================================
namespace model_settings {
    // place your model in the TorchScripts/MDL folder
    // make sure to reload the cmake project after adding a new model
    inline const char *model_name = "drumLoopVAE.pt";}

struct Configs_Model
    {
    string path;
    std::optional<torch::jit::script::Module> model;

    /*
     * constructor for Model to be loaded from a path
     */
    explicit Configs_Model(const string& model_name) {
        std::string path_ = std::string(MDL_path::default_model_path) +
                            std::string(MDL_path::path_separator) +
                            std::string(model_name);

        ifstream myFile;
        myFile.open(path_);
        if (myFile.is_open()) {
            cout << "Model file found at: " + path_ << " -- Trying to load model..." << endl;
            myFile.close();
            model = torch::jit::load(path_);
            myFile.close();
            this->path = path_;
        } else {
            cout << "Model file not found at: " + path_ << endl;
            model = std::nullopt;
            this->path = "";
        }
    }

    // better to return an optional ModelOutput to ensure that
    // plugin does not crash if model is not loaded/found
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

