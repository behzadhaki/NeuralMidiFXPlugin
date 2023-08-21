//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "Source/Includes/chrono_timer.h"

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

 update in CustomStructs.h if necessary
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



// ======================================================================================
// ==================       Structure For Sending Data       ============================
//                              From MDL to PPP
// ======================================================================================
/* All necessary data to be sent from MDL to PPP thread need
 to be wrapped in ModelOutput structure.
 Make sure you modify the following to reflect the actual data you need to send.

||| DO NOT CHANGE THE NAME OF THE STRUCTURE |||

 update in CustomStructs.h if necessary
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
// ==================       Structures For Custom Data       ============================
//                                 Per Thread
//                                  user_data
// ======================================================================================
/* If you need to reuse some data within each of the deploy() functions
 * of each thread, you can update the following structures to hold the data.
 * An instance of these structures will be available in each of the corresponding
 * threads ---> This instance is called `user_data`.
 *
 *
 * */

struct ITPData {

};

struct MDLData {

};

struct PPPData {

};