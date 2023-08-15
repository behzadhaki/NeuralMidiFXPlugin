//
// Created by Behzad Haki on 2022-12-13.
//

#ifndef JUCECMAKEREPO_PLAYBACKPREPARATORTHREAD_H
#define JUCECMAKEREPO_PLAYBACKPREPARATORTHREAD_H

#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "../Includes/GuiParameters.h"
#include "../Includes/GenerationEvent.h"
#include "../Includes/LockFreeQueue.h"
#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include "../DeploymentSettings/Model.h"
#include "../Includes/colored_cout.h"
#include "../Includes/chrono_timer.h"
#include "../DeploymentSettings/Debugging.h"

class PlaybackPreparatorThread : public juce::Thread {
public:
    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    PlaybackPreparatorThread();

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
        LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_,
        LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr_,
        LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_Parameters_Queu_ptr_,
        LockFreeQueue<juce::MidiFile, 4> *PPP2GUI_GenerationMidiFile_Que_ptr_,
        RealTimePlaybackInfo *realtimePlaybackInfo_ptr_);

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ---                  (Implement what the thread does inside the run() method
    // ------------------------------------------------------------------------------------------------------------
    void run() override;

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 4 . Implement Deploy Method -----> DO NOT MODIFY ANY PART EXCEPT THE BODY OF THE METHOD
    // ------------------------------------------------------------------------------------------------------------
    std::pair<bool, bool> deploy(bool new_model_output_received, bool did_any_gui_params_change);

    // ============================================================================================================

    // ============================================================================================================
    // ===          Preparing Thread for Stopping
    // ============================================================================================================
    void prepareToStop();     // run this in destructor destructing object
    ~PlaybackPreparatorThread() override;
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped
    // ============================================================================================================

private:
    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr{};
    LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr{};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_Parameters_Queu_ptr{};
    LockFreeQueue<juce::MidiFile, 4> *PPP2GUI_GenerationMidiFile_Que_ptr{};
    RealTimePlaybackInfo *realtimePlaybackInfo{};
    // ============================================================================================================

    // ============================================================================================================
    // ===          Deployment Data
    // ===        (If you need additional data for input processing, add them here)
    // ============================================================================================================
    ModelOutput model_output;
    PlaybackPolicies playbackPolicy;
    PlaybackSequence playbackSequence;
    double PlaybackDelaySlider{-1};

    // ============================================================================================================
    // ===          GuiParameters
    // ============================================================================================================
    GuiParams gui_params;

    // ============================================================================================================
    // ===          Debugging Methods
    // ============================================================================================================
    static void PrintMessage(const std::string &input);
};

#endif //JUCECMAKEREPO_PLAYBACKPREPARATORTHREAD_H
