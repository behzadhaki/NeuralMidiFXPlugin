//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "Source/Includes/chrono_timer.h"
#include "Source/Includes/ProcessingScriptsLoader.h"

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
    torch::Tensor hits;
    torch::Tensor velocities;
    torch::Tensor offsets;

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
//
// ======================================================================================
/* If you need to reuse some data within each of the deploy() functions
 * of each thread, you can update the following structures to hold the data.
 * An instance of these structures will be available in each of the corresponding
 * threads ---> These instance are called `ITPdata`, `MDLdata`, `PPPdata`.
 *
 *
 * */


// Any Extra Variables You need in ITP can be defined here
// An instance called 'ITPdata' will be provided to you in Deploy() method
struct ITPData {

};

// Any Extra Variables You need in MDL can be defined here
// An instance called 'MDLdata' will be provided to you in Deploy() method
struct MDLData {

};

// Any Extra Variables You need in PPP can be defined here
// An instance called 'PPPdata' will be provided to you in Deploy() method
struct PPPData {

};