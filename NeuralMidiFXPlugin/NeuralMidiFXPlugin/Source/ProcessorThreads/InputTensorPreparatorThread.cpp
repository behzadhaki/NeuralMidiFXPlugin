//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"

InputTensorPreparatorThread::InputTensorPreparatorThread(): juce::Thread("InputPreparatorThread") {
}

void InputTensorPreparatorThread::startThreadUsingProvidedResources(
        LockFreeQueue<Event, 512>* NMP2ITP_Event_Que_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_Event_Que_ptr = NMP2ITP_Event_Que_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}


void InputTensorPreparatorThread::run() {
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    double cnt = 0;
    while (!bExit) {
        // check if thread is ready to be stopped
        if (readyToStop) {
            break;
        }

        // ============================================================================================================
        // ===         Step 1 . Get MIDI messages from host
        // ============================================================================================================


        // Declare an Optional Double value for time
        if (NMP2ITP_Event_Que_ptr->getNumReady() > 0) {

            auto event = NMP2ITP_Event_Que_ptr->pop();
            DBG(event.getDescription().str() << " --- " << cnt);
            cnt++;
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
