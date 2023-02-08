//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include "../DeploymentSettings/GuiAndParams.h"

#include <torch/script.h> // One-stop header.

#include <utility>


using namespace std;

// ============================================================================================================
// ==========          param struct for gui objects  (holds single param)                 =====================
// ============================================================================================================

static string label2ParamID(const string &label) {
    // capitalize label
    string paramID = label;
    std::transform(paramID.begin(), paramID.end(), paramID.begin(), ::toupper);
    return paramID;
}


struct param {
    string label{};
    double value{};
    string paramID{};
    std::optional<double> min = std::nullopt;
    std::optional<double> max = std::nullopt;
    std::optional<double> defaultVal = std::nullopt;       // for sliders, rotaries, comboBoxes,...
    bool isSlider{false};           // for sliders
    bool isRotary{false};           // for rotaries
    bool isButton{false};
    bool isToggle{false};           // for buttons
    bool isChanged{false};

    using slider_or_rotary_tuple = std::tuple<const char *, double, double, double>;

    param() = default;

    explicit param(slider_or_rotary_tuple slider_or_rotary_tuple, bool isSlider_) {
        label = std::get<0>(slider_or_rotary_tuple);
        value = std::get<3>(slider_or_rotary_tuple);
        paramID = label2ParamID(label);
        min = std::get<1>(slider_or_rotary_tuple);
        max = std::get<2>(slider_or_rotary_tuple);
        defaultVal = std::get<3>(slider_or_rotary_tuple);
        isSlider = isSlider_;
        isRotary = !isSlider_;
        isButton = false;
        isToggle = false;
        isChanged = true;   // first time update is always true
    }

    explicit param(UIObjects::button_tuple button_tuple) {
        label = std::get<0>(button_tuple);
        value = 0;
        paramID = label2ParamID(label);
        min = std::nullopt;
        max = std::nullopt;
        defaultVal = 0;
        isSlider = false;
        isRotary = false;
        isButton = true;
        isToggle = std::get<1>(button_tuple);
        isChanged = true; // first time update is always true
    }

    bool update(juce::AudioProcessorValueTreeState *apvts) {
        auto new_val_ptr = apvts->getRawParameterValue(paramID);

        if (value != *new_val_ptr) {
            value = *new_val_ptr;
            isChanged = true;
        } else {
            isChanged = false;
        }
        return isChanged;
    }

    // ensures new paramID is available
    void assertIfSameLabelOrID(const string& new_string) const {
        if (label == new_string) {
            std::stringstream ss;
            DBG("Duplicate label found: " << label) ;
            assert(false && "Duplicate label found");
        }

    }
};


// ============================================================================================================
// ==========          GUI PARAMS STRUCTS (holds all params)         ==========================================
// ============================================================================================================

// to get the value of a parameter, use the following syntax:
//      auto paramID = GuiParams.getParamValue(LabelOnGUi);
struct GuiParams {

    GuiParams() { construct(); }

    explicit GuiParams(juce::AudioProcessorValueTreeState *apvtsPntr) {
        construct();
        for (auto &parameter: Parameters) {
            parameter.update(apvtsPntr);
        }
    }

    bool update(juce::AudioProcessorValueTreeState *apvtsPntr) {
        isChanged = false;
        for (auto &parameter: Parameters) {
            if (parameter.update(apvtsPntr)) {
                isChanged = true;
            }
        }
        return isChanged;
    }

    void print() {
        for (auto &parameter: Parameters) {
            DBG(parameter.label << " " << parameter.value);
        }
    }

    bool changed() const {
        return isChanged;
    }

    void setChanged(bool isChanged_) {
        isChanged = isChanged_;
    }

