//
// Created by Behzad Haki on 2022-02-11.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "Source/Includes/json.hpp"

// ======================================================================================
// ==================     UI Settings                      ==============================
// ======================================================================================

// Using nlohmann/json library for JSON parsing
using json = nlohmann::json;

// GUI settings
namespace UIObjects {

    // ---------------------------------------------------------------------------------
    // Don't Modify the following
    // ---------------------------------------------------------------------------------

    using slider_tuple = std::tuple<const char*, double, double, double, const char*, const char*>;
    using rotary_tuple = std::tuple<const char*, double, double, double, const char*, const char*>;
    using button_tuple = std::tuple<const char*, bool, const char*, const char*>;

    using slider_list = std::vector<slider_tuple>;
    using rotary_list = std::vector<rotary_tuple>;
    using button_list = std::vector<button_tuple>;

    using tab_tuple = std::tuple<const char*, slider_list, rotary_list, button_list>;

    // ---------------------------------------------------------------------------------
    // Feel Free to Modify the following
    // ---------------------------------------------------------------------------------

    namespace Tabs {
        bool show_grid;
        bool draw_borders_for_components;
        std::vector<tab_tuple> tabList;
    }

        void populateFromJson(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "Error opening settings file: " << filePath << std::endl;
            throw std::runtime_error("Failed to open settings file");
        }
        json j;
        file >> j;

        Tabs::show_grid = j["UI"]["Tabs"]["show_grid"];
        Tabs::draw_borders_for_components = j["UI"]["Tabs"]["draw_borders_for_components"];

        for (const auto& tabJson : j["UI"]["Tabs"]["tabList"]) {
            const char* tabName = tabJson["name"].get<std::string>().c_str();

            slider_list tabSliders;
            for (const auto& sliderJson : tabJson["sliders"]) {
                tabSliders.emplace_back(
                    sliderJson["label"].get<std::string>().c_str(),
                    sliderJson["min"].get<double>(),
                    sliderJson["max"].get<double>(),
                    sliderJson["defaultVal"].get<double>(),
                    sliderJson["topLeftCorner"].get<std::string>().c_str(),
                    sliderJson["bottomRightCorner"].get<std::string>().c_str()
                );
            }

            rotary_list tabRotaries; // Similar logic for rotary_list and button_list
            button_list tabButtons;

            // Add additional parsing for rotary and button lists here...

            Tabs::tabList.emplace_back(tabName, tabSliders, tabRotaries, tabButtons);
        }
    }
}