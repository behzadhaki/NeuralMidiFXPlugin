//
// Created by Behzad Haki on 2022-12-13.
//

#ifndef JUCECMAKEREPO_PLAYBACKPREPARATORTHREAD_H
#define JUCECMAKEREPO_PLAYBACKPREPARATORTHREAD_H


#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../Includes/GenerationEvent.h"
#include "../settings.h"
#include "../model_settings.h"


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
            LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr_);

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
    ~PlaybackPreparatorThread() override;
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped
    // ============================================================================================================

private:
    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr{};
    LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr{};
    // ============================================================================================================

    // ============================================================================================================
    // ===          Debugging Methods
    // ============================================================================================================
};

#endif //JUCECMAKEREPO_PLAYBACKPREPARATORTHREAD_H
