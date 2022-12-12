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

    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    APVTSMediatorThread(GrooveThread* grooveThreadPntr, ModelThread* modelThreadPntr): juce::Thread("APVTSMediatorThread")
    {
        grooveThread = grooveThreadPntr;
        modelThread = modelThreadPntr;
        paths = get_pt_files_in_default_path();
    }

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        juce::AudioProcessorValueTreeState* APVTSPntr,
        LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr,
        LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_record_overdubToggles_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices+1>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_QuePntr,
        LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_QuePntr
        )
    {
        APVTS = APVTSPntr;
        APVTS2GrooveThread_groove_vel_offset_ranges_Que = APVTS2GrooveThread_groove_vel_offset_ranges_QuePntr;
        APVTS2GrooveThread_groove_record_overdubToggles_Que = APVTS2GrooveThread_groove_record_overdubToggles_QuePntr;
        APVTS2ModelThread_max_num_hits_Que = APVTS2ModelThread_max_num_hits_QuePntr;
        APVTS2ModelThread_sampling_thresholds_and_temperature_Que = APVTS2ModelThread_sampling_thresholds_and_temperature_QuePntr;
        APVTS2ModelThread_midi_mappings_Que = APVTS2ModelThread_midi_mappings_QuePntr;

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

        auto current_overdub_record_toggle_states = get_overdub_record_toggle_states();
        auto current_groove_vel_offset_ranges = get_groove_vel_offset_ranges();
        auto current_max_num_hits = get_max_num_hits();
        auto current_sampling_thresholds_with_temperature = get_sampling_thresholds_with_temperature();
        auto current_per_voice_midi_numbers = get_per_voice_midi_numbers();
        auto current_reset_buttons = get_reset_buttons();
        auto current_randomize_groove_buttons = get_randomize_groove_buttons();
        auto current_model_selected = get_model_selected();

        while (!bExit)
        {
            if (APVTS != nullptr)
            {
                auto new_overdub_record_toggle_states =
                    get_overdub_record_toggle_states();
                if (current_overdub_record_toggle_states
                    != new_overdub_record_toggle_states)
                {
                    current_overdub_record_toggle_states =
                        new_overdub_record_toggle_states;
                    APVTS2GrooveThread_groove_record_overdubToggles_Que->push(
                        new_overdub_record_toggle_states);
                }

                auto new_groove_vel_offset_ranges = get_groove_vel_offset_ranges();
                // check if vel/offset values changed
                if (current_groove_vel_offset_ranges != new_groove_vel_offset_ranges)
                {
                    current_groove_vel_offset_ranges = new_groove_vel_offset_ranges;
                    APVTS2GrooveThread_groove_vel_offset_ranges_Que->push(
                        new_groove_vel_offset_ranges);
                }

                // check if per voice allowed maximum number of hits has changed
                auto new_max_num_hits = get_max_num_hits();
                if (current_max_num_hits != new_max_num_hits)
                {
                    current_max_num_hits = new_max_num_hits;
                    APVTS2ModelThread_max_num_hits_Que->push(new_max_num_hits);
                }

                // check if per voice allowed sampling thresholds have changed
                auto new_sampling_thresholds_with_temperature =
                    get_sampling_thresholds_with_temperature();

                if (current_sampling_thresholds_with_temperature
                    != new_sampling_thresholds_with_temperature)
                {
                    current_sampling_thresholds_with_temperature =
                        new_sampling_thresholds_with_temperature;
                    APVTS2ModelThread_sampling_thresholds_and_temperature_Que->push(
                        new_sampling_thresholds_with_temperature);
                }

                // check if per voice midi numbers have changed
                auto new_per_voice_midi_numbers = get_per_voice_midi_numbers();
                if (current_per_voice_midi_numbers != get_per_voice_midi_numbers())
                {
                    current_per_voice_midi_numbers = new_per_voice_midi_numbers;
                    APVTS2ModelThread_midi_mappings_Que->push(new_per_voice_midi_numbers);
                }

                // check if per reset buttons have been clicked
                auto new_reset_buttons = get_reset_buttons();
                if (current_reset_buttons != new_reset_buttons)
                {
                    auto resetGrooveButtonClicked = (current_reset_buttons[0] !=  new_reset_buttons[0]);
                    auto resetSampleParamsClicked = (current_reset_buttons[1] !=  new_reset_buttons[1]);
                    auto resetAllClicked = (current_reset_buttons[2] !=  new_reset_buttons[2]);
                    
                    if (resetGrooveButtonClicked or resetAllClicked)
                    {
                        grooveThread->ForceResetGroove();
                    }
                    if (resetSampleParamsClicked  or resetAllClicked)
                    {
                        // reset parameters to default
                        for(const string &ParamID : {"VEL_BIAS", "VEL_DYNAMIC_RANGE", "OFFSET_BIAS", "OFFSET_RANGE"})
                        {
                            auto param = APVTS->getParameter(ParamID);
                            param->setValueNotifyingHost(param->getDefaultValue());
                        }
                        
                        for (size_t i=0; i < HVO_params::num_voices; i++)
                        {
                            auto ParamID = nine_voice_kit_labels[i];
                            auto param = APVTS->getParameter(ParamID+"_X");
                            param->setValueNotifyingHost(param->getDefaultValue());
                            param = APVTS->getParameter(ParamID+"_Y");
                            param->setValueNotifyingHost(param->getDefaultValue());
                            param = APVTS->getParameter(ParamID+"_MIDI");
                            param->setValueNotifyingHost(param->getDefaultValue());
                        }
                    }

                    // current_reset_buttons = new_reset_buttons;
                    reset_reset_buttons();
                }

                // check if random buttons have been clicked
                auto new_randomize_groove_buttons = get_randomize_groove_buttons();
                if (current_randomize_groove_buttons != new_randomize_groove_buttons)
                {
                    if (current_randomize_groove_buttons[0] != new_randomize_groove_buttons[0])
                    {
                        grooveThread->randomizeExistingVelocities();
                    }

                    if (current_randomize_groove_buttons[1] != new_randomize_groove_buttons[1])
                    {
                        grooveThread->randomizeExistingOffsets();
                    }

                    if (current_randomize_groove_buttons[2] != new_randomize_groove_buttons[2])
                    {
                        grooveThread->randomizeAll();
                    }

                    reset_random_buttons();
                }

                // check if new model selected
                auto new_model_selected = get_model_selected();
                if (current_model_selected != new_model_selected)
                {
                    current_model_selected = new_model_selected;
                    auto new_model_path = (string)GeneralSettings::default_model_folder + "/" + paths[current_model_selected].toStdString() + ".pt";
                    modelThread->UpdateModelPath(new_model_path);
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

    std::array<int, 2> get_overdub_record_toggle_states()
    {
        return {
            int(*APVTS->getRawParameterValue("OVERDUB")), int(*APVTS->getRawParameterValue("RECORD"))
        };
    }

    std::array<float, 4> get_groove_vel_offset_ranges()
    {
        return {
            *APVTS->getRawParameterValue("VEL_BIAS"), *APVTS->getRawParameterValue("VEL_DYNAMIC_RANGE"),
            *APVTS->getRawParameterValue("OFFSET_BIAS"), *APVTS->getRawParameterValue("OFFSET_RANGE"),
        };
    }

    std::array<float, HVO_params::num_voices> get_max_num_hits()
    {
        std::array<float, HVO_params::num_voices> max_num_hits {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            max_num_hits[i] = *APVTS->getRawParameterValue(voice_label+"_X");
        }
        return max_num_hits;
    }

    std::array<float, HVO_params::num_voices+1> get_sampling_thresholds_with_temperature()
    {
        std::array<float, HVO_params::num_voices+1> sampling_thresholds_with_temperature {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            sampling_thresholds_with_temperature[i] = *APVTS->getRawParameterValue(voice_label+"_Y");
        }
        sampling_thresholds_with_temperature[HVO_params::num_voices] = *APVTS->getRawParameterValue("Temperature");
        return sampling_thresholds_with_temperature;
    }

    std::array<int, HVO_params::num_voices> get_per_voice_midi_numbers()
    {
        std::array<int, HVO_params::num_voices> midiNumbers {};
        for (size_t i=0; i<HVO_params::num_voices; i++)
        {
            auto voice_label = nine_voice_kit_labels[i];
            midiNumbers[i] = int(*APVTS->getRawParameterValue(voice_label + "_MIDI"));
        }
        return midiNumbers;
    }

    // returns reset_groove, reset_samplingparams and reset all
    std::array<int, 3> get_reset_buttons()
    {
        return {(int)*APVTS->getRawParameterValue("RESET_GROOVE"),
                (int)*APVTS->getRawParameterValue("RESET_SAMPLINGPARAMS"),
                (int)*APVTS->getRawParameterValue("RESET_ALL")};
    }

    // returns all reset buttons (buttons are toggles, so when mouse release they should jump back)
    void reset_reset_buttons()
    {
        auto param = APVTS->getParameter("RESET_GROOVE");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RESET_SAMPLINGPARAMS");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RESET_ALL");
        param->setValueNotifyingHost(0);
    }

    // returns RANDOMIZE_VEL, RANDOMIZE_OFFSET and RANDOMIZE_ALL all
    std::array<int, 3> get_randomize_groove_buttons()
    {
        return {(int)*APVTS->getRawParameterValue("RANDOMIZE_VEL"),
                (int)*APVTS->getRawParameterValue("RANDOMIZE_OFFSET"),
                (int)*APVTS->getRawParameterValue("RANDOMIZE_ALL")};
    }

    // returns all reset buttons (buttons are toggles, so when mouse release they should jump back)
    void reset_random_buttons()
    {
        auto param = APVTS->getParameter("RANDOMIZE_VEL");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RANDOMIZE_OFFSET");
        param->setValueNotifyingHost(0);
        param = APVTS->getParameter("RANDOMIZE_ALL");
        param->setValueNotifyingHost(0);
    }

    // returns RANDOMIZE_VEL, RANDOMIZE_OFFSET and RANDOMIZE_ALL all
    int get_model_selected()
    {
        auto model_selected = (int)*APVTS->getRawParameterValue("MODEL");
        return model_selected;
    }

    // ============================================================================================================
    // ===          Output Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<std::array<float, 4>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_vel_offset_ranges_Que {nullptr};
    LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>* APVTS2GrooveThread_groove_record_overdubToggles_Que {nullptr};
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_Que {nullptr};
    LockFreeQueue<std::array<float, HVO_params::num_voices+1>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_Que {nullptr};
    LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_Que {nullptr};


    // ============================================================================================================
    // ===          pointer to NeuralMidiFXPluginProcessor
    // ============================================================================================================
    GrooveThread* grooveThread;
    ModelThread* modelThread;


    // ============================================================================================================
    // ===          Pointer to APVTS hosted in the Main Processor
    // ============================================================================================================
    juce::AudioProcessorValueTreeState* APVTS {nullptr};


};
