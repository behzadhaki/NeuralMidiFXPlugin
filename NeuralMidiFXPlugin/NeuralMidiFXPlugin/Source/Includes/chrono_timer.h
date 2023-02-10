//
// Created by Behzad Haki on 2022-09-02.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"

#pragma once

using namespace std;
#include <chrono>

using sys_time = std::optional<std::chrono::time_point<std::chrono::system_clock>>;

struct chrono_timer {
    chrono_timer() = default;

    void registerStartTime() {
        startTime = std::chrono::system_clock::now();
    }

    void registerEndTime() {
        endTime = std::chrono::system_clock::now();
    }

    std::optional<string> getDescription(const string& text=" | Duration : ") const{
        std::stringstream ss;

        string description;
        if (!isValid()) {
            return std::nullopt;
        } else {
            auto duration = *endTime - *startTime;
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            ss << text << std::to_string(duration_ms) + " (ms)";
        }
        return ss.str();
    }

    bool isValid() const {
        return startTime && endTime;
    }
private:

    sys_time startTime{std::nullopt};
    sys_time endTime{std::nullopt};
};


