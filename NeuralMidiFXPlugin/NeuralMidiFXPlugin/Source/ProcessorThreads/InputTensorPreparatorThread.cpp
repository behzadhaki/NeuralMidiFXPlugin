//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"

InputTensorPreparatorThread::InputTensorPreparatorThread():
juce::Thread("InputPreparatorThread") {
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

void InputTensorPreparatorThread::run() {
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    while (!bExit) {
        // check if thread is ready to be stopped
        if (readyToStop) {
            break;
        }

        // Update Local Event Tracker
        if (NMP2ITP_NoteOn_Que_ptr->getNumReady() > 0) {
            auto noteOn = NMP2ITP_NoteOn_Que_ptr->pop();
            DBG("RECEIVED");
            MultiTimeEventTracker.addNoteOn(noteOn.channel, noteOn.number, noteOn.velocity,
                                            noteOn.time_ppq_absolute, noteOn.time_sec_absolute,
                                            noteOn.time_ppq_relative, noteOn.time_sec_relative);
            NewEventsBuffer.addNoteOn(noteOn.channel, noteOn.number, noteOn.velocity,
                                      noteOn.time_ppq_absolute, noteOn.time_sec_absolute,
                                      noteOn.time_ppq_relative, noteOn.time_sec_relative);

        }

        auto NewEventsVector = NewEventsBuffer.get_events_with_duration(true, true, true);
        DBG("NewEventsVector.size() = " << NewEventsVector.size());
        if (!NewEventsVector.empty()) {
            for (const auto& event: NewEventsVector) {
                auto message = event.first;
                auto duration = event.second;
                if (message.isNoteOn()) {
                    DBG("NoteOn: " + juce::String(message.getNoteNumber()) + " " +
                    juce::String(message.getVelocity()) + " " +
                    juce::String(duration));
                }
            }
        }

//        if (NMP2ITP_NoteOff_Que_ptr->getNumReady() > 0) {
//            auto noteOff = NMP2ITP_NoteOff_Que_ptr->pop();
//
//            MultiTimeEventTracker.addNoteOff(noteOff.channel, noteOff.number, noteOff.velocity,
//                                             noteOff.time_ppq_absolute, noteOff.time_sec_absolute,
//                                             noteOff.time_ppq_relative, noteOff.time_sec_relative);
//        }
//
//        if (NMP2ITP_Controller_Que_ptr->getNumReady() > 0) {
//            auto controller = NMP2ITP_Controller_Que_ptr->pop();
//
//            MultiTimeEventTracker.addCC(controller.channel, controller.number, controller.value,
//                                        controller.time_ppq_absolute, controller.time_sec_absolute,
//                                        controller.time_ppq_relative, controller.time_sec_relative);
//        }
//
//        if (NMP2ITP_TempoTimeSig_Que_ptr->getNumReady() > 0) {
//            auto tempoTimeSig = NMP2ITP_TempoTimeSig_Que_ptr->pop();
//
//            MultiTimeEventTracker.addTempoAndTimeSignature(tempoTimeSig.qpm, tempoTimeSig.numerator, tempoTimeSig.denominator,
//                                                           tempoTimeSig.time_ppq_absolute, tempoTimeSig.time_sec_absolute,
//                                                           tempoTimeSig.time_ppq_relative, tempoTimeSig.time_sec_relative);
//        }




        // ============================================================================================================
        // ===          Prepare the Input Tensor Using the Events Registered in the Event Tracker
        // ============================================================================================================


        // ============================================================================================================

        // check if thread is still running
        bExit = threadShouldExit();
    }

    sleep (thread_configurations::InputTensorPreparator::waitTimeBtnIters);
}

void InputTensorPreparatorThread::prepareToStop() {
    //Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_settings::GrooveThread::waitTimeBtnIters);

    readyToStop = true;
}

InputTensorPreparatorThread::~InputTensorPreparatorThread() {
    if (not readyToStop)
    {
        prepareToStop();
    }
}

void InputTensorPreparatorThread::ClearTrackedEventsStartingAt(float start) {

}

void InputTensorPreparatorThread::clearStep(int grid_ix, float start_ppq) {

}
