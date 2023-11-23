//
// Created by Behzad Haki on 2022-12-13.
//

#ifndef JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
#define JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H


#include "shared_plugin_helpers/shared_plugin_helpers.h"
#include "../Includes/GuiParameters.h"
#include "../Includes/InputEvent.h"
#include "../Includes/LockFreeQueue.h"
#include "../../NeuralMidiFXPlugin/Configs_HostEvents.h"
#include "../Includes/Configs_Model.h"
#include "../Includes/colored_cout.h"
#include "../Includes/chrono_timer.h"
#include "../../NeuralMidiFXPlugin/Configs_Debugging.h"

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
        LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr_,
        LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
        LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr_,
        LockFreeQueue<juce::MidiFile, 4> *GUI2ITP_DroppedMidiFile_Que_ptr_,
        RealTimePlaybackInfo *realtimePlaybackInfo_ptr_);

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 3 . start run() thread by calling startThread().
    // ---                  !!DO NOT!! Call run() directly. startThread() internally makes a call to run().
    // ------------------------------------------------------------------------------------------------------------
    void run() override;

    // ------------------------------------------------------------------------------------------------------------
    // ---         Step 4 . Implement Deploy Method -----> DO NOT MODIFY ANY PART EXCEPT THE BODY OF THE METHOD
    // ------------------------------------------------------------------------------------------------------------
    bool deploy(std::optional<MidiFileEvent> & new_midi_event_dragdrop,
                std::optional<EventFromHost> & new_event_from_host, bool did_any_gui_params_change);
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
    EventFromHost last_event{};
    EventFromHost first_frame_metadata_event{};                      // keeps metadata of the first frame
    EventFromHost frame_metadata_event{};                            // keeps metadata of the next frame
    EventFromHost last_bar_event{};                                  // keeps metadata of the last bar passed
    EventFromHost
        last_complete_note_duration_event{};               // keeps metadata of the last beat passed

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================
    LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr{};
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr{};
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr{};
    LockFreeQueue<juce::MidiFile, 4>* GUI2ITP_DroppedMidiFile_Que_ptr{};
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

    // You can update the ITPData struct in CustomStructs.h if you need any additional data
    ITPData ITPdata {};
};


#endif //JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
