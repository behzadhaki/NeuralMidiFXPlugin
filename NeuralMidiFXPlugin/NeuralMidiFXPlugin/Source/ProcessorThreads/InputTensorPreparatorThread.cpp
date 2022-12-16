//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"

InputTensorPreparatorThread::InputTensorPreparatorThread():
juce::Thread("InputPreparatorThread") {
}

void InputTensorPreparatorThread::startThreadUsingProvidedResources() {
    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------

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
        // if fifo.available() > 0
        //      get event from fifo




        // ============================================================================================================
        // ===          Prepare the Input Tensor Using the Events Registered in the Event Tracker
        // ============================================================================================================

        // PLACE CODE HERE

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
