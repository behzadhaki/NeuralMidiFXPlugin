//
// Created by Behzad Haki on 2023-01-12.
//

#include "ModelThread.h"
using namespace debugging_settings::ModelThread;

// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                                    DO NOT CHANGE ANYTHING BELOW THIS LINE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

ModelThread::ModelThread() : juce::Thread("ModelThread") {}

void ModelThread::startThreadUsingProvidedResources(
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
    LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size> *MDL2PPP_ModelOutput_Que_ptr_,
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2MDL_Parameters_Queu_ptr_,
    RealTimePlaybackInfo *realtimePlaybackInfo_ptr_){

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    ITP2MDL_ModelInput_Que_ptr = ITP2MDL_ModelInput_Que_ptr_;
    MDL2PPP_ModelOutput_Que_ptr = MDL2PPP_ModelOutput_Que_ptr_;
    APVM2MDL_Parameters_Queu_ptr = APVM2MDL_Parameters_Queu_ptr_;
    realtimePlaybackInfo = realtimePlaybackInfo_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void ModelThread::PrintMessage(const std::string& input) {
    using namespace debugging_settings::ModelThread;
    if (disable_user_print_requests) { return; }

    // if input is multiline, split it into lines and print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::blue << "[MDL] " << line << std::endl; }
}

void ModelThread::DisplayTensor(const torch::Tensor &tensor, const string Label) {
    if (disable_user_tensor_display_requests) { return; }
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines and print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::blue << "[MDL] " << line << std::endl;
        }
    };

    std::stringstream ss;
    ss << "TENSOR:" << Label ;
    if (!disable_printing_tensor_info) {
        ss << " | Tensor metadata: " ;
        ss << " | Device: " << tensor.device();
        ss << " | Size: " << tensor.sizes();
        ss << " |  - Storage data pointer: " << tensor.storage().data_ptr();
    }
    if (!disable_printing_tensor_content) {
        ss << "Tensor content:" << std::endl;
        ss << tensor << std::endl;
    }
    showMessage(ss.str());
}


void ModelThread::run() {
    using namespace debugging_settings::ModelThread;

    // notify if the thread is still running
    bool bExit = threadShouldExit();

    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines and print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::blue << "[MDL] " << line << std::endl;
        }
    };

    bool new_model_input_received{false};
    bool new_model_output_to_push{false};
    int generation_cnt{0};

    chrono_timer chrono_timed_deploy;
    chrono_timed_deploy.registerStartTime();

    using namespace debugging_settings::ModelThread;

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped

        if (APVM2MDL_Parameters_Queu_ptr->getNumReady() > 0) {
            // print updated values for debugging
            gui_params = APVM2MDL_Parameters_Queu_ptr->pop(); // pop the latest parameters from the queue
            gui_params.registerAccess();                      // set the time when the parameters were accessed

            if (print_received_gui_params) { // if set in Debugging.h
                showMessage(gui_params.getDescriptionOfUpdatedParams());
            }
        } else {
            gui_params.setChanged(false); // no change in parameters since last check
        }

        if (ITP2MDL_ModelInput_Que_ptr->getNumReady() > 0) {
            model_input = ITP2MDL_ModelInput_Que_ptr->pop(); // get the earliest queued input
            new_model_input_received = true;
        } else {
            new_model_input_received = false;
        }

        if (new_model_input_received || gui_params.changed())
        {
            new_model_output_to_push = deploy(new_model_input_received, gui_params.changed());
            if (new_model_output_to_push) {
                model_output.timer.registerStartTime();
                MDL2PPP_ModelOutput_Que_ptr->push(model_output);
                generation_cnt++;
                chrono_timed_deploy.registerEndTime();
                if (print_model_inference_time) {
                    if (generation_cnt > 1) {
                        auto text = "Time Duration Between Model Generation #" + std::to_string(generation_cnt);
                        text += " and #" + std::to_string(generation_cnt - 1) + ": ";
                        showMessage(*chrono_timed_deploy.getDescription(text));
                    } else {
                        auto text = "Time Duration Between Start and First Pushed Model Generation: ";
                        showMessage(*chrono_timed_deploy.getDescription(text));
                    }
                }
                chrono_timed_deploy.registerStartTime();
            }
            bExit = threadShouldExit();
        } else {
            bExit = threadShouldExit();
            // wait for a few ms to avoid burning the CPU
            // do not remove this, for super strict timing requirements
            // adjust the wait time to your needs
            sleep(thread_configurations::Model::waitTimeBtnIters);
        }
    }
}

void ModelThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::Model::waitTimeBtnIters);

    readyToStop = true;
}

ModelThread::~ModelThread() {
    if (!readyToStop) {
        prepareToStop();
    }
}