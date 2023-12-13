//
// Created by Behzad Haki on 2022-09-02.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "GuiParameters.h"
#include "LockFreeQueue.h"

#pragma once


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
                if (guiParamsPntr->update(APVTSPntr)) {
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
                sleep(thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
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
            juce::ValueTree xmlState = juce::ValueTree::fromXml(*xml);

            if (xml->hasTagName(APVTSPntr->state.getType()))
            {
                APVTSPntr->replaceState(juce::ValueTree::fromXml(*xml));

                auto filePath = xml->getStringAttribute("filePath");
                // Handle the file path as needed

                // cout << "Loading preset from: " << filePath << endl;
                auto tensormap = load_tensor_map(filePath.toStdString());

                CustomPresetData->copy_from_map(tensormap);

                // cout << "Loaded tensor map: [x1] = " << *CustomPresetData->tensor("x1") << endl;
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
        this->stopThread(100 * thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
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
};
