//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "../Includes/GuiParameters.h"
#include "../Includes/InputEvent.h"
# include "../../CustomStructs.h"
#include "../../CustomStructs.h"

#define DEFAULT_MODEL_DIR_STR(x) #x
#define DEFAULT_MODEL_DIR_EXPAND(x) DEFAULT_MODEL_DIR_STR(x)

namespace MDL_path {
    // -----------------  DO NOT CHANGE THE FOLLOWING -----------------
    #if defined(_WIN32) || defined(_WIN64)
        inline const char* default_model_path = R"(C:\NeuralMidiFxPlugin\TorchScripts\MDL)";
        inline const char* path_separator = R"(\)";
    #else
        inline const char* default_model_path = "/Library/NeuralMidiFxPlugin/TorchScripts/MDL";
        inline const char* path_separator = "/";
    #endif
}



