//
// Created by Behzad Haki on 2023-01-12.
//

#ifndef JUCECMAKEREPO_MODELTHREAD_H
#define JUCECMAKEREPO_MODELTHREAD_H

#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/GuiParameters.h"
#include "../Includes/LockFreeQueue.h"
#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include "../DeploymentSettings/Model.h"

class ModelThread : public juce::Thread {
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
            LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
            LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_);

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
    bool readyToStop{false}; // Used to check if thread is ready to be stopped or externally stopped
    // ============================================================================================================

private:
    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    Model model{model_settings::default_model_path};

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr{};
    LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr{};
    // ============================================================================================================

    // ============================================================================================================
    // ===          Debugging Methods
    // ============================================================================================================
    static void DisplayTensor(const torch::Tensor &tensor, const string Label, bool show_metadata, bool show_content);

};


#endif //JUCECMAKEREPO_MODELTHREAD_H
