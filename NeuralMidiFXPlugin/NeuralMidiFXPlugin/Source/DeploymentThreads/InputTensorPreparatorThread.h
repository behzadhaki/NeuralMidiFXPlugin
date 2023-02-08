//
// Created by Behzad Haki on 2022-12-13.
//

#ifndef JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
#define JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H


#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/GuiParameters.h"
#include "../includes/InputEvent.h"
#include "../Includes/LockFreeQueue.h"
#include "../DeploymentSettings/ThreadsAndQueuesAndInputEvents.h"
#include "../DeploymentSettings/Model.h"
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
            LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
            LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr_);

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ------------------------------------------------------------------------------------------------------------
    void run() override;

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 4 . Implement Deploy Method -----> DO NOT MODIFY ANY PART EXCEPT THE BODY OF THE METHOD
    // ------------------------------------------------------------------------------------------------------------
    bool deploy(std::optional<Event> &new_event, bool did_any_gui_params_change);
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
    // ===          Deployment Data
    // ===        (If you need additional data for input processing, add them here)
    // ===  NOTE: All data needed by the model MUST be wrapped as ModelInput struct (modifiable in ModelInput.h)
    // ============================================================================================================
    ModelInput model_input{};

    /* Some data are pre-implemented for easier access */
    Event last_event{};
    Event first_frame_metadata_event{};                      // keeps metadata of the first frame
    Event frame_metadata_event{};                            // keeps metadata of the next frame
    Event last_bar_event{};                                  // keeps metadata of the last bar passed
    Event last_complete_note_duration_event{};               // keeps metadata of the last beat passed

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<Event, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr{};
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr{};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr{};
    // ============================================================================================================

    // ============================================================================================================
    // ===          GuiParameters
    // ============================================================================================================
    GuiParams gui_params;

    // ============================================================================================================
    // ===          Debugging Methods
    // ============================================================================================================
    static void DisplayEvent(const Event &event, bool compact_mode, double event_count);
    static void PrintMessage(std::string input);
};


#endif //JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
