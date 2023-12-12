//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "GuiParameters.h"
#include "InputEvent.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

namespace MDL_path {
    // -----------------  DO NOT CHANGE THE FOLLOWING -----------------
    #if defined(_WIN32) || defined(_WIN64)
    inline const char* default_model_path = TOSTRING(DEFAULT_MODEL_DIR);
    inline const char* path_separator = R"(\)";
    #else
    inline const char* default_model_path = TOSTRING(DEFAULT_MODEL_DIR);
    inline const char* path_separator = "/";
    #endif
}




