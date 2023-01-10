//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "includes/CustomStructsAndLockFreeQueue.h"
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
    torch::Tensor input1{};
    BufferMetaData metadata{};
};

/* Similarly, all necessary data to be sent from MDL to PPP thread (i.e. model output)
    need to be wrapped in ModelOutput structure.

            ||| DO NOT CHANGE THE NAME OF THE STRUCTURE |||

 */
struct ModelOutput {
    torch::Tensor output1{};
    torch::Tensor output2{};
    torch::Tensor output3{};
    std::vector<int> intVector{};
};





