//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "../../Source/Includes/chrono_timer.h"
#include "../../Source/Includes/TorchScriptAndPresetLoaders.h"

// ======================================================================================
// ==================       Structures For Custom Data       ============================
//                                 Per Thread
//                                  user_data
// ======================================================================================
/* If you need to reuse some data within each of the 'deploy()' functions
 * of each thread, you can update the following structures to hold the data.
 * An instance of these structures will be available in each of the corresponding
 * threads ---> This instance is called `user_data`.
 *
 *
 * */

struct DPLdata {

};
