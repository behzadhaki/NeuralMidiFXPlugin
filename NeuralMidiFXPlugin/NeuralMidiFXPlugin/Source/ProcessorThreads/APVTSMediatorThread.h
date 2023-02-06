//
// Created by Behzad Haki on 2022-09-02.
//

#pragma once

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../settings.h"
#include "../Includes/CustomStructsAndLockFreeQueue.h"

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
// ==========           ProcessorThreads as well as the processBlock()
// ============================================================================================================
class APVTSMediatorThread: public juce::Thread
{
public:
    juce::StringArray paths;

    // I want to declare these as private, but if I do it lower down it doesn't work..
    size_t numSliders{};
    size_t numRotaries{};


    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    APVTSMediatorThread(): juce::Thread("APVTSMediatorThread"){}

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
            juce::AudioProcessorValueTreeState *APVTSPntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_GuiParams_QuePntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2MDL_GuiParams_QuePntr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_GuiParams_QuePntr_) {

        // Resources Provided from NMP
        APVTSPntr = APVTSPntr_;
        APVM2ITP_GuiParams_QuePntr = APVM2ITP_GuiParams_QuePntr_;
        APVM2MDL_GuiParams_QuePntr = APVM2MDL_GuiParams_QuePntr_;
        APVM2PPP_GuiParams_QuePntr = APVM2PPP_GuiParams_QuePntr_;

        // Get UIObjects in settings.h
        auto tabList = UIObjects::Tabs::tabList;
        size_t numTabs = tabList.size();


        // Naming of Slider + Rotary Parameter ID's per tab
        for (size_t j = 0; j < numTabs; j++) {
            numSliders = std::get<1>(tabList[j]).size();
            numRotaries = std::get<2>(tabList[j]).size();

            for (size_t i = 0; i < numSliders; i++) {
                auto ID = "Slider_" + to_string(j) + to_string(i);
                sliderParamIDS.push_back(ID);
            }

            for (size_t i = 0; i < numRotaries; i++) {
                auto ID = "Rotary_" + to_string(j) + to_string(i);
                rotaryParamIDS.push_back(ID);
            }
        }

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

        auto current_slider_values = get_slider_values();
        auto current_rotary_values = get_rotary_values();
        guiParams.print();

        while (!bExit) {
            if (APVTSPntr != nullptr) {
                auto new_slider_values = get_slider_values();
                if (new_slider_values != current_slider_values) {
                    current_slider_values = new_slider_values;
                    // Update Lock Free Queues with new values
//                    APVM2ITP_slider_values_que->push(new_slider_values);
//                    APVM2PPP_slider_values_que->push(new_slider_values);
//                    APVM2MDL_slider_values_que->push(new_slider_values);
//                    APVM2NMP_slider_values_que->push(new_slider_values);
                }

                auto new_rotary_values = get_rotary_values();
                if (new_rotary_values != current_rotary_values) {
                    current_rotary_values = new_rotary_values;
//                    APVM2ITP_rotary_values_que->push(new_rotary_values);
//                    APVM2PPP_rotary_values_que->push(new_rotary_values);
//                    APVM2MDL_rotary_values_que->push(new_rotary_values);
//                    APVM2NMP_rotary_values_que->push(new_rotary_values);
                }

                bExit = threadShouldExit();

                // avoid burning CPU, if reading is returning immediately
                sleep(thread_configurations::APVTSMediatorThread::waitTimeBtnIters);
            }
        }
    }
    // ============================================================================================================


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
        if (not readyToStop) {
            prepareToStop();
        }
    }

private:
    // ============================================================================================================
    // ===          Utility Methods and Parameters
    // ============================================================================================================

    std::vector<float> get_slider_values() {
        std::vector<float> newSliderArray;
        for (size_t i = 0; i < numSliders; ++i) {
            auto paramRef = sliderParamIDS[i];
            newSliderArray.push_back(float(*APVTSPntr->getRawParameterValue(paramRef)));
        }
        return newSliderArray;
    }


    std::vector<float> get_rotary_values() {
        std::vector<float> newRotaryArray;
        for (size_t i = 0; i < numRotaries; ++i) {
            auto paramRef = rotaryParamIDS[i];
            newRotaryArray.push_back(float(*APVTSPntr->getRawParameterValue(paramRef)));
        }
        return newRotaryArray;
    }



    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_GuiParams_QuePntr{nullptr};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2MDL_GuiParams_QuePntr{nullptr};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_GuiParams_QuePntr{nullptr};

    std::vector<std::string> sliderParamIDS;
    std::vector<std::string> rotaryParamIDS;
    GuiParams guiParams{};

//    // ============================================================================================================
//    // ===          pointer to NeuralMidiFXPluginProcessor
//    // ============================================================================================================
//    InputTensorPreparatorThread *inputThread{nullptr};
//    ModelThread *modelThread{nullptr};
//    PlaybackPreparatorThread *playbackPreparatorThread{nullptr};

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState *APVTSPntr{nullptr};
};
