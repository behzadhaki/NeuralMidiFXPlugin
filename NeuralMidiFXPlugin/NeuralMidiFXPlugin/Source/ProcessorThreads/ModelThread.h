//
// Created by Behzad Haki on 2022-08-13.
//

#ifndef JUCECMAKEREPO_MODELTHREAD_H
#define JUCECMAKEREPO_MODELTHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../InterThreadFifos.h"
#include "../settings.h"
#include "../Model/ModelAPI.h"

inline juce::StringArray get_pt_files_in_default_path()
{
    // find models in default folder
    juce::StringArray paths;
    paths.clear();
    for (const auto& filenameThatWasFound : juce::File (GeneralSettings::default_model_folder).findChildFiles (2, true, "*.pt"))
    {
        paths.add (filenameThatWasFound.getFileNameWithoutExtension());
    }
    paths.sort(false);
    return paths;
}


class ModelThread: public juce::Thread/*, public juce::ChangeBroadcaster*/
{
public:
    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    ModelThread();
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQuesPntr,
        GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>*  ModelThreadToProcessBlockQuesPntr,
        HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>* ModelThreadToDrumPianoRollWidgetQuesPntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_QuePntr,
        LockFreeQueue<std::array<float, HVO_params::num_voices+1>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_QuePntr,
        LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_QuePntr);
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override;
    // ============================================================================================================


    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    void prepareToStop();     // run this in destructor destructing object
    ~ModelThread() override;


    // ============================================================================================================
    // ===          Utility Methods and Parameters
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Per voice generation controls stored locally (defaults are in settings.h)
    // ------------------------------------------------------------------------------------------------------------
    vector<float> perVoiceSamplingThresholds {nine_voice_kit_default_sampling_thresholds};
    vector<float> perVoiceMaxNumVoicesAllowed {nine_voice_kit_default_max_voices_allowed};
    // ------------------------------------------------------------------------------------------------------------
    // ---         Input Groove and Generated HVO stored locally
    // ------------------------------------------------------------------------------------------------------------
    MonotonicGroove<HVO_params::time_steps> scaled_groove;
    HVO<HVO_params::time_steps, HVO_params::num_voices > generated_hvo;
    // ------------------------------------------------------------------------------------------------------------
    // ---         Other
    // ------------------------------------------------------------------------------------------------------------
    bool readyToStop; // Used to check if thread is ready to be stopped or externally stopped from a parent thread

    void UpdateModelPath(std::string new_model_path_);
    // ============================================================================================================


private:

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---          Output Queues
    // ------------------------------------------------------------------------------------------------------------
    GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>* ModelThreadToProcessBlockQue;
    HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>* ModelThreadToDrumPianoRollWidgetQue;
    // ------------------------------------------------------------------------------------------------------------
    // ---          Input Queues
    // ------------------------------------------------------------------------------------------------------------
    MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>* GrooveThreadToModelThreadQue;
    LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_max_num_hits_Que;
    LockFreeQueue<std::array<float, HVO_params::num_voices+1>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_sampling_thresholds_and_temperature_Que;
    LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>* APVTS2ModelThread_midi_mappings_Que;

    // ============================================================================================================
    // ===          Generative Torch Model
    // ============================================================================================================
    MonotonicGrooveTransformerV1 modelAPI;
    array <int, HVO_params::num_voices> drum_kit_midi_map {};
    string new_model_path {""};
};

#endif //JUCECMAKEREPO_MODELTHREAD_H
