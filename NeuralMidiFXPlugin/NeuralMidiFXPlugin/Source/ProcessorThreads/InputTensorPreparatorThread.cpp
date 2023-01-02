//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"

InputTensorPreparatorThread::InputTensorPreparatorThread(): juce::Thread("InputPreparatorThread") {
}

void InputTensorPreparatorThread::startThreadUsingProvidedResources(
        LockFreeQueue<NoteOn, 512>* NMP2ITP_NoteOn_Que_ptr_,
        LockFreeQueue<NoteOff, 512>* NMP2ITP_NoteOff_Que_ptr_,
        LockFreeQueue<CC, 512>* NMP2ITP_Controller_Que_ptr_,
        LockFreeQueue<TempoTimeSignature, 512>* NMP2ITP_TempoTimeSig_Que_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_NoteOn_Que_ptr = NMP2ITP_NoteOn_Que_ptr_;
    NMP2ITP_NoteOff_Que_ptr = NMP2ITP_NoteOff_Que_ptr_;
    NMP2ITP_Controller_Que_ptr = NMP2ITP_Controller_Que_ptr_;
    NMP2ITP_TempoTimeSig_Que_ptr = NMP2ITP_TempoTimeSig_Que_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

// New notes can be accessed in two modes in the ITP thread:
// 1. as Note Ons with corresponding durations -> NewEventsBuffer.get_notes_with_duration()
//     This is useful if your model works with note onsets and durations. Keep in mind that this mode
//     only returns COMPLETED notes. If a note is not completed, it will not be returned and kept in the buffer
//     until it a corresponding note off is received.
// 2. as NoteOn and NoteOff events -> NewEventsBuffer.get_note_midi_messages()
//     This is useful if your model works with note onsets and offsets as separate events.
//     This mode returns all notes, even if they are not completed. This is useful if you want to
//     prepare the input sequentially in the same order as received in the host. The durations of events
//     in this case are assumed to be zero
MultiTimedStructure<vector<pair<juce::MidiMessage, double>>> InputTensorPreparatorThread::get_new_notes(int mode)
{
    MultiTimedStructure<vector<pair<juce::MidiMessage, double>>> received_notes;
    if (mode == 1) {
        return NewEventsBuffer.get_notes_with_duration();
    } else {
        return NewEventsBuffer.get_note_midi_messages();
    }
}

void InputTensorPreparatorThread::run() {
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    while (!bExit) {
        // check if thread is ready to be stopped
        if (readyToStop) {
            break;
        }

        // ============================================================================================================
        // ===         Step 1 . Get MIDI messages from host
        // ============================================================================================================
        bool newDataAvailable = accessNewMessagesIfAny();
        bool tempoOrTimeSigChanged = accessTempoTimeSignatureChangesIfAny();


        if (newDataAvailable) {

            auto received_notes = get_new_notes(thread_configurations::InputTensorPreparator::new_note_access_mode);

            for (const auto& note: received_notes.absolute_time_in_ppq) {
                juce::MidiMessage message = note.first;
                double duration = note.second;
                // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                // HERE, UPDATE YOUR INPUT TENSORS ACCORDINGLY USING THE RECEIVED MIDI MESSAGE
                // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                DBG(message.getDescription() << ", time: " << message.getTimeStamp() << ", duration: " << duration);

                // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            }
        }


        // ============================================================================================================
        // ===
        // ============================================================================================================


        // ============================================================================================================

        // check if thread is still running
        bExit = threadShouldExit();
    }

    sleep (thread_configurations::InputTensorPreparator::waitTimeBtnIters);
}

// updates the local trackers and buffer if new messages are available
bool InputTensorPreparatorThread::accessNewMessagesIfAny() {    // Update Local Event Tracker

    bool newMessageReceived = false;

    if (NMP2ITP_NoteOn_Que_ptr->getNumReady() > 0) {
        auto noteOn = NMP2ITP_NoteOn_Que_ptr->pop();
        MultiTimeEventTracker.addNoteOn(noteOn.channel, noteOn.number, noteOn.velocity,
                                        noteOn.time_ppq_absolute, noteOn.time_sec_absolute,
                                        noteOn.time_ppq_relative, noteOn.time_sec_relative);
        NewEventsBuffer.addNoteOn(noteOn.channel, noteOn.number, noteOn.velocity,
                                  noteOn.time_ppq_absolute, noteOn.time_sec_absolute,
                                  noteOn.time_ppq_relative, noteOn.time_sec_relative);

        newMessageReceived = true;
    }

    if (NMP2ITP_NoteOff_Que_ptr->getNumReady() > 0) {
        auto noteOff = NMP2ITP_NoteOff_Que_ptr->pop();
        MultiTimeEventTracker.addNoteOff(noteOff.channel, noteOff.number, noteOff.velocity,
                                         noteOff.time_ppq_absolute, noteOff.time_sec_absolute,
                                         noteOff.time_ppq_relative, noteOff.time_sec_relative);
        NewEventsBuffer.addNoteOff(noteOff.channel, noteOff.number, noteOff.velocity,
                                   noteOff.time_ppq_absolute, noteOff.time_sec_absolute,
                                   noteOff.time_ppq_relative, noteOff.time_sec_relative);

        newMessageReceived = true;
    }

    if (NMP2ITP_Controller_Que_ptr->getNumReady() > 0) {
        auto controller = NMP2ITP_Controller_Que_ptr->pop();
        MultiTimeEventTracker.addCC(controller.channel, controller.number, controller.value,
                                    controller.time_ppq_absolute, controller.time_sec_absolute,
                                    controller.time_ppq_relative, controller.time_sec_relative);
        NewEventsBuffer.addCC(controller.channel, controller.number, controller.value,
                              controller.time_ppq_absolute, controller.time_sec_absolute,
                              controller.time_ppq_relative, controller.time_sec_relative);
        newMessageReceived = true;
    }

    return newMessageReceived;
}

bool InputTensorPreparatorThread::accessTempoTimeSignatureChangesIfAny() {

    bool newMessageReceived {false};

    if (NMP2ITP_TempoTimeSig_Que_ptr->getNumReady() > 0) {
        auto tempoTimeSig = NMP2ITP_TempoTimeSig_Que_ptr->pop();
        MultiTimeEventTracker.addTempoAndTimeSignature(tempoTimeSig.qpm, tempoTimeSig.numerator,
                                                       tempoTimeSig.denominator,
                                                       tempoTimeSig.time_ppq_absolute,
                                                       tempoTimeSig.time_sec_absolute,
                                                       tempoTimeSig.time_ppq_relative,
                                                       tempoTimeSig.time_sec_relative);
        NewEventsBuffer.addTempoAndTimeSignature(tempoTimeSig.qpm, tempoTimeSig.numerator,
                                                 tempoTimeSig.denominator,
                                                 tempoTimeSig.time_ppq_absolute,
                                                 tempoTimeSig.time_sec_absolute,
                                                 tempoTimeSig.time_ppq_relative,
                                                 tempoTimeSig.time_sec_relative);
        newMessageReceived = true;
    }

    return newMessageReceived;
}

void InputTensorPreparatorThread::prepareToStop() {
    //Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_settings::GrooveThread::waitTimeBtnIters);

    readyToStop = true;
}

InputTensorPreparatorThread::~InputTensorPreparatorThread() {
    if (not readyToStop) {
        prepareToStop();
    }
}

void InputTensorPreparatorThread::ClearTrackedEventsStartingAt(float start) {

}

void InputTensorPreparatorThread::clearStep(int grid_ix, float start_ppq) {

}
