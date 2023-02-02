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
    int numSliders;
    int numRotaries;

    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    APVTSMediatorThread(InputTensorPreparatorThread* inputThreadPntr,
                        PlaybackPreparatorThread* playbackThreadPntr)
        : juce::Thread("APVTSMediatorThread")
    {
        inputThread = inputThreadPntr;
        modelThread = playbackThreadPntr;
        //paths = get_pt_files_in_default_path(); //JL: Where is this defined?
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(juce::AudioProcessorValueTreeState* APVTSPntr)
    {
        APVTS = APVTSPntr;

        auto tabList = UIObjects::Tabs::tabList;
        int numTabs = tabList.size();


        // Naming of Slider + Rotary Parameter ID's per tab
        for (int j = 0; j < numTabs; j++)
        {
            numSliders = std::get<1>(tabList[j]).size();
            numRotaries = std::get<2>(tabList[j]).size();


            for (int i = 0; i < numSliders; i++)
            {
                auto ID = "Slider_" + to_string(j) + to_string(i);
                sliderParamIDS.push_back(ID);
            }

            for (int i = 0; i < numRotaries; i++)
            {
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
    void run() override
    {
        // notify if the thread is still running
        bool bExit = threadShouldExit();

        auto current_slider_values = get_slider_values();
        auto current_rotary_values = get_rotary_values();

        while (!bExit)
        {
            if (APVTS != nullptr)
            {
                auto new_slider_values = get_slider_values();
                if (new_slider_values != current_slider_values)
                {
                    current_slider_values = new_slider_values;
                    // Update Lock Free Queues with new values
//                    APVM2ITP_slider_values_que->push(new_slider_values);
//                    APVM2PPP_slider_values_que->push(new_slider_values);
//                    APVM2MDL_slider_values_que->push(new_slider_values);
//                    APVM2NMP_slider_values_que->push(new_slider_values);
                }

                auto new_rotary_values = get_rotary_values();
                if (new_rotary_values != current_rotary_values)
                {
                    current_rotary_values = new_rotary_values;
//                    APVM2ITP_rotary_values_que->push(new_rotary_values);
//                    APVM2PPP_rotary_values_que->push(new_rotary_values);
//                    APVM2MDL_rotary_values_que->push(new_rotary_values);
//                    APVM2NMP_rotary_values_que->push(new_rotary_values);
                }

                bExit = threadShouldExit();

                sleep (thread_settings::APVTSMediatorThread::waitTimeBtnIters); // avoid burning CPU, if reading is returning immediately
            }
        }
    }
    // ============================================================================================================


    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    bool readyToStop {false}; // Used to check if thread is ready to be stopped or externally stopped from a parent thread

    // run this in destructor destructing object
    void prepareToStop(){
        //Need to wait enough to ensure the run() method is over before killing thread
        this->stopThread(100 * thread_settings::APVTSMediatorThread::waitTimeBtnIters);
        readyToStop = true;
    }

    ~APVTSMediatorThread() override {
        if (not readyToStop)
        {
            prepareToStop();
        }
    }

private:
    // ============================================================================================================
    // ===          Utility Methods and Parameters
    // ============================================================================================================

    std::vector<float> get_slider_values()
    {
        std::vector<float> newSliderArray;
        for (int i = 0; i < numSliders; ++i)
        {
            auto paramRef = sliderParamIDS[i];
            newSliderArray.push_back(float(*APVTS->getRawParameterValue(paramRef)));
        }

        return newSliderArray;
    }


    std::vector<float> get_rotary_values()
    {
        std::vector<float> newRotaryArray;
        for (int i = 0; i < numRotaries; ++i)
        {
            auto paramRef = rotaryParamIDS[i];
            newRotaryArray.push_back(float(*APVTS->getRawParameterValue(paramRef)));
        }

        return newRotaryArray;
    }

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================

//    LockFreeQueue<std::array<float, numSliders>, 512>* APVM2ITP_slider_values_que {nullptr};
//    LockFreeQueue<std::array<float, numSliders>, 512>* APVM2PPP_slider_values_que {nullptr};
//    LockFreeQueue<std::array<float, numSliders>, 512>* APVM2MDL_slider_values_que {nullptr};
//    LockFreeQueue<std::array<float, numSliders>, 512>* APVM2NMP_slider_values_que {nullptr};
//
//    LockFreeQueue<std::array<float, numRotaries>, 512>* APVM2ITP_rotary_values_que {nullptr};
//    LockFreeQueue<std::array<float, numRotaries>, 512>* APVM2PPP_rotary_values_que {nullptr};
//    LockFreeQueue<std::array<float, numRotaries>, 512>* APVM2MDL_rotary_values_que {nullptr};
//    LockFreeQueue<std::array<float, numRotaries>, 512>* APVM2NMP_rotary_values_que {nullptr};
//
    std::vector<std::string> sliderParamIDS;
    std::vector<std::string> rotaryParamIDS;

    // ============================================================================================================
    // ===          pointer to NeuralMidiFXPluginProcessor
    // ============================================================================================================
    InputTensorPreparatorThread* inputThread;
    PlaybackPreparatorThread* modelThread;

    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState* APVTS {nullptr};
};
