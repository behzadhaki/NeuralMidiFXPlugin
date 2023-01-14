//
// Created by Behzad Haki on 2023-01-12.
//

#include "ModelThread.h"

ModelThread::ModelThread() : juce::Thread("ModelThread") {}

void ModelThread::startThreadUsingProvidedResources(
        LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
        LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    ITP2MDL_ModelInput_Que_ptr = ITP2MDL_ModelInput_Que_ptr_;
    MDL2PPP_ModelOutput_Que_ptr = MDL2PPP_ModelOutput_Que_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void ModelThread::DisplayTensor(const torch::Tensor &tensor, const string Label,
                                bool show_metadata, bool show_content) {
    stringstream ss;
    ss << "TENSOR:" << Label ;
    if (show_metadata) {
        ss << " | Tensor metadata: " ;
        ss << " | Type: " << tensor.type().toString();
        ss << " | Device: " << tensor.device();
        ss << " | Size: " << tensor.sizes();
        ss << " |  - Storage data pointer: " << tensor.storage().data_ptr();
    }
    if (show_content) {
        ss << "Tensor content:" << std::endl;
        ss << tensor << std::endl;
    }
    DBG(ss.str());
}

void ModelThread::run() {
    // notify if the thread is still running
    bool bExit = threadShouldExit();

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped

        // Check if new events are available in the queue
        while (ITP2MDL_ModelInput_Que_ptr->getNumReady() > 0) {
            // =================================================================================
            // ===         Step 1 . Get New ModelInput data if Any:
            // ===          a. either get the earliest queued input --> ->pop()
            // ===          b. only the latest queued input (while discarding the earlier ones)
            // ===                      --> ->getLatestOnly();
            // =================================================================================
            auto modelInput = ITP2MDL_ModelInput_Que_ptr->pop(); // get the earliest queued input
            // auto modelInput = ITP2MDL_ModelInput_Que_ptr->getLatestOnly(); // latest input

            DisplayTensor(modelInput.tensor1, "INPUT", true, false);          // display the data if debugging

            // =================================================================================
            // ===         Step 2 . Run the Model
            // =================================================================================
            auto out = model.forwardPredict(modelInput);

            // =================================================================================
            // ===         Step 3 . Send the ModelOutput data to the next thread
            // =================================================================================
            if (out.has_value()) {
                MDL2PPP_ModelOutput_Que_ptr->push(*out);
                DisplayTensor(out->tensor1, "OUTPUT", true, false);          // display the data if debugging
            }

        }

        // check if thread is still running
        // Don't remove this line!
        bExit = threadShouldExit();
    }

    // wait for a few ms to avoid burning the CPU
    // do not remove this, for super strict timing requirements
    // adjust the wait time to your needs
    sleep(thread_configurations::Model::waitTimeBtnIters);
}

void ModelThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::Model::waitTimeBtnIters);

    readyToStop = true;
}

ModelThread::~ModelThread() {
    if (not readyToStop) {
        prepareToStop();
    }
}