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
        LockFreeQueue<Tempo, 512>* NMP2ITP_Tempo_Que_ptr_,
        LockFreeQueue<TimeSignature, 512>* NMP2ITP_TimeSignature_Que_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_NoteOn_Que_ptr = NMP2ITP_NoteOn_Que_ptr_;
    NMP2ITP_NoteOff_Que_ptr = NMP2ITP_NoteOff_Que_ptr_;
    NMP2ITP_Controller_Que_ptr = NMP2ITP_Controller_Que_ptr_;
    NMP2ITP_Tempo_Que_ptr = NMP2ITP_Tempo_Que_ptr_;
    NMP2ITP_TimeSignature_Que_ptr = NMP2ITP_TimeSignature_Que_ptr_;


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
            DBG("NoteOn time_sec_relative: " + juce::String(noteOn.time_sec_relative));
            DBG("NoteOn time_sec_absolute: " + juce::String(noteOn.time_sec_absolute));
            DBG("NoteOn time_ppq_absolute: " + juce::String(noteOn.time_ppq_absolute));
            DBG("NoteOn time_ppq_relative: " + juce::String(noteOn.time_ppq_relative));
        }




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