    bool wasParamUpdated(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                return parameter.isChanged;
            }
        }
    }

    std::vector<string> getLabelsForUpdatedParams(){
        std::vector<string> changedLabels;
        for (auto &parameter: Parameters) {
            if (parameter.isChanged) {
                changedLabels.push_back(parameter.label);
            }
        }
        return changedLabels;
    }

    // only use this to get the value for a slider, rotary, or toggleable button
    double getValueFor(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isRotary or parameter.isSlider or (parameter.isButton and parameter.isToggle)) {
                    return parameter.value;
                }
                assert( false && "This method is to be used only with Rotary/Slider/Toggleable_Buttons" );
            }
        }
        DBG("Label: " << label << " not found");
        assert( false && "Invalid Label Used. Check settings.h/UISettings" );

    }


    // only use this to check whether the button was clicked (regardless of whether toggleable or not
    bool wasButtonClicked(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isButton) {
                    return parameter.isChanged;
                }
                assert( false && "This method is to be used only with Buttons (regardless of Toggleable)" );
            }
        }
        DBG("Label: " << label << " not found");
        assert( false && "Invalid Label Used. Check settings.h/UISettings" );
    }

    // only use this to check whether a Toggleable button is on
    bool isToggleButtonOn(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isButton and parameter.isToggle) {
                    return bool(parameter.value);
                }
                assert( false && "This method is to be used only with Toggleable Buttons" );
            }
        }
        DBG("Label: " << label << " not found");
        assert( false && "Invalid Label Used. Check settings.h/UISettings" );
    }

    string getDescriptionOfUpdatedParams() {
        std::stringstream ss;
        for (auto &parameter: Parameters) {
            if (parameter.isChanged and (parameter.isRotary or parameter.isSlider)) {
                ss << "Param: `" << parameter.label << "` value changed to " << parameter.value <<
                "-- use gui_params.getValueFor(" << parameter.label << ") to get the value" << std::endl;
            }
            if (parameter.isChanged and parameter.isButton and parameter.isToggle) {
                if (parameter.value == 1.0) {
                    ss << "Toggle Button: `" << parameter.label <<
                    "` " << bool2string(isToggleButtonOn(parameter.label))
                    << " -- use gui_params.isToggleButtonOn(" << parameter.label <<
                    ") to check toggle state" << std::endl;
                } else {
                    ss << "Toggle Button: `" << parameter.label << "` " <<
                    bool2string(isToggleButtonOn(parameter.label))
                    <<" -- use gui_params.isToggleButtonOn(" << parameter.label <<
                    ") to check toggle state" << std::endl;
                }
            }
            if (parameter.isChanged and parameter.isButton and !parameter.isToggle) {
                ss << "Button `" << parameter.label << "` was clicked -- use gui_params.wasButtonClicked(" <<
                parameter.label << ") to check toggle state" << std::endl;
            }
        }
        return ss.str();
    }

private:
    vector<param> Parameters;
    bool isChanged = false;

    void assertLabelIsUnique(const string &label_) {
        for (const auto &previous_param: Parameters) {
            // if assert is thrown, then you have a
            // duplicate parameter label
            previous_param.assertIfSameLabelOrID(label_);
        }
    }

    void construct() {
        auto tabList = UIObjects::Tabs::tabList;

        for (auto tab_list: tabList) {

            auto slidersList = std::get<1>(tab_list);
            auto rotariesList = std::get<2>(tab_list);
            auto buttonsList = std::get<3>(tab_list);

            for (const auto &slider_tuple: slidersList) {
                auto label = std::get<0>(slider_tuple);
                assertLabelIsUnique(label);
                Parameters.emplace_back(slider_tuple, true);
            }

            for (const auto &rotary_tuple: rotariesList) {
                auto label = std::get<0>(rotary_tuple);
                assertLabelIsUnique(label);
                Parameters.emplace_back(rotary_tuple, false);
            }


            for (const auto &button_tuple: buttonsList) {
                std::string label = std::get<0>(button_tuple);
                assertLabelIsUnique(label);
                Parameters.emplace_back(button_tuple);
            }
        }
    }

    static string bool2string(bool b) { return b ? "true" : "false"; }
};



