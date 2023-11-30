//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../Deployment/Configs_HostEvents.h"
#include "Configs_Parser.h"
#include "chrono_timer.h"
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
    bool isComboBox{false};         // for comboBoxes
    vector<string> comboBoxOptions{};

    using slider_or_rotary_tuple = std::tuple<std::string, double, double, double, std::string, std::string>;

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

    explicit param(button_tuple button_tuple) {
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

    explicit param(comboBox_tuple comboBox_tuple) {
        label = std::get<0>(comboBox_tuple);
        value = 1;
        paramID = label2ParamID(label);
        min = 1;
        max = std::get<1>(comboBox_tuple).size();
        defaultVal = 1;
        isSlider = false;
        isRotary = false;
        isButton = false;
        isToggle = false;
        isComboBox = true;
        isChanged = true; // first time update is always true

        comboBoxOptions = std::get<1>(comboBox_tuple);
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
            std::cout << "Duplicate label found: " << label << std::endl;
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
        chrono_timed.registerStartTime();
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

    // only use this to get the value for a slider, rotary, || toggleable button
    // if label is invalid (i.e. not defined in Configs_GUI.h), then it returns 0
    double getValueFor(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isRotary || parameter.isSlider || (parameter.isButton && parameter.isToggle)) {
                    return parameter.value;
                }
            }
        }
        cout << "Label: " << label << " not found";
        return 0;
    }


    // only use this to check whether the button was clicked (regardless of whether toggleable || not
    // if label is invalid (i.e. not defined in Configs_GUI.h), then it returns false
    bool wasButtonClicked(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isButton) {
                    return parameter.isChanged;
                }
            }
        }
        cout << "Label: " << label << " not found";
        return false;
    }

    // only use this to check whether a Toggleable button is on
    // if label is invalid (i.e. not defined in Configs_GUI.h), then it returns false
    bool isToggleButtonOn(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isButton && parameter.isToggle) {
                    return bool(parameter.value);
                }
            }
        }
        cout << "Label: " << label << " not found";
        return false;
    }

    string getComboBoxSelectionText(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isComboBox) {
                    return parameter.comboBoxOptions[parameter.value];
                }
            }
        }
        cout << "Label: " << label << " not found";
        return "";
    }

    string getDescriptionOfUpdatedParams() {
        std::stringstream ss;
        for (auto &parameter: Parameters) {
            if (parameter.isChanged && (parameter.isRotary || parameter.isSlider)) {
                ss << " | Slider/Rotary: `" << parameter.label << "` :" << parameter.value ;
            }
            if (parameter.isChanged && parameter.isButton && parameter.isToggle) {
                ss << " | Toggle Button: `" << parameter.label << "` :" << bool2string(
                        isToggleButtonOn(parameter.label));
            }
            if (parameter.isChanged && parameter.isButton && !parameter.isToggle) {
                ss << " | Trigger Button:  `" << parameter.label << "` was clicked";
            }
            if (parameter.isChanged && parameter.isComboBox) {
                ss << " | ComboBox:  `" << parameter.label << "` :" << parameter.value << " (" << getComboBoxSelectionText(parameter.label) << ")";
            }
        }
        if (chrono_timed.isValid()) {
            ss << *chrono_timed.getDescription(", ReceptionFromHostToAccess Delay: ") << std::endl;
        } else {
            ss << std::endl;
        }
        return ss.str();
    }

    void registerAccess() { chrono_timed.registerEndTime(); }

private:
    vector<param> Parameters;
    bool isChanged = false;

    // uses chrono::system_clock to time parameter arrival to consumption (for debugging only)
    // don't use this for anything else than debugging.
    // used to keep track of when the object was created && when it was accessed
    chrono_timer chrono_timed;

    void assertLabelIsUnique(const string &label_) {
        for (const auto &previous_param: Parameters) {
            // if assert is thrown, then you have a
            // duplicate parameter label
            previous_param.assertIfSameLabelOrID(label_);
        }
    }

    void construct() {
        chrono_timed.registerStartTime();

        auto tabList = UIObjects::Tabs::tabList;

        for (auto tab_list: tabList) {

            auto slidersList = std::get<1>(tab_list);
            auto rotariesList = std::get<2>(tab_list);
            auto buttonsList = std::get<3>(tab_list);
            auto vslidersList = std::get<4>(tab_list);
            auto comboBoxesList = std::get<5>(tab_list);

            for (const auto &slider_tuple: slidersList) {
                auto label = std::get<0>(slider_tuple);
                assertLabelIsUnique(label);
                Parameters.emplace_back(slider_tuple, true);
            }

            for (const auto &vslider_tuple: vslidersList) {
                auto label = std::get<0>(vslider_tuple);
                assertLabelIsUnique(label);
                Parameters.emplace_back(vslider_tuple, true);
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

            for (const auto &comboBox_tuple: comboBoxesList) {
                std::string label = std::get<0>(comboBox_tuple);
                assertLabelIsUnique(label);
                Parameters.emplace_back(comboBox_tuple);
            }
        }
    }

    static string bool2string(bool b) { return b ? "true" : "false"; }
};



