//
// Created by Behzad Haki on 2022-09-02.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "GuiParameters.h"
#include "LockFreeQueue.h"
#include <mutex>

inline std::vector<juce::ParameterID> get_params_to_default() {
    std::vector<std::string> params;

    auto loaded_json_ = load_settings_json();

    for (const auto& tabJson: loaded_json_["UI"]["Tabs"]["tabList"])
    {
        if (tabJson.contains("sliders")) {
            for (const auto& sliderJson: tabJson["sliders"])
            {
                if (sliderJson.contains("allow_reset_to_default"))
                {
                    if (sliderJson["allow_reset_to_default"].get<bool>())
                    {
                        params.push_back(sliderJson["label"].get<std::string>());
                    }
                } else {
                    params.push_back(sliderJson["label"].get<std::string>());
                }
            }
        }

        if (tabJson.contains("rotaries")) {
            for (const auto& rotaryJson: tabJson["rotaries"])
            {
                if (rotaryJson.contains("allow_reset_to_default"))
                {
                    if (rotaryJson["allow_reset_to_default"].get<bool>())
                    {
                        params.push_back(rotaryJson["label"].get<std::string>());
                    }
                } else {
                    params.push_back(rotaryJson["label"].get<std::string>());
                }
            }
        }

        if (tabJson.contains("buttons")) {
            for (const auto& buttonJson: tabJson["buttons"])
            {
                bool isToggle = false;
                if (buttonJson.contains("isToggle"))
                {
                    isToggle = buttonJson["isToggle"].get<bool>();
                }

                if (isToggle) {
                    if (buttonJson.contains("allow_reset_to_default"))
                    {
                        if (buttonJson["allow_reset_to_default"].get<bool>())
                        {
                            params.push_back(buttonJson["label"].get<std::string>());
                        }
                    } else {
                        params.push_back(buttonJson["label"].get<std::string>());
                    }
                }
            }
        }

        if (tabJson.contains("hsliders")) {
            for (const auto& hsliderJson: tabJson["hsliders"])
            {
                if (hsliderJson.contains("allow_reset_to_default"))
                {
                    if (hsliderJson["allow_reset_to_default"].get<bool>())
                    {
                        params.push_back(hsliderJson["label"].get<std::string>());
                    }
                } else {
                    params.push_back(hsliderJson["label"].get<std::string>());
                }
            }
        }

        if (tabJson.contains("comboBoxes")) {
            for (const auto& comboBoxJson: tabJson["comboBoxes"])
            {
                if (comboBoxJson.contains("allow_reset_to_default"))
                {
                    if (comboBoxJson["allow_reset_to_default"].get<bool>())
                    {
                        params.push_back(
                            comboBoxJson["label"].get<std::string>());
                    }
                } else {
                    params.push_back(
                        comboBoxJson["label"].get<std::string>());
                }
            }
        }

    }

    // parse using label2ParamID
    std::vector<juce::ParameterID> params_juce;
    for (const auto& param: params) {
        params_juce.emplace_back(label2ParamID(param));
    }

    return params_juce;
}

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
                bool isToggle = false;
                if (buttonJson.contains("isToggle"))
                {
                    isToggle = buttonJson["isToggle"].get<bool>();
                }

                if (isToggle) {
                    if (buttonJson.contains("exclude_from_presets"))
                    {
                        if (buttonJson["exclude_from_presets"].get<bool>())
                        {
                            excluded_params.push_back(buttonJson["label"].get<std::string>());
                        }
                    }
                } else {
                    // other buttons should be excluded from presets
                    excluded_params.push_back(buttonJson["label"].get<std::string>());
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

template<typename T> struct ThreadSafeData {
private:
    T data{};
    std::mutex mutex;

public:
    ThreadSafeData() = default;

    explicit ThreadSafeData(const T& data_) : data(data_) {}

    void setValues(T data_) {
        if (mutex.try_lock()) {
            data = data_;
            mutex.unlock(); // Don't forget to unlock after use
        } else {
            // Mutex was already locked; handle this case as needed
        }
    }

    T get() {
        std::lock_guard<std::mutex> lock(mutex);
        return data;
    }
};


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
        juce::Thread("APVTSMediatorThread"), CustomPresetData(prst) {
        paramIDs2Exclude = get_preset_excluded_params();
        paramIDs2ResetToDefault = get_params_to_default();
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
            juce::AudioProcessorValueTreeState *APVTSPntr_,
            StaticLockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2DPL_GuiParams_QuePntr_,
            StaticLockFreeQueue<vector<float>, queue_settings::APVM_que_size>  *APVTM2NMP_StandaloneParameters_QuePntr_
        ) {

        // Resources Provided from NMP
        APVTSPntr = APVTSPntr_;
        APVM2DPL_GuiParams_QuePntr = APVM2DPL_GuiParams_QuePntr_;
        APVTM2NMP_StandaloneParameters_QuePntr = APVTM2NMP_StandaloneParameters_QuePntr_;

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
                // check if reset requested
                auto resetToDefault = APVTSPntr->getRawParameterValue(label2ParamID("ResetToDefaults__"));
                if(resetToDefault != nullptr) {
                    if (resetToDefault->load() > 0.5f) {
                        // reset the parameters to default
                        resetAllParamsToDefault();
                        // reset the resetToDefault parameter
                        APVTSPntr->getParameter(label2ParamID("ResetToDefaults__"))->setValueNotifyingHost(0.0f);
                    }
                }

                // check standalone parameters
                if (APVTM2NMP_StandaloneParameters_QuePntr != nullptr) {
                    vector<float> new_params;
                    bool changed = false;
                    auto qpm_ptr = APVTSPntr->getRawParameterValue(label2ParamID("TempoStandalone"));
                    if (qpm_ptr != nullptr) {
                        float new_qpm = *qpm_ptr;
                        if (abs(new_qpm-qpm) > 0.01) {
                            qpm = new_qpm;
                            changed = true;
                        }
                        new_params.push_back(qpm);
                    }
                    auto is_playing_ptr = APVTSPntr->getRawParameterValue(label2ParamID("IsPlayingStandalone"));
                    if (is_playing_ptr != nullptr) {
                        int new_is_playing = int(*is_playing_ptr);
                        if (new_is_playing != is_playing) {
                            is_playing = new_is_playing;
                            changed = true;
                        }
                        new_params.push_back((float)is_playing);
                    }

                    auto is_recording_ptr = APVTSPntr->getRawParameterValue(label2ParamID("IsRecordingStandalone"));
                    if (is_recording_ptr != nullptr) {
                        int new_is_recording = int(*is_recording_ptr);
                        if (new_is_recording != is_recording) {
                            is_recording = new_is_recording;
                            changed = true;
                        }
                        new_params.push_back((float)is_recording);
                    }

                    auto denominator_ptr = APVTSPntr->getRawParameterValue(label2ParamID("TimeSigDenominatorStandalone"));
                    if (denominator_ptr != nullptr) {
                        int new_denominator = int(*denominator_ptr);
                        if (new_denominator != denominator) {
                            denominator = new_denominator;
                            changed = true;
                        }
                        new_params.push_back((float)denominator);
                    }

                    auto numerator_ptr = APVTSPntr->getRawParameterValue(label2ParamID("TimeSigNumeratorStandalone"));
                    if (numerator_ptr != nullptr) {
                        int new_numerator = int(*numerator_ptr);
                        if (new_numerator != numerator) {
                            numerator = new_numerator;
                            changed = true;
                        }
                        new_params.push_back((float)numerator);

                    }

                    if (changed) {
                        APVTM2NMP_StandaloneParameters_QuePntr->push(new_params);
                    }
                }


                // check if selected preset has changed
                auto selectedPreset = (int) *APVTSPntr->getRawParameterValue(label2ParamID("Preset"));
                if (selectedPreset != prev_selectedPreset) {

                    // if selected preset has changed, send a message to the deployment thread
                    // to load the new preset
                    prev_selectedPreset = selectedPreset;

                    load_preset(selectedPreset, false);

                }

                // check if host requested loading a new preset
                if (shouldForcePushPreset.get()) {
                    load_preset(hostProvidedPreset.get(), true);
                    shouldForcePushPreset.setValues(false);
                }

                // check if the host state has provided any new saved states
                if (isHostStateUpdated.get()) {
                    auto newHostState = newHostProvidedState.get();
                    APVTSPntr->replaceState(newHostState);
                    guiParamsPntr = make_unique<GuiParams>(APVTSPntr);
                    guiParamsPntr->ignoreTriggerButtons();
                    APVM2DPL_GuiParams_QuePntr->push(*guiParamsPntr);
                    isHostStateUpdated.setValues(false);
                } else {
                    if (guiParamsPntr->update()) {
                        if (APVM2DPL_GuiParams_QuePntr != nullptr) {
                            APVM2DPL_GuiParams_QuePntr->push(*guiParamsPntr);
                        }
                    }
                }


                bExit = threadShouldExit();

                // avoid burning CPU, if reading is returning immediately
                sleep((int)thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
            }
        }


    }
    // ============================================================================================================
    // ===          Reset All Parameters to Default
    void resetAllParamsToDefault() {
        // reset all parameters to default
        for (const auto& paramID: paramIDs2ResetToDefault) {
            if (APVTSPntr->getParameter(paramID.getParamID()) != nullptr) {
                auto param = APVTSPntr->getParameter(paramID.getParamID());
                param->setValueNotifyingHost(param->getDefaultValue());
            }
        }
    }

    // ============================================================================================================
    void load_preset(const int preset_idx, bool force_push_preset) {
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
                std::vector<pair<juce::ParameterID, float>> values_not_to_load;
                for (const auto& paramID: paramIDs2Exclude) {
                    if (APVTSPntr->getParameter(paramID.getParamID()) != nullptr) {
                        values_not_to_load.emplace_back(paramID.getParamID(), APVTSPntr->getParameter(paramID.getParamID())->getValue());
                    }
                }

                // 3.b load the preset and reset the APVTS with the new preset
                APVTSPntr->replaceState(juce::ValueTree::fromXml(*xml));

                // 3.c use the values in 3.a to reset the parameters that are not to be loaded
                // We store all params in APVTS but just loading it when needed
                for (const auto& pair: values_not_to_load) {
                    auto paramID = pair.first;
                    auto value = pair.second;
                    // check if the parameter is in the APVTS
                    if (APVTSPntr->getParameter(paramID.getParamID()) != nullptr) {
                        APVTSPntr->getParameter(paramID.getParamID())->setValueNotifyingHost(value);
                    }
                }

                // 4. load the tensor preset data associated with the preset
                auto filePath = xml->getStringAttribute("filePath");
                auto tensormap = load_tensor_map(filePath.toStdString());
                CustomPresetData->copy_from_map(tensormap);


               // 5. force push the preset to the deployment thread (this will be used on startup only)
                if (force_push_preset) {
                    CustomPresetData->forceAllToChanged();
                    guiParamsPntr = make_unique<GuiParams>(APVTSPntr);
                    APVM2DPL_GuiParams_QuePntr->push(*guiParamsPntr);
                }

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

    // ============================================================================================================
    // ===          Load the Preset
    // ============================================================================================================
    ThreadSafeData<bool> shouldForcePushPreset{false};
    ThreadSafeData<int> hostProvidedPreset{-1};

    ThreadSafeData<bool> isHostStateUpdated {false};
    ThreadSafeData<juce::ValueTree> newHostProvidedState;



private:
    CustomPresetDataDictionary *CustomPresetData;

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    StaticLockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2DPL_GuiParams_QuePntr{nullptr};
    StaticLockFreeQueue<vector<float>, queue_settings::APVM_que_size>  *APVTM2NMP_StandaloneParameters_QuePntr{nullptr};

    unique_ptr<GuiParams> guiParamsPntr;

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState *APVTSPntr{nullptr};

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    std::vector<ParameterID> paramIDs2Exclude; // {get_preset_excluded_params()};
    std::vector<ParameterID> paramIDs2ResetToDefault; // {get_params_to_default()};


    // ============================================================================================================
    // ===          Standalone Params
    // ============================================================================================================
    float qpm{-1};
    int is_playing{0};
    int is_recording{0};
    int denominator{4};
    int numerator{4};



};
