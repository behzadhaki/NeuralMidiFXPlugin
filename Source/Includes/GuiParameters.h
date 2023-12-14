//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

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

    param() = default;

    void InitializeSlider(json slider_or_rotary_json, bool isSlider_) {
        label = slider_or_rotary_json["label"].get<std::string>();
        value = slider_or_rotary_json["default"].get<double>();
        paramID = label2ParamID(label);
        min = slider_or_rotary_json["min"].get<double>();
        max = slider_or_rotary_json["max"].get<double>();
        defaultVal = slider_or_rotary_json["default"].get<double>();
        isSlider = isSlider_;
        isRotary = !isSlider_;
        isButton = false;
        isToggle = false;
        isChanged = true;   // first time update is always true
    }

    void InitializeButton(json button_json) {
        label = button_json["label"].get<std::string>();
        value = 0;
        paramID = label2ParamID(label);
        min = std::nullopt;
        max = std::nullopt;
        defaultVal = 0;
        isSlider = false;
        isRotary = false;
        isButton = true;
        isToggle = button_json["isToggle"].get<bool>();
        isChanged = isToggle; // only true if isToggle.
        // We don't want to trigger the button if it's not a toggle
    }

    void InitializeCombobox(json comboBox_json) {
        label = comboBox_json["label"].get<std::string>();
        value = 1;
        paramID = label2ParamID(label);
        min = 1;
        comboBoxOptions = comboBox_json["options"].get<std::vector<std::string>>();
        max = comboBoxOptions.size();
        defaultVal = 1;
        isSlider = false;
        isRotary = false;
        isButton = false;
        isToggle = false;
        isComboBox = true;
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
                break;
            }
        }
        return isChanged;
    }

    void print() {
        for (auto &parameter: Parameters) {
            cout << parameter.label << " " << parameter.value << endl;
        }
    }

    [[nodiscard]] bool changed() const {
        return isChanged;
    }

    void setChanged(bool isChanged_) {
        isChanged = isChanged_;
    }

    [[maybe_unused]] bool wasParamUpdated(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                return parameter.isChanged;
            }
        }
    }

    [[maybe_unused]] std::vector<string> getLabelsForUpdatedParams(){
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
    [[maybe_unused]] double getValueFor(const string &label) {
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
    [[maybe_unused]] bool wasButtonClicked(const string &label) {
        for (auto &parameter: Parameters) {
            if (parameter.paramID == label2ParamID(label)) {
                if (parameter.isButton) {
                    if(parameter.isChanged) {
                        parameter.isChanged = false;
                        return true;
                    } else {
                        return false;
                    }
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
                    return parameter.comboBoxOptions[(size_t) parameter.value];
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

            for (const auto &slider_json: slidersList) {
                auto label = slider_json["label"].get<std::string>();
                assertLabelIsUnique(label);
                auto param_ = param();
                param_.InitializeSlider(slider_json, true);
                Parameters.emplace_back(param_);
            }

            for (const auto &vslider_json: vslidersList) {
                auto label = vslider_json["label"].get<std::string>();
                assertLabelIsUnique(label);
                auto param_ = param();
                param_.InitializeSlider(vslider_json, true);
                Parameters.emplace_back(param_);
            }

            for (const auto &rotary_json: rotariesList) {
                auto label = rotary_json["label"].get<std::string>();
                assertLabelIsUnique(label);
                auto param_ = param();
                param_.InitializeSlider(rotary_json, false);
                Parameters.emplace_back(param_);
            }

            for (const auto &button_json: buttonsList) {
                std::string label = button_json["label"].get<std::string>();
                assertLabelIsUnique(label);
                auto param_ = param();
                param_.InitializeButton(button_json);
                Parameters.emplace_back(param_);
            }

            for (const auto &comboBox_json: comboBoxesList) {
                std::string label = comboBox_json["label"].get<std::string>();
                assertLabelIsUnique(label);
                auto param_ = param();
                param_.InitializeCombobox(comboBox_json);
                Parameters.emplace_back(param_);
            }
        }
    }

    static string bool2string(bool b) { return b ? "true" : "false"; }
};



