//
// Created by Behzad Haki on 2022-12-13.
//

#include "PlaybackPreparatorThread.h"

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                                    DO NOT CHANGE ANYTHING BELOW THIS LINE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

using namespace debugging_settings::PlaybackPreparatorThread;

PlaybackPreparatorThread::PlaybackPreparatorThread() : juce::Thread("PlaybackPreparatorThread") {
}

void PlaybackPreparatorThread::startThreadUsingProvidedResources(
    LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_,
    LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *PPP2NMP_GenerationEvent_Que_ptr_,
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2PPP_Parameters_Queu_ptr_,
    LockFreeQueue<juce::MidiFile, 4> *PPP2GUI_GenerationMidiFile_Que_ptr_,
    RealTimePlaybackInfo *realtimePlaybackInfo_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    MDL2PPP_ModelOutput_Que_ptr = MDL2PPP_ModelOutput_Que_ptr_;
    PPP2NMP_GenerationEvent_Que_ptr = PPP2NMP_GenerationEvent_Que_ptr_;
    APVM2PPP_Parameters_Queu_ptr = APVM2PPP_Parameters_Queu_ptr_;
    PPP2GUI_GenerationMidiFile_Que_ptr = PPP2GUI_GenerationMidiFile_Que_ptr_;
    realtimePlaybackInfo = realtimePlaybackInfo_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void PlaybackPreparatorThread::PrintMessage(const std::string& input) {

    if (disable_user_print_requests) { return; }

    // if input is multiline, split it into lines && print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::magenta << "[PPP] " << line << clr::reset << std::endl; }
}

void PlaybackPreparatorThread::run() {

    // convert showMessage to a lambda function
    auto showMessage = [](const std::string &input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::magenta << "[PPP] " << line << clr::reset << std::endl;
        }
    };

    bool new_model_output_received{false};

    // notify if the thread is still running
    bool bExit = threadShouldExit();

    int cnt{0};

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped

        if (APVM2PPP_Parameters_Queu_ptr->getNumReady() > 0) {
            // print updated values for debugging
            gui_params = APVM2PPP_Parameters_Queu_ptr->pop(); // pop the latest parameters from the queue
            gui_params.registerAccess();                      // set the time when the parameters were accessed

            if (print_received_gui_params) { // if set in Debugging.h
                showMessage(gui_params.getDescriptionOfUpdatedParams());
            }
        } else {
            gui_params.setChanged(false); // no change in parameters since last check
        }

        if (MDL2PPP_ModelOutput_Que_ptr->getNumReady() > 0) {
            new_model_output_received = true;
            model_output = MDL2PPP_ModelOutput_Que_ptr->pop(); // get the earliest queued generation data
            model_output.timer.registerEndTime();

            if (print_ModelOutput_transfer_delay) {
                showMessage(*model_output.timer.getDescription("ModelOutput Transfer Delay from MDL to PPP: "));
            }
        } else {
            new_model_output_received = false;
        }

        if (new_model_output_received || gui_params.changed()) {
            auto status = deploy(new_model_output_received, gui_params.changed());
            auto shouldSendNewPlaybackPolicy = status.first;
            auto shouldSendNewPlaybackSequence = status.second;
            if (shouldSendNewPlaybackPolicy) {
                // send to the the main thread (NMP)
                if (playbackPolicy.IsReadyForTransmission()) {
                    PPP2NMP_GenerationEvent_Que_ptr->push(GenerationEvent(playbackPolicy));
                }
            }

            if (shouldSendNewPlaybackSequence) {
                // send to the the main thread (NMP)
                PPP2NMP_GenerationEvent_Que_ptr->push(GenerationEvent(playbackSequence));
                cnt++;
            }
        }

        // ============================================================================================================

        // check if thread is still running
        bExit = threadShouldExit();
    }

    // wait for a few ms to avoid burning the CPU
    sleep(thread_configurations::PlaybackPreparator::waitTimeBtnIters);
}

void PlaybackPreparatorThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::PlaybackPreparator::waitTimeBtnIters);

    readyToStop = true;
}

PlaybackPreparatorThread::~PlaybackPreparatorThread() {
    if (!readyToStop) {
        prepareToStop();
    }
}

void PlaybackPreparatorThread::DisplayTensor(const torch::Tensor &tensor, const string Label) {
    // if (disable_user_tensor_display_requests) { return; }
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines and print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::magenta << "[PPP] " << line << std::endl;
        }
    };

    std::stringstream ss;
    ss << "TENSOR:" << Label ;
    if (true) { //(!disable_printing_tensor_info) {
        ss << " | Tensor metadata: " ;
        ss << " | Device: " << tensor.device();
        ss << " | Size: " << tensor.sizes();
        ss << " |  - Storage data pointer: " << tensor.storage().data_ptr();
    }
    if (true) { // (!disable_printing_tensor_content) {
        ss << "Tensor content:" << std::endl;
        ss << tensor << std::endl;
    }
    showMessage(ss.str());
}