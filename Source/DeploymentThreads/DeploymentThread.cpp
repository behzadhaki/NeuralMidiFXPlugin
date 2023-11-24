//
// Created by u153171 on 11/22/2023.
//

#include "DeploymentThread.h"

DeploymentThread::DeploymentThread(): juce::Thread("BackgroundDPLThread") {}

void DeploymentThread::startThreadUsingProvidedResources(
    LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size> *NMP2DPL_Event_Que_ptr_,
    LockFreeQueue<GuiParams, queue_settings::APVM_que_size> *APVM2NMD_Parameters_Que_ptr_,
    LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size> *DPL2NMP_GenerationEvent_Que_ptr_,
    LockFreeQueue<juce::MidiFile, 4>* GUI2DPL_DroppedMidiFile_Que_ptr_,
    RealTimePlaybackInfo *realtimePlaybackInfo_ptr_)
{
    NMP2DPL_Event_Que_ptr = NMP2DPL_Event_Que_ptr_;
    APVM2NMD_Parameters_Que_ptr = APVM2NMD_Parameters_Que_ptr_;
    DPL2NMP_GenerationEvent_Que_ptr = DPL2NMP_GenerationEvent_Que_ptr_;
    GUI2DPL_DroppedMidiFile_Que_ptr = GUI2DPL_DroppedMidiFile_Que_ptr_;
    realtimePlaybackInfo = realtimePlaybackInfo_ptr_;

    // Start the thread. This function internally calls run() method. DO NOT CALL run() DIRECTLY.
    // ---------------------------------------------------------------------------------------------
    startThread();
}

void DeploymentThread::run() {

    // convert showMessage to a lambda function
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::green << "[DPL] " << line << std::endl;
        }
    };

    // notify if the thread is still running
    bool bExit = threadShouldExit();

    double events_received_count = 0;

    chrono_timer chrono_timed_deploy;
    chrono_timer chrono_timed_consecutive_pushes;
    chrono_timed_consecutive_pushes.registerStartTime();
    std::optional<EventFromHost> new_event_from_DAW {};
    std::optional<MidiFileEvent> new_midi_event_dropped_manually {};
    std::optional<ModelInput> ModelInput2send2MDL;

    bool shouldSendNewPlaybackPolicy{false};
    bool shouldSendNewPlaybackSequence{false};

    int cnt{0};

    while (!bExit) {

        if (readyToStop) { break; } // check if thread is ready to be stopped
        if (APVM2NMD_Parameters_Que_ptr->getNumReady() > 0) {
            // print updated values for debugging
            gui_params = APVM2NMD_Parameters_Que_ptr->pop(); // pop the latest parameters from the queue
            gui_params.registerAccess();                      // set the time when the parameters were accessed

            if (debugging_settings::InputTensorPreparatorThread::print_received_gui_params) { // if set in Debugging.h
                showMessage(gui_params.getDescriptionOfUpdatedParams());
            }

        } else {
            gui_params.setChanged(false); // no change in parameters since last check
        }

        if (NMP2DPL_Event_Que_ptr->getNumReady() > 0) {
            new_event_from_DAW = NMP2DPL_Event_Que_ptr->pop();      // get the next available event
            new_event_from_DAW
                ->registerAccess();                    // set the time when the event was accessed

            events_received_count++;

            if (debugging_settings::InputTensorPreparatorThread::print_input_events) { // if set in Debugging.h
                DisplayEvent(*new_event_from_DAW, false, events_received_count);   // display the event
            }

        } else {
            new_event_from_DAW = std::nullopt;
        }

        if (new_event_from_DAW.has_value() || gui_params.changed()) {
            new_midi_event_dropped_manually = std::nullopt;
            chrono_timed_deploy.registerStartTime();
            auto status = deploy(new_midi_event_dropped_manually, new_event_from_DAW, gui_params.changed());
            shouldSendNewPlaybackPolicy = status.first;
            shouldSendNewPlaybackSequence = status.second;

            chrono_timed_deploy.registerEndTime();

            if (debugging_settings::InputTensorPreparatorThread::print_deploy_method_time &&
                chrono_timed_deploy.isValid()) { // if set in Debugging.h
                showMessage(*chrono_timed_deploy.getDescription(" deploy() execution time: "));
            }
        }

        // check if notes received from a manually dropped midi file
        if (GUI2DPL_DroppedMidiFile_Que_ptr->getNumReady() > 0){
            auto midifile = GUI2DPL_DroppedMidiFile_Que_ptr->getLatestOnly();

            if (midifile.getNumTracks() > 0) {
                auto track = midifile.getTrack(0);
                for (int i = 0; i < track->getNumEvents(); ++i)
                {
                    auto msg_ = track->getEventPointer(i)->message;

                    if (debugging_settings::InputTensorPreparatorThread::print_manually_dropped_midi_messages) { // if set in Debugging.h
                        showMessage("Midi Message Received from Dropped Midi File: ");
                        showMessage(msg_.getDescription().toStdString());
                    }

                    auto isFirst = (i == 0);
                    auto isLast = (i == track->getNumEvents() - 1);
                    new_midi_event_dropped_manually = MidiFileEvent(msg_, isFirst, isLast);
                    new_event_from_DAW = std::nullopt;
                    auto status =  deploy(new_midi_event_dropped_manually, new_event_from_DAW, false);
                    shouldSendNewPlaybackPolicy = status.first;
                    shouldSendNewPlaybackSequence = status.second;
                }
            }
        }

        // push to next thread if a new input is provided
        if (shouldSendNewPlaybackPolicy) {
            // send to the main thread (NMP)
            if (playbackPolicy.IsReadyForTransmission()) {
                DPL2NMP_GenerationEvent_Que_ptr->push(GenerationEvent(playbackPolicy));
            }
        }

        if (shouldSendNewPlaybackSequence) {
            // send to the main thread (NMP)
            DPL2NMP_GenerationEvent_Que_ptr->push(GenerationEvent(playbackSequence));
            cnt++;
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
            sleep((int)thread_configurations::SingleMidiThread::waitTimeBtnIters);
        }
    }

}

void DeploymentThread::prepareToStop()
{
    // Need to wait enough to ensure the run() method is over before killing thread
    this->stopThread(100 * thread_configurations::SingleMidiThread::waitTimeBtnIters);

    readyToStop = true;
}

DeploymentThread::~DeploymentThread()
{
    if (!readyToStop) {
        prepareToStop();
    }
}

void DeploymentThread::DisplayEvent(const EventFromHost& event,
                                    bool compact_mode,
                                    double event_count)
{
    auto showMessage = [](const std::string& input) {
        // if input is multiline, split it into lines && print each line separately
        std::stringstream ss(input);
        std::string line;

        while (std::getline(ss, line)) {
            std::cout << clr::on_red << "[DPL] " << line << std::endl;
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

void DeploymentThread::PrintMessage(const string& input)
{
    // if input is multiline, split it into lines && print each line separately
    std::stringstream ss(input);
    std::string line;
    while (std::getline(ss, line)) { std::cout << clr::on_red << "[ITP] " << line << std::endl; }
}