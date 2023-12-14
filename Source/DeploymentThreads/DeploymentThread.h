//
// Created by Behzad Haki on 2022-12-13.
//

#pragma once

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "../Includes/GuiParameters.h"
#include "../Includes/InputEvent.h"
#include "../Includes/LockFreeQueue.h"
#include "../Includes/Configs_Model.h"
#include "../Includes/colored_cout.h"
#include "../Includes/chrono_timer.h"
#include "../Includes/GenerationEvent.h"
#include "../Includes/TorchScriptAndPresetLoaders.h"
#include "PluginCode/DeploymentData.h"
#include "../Includes/MidiDisplayWidget.h"

class DeploymentThread : public juce::Thread {
public:
    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    DeploymentThread();

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        LockFreeQueue<EventFromHost, queue_settings::NMP2DPL_que_size> *NMP2DPL_Event_Que_ptr_,
        LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2NMD_Parameters_Que_ptr_,
        LockFreeQueue<GenerationEvent, queue_settings::DPL2NMP_que_size> *DPL2NMP_GenerationEvent_Que_ptr_,
        LockFreeQueue<juce::MidiFile, 4>* GUI2DPL_DroppedMidiFile_Que_ptr_,
        RealTimePlaybackInfo *realtimePlaybackInfo_ptr_,
        MidiVisualizersData* visualizerData_ptr_,
        AudioVisualizersData* audioVisualizersData_ptr_);

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ------------------------------------------------------------------------------------------------------------
    void run() override;

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 4 . Implement Deploy Method -----> DO NOT MODIFY ANY PART EXCEPT THE BODY OF THE METHOD
    // ------------------------------------------------------------------------------------------------------------
    std::pair<bool, bool> deploy(
        std::optional<MidiFileEvent> & new_midi_event_dragdrop,
        std::optional<EventFromHost> & new_event_from_host,
        bool did_any_gui_params_change,
        bool new_preset_loaded_since_last_call,
        bool new_midi_file_dropped_on_visualizers,
        bool new_audio_file_dropped_on_visualizers);

    // ============================================================================================================

    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    void prepareToStop();     // run this in destructor destructing object
    ~DeploymentThread() override;
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped
    // ============================================================================================================

    // ============================================================================================================
    // ===          User Customizable Struct
    // ============================================================================================================
    unique_ptr<CustomPresetDataDictionary>
        CustomPresetData; // data stored here will be saved automatically when the plugin is saved/loaded/preset changed
    //    mutable std::mutex  preset_loaded_mutex;
    //    bool newPresetLoaded{false};

private:
    // ============================================================================================================
    // ===          1_RandomGeneration Data
    // ===        (If you need additional data for input processing, add them here)
    // ===  NOTE: All data needed by the model MUST be wrapped as ModelInput struct (modifiable in ModelInput.h)
    // ============================================================================================================

    // Host Event 1_RandomGeneration Data
    EventFromHost last_event{};
    EventFromHost first_frame_metadata_event{};                      // keeps metadata of the first frame
    EventFromHost frame_metadata_event{};                            // keeps metadata of the next frame
    EventFromHost last_bar_event{};                                  // keeps metadata of the last bar passed
    EventFromHost
        last_complete_note_duration_event{};               // keeps metadata of the last beat passed

    // Playback 1_RandomGeneration Data
    PlaybackPolicies playbackPolicy;
    PlaybackSequence playbackSequence;


    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<EventFromHost, queue_settings::NMP2DPL_que_size> *NMP2DPL_Event_Que_ptr{};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *
        APVM2DPL_Parameters_Que_ptr {};
    LockFreeQueue<GenerationEvent, queue_settings::DPL2NMP_que_size> *DPL2NMP_GenerationEvent_Que_ptr{};
    LockFreeQueue<juce::MidiFile, 4>* GUI2DPL_DroppedMidiFile_Que_ptr{};
    RealTimePlaybackInfo *realtimePlaybackInfo{};
    // ============================================================================================================

    // ============================================================================================================
    // ===          GuiParameters
    // ============================================================================================================
    GuiParams gui_params;

    // ============================================================================================================
    // ===          Debugging Methods
    // ============================================================================================================
    static void DisplayEvent(const EventFromHost&event, bool compact_mode, double event_count);
    static void PrintMessage(const std::string &input);

    // ============================================================================================================
    // ===          User Customizable Struct
    // ============================================================================================================

    // You can update the DeploymentData struct in CustomStructs.h if you need any additional data
    DPLData DPLdata {};

    MidiVisualizersData* midiVisualizersData {};
    AudioVisualizersData* audioVisualizersData {};

    // ============================================================================================================
    // ===          TorchScript Model
    // ============================================================================================================
    torch::jit::script::Module model;
    bool isModelLoaded{false};
    bool load(const std::string& model_name_);
    std::string model_path;
    void DisplayTensor(const torch::Tensor &tensor, const string& Label,
                       bool display_content);
};


