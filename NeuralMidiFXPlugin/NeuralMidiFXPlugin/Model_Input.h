//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.

// ======================================================================================
// ==================       Structure For Sending Data       ============================
//                              From ITP to MDL
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



