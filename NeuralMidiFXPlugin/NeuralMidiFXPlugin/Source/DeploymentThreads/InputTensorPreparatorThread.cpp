//
// Created by Behzad Haki on 2022-12-13.
//

#include "InputTensorPreparatorThread.h"


// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
//                                    DO NOT CHANGE ANYTHING BELOW THIS LINE
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
InputTensorPreparatorThread::InputTensorPreparatorThread() : juce::Thread("InputPreparatorThread") {}

void InputTensorPreparatorThread::startThreadUsingProvidedResources(
    LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size> *NMP2ITP_Event_Que_ptr_,
    LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size> *ITP2MDL_ModelInput_Que_ptr_,
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2ITP_Parameters_Queu_ptr_,
    LockFreeQueue<juce::MidiFile, 4> *GUI2ITP_DroppedMidiFile_Que_ptr_,
    RealTimePlaybackInfo *realtimePlaybackInfo_ptr_) {

    // Provide access to resources needed to communicate with other threads
    // ---------------------------------------------------------------------------------------------
    NMP2ITP_Event_Que_ptr = NMP2ITP_Event_Que_ptr_;
    ITP2MDL_ModelInput_Que_ptr = ITP2MDL_ModelInput_Que_ptr_;
    APVM2ITP_Parameters_Queu_ptr = APVM2ITP_Parameters_Queu_ptr_;
    GUI2ITP_DroppedMidiFile_Que_ptr = GUI2ITP_DroppedMidiFile_Que_ptr_;
    realtimePlaybackInfo = realtimePlaybackInfo_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}


/*
 * Use this method to print the content of an event
 *  in debugging mode (using DBG() macro)
 *
 * @param event: the event to be printed
 * @param compact_mode: Skips buffer events where no change in buffer is detected
 *      compared to the previous buffer
 * @param event_count: the number of events that have been received so far
 */
void InputTensorPreparatorThread::DisplayEvent(
        const EventFromHost&event, bool compact_mode, double event_count) {

    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[ITP] " << line << std::endl;
        }
    };

    if (event.isFirstBufferEvent() || !compact_mode) {
        auto dscrptn = event.getDescription().str();
        if (dscrptn.length() > 0) { showMessage(dscrptn); }
    } else {
        auto dscrptn = event.getDescriptionOfChangedFeatures(event, true).str();
        if (dscrptn.length() > 0) { showMessage(dscrptn); }
    }
}

static string bool2str(bool b) {
    std::to_string(b);
    if (b) { return "true"; } else { return "false"; }
}

void InputTensorPreparatorThread::PrintMessage(const std::string& input) {
    using namespace debugging_settings::InputTensorPreparatorThread;
    if (disable_user_print_requests) { return; }

    // if input is multiline, split it into lines && print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::green << "[ITP] " << line << std::endl; }
}

