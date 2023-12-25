//
// Created by Behzad Haki on 2022-09-02.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "GuiParameters.h"
#include "LockFreeQueue.h"

#pragma once


inline std::vector<juce::ParameterID> get_preset_excluded_params() {
    std::vector<std::string> excluded_params;
    auto loaded_json_ = load_settings_json();
    for (const auto& tabJson: loaded_json_["UI"]["Tabs"]["tabList"])
    {
        if (tabJson.contains("sliders")) {
            for (const auto& sliderJson: tabJson["sliders"])
            {
                if (sliderJson.contains("exclude_from_presets"))
                {
                    if (sliderJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(sliderJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("rotaries")) {
            for (const auto& rotaryJson: tabJson["rotaries"])
            {
                if (rotaryJson.contains("exclude_from_presets"))
                {
                    if (rotaryJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(rotaryJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("buttons")) {
            for (const auto& buttonJson: tabJson["buttons"])
            {
                if (buttonJson.contains("exclude_from_presets"))
                {
                    if (buttonJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(buttonJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("hsliders")) {
            for (const auto& hsliderJson: tabJson["hsliders"])
            {
                if (hsliderJson.contains("exclude_from_presets"))
                {
                    if (hsliderJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(hsliderJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("comboBoxes")) {
            for (const auto& comboBoxJson: tabJson["comboBoxes"])
            {
                if (comboBoxJson.contains("exclude_from_presets"))
                {
                    if (comboBoxJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(
                                comboBoxJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("MidiDisplays")) {
            for (const auto& midiDisplayJson: tabJson["MidiDisplays"])
            {
                if (midiDisplayJson.contains("exclude_from_presets"))
                {
                    if (midiDisplayJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(
                                midiDisplayJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("AudioDisplays")) {
            for (const auto& audioDisplayJson: tabJson["AudioDisplays"])
            {
                if (audioDisplayJson.contains("exclude_from_presets"))
                {
                    if (audioDisplayJson["exclude_from_presets"].get<bool>())
                    {
                        excluded_params.push_back(
                                audioDisplayJson["label"].get<std::string>());
                    }
                }
            }
        }
    }

    if (loaded_json_["StandaloneTransportPanel"].contains("exclude_tempo_from_presets")) {
        if (loaded_json_["StandaloneTransportPanel"]["exclude_tempo_from_presets"].get<bool>()) {
            excluded_params.emplace_back("TempoStandalone");
        }
    }

    if (loaded_json_["StandaloneTransportPanel"].contains("exclude_time_signature_from_presets")) {
        if (loaded_json_["StandaloneTransportPanel"]["exclude_time_signature_from_presets"].get<bool>()) {
            excluded_params.emplace_back("TimeSigNumeratorStandalone");
            excluded_params.emplace_back("TimeSigDenominatorStandalone");
        }
    }

    if (loaded_json_["StandaloneTransportPanel"].contains("exclude_play_button_from_presets")) {
        if (loaded_json_["StandaloneTransportPanel"]["exclude_play_button_from_presets"].get<bool>()) {
            excluded_params.emplace_back("IsPlayingStandalone");
        }
    }

    if (loaded_json_["StandaloneTransportPanel"].contains("exclude_record_button_from_presets")) {
        if (loaded_json_["StandaloneTransportPanel"]["exclude_record_button_from_presets"].get<bool>()) {
            excluded_params.emplace_back("IsRecordingStandalone");
        }
    }

    // parse using label2ParamID
    std::vector<juce::ParameterID> excluded_params_juce;
    for (const auto& param: excluded_params) {
        excluded_params_juce.emplace_back(label2ParamID(param));
    }

    return excluded_params_juce;

}

// ============================================================================================================
// ==========         This Thread is in charge of checking which parameters in APVTS have been changed.
// ==========           If changed, the updated value will be pushed to a corresponding queue to be read
// ==========           by the receiving/destination thread.
// ==========
// ==========         To read from APVTS, we always get a std::atomic pointer, which potentially
// ==========           can block a thread if some read/write race is happening. As a result, it is not
// ==========           safe to directly read from APVTS inside the processBlock() thread or any other
// ==========           time sensitive threads
// ==========
// ==========         In this plugin, the GrooveThread and the ModelThread are not time-sensitive. So,
// ==========           they can read from APVTS directly. Regardless, in future iterations, perhaps
// ==========           these requirements change. To be future-proof, this thread has been implemented
// ==========           to take care of mediating the communication of parameters in the APVTS to the
// ==========           DeploymentThreads as well as the processBlock()
// ============================================================================================================
class APVTSMediatorThread: public juce::Thread
{
public:
    juce::StringArray paths;

    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    explicit APVTSMediatorThread(CustomPresetDataDictionary *prst) :
        juce::Thread("APVTSMediatorThread"), CustomPresetData(prst) {}

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
            juce::AudioProcessorValueTreeState *APVTSPntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2DPL_GuiParams_QuePntr_) {

        // Resources Provided from NMP
        APVTSPntr = APVTSPntr_;
        APVM2DPL_GuiParams_QuePntr = APVM2DPL_GuiParams_QuePntr_;

        guiParamsPntr = make_unique<GuiParams>(APVTSPntr_);
        APVM2DPL_GuiParams_QuePntr->push(*guiParamsPntr);

        startThread();
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override {
        // notify if the thread is still running
        bool bExit = threadShouldExit();

        // check selected preset
        int prev_selectedPreset = -1;
        while (!bExit) {
            if (APVTSPntr != nullptr) {
                if (guiParamsPntr->update()) {
                    if (APVM2DPL_GuiParams_QuePntr != nullptr) {
                        APVM2DPL_GuiParams_QuePntr->push(*guiParamsPntr);
                    }
                }

                // check if selected preset has changed
                auto selectedPreset = (int) *APVTSPntr->getRawParameterValue(label2ParamID("Preset"));
                if (selectedPreset != prev_selectedPreset) {
                    // if selected preset has changed, send a message to the deployment thread
                    // to load the new preset
                    prev_selectedPreset = selectedPreset;

                    load_preset(selectedPreset);

                }

                bExit = threadShouldExit();

                // avoid burning CPU, if reading is returning immediately
                sleep((int)thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
            }
        }


    }
    // ============================================================================================================
    void load_preset(const int preset_idx) {
        // XML file paths
        std::string fp = stripQuotes(default_preset_dir) + path_separator + std::to_string(preset_idx) + ".apvts";
        std::string fp_data = stripQuotes(default_preset_dir) + path_separator + std::to_string(preset_idx) + ".preset_data";

        // Step 1: Read the XML from the file
        auto xml = juce::XmlDocument::parse(juce::File(fp));

        // Step 2: Get the ValueTree from the XML
        if (xml != nullptr)
        {
            if (xml->hasTagName(APVTSPntr->state.getType()))
            {
                // 3.a get the existing values of the parameters that are not to be loaded
                std::vector<float> values_not_to_load;
                for (const auto& paramID: paramIDs2Exclude) {
                    values_not_to_load.emplace_back(APVTSPntr->getParameter(paramID.getParamID())->getValue());
                }

                // 3.b load the preset and reset the APVTS with the new preset
                APVTSPntr->replaceState(juce::ValueTree::fromXml(*xml));

                // 3.c use the values in 3.a to reset the parameters that are not to be loaded
                // We store all params in APVTS but just loading it when needed
                for (int i = 0; i < paramIDs2Exclude.size(); i++) {
                    APVTSPntr->getParameter(paramIDs2Exclude[i].getParamID())->setValueNotifyingHost(values_not_to_load[i]);
                }

                // 4. load the tensor preset data associated with the preset
                auto filePath = xml->getStringAttribute("filePath");
                auto tensormap = load_tensor_map(filePath.toStdString());
                CustomPresetData->copy_from_map(tensormap);
                CustomPresetData->printTensorMap();
            }

        }

}

    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped from a parent thread

    // run this in destructor destructing object
    void prepareToStop() {
        //Need to wait enough to ensure the run() method is over before killing thread
        this->stopThread(int(100 * thread_configurations::APVTSMediatorThread::waitTimeBtnIters));
        readyToStop = true;
    }

    ~APVTSMediatorThread() override {
        if (!readyToStop) {
            prepareToStop();
        }
    }

private:
    CustomPresetDataDictionary *CustomPresetData;

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2DPL_GuiParams_QuePntr{nullptr};

    unique_ptr<GuiParams> guiParamsPntr;

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState *APVTSPntr{nullptr};

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    std::vector<ParameterID> paramIDs2Exclude {get_preset_excluded_params()};

};
