//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "Source/Includes/chrono_timer.h"

// ======================================================================================
// ==================       Structure For Sending Data       ============================
//                              From MDL to PPP
// ======================================================================================
/* All necessary data to be sent from MDL to PPP thread need
 to be wrapped in ModelOutput structure.
 Make sure you modify the following to reflect the actual data you need to send.

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


