//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"

InputTensorPreparatorThread::InputTensorPreparatorThread() : juce::Thread("InputPreparatorThread") {
}

void InputTensorPreparatorThread::startThreadUsingProvidedResources(
        LockFreeQueue<Event, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_Event_Que_ptr = NMP2ITP_Event_Que_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void InputTensorPreparatorThread::DisplayEvent(
        const Event &event, bool compact_mode, double event_count) {
    if (event.isFirstBufferEvent() or !compact_mode) {
        auto dscrptn = event.getDescription();
        if (dscrptn.str().length() > 0) { DBG(event.getDescription().str() << " | event # " << event_count); }
    } else {
        auto dscrptn = event.getDescriptionOfChangedFeatures(event, true).str();
        if (dscrptn.length() > 0) { DBG(dscrptn << " | event # " << event_count); }
    }
}

void InputTensorPreparatorThread::run() {
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    double cnt = 0;

    Event last_event{};
    Event first_frame_metadata_event{};     // keeps metadata of the first frame
    Event frame_metadata_event{};           // keeps metadata of the next frame
    Event last_bar_event{};                // keeps metadata of the last bar passed
    Event last_complete_note_duration_event{};               // keeps metadata of the last beat passed

    /* Use the following to keep track of when/how often to send the updated
     tensor to the neural network for inference
     you can set this flag to true, anywhere in the code below.

     For example,
     if you want to generate a new pattern on every bar:

        >> if (event.isBarEvent()) { SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = true; }  */
    bool SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = false;

    while (!bExit) {
        if (readyToStop) { break; } // check if thread is ready to be stopped

        // Check if new events are available in the queue
        while (NMP2ITP_Event_Que_ptr->getNumReady() > 0) {

            // =================================================================================
            // ===         Step 1 . Get New Events if Any
            // =================================================================================
            auto new_event = NMP2ITP_Event_Que_ptr->pop();      // get the next available event

            DisplayEvent(new_event, false, cnt);                // display the event

            // =================================================================================
            // ===         Step 2 . Process/Tokenize the Event
            // =================================================================================
            /*                         [Replace With Your Code Here]                          */
            if (new_event.isFirstBufferEvent()) {
                first_frame_metadata_event = new_event;
                /* get bpm, time signature, etc. from the event
                 exp:
                 auto qpm = new_event.bufferMetaData.qpm
                 auto numerator = new_event.bufferMetaData.numerator
                 auto denominator = new_event.bufferMetaData.denominator
                 ...
                 */
            } else if (new_event.isNewBufferEvent()) {
                frame_metadata_event = new_event;
                /* These events are only available if the following flags are specified
                      in the settings.h file:
                          SendEventAtBeginningOfNewBuffers_FLAG     ---> Must be set to True
                          SendEventForNewBufferIfMetadataChanged_FLAG ---> True or False
                 ...
                 */
            } else if (new_event.isNewBarEvent()) {
                last_bar_event = new_event;
                /* These events are only available if the following flags are specified
                      in the settings.h file:
                          SendNewBarEvents_FLAG     ---> Must be set to True

                 You can check the extact timing of the event, or the metadata of the
                    buffer in which the event occurred, by using the following functions:
                 last_bar_event.time_in_samples; last_bar_event.time_in_seconds;
                 last_bar_event.bufferMetaData.qpm;
                 last_bar_event.bufferMetaData.time_in_ppq; --> time of the beginning of
                                                                the corresponding buffer
                 ...
                 */
            } else if (new_event.isNewTimeShiftEvent()) {
                last_complete_note_duration_event = new_event;
                /* These events are only available if the following flags are specified
                      in the settings.h file:
                          SendTimeShiftEvents_FLAG{true};
                          delta_TimeShiftEventRatioOfQuarterNote{0.5}; --> half a quarter, or 1/8 note
                 you can check the extact timing of the event, or the metadata of the
                  buffer in which the event occurred, by using the following functions:
                 last_complete_note_duration_event.time_in_samples; last_bar_event.time_in_seconds;
                 last_complete_note_duration_event.bufferMetaData.qpm;
                 last_complete_note_duration_event.bufferMetaData.time_in_ppq; --> time of the beginning of
                                                                                   the corresponding buffer
                 ...
                 */
            } else if (new_event.isNoteOnEvent()) {

                /* only available if FilterNoteOnEvents_FLAG is false in settings.h
                 auto pitch = new_event.message.getNoteNumber();
                 auto velocity = new_event.message.getVelocity();
                 auto channel = new_event.message.getChannel();
                 auto time_in_ppq = new_event.time_in_ppq;
                 auto bpm = new_event.bufferMetaData.qpm; */
            } else if (new_event.isNoteOffEvent()) {
                /* only available if FilterNoteOnEvents_FLAG is false in settings.h
                 similar to note on events */
            } else if (new_event.isCCEvent()) {
                /* only available if FilterCCEvents_FLAG is false in settings.h
                 auto cc_number = new_event.message.getControllerNumber();
                 auto cc_value = new_event.message.getControllerValue();
                 auto channel = new_event.message.getChannel();
                 ....
                 */
            }
            // =================================================================================

            // =================================================================================
            // ===         Step 3. Send to Model Thread if
            //                     SHOULD_SEND_TO_MODEL_FOR_GENERATION_ is set to TRUE
            // =================================================================================
            if (SHOULD_SEND_TO_MODEL_FOR_GENERATION_) {
                // Push To Queue

                // reset the generation flag
                SHOULD_SEND_TO_MODEL_FOR_GENERATION_ = false;
            }
            // =================================================================================


            // =================================================================================
            // Don't Modify the Following
            last_event = new_event;
            cnt++;
            // =================================================================================
        }

        // ============================================================================================================

        // check if thread is still running
        bExit = threadShouldExit();
    }

    // wait for a few ms to avoid burning the CPU
    sleep(thread_configurations::InputTensorPreparator::waitTimeBtnIters);
}

void InputTensorPreparatorThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::InputTensorPreparator::waitTimeBtnIters);

    readyToStop = true;
}

InputTensorPreparatorThread::~InputTensorPreparatorThread() {
    if (not readyToStop) {
        prepareToStop();
    }
}
