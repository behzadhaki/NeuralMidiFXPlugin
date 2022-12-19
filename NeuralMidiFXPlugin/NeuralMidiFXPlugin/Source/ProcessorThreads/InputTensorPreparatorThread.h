//
// Created by Behzad Haki on 2022-12-13.
//

#ifndef JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
#define JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H


#include <shared_plugin_helpers/shared_plugin_helpers.h>
#include "../Includes/CustomStructsAndLockFreeQueue.h"
#include "../settings.h"
#include "../Includes/EventTracker.h"

class InputTensorPreparatorThread:public juce::Thread {
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
            LockFreeQueue<NoteOn, 512>* NMP2ITP_NoteOn_Que_ptr,
            LockFreeQueue<NoteOff, 512>* NMP2ITP_NoteOff_Que_ptr,
            LockFreeQueue<CC, 512>* NMP2ITP_Controller_Que_ptr,
            LockFreeQueue<TempoTimeSignature, 512>* NMP2ITP_TempoTimeSig_Que_ptr);

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
    bool readyToStop {false}; // Used to check if thread is ready to be stopped or externally stopped from a parent thread
    // ============================================================================================================

    // ============================================================================================================
    // ===          Utility Methods and Parameters
    // ============================================================================================================
    void ClearTrackedEventsStartingAt(float start);   // clears a time step starting at start
    void clearStep(int grid_ix, float start_ppq);    // clears a time step ONLY IF OVERDUBBING IS OFF!!!
    // ============================================================================================================

private:
    // ============================================================================================================
    // ===          Internal MIDI Buffer Memory
    // ============================================================================================================
    // RT_STREAM_FROM_HOST      Real-time stream of MIDI messages from host stored in this stream
    juce::MidiMessageSequence RT_MidiMessageSequence;
    // ============================================================================================================

    // ============================================================================================================
    // ===          I/O Queues for Receiving/Sending Data
    // ============================================================================================================

    // ------------------------------------------------------------------------------------------------------------
    // ---          Input Queues, Event Tracker and Internal Event Buffer
    // ------------------------------------------------------------------------------------------------------------
    LockFreeQueue<NoteOn, 512>* NMP2ITP_NoteOn_Que_ptr;
    LockFreeQueue<NoteOff, 512>* NMP2ITP_NoteOff_Que_ptr;
    LockFreeQueue<CC, 512>* NMP2ITP_Controller_Que_ptr;
    LockFreeQueue<TempoTimeSignature, 512>* NMP2ITP_TempoTimeSig_Que_ptr;

    // keeps track of all events so far (unless clear is called)
    ITP_MultiTime_EventTracker MultiTimeEventTracker {false};
    // set true so as to remove events as soon as accessed (this way only new events are tracked)
    ITP_MultiTime_EventTracker NewEventsBuffer {true};


    // ------------------------------------------------------------------------------------------------------------
    // ---          Output to Model Thread
    // ------------------------------------------------------------------------------------------------------------

    // Torch.tensor

    // ------------------------------------------------------------------------------------------------------------
    // ---          Output to GUI
    // ------------------------------------------------------------------------------------------------------------

    // Torch.note
    // ============================================================================================================




};


#endif //JUCECMAKEREPO_INPUTTENSORPREPARATORTHREAD_H