void InputTensorPreparatorThread::run() {

    // convert showMessage to a lambda function
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[ITP] " << line << std::endl;
        }
    };

    // notify if the thread is still running
    bool bExit = threadShouldExit();

    double events_received_count = 0;
    int inputs_sent_count = 0;

    chrono_timer chrono_timed_deploy;
    chrono_timer chrono_timed_consecutive_pushes;
    chrono_timed_consecutive_pushes.registerStartTime();
    std::optional<EventFromHost> new_event_from_DAW {};
    std::optional<MidiFileEvent> new_midi_event_dropped_manually {};
    std::optional<ModelInput> ModelInput2send2MDL;

    using namespace debugging_settings::InputTensorPreparatorThread;

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped
        if (APVM2ITP_Parameters_Queu_ptr->getNumReady() > 0) {
            // print updated values for debugging
            gui_params = APVM2ITP_Parameters_Queu_ptr->pop(); // pop the latest parameters from the queue
            gui_params.registerAccess();                      // set the time when the parameters were accessed

            if (print_received_gui_params) { // if set in Debugging.h
                showMessage(gui_params.getDescriptionOfUpdatedParams());
            }

        } else {
            gui_params.setChanged(false); // no change in parameters since last check
        }

        if (NMP2ITP_Event_Que_ptr->getNumReady() > 0) {
            new_event_from_DAW = NMP2ITP_Event_Que_ptr->pop();      // get the next available event
            new_event_from_DAW
                ->registerAccess();                    // set the time when the event was accessed

            events_received_count++;

            if (print_input_events) { // if set in Debugging.h
                DisplayEvent(*new_event_from_DAW, false, events_received_count);   // display the event
            }

        } else {
            new_event_from_DAW = std::nullopt;
        }
        bool ready2send2MDL = false;

        if (new_event_from_DAW.has_value() || gui_params.changed()) {
            new_midi_event_dropped_manually = std::nullopt;
            chrono_timed_deploy.registerStartTime();
            ready2send2MDL = deploy(new_midi_event_dropped_manually, new_event_from_DAW, gui_params.changed());
            chrono_timed_deploy.registerEndTime();

            if (print_deploy_method_time && chrono_timed_deploy.isValid()) { // if set in Debugging.h
                showMessage(*chrono_timed_deploy.getDescription(" deploy() execution time: "));
            }

            // push to next thread if a new input is provided
            if (ready2send2MDL) {
                model_input.timer.registerStartTime();
                ITP2MDL_ModelInput_Que_ptr->push(model_input);
                inputs_sent_count++;
                chrono_timed_consecutive_pushes.registerEndTime();

                if (print_timed_consecutive_ModelInputs_pushed && chrono_timed_consecutive_pushes.isValid()) {
                    if (inputs_sent_count > 1) {
                        auto text = "Time Duration Between ModelInput #" + std::to_string(inputs_sent_count);
                        text += " && #" + std::to_string(inputs_sent_count - 1) + ": ";
                        showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                    } else {
                        auto text = "Time Duration Between Start && First Pushed ModelInput: ";
                        showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                    }
                }
                chrono_timed_consecutive_pushes.registerStartTime();
            }

        }

        // check if notes received from a manually dropped midi file
        if (GUI2ITP_DroppedMidiFile_Que_ptr->getNumReady() > 0){
            auto midifile = GUI2ITP_DroppedMidiFile_Que_ptr->getLatestOnly();

            if (midifile.getNumTracks() > 0) {
                auto track = midifile.getTrack(0);
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto msg_ = track->getEventPointer(i)->message;

                    if (print_manually_dropped_midi_messages) { // if set in Debugging.h
                        showMessage("Midi Message Received from Dropped Midi File: ");
                        showMessage(msg_.getDescription().toStdString());
                    }

                    auto isFirst = (i == 0);
                    auto isLast = (i == track->getNumEvents() - 1);
                    new_midi_event_dropped_manually = MidiFileEvent(msg_, isFirst, isLast);
                    new_event_from_DAW = std::nullopt;
                    ready2send2MDL = deploy(new_midi_event_dropped_manually, new_event_from_DAW, false);

                    // push to next thread if a new input is provided
                    if (ready2send2MDL) {
                        model_input.timer.registerStartTime();
                        ITP2MDL_ModelInput_Que_ptr->push(model_input);
                        inputs_sent_count++;
                        chrono_timed_consecutive_pushes.registerEndTime();

                        if (print_timed_consecutive_ModelInputs_pushed && chrono_timed_consecutive_pushes.isValid()) {
                            if (inputs_sent_count > 1) {
                                auto text = "Time Duration Between ModelInput #" + std::to_string(inputs_sent_count);
                                text += " && #" + std::to_string(inputs_sent_count - 1) + ": ";
                                showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                            } else {
                                auto text = "Time Duration Between Start && First Pushed ModelInput: ";
                                showMessage(*chrono_timed_consecutive_pushes.getDescription(text));
                            }
                        }
                        chrono_timed_consecutive_pushes.registerStartTime();
                    }
                }
            }
        }



        // update event trackers accordingly if applicable
        if (new_event_from_DAW.has_value()) {

            if (new_event_from_DAW->isFirstBufferEvent()) { first_frame_metadata_event = *new_event_from_DAW; }
            else if (new_event_from_DAW->isNewBufferEvent()) { frame_metadata_event = *new_event_from_DAW; }
            else if (new_event_from_DAW->isNewBarEvent()) { last_bar_event = *new_event_from_DAW; }
            else if (new_event_from_DAW->isNewTimeShiftEvent()) { last_complete_note_duration_event = *new_event_from_DAW; }

            last_event = *new_event_from_DAW;
        }

        // check if thread is still running
        bExit = threadShouldExit();

        if (!new_event_from_DAW.has_value() && !gui_params.changed()) {
            // wait for a few ms to avoid burning the CPU if new data is not available
            sleep(thread_configurations::InputTensorPreparator::waitTimeBtnIters);
        }
    }


}


void InputTensorPreparatorThread::prepareToStop() {
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::InputTensorPreparator::waitTimeBtnIters);

    readyToStop = true;
}

InputTensorPreparatorThread::~InputTensorPreparatorThread() {
    if (!readyToStop) {
        prepareToStop();
    }
}

void InputTensorPreparatorThread::DisplayTensor(const torch::Tensor &tensor, const string Label) {
    // if (disable_user_tensor_display_requests) { return; }
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines and print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[ITP] " << line << std::endl;
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
