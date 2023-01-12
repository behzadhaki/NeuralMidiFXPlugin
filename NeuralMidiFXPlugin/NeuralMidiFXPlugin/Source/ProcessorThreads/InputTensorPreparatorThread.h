//
// Created by Behzad Haki on 2022-12-13.
//

#ifndef JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
#define JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H


#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../settings.h"
#include "../model_settings.h"
#include "../Includes/EventTracker.h"

class InputTensorPreparatorThread : public juce::Thread {
public:
    // ============================================================================================================
    // ===          Preparing Thread for Running
    // ============================================================================================================
    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 1 . Construct
    // ------------------------------------------------------------------------------------------------------------
    InputTensorPreparatorThread();

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 2 . give access to resources needed to communicate with other threads
    // ------------------------------------------------------------------------------------------------------------
    void startThreadUsingProvidedResources(
            LockFreeQueue<Event, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr_,
            LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_);

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
    ~InputTensorPreparatorThread() override;
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped
    // ============================================================================================================

private:
    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<Event, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr{};
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr{};
    // ============================================================================================================

    // ============================================================================================================
    // ===          Debugging Methods
    // ============================================================================================================
    static void DisplayEvent(const Event &event, bool compact_mode, double event_count);
};


#endif //JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
