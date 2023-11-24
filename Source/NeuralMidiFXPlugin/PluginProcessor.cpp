#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace std;
using namespace debugging_settings::ProcessorThread;

inline double mapToLoopRange(double value, double loopStart, double loopEnd) {

    double loopDuration = loopEnd - loopStart;

    // Compute the offset relative to the loop start
    double offset = fmod(value - loopStart, loopDuration);

    // Handle negative values if 'value' is less than 'loopStart'
    if (offset < 0) offset += loopDuration;

    // Add the offset to the loop start to get the mapped value
    double mappedValue = loopStart + offset;

    return mappedValue;
}

NeuralMidiFXPluginProcessor::NeuralMidiFXPluginProcessor() : apvts(
        *this, nullptr, "PARAMETERS",
        createParameterLayout()) {

    realtimePlaybackInfo = make_unique<RealTimePlaybackInfo>();

    //       Make_unique pointers for Queues
    // ----------------------------------------------------------------------------------
    if (std::strcmp(PROJECT_NAME, "NMFx_ThreeThreads") == 0)
    {
        NMP2ITP_Event_Que =
            make_unique<LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size>>();
        ITP2MDL_ModelInput_Que =
            make_unique<LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size>>();
        MDL2PPP_ModelOutput_Que =
            make_unique<LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size>>();
        PPP2NMP_GenerationEvent_Que = make_unique<
            LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size>>();
    } else { // single thread mode
        // used for NMP2DPL_Event_Que
        NMP2ITP_Event_Que =
            make_unique<LockFreeQueue<EventFromHost, queue_settings::NMP2ITP_que_size>>();
        // used for DPL2NMP_GenerationEvent_Que
        PPP2NMP_GenerationEvent_Que = make_unique<
            LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size>>();
    }

    //     Make_unique pointers for APVM Queues
    // ----------------------------------------------------------------------------------
    if (std::strcmp(PROJECT_NAME, "NMFx_ThreeThreads") == 0) {
        APVM2ITP_GuiParams_Que =
            make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();
        APVM2MDL_GuiParams_Que =
            make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();
        APVM2PPP_GuiParams_Que =
            make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();
    } else { // single thread mode
        // used for APVM2DPL_Parameters_Que
        APVM2ITP_GuiParams_Que =
            make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();
    }

    // Queues used in both single and three thread mode
    GUI2ITP_DroppedMidiFile_Que =
        make_unique<LockFreeQueue<juce::MidiFile, 4>>();
    PPP2GUI_GenerationMidiFile_Que =
        make_unique<LockFreeQueue<juce::MidiFile, 4>>();
    NMP2GUI_IncomingMessageSequence =
        make_unique<LockFreeQueue<juce::MidiMessageSequence, 32>>() ;

    //       Create shared pointers for Three Threads if PROJECT_NAME is NMFx_ThreeThreads
    // ----------------------------------------------------------------------------------
    if (std::strcmp(PROJECT_NAME, "NMFx_ThreeThreads") == 0) {
        inputTensorPreparatorThread = make_shared<InputTensorPreparatorThread>();
        modelThread = make_shared<ModelThread>();
        playbackPreparatorThread = make_shared<PlaybackPreparatorThread>();
    } else { // single thread mode
        singleMidiThread = make_shared<DeploymentThread>();
    }

    //      Create shared pointers for APVTSMediator
    // ----------------------------------------------------------------------------------
    apvtsMediatorThread = make_unique<APVTSMediatorThread>();

    //       give access to resources && run threads
    // ----------------------------------------------------------------------------------
    if (std::strcmp(PROJECT_NAME, "NMFx_ThreeThreads") == 0) {

        inputTensorPreparatorThread->startThreadUsingProvidedResources(
            NMP2ITP_Event_Que.get(),
            ITP2MDL_ModelInput_Que.get(),
            APVM2ITP_GuiParams_Que.get(),
            GUI2ITP_DroppedMidiFile_Que.get(),
            realtimePlaybackInfo.get());
        modelThread->startThreadUsingProvidedResources(
            ITP2MDL_ModelInput_Que.get(),
            MDL2PPP_ModelOutput_Que.get(),
            APVM2MDL_GuiParams_Que.get(),
            realtimePlaybackInfo.get());
        playbackPreparatorThread->startThreadUsingProvidedResources(
            MDL2PPP_ModelOutput_Que.get(),
            PPP2NMP_GenerationEvent_Que.get(),
            APVM2PPP_GuiParams_Que.get(),
            realtimePlaybackInfo.get());
    } else {
        singleMidiThread->startThreadUsingProvidedResources(
            NMP2ITP_Event_Que.get(),
            APVM2ITP_GuiParams_Que.get(),
            PPP2NMP_GenerationEvent_Que.get(),
            GUI2ITP_DroppedMidiFile_Que.get(),
            realtimePlaybackInfo.get());
    }

    // check if PROJECT_NAME is NMFx_ThreeThreads
    if (std::strcmp(PROJECT_NAME, "NMFx_ThreeThreads") == 0) {
    // give access to resources && run threads
        apvtsMediatorThread->startThreadUsingProvidedResources(
            &apvts,
            APVM2ITP_GuiParams_Que.get(),
            APVM2MDL_GuiParams_Que.get(),
            APVM2PPP_GuiParams_Que.get());
    } else {
        // give access to resources && run threads
        apvtsMediatorThread->startThreadUsingProvidedResources(
            &apvts,
            APVM2ITP_GuiParams_Que.get(),
            nullptr,
            nullptr);
    }

    /*
    if (JUCEApplicationBase::isStandaloneApp()) {
        DBG("Running as standalone");
    } else
    {
        DBG("Running as plugin");
    }
    */

    // check if standalone and osx
    // --------------------------------------------------------------------------------------
    #if JUCE_MAC
        if (UIObjects::StandaloneTransportPanel::NeedVirtualMidiOutCable) {
            string virtualOutName = string(PROJECT_NAME) + "Generations";
            vector<string> existingInstanceLabels{};

            // add any midi in device that has (even partially)
            // the same name as the virtual midi out device
            // NOTE: PREVIOUSLY CREATED MIDIOUTS WILL NOW SHOW UP AS MIDIINS!
            int count = 0;
            for (auto device : MidiInput::getAvailableDevices()) {
                DBG("Found midi out device: " + device.name.toStdString());
                if (device.name.toStdString().find(virtualOutName) != string::npos) {
                    DBG("Found existing instance of virtual midi out device: "
                    + device.name.toStdString());
                    existingInstanceLabels.push_back(device.name.toStdString());
                    count++;
                }

                if (existingInstanceLabels.size() >= 1) {
                    // get longest one
                    string longestLabel = "";
                    for (auto label : existingInstanceLabels) {
                        if (label.length() > longestLabel.length()) {
                            longestLabel = label;
                        }
                    }
                    virtualOutName = longestLabel + "_" + std::to_string(count);
                }
            }

            // create a virtual midi output device
            mVirtualMidiOutput = juce::MidiOutput::createNewDevice (virtualOutName);
            mVirtualMidiOutput->startBackgroundThread();
        }
    #endif

    if (UIObjects::StandaloneTransportPanel::enable) {
        standAloneParams = make_unique<StandAloneParams>(&apvts);
    }
}

NeuralMidiFXPluginProcessor::~NeuralMidiFXPluginProcessor() {
    if (mVirtualMidiOutput) {
        mVirtualMidiOutput->stopBackgroundThread();
        mVirtualMidiOutput = nullptr;
    }

    if (std::strcmp(PROJECT_NAME, "NMFx_ThreeThreads") == 0) {

        if (!modelThread->readyToStop) {
                modelThread->prepareToStop();
        }
        if (!inputTensorPreparatorThread->readyToStop) {
                inputTensorPreparatorThread->prepareToStop();
        }
        if (!playbackPreparatorThread->readyToStop) {
                playbackPreparatorThread->prepareToStop();
        }
    } else {
        if (!singleMidiThread->readyToStop) {
                singleMidiThread->prepareToStop();
        }
    }
}

void NeuralMidiFXPluginProcessor::PrintMessage(const std::string& input) {
    using namespace debugging_settings::ProcessorThread;
    if (disableAllPrints) { return; }

    // if input is multiline, split it into lines && print each line separately
    std::stringstream ss(input);
    std::string line;

    // get string from now
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::string now_str = std::ctime(&now_c);
    // remove newline
    now_str.erase(std::remove(now_str.begin(), now_str.end(), '\n'),
                  now_str.end());

    while (std::getline(ss, line)) {
        std::cout << clr::cyan << "[NMP] " << now_str << "|" <<
            line << clr::reset << std::endl;
    }
}

// checks whether the note timing (adjusted by the time anchor) is within the current buffer
// if so, returns the number of samples from the start of the buffer to the note
// if not, returns -1
std::optional<juce::MidiMessage> NeuralMidiFXPluginProcessor::getMessageIfToBePlayed(
    time_ now_, const juce::MidiMessage &msg_, int buffSize, double fs,
    double qpm) const {

    auto msg = juce::MidiMessage(msg_);
    if (msg_.getTimeStamp() <= 0) {
        msg = msg.withTimeStamp(0.01);
    }
    auto now_in_user_unit = now_.getTimeWithUnitType(playbackPolicies.getTimeUnitIndex());
    auto msg_time = msg.getTimeStamp();

    double end = 0;
    double user_unit_to_samples_adjustment_factor = 1;

    if (playbackPolicies.getLoopDuration() > 0) {
        auto loop_start = time_anchor_for_playback.getTimeWithUnitType(
            playbackPolicies.getTimeUnitIndex());
        auto loop_end = loop_start + playbackPolicies.getLoopDuration();
        auto now_ppq_mapped = now_.inQuarterNotes();
        now_ppq_mapped = mapToLoopRange(
            now_ppq_mapped, loop_start, loop_end);
        // convert ppq back into user unit
        switch (playbackPolicies.getTimeUnitIndex()) {
            case 1: // samples
                // convert quarter notes to samples
                now_in_user_unit = now_ppq_mapped * fs * 60.0f / qpm;
                break;
            case 2: // seconds
                // convert quarter notes to seconds
                now_in_user_unit = now_ppq_mapped * 60.0f / qpm;
                break;
            case 3: // QuarterNotes
                now_in_user_unit = now_ppq_mapped;
                break;
        }
    }

    switch (playbackPolicies.getTimeUnitIndex()) {
        case 1: // samples
            end = now_in_user_unit + buffSize;
            break;
        case 2: // seconds
            user_unit_to_samples_adjustment_factor = fs;
            end = now_in_user_unit + buffSize / fs;
            break;
        case 3: // QuarterNotes
            user_unit_to_samples_adjustment_factor = fs * 60.0f / qpm;
            end = now_in_user_unit + buffSize / fs * qpm / 60.0f;
            break;
        default: // Unknown index
            return {};
    }

    if (now_in_user_unit <= msg_time && msg_time < end) {
        auto time_diff = msg_time - now_in_user_unit;
        auto msg_copy = juce::MidiMessage(msg);
        msg_copy.setTimeStamp(std::max(
            0.0, std::floor(time_diff * user_unit_to_samples_adjustment_factor)));
        return msg_copy;
    }
    return {};
}


void NeuralMidiFXPluginProcessor::sendReceivedInputsAsEvents(
        MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo,
        double fs, int buffSize) {

    using namespace event_communication_settings;
    if (Pinfo) {

        if (!last_frame_meta_data.isPlaying() != !Pinfo->getIsPlaying()) {
            // if just started, register the playhead starting position
            if ((!last_frame_meta_data.isPlaying()) && Pinfo->getIsPlaying()) {
                if (print_start_stop_times) {
                    PrintMessage("Started playing");
                }
                playhead_start_time = time_{*Pinfo->getTimeInSamples(),
                                            *Pinfo->getTimeInSeconds(),
                                            *Pinfo->getPpqPosition()};

                time_anchor_for_playback = time_{*Pinfo->getTimeInSamples(),
                                                 *Pinfo->getTimeInSeconds(),
                                                 *Pinfo->getPpqPosition()};

                auto frame_meta_data = EventFromHost {Pinfo, fs,
                                                      buffSize, true};
                NMP2ITP_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;
                incoming_messages_sequence = juce::MidiMessageSequence();
                cout << "NMP sending empty message to GUI" << endl;
                NMP2GUI_IncomingMessageSequence->push(incoming_messages_sequence);
            } else {
                // if just stopped, register the playhead stopping position
                auto frame_meta_data = EventFromHost {Pinfo, fs,
                                                      buffSize, false};
                if (print_start_stop_times) { PrintMessage("Stopped playing"); }
                frame_meta_data.setPlaybackStoppedEvent();
                NMP2ITP_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;     // reset last frame meta data

                // clear playback sequence if playbackpolicy specifies so
                if (playbackPolicies.getShouldClearGenerationsAfterPauseStop() &&
                    UIObjects::MidiInVisualizer::deletePreviousIncomingMidiMessagesOnRestart) {
                    PrintMessage("Clearing Generations");
                    playbackMessageSequence.clear();
                }
            }
        } else {
            // if still playing, register the playhead position
            if (Pinfo->getIsPlaying()) {
                if (print_new_buffer_started) { PrintMessage("New Buffer Arrived"); }
                auto frame_meta_data = EventFromHost {Pinfo, fs,
                                                      buffSize,
                                                      false};
                if (SendEventAtBeginningOfNewBuffers_FLAG) {
                    if (SendEventForNewBufferIfMetadataChanged_FLAG) {
                        if (frame_meta_data.getBufferMetaData() !=
                            last_frame_meta_data.getBufferMetaData()) {
                            NMP2ITP_Event_Que->push(frame_meta_data);
                        }
                    } else {
                        NMP2ITP_Event_Que->push(frame_meta_data);
                    }
                }

                // if playhead moved manually backward clear all events after now
                if (frame_meta_data.Time().inQuarterNotes() < last_frame_meta_data.Time().inQuarterNotes())
                {
                    if (UIObjects::MidiInVisualizer::deletePreviousIncomingMidiMessagesOnBackwardPlayhead) { // todo add flag to settings file for this
                        auto incoming_messages_sequence_temp = juce::MidiMessageSequence();
                        for (int i = 0; i < incoming_messages_sequence.getNumEvents(); i++) {
                            auto msg = incoming_messages_sequence.getEventPointer(i);
                            if (msg->message.getTimeStamp() < frame_meta_data.Time().inQuarterNotes()) {
                                incoming_messages_sequence_temp.addEvent(msg->message, 0);
                            }
                        }
                        // add all note off at last frame metadata time
                        for (int i = 0; i < 128; i++) {
                            incoming_messages_sequence_temp.addEvent(juce::MidiMessage::noteOff(1, i),
                                                                     last_frame_meta_data.Time().inQuarterNotes());
                        }
                        incoming_messages_sequence.swapWith(incoming_messages_sequence_temp);
                        NMP2GUI_IncomingMessageSequence->push(incoming_messages_sequence);
                    } else {
                        // add all note off at last frame meta data time to incoming messages sequence
                        for (int i = 0; i < 128; i++) {
                            incoming_messages_sequence.addEvent(juce::MidiMessage::noteOff(1, i),
                                                                 last_frame_meta_data.Time().inQuarterNotes());
                        }
                        NMP2GUI_IncomingMessageSequence->push(incoming_messages_sequence);
                    }
                }

                last_frame_meta_data = frame_meta_data;
            }
        }

        if (Pinfo->getIsPlaying()) {
            // check if new bar within buffer
            NewBarEvent = last_frame_meta_data.checkIfNewBarHappensWithinBuffer();
            // check if new bar within buffer
            NewTimeShiftEvent = last_frame_meta_data.checkIfTimeShiftEventHappensWithinBuffer(
                    delta_TimeShiftEventRatioOfQuarterNote);
        } else {
            NewBarEvent = std::nullopt;
            NewTimeShiftEvent = std::nullopt;
            last_frame_meta_data.setIsPlaying(false);     // reset last frame meta data
        }

        // Step 4. see if new notes are played on the input side
        if (!midiMessages.isEmpty() && Pinfo->getIsPlaying()) {
            bool NoteOnOffReceived = false;

            // if there are new notes, send them to the groove thread
            for (const auto midiMessage: midiMessages) {
                auto msg = midiMessage.getMessage();
                auto midiEvent = EventFromHost {Pinfo, fs, buffSize, msg};

                // check if new bar event exists && it is before the current midi event
                if (NewBarEvent.has_value() && SendNewBarEvents_FLAG) {
                    if (midiEvent.Time().inSamples() >= NewBarEvent->Time().inSamples()) {
                        NMP2ITP_Event_Que->push(*NewBarEvent);
                        NewBarEvent = std::nullopt;
                    }
                }

                // check if a specified number of whole notes has passed
                if (NewTimeShiftEvent.has_value() && SendTimeShiftEvents_FLAG) {
                    if (midiEvent.Time().inSamples() >= NewTimeShiftEvent->Time().inSamples()) {
                        NMP2ITP_Event_Que->push(*NewTimeShiftEvent);
                        NewTimeShiftEvent = std::nullopt;
                    }
                }

                if (midiEvent.isMidiMessageEvent()) {
                    if (midiEvent.isNoteOnEvent()) {
                        if (!FilterNoteOnEvents_FLAG) {
                            NMP2ITP_Event_Que->push(midiEvent);
                        }
                        NoteOnOffReceived = true;
                        incoming_messages_sequence.addEvent(
                            juce::MidiMessage::noteOn(
                                1,midiEvent.getNoteNumber(),
                                 midiEvent.getVelocity()
                            ), midiEvent.Time().inQuarterNotes());
                    }

                    if (midiEvent.isNoteOffEvent()) {
                        if (!FilterNoteOffEvents_FLAG) {
                            NMP2ITP_Event_Que->push(midiEvent);
                        }
                        NoteOnOffReceived = true;
                        incoming_messages_sequence.addEvent(
                            juce::MidiMessage::noteOff(
                                1, midiEvent.getNoteNumber(),
                                midiEvent.getVelocity()
                            ), midiEvent.Time().inQuarterNotes());
                    }

                    if (NoteOnOffReceived) {
                        NMP2GUI_IncomingMessageSequence->push(incoming_messages_sequence);
                    }
                    if (midiEvent.isCCEvent()) {
                        if (!FilterCCEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
                    }
                }
            }
        }

        // if there is a new bar event, && hasn't been sent yet, send it
        if (NewBarEvent.has_value() && SendNewBarEvents_FLAG) {
            NMP2ITP_Event_Que->push(*NewBarEvent);
            NewBarEvent = std::nullopt;
        }
        if (NewTimeShiftEvent.has_value() && SendTimeShiftEvents_FLAG) {
            NMP2ITP_Event_Que->push(*NewTimeShiftEvent);
            NewTimeShiftEvent = std::nullopt;
        }
    }
}

juce::AudioProcessorEditor *NeuralMidiFXPluginProcessor::createEditor() {
    auto editor = new NeuralMidiFXPluginEditor(*this);
    /*modelThread.addChangeListener(editor);*/
    return editor;
}


juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
    return new NeuralMidiFXPluginProcessor();
}

juce::AudioProcessorValueTreeState::ParameterLayout NeuralMidiFXPluginProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    int version_hint = 1;

    std::string name;
    bool isToggleable;
    double minValue, maxValue, initValue;
    std::string topleftCorner{};
    std::string bottomrightCorner{};

    size_t numTabs = UIObjects::Tabs::tabList.size();
    size_t numSliders;
    size_t numRotaries;
    size_t numButtons;
    size_t numhsliders;
    size_t numComboBoxes;

    tab_tuple tabTuple;

    slider_list sliderList;
    rotary_list rotaryList;
    button_list buttonList;
    hslider_list hsliderList;
    comboBox_list comboboxList;

    slider_tuple sliderTuple;
    rotary_tuple rotaryTuple;
    button_tuple buttonTuple;
    hslider_tuple hsliderTuple;
    comboBox_tuple comboboxTuple;


    for (size_t j = 0; j < numTabs; j++) {
        tabTuple = UIObjects::Tabs::tabList[j];

        sliderList = std::get<1>(tabTuple);
        rotaryList = std::get<2>(tabTuple);
        buttonList = std::get<3>(tabTuple);
        hsliderList = std::get<4>(tabTuple);
        comboboxList = std::get<5>(tabTuple);

        numSliders = sliderList.size();
        numRotaries = rotaryList.size();
        numButtons = buttonList.size();
        numhsliders = hsliderList.size();
        numComboBoxes = comboboxList.size();

        // Sliders
        for (size_t i = 0; i < numSliders; ++i) {
            sliderTuple = sliderList[i];
            std::tie(name, minValue, maxValue, initValue, topleftCorner, bottomrightCorner) = sliderTuple;

            // Param ID will read "Slider" + [tab, item] i.e. 'Slider_13"
            juce::String paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            layout.add(std::make_unique<juce::AudioParameterFloat>(paramID, name, minValue, maxValue, initValue));
        }

        // Rotaries
        for (size_t i = 0; i < numRotaries; ++i) {
            rotaryTuple = rotaryList[i];
            std::tie(name, minValue,maxValue, initValue, topleftCorner, bottomrightCorner) = rotaryTuple;

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            layout.add (std::make_unique<juce::AudioParameterFloat> (paramID, name, minValue, maxValue, initValue));
        }

        // Buttons
        for (size_t i = 0; i < numButtons; ++i) {
            std::tie(name, isToggleable, topleftCorner, bottomrightCorner) = buttonList[i];
            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);
            layout.add (std::make_unique<juce::AudioParameterInt> (paramID, name, 0, 1, 0));
        }

        // Vertical Sliders
        for (size_t i = 0; i < numhsliders; ++i) {
            hsliderTuple = hsliderList[i];
            std::tie(name, minValue, maxValue, initValue, topleftCorner, bottomrightCorner) = hsliderTuple;

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            layout.add (std::make_unique<juce::AudioParameterFloat> (paramID, name, minValue, maxValue, initValue));
        }

        // Combo Boxes
        for (size_t i = 0; i < numComboBoxes; ++i) {
            comboboxTuple = comboboxList[i];
            std::vector<std::string> items;

            std::tie(name, items, topleftCorner, bottomrightCorner) = comboboxTuple;

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);
            layout.add (std::make_unique<juce::AudioParameterInt> (paramID, name, 0, items.size() - 1, 0));
        }

    }

    // check if standalone mode enabled
    if (UIObjects::StandaloneTransportPanel::enable) {
        // add standalone only parameters
        layout.add(
            std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(label2ParamID("IsPlayingStandalone"), version_hint),
                "IsPlayingStandalone", 0, 1, 0));

        layout.add(
            std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(label2ParamID("IsRecordingStandalone"), version_hint),
                "IsRecordingStandalone", 0, 1, 0));

        layout.add(
            std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(label2ParamID("TempoStandalone"), version_hint),
                "TempoStandalone", 20, 300, 100));

        layout.add(
            std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(label2ParamID("TimeSigNumeratorStandalone"), version_hint),
                "TimeSigNumeratorStandalone", 1, 32, 4));

        layout.add(
            std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID(label2ParamID("TimeSigDenominatorStandalone"), version_hint),
                "TimeSigDenominatorStandalone", 1, 32, 4));

    }

    return layout;
}

void NeuralMidiFXPluginProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);

//    std::vector<torch::Tensor> myTensors = ...; // Your vector of tensors
    // random tensor
    std::vector<torch::Tensor> myTensors;

    // Tensor 1: A 1D tensor of floats
    myTensors.push_back(torch::tensor({1.0, 2.0, 3.0, 4.0}));

    // Tensor 2: A 2D tensor of integers
    myTensors.push_back(torch::tensor({{1, 2}, {3, 4}}, torch::dtype(torch::kInt32)));

    // Tensor 3: A 3D tensor of bytes
    myTensors.push_back(torch::tensor({{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}}, torch::dtype(torch::kUInt8)));

    // Tensor 4: A 2D tensor of doubles
    myTensors.push_back(torch::tensor({{1.5, 2.5}, {3.5, 4.5}}, torch::dtype(torch::kFloat64)));


    size_t numTensors = myTensors.size();
    destData.append(&numTensors, sizeof(numTensors));

    for (const auto& tensor : myTensors) {
        // Serialize tensor type
        torch::ScalarType tensorType = tensor.scalar_type();
        int tensorTypeId = static_cast<int>(tensorType);
        destData.append(&tensorTypeId, sizeof(tensorTypeId));

        // Serialize tensor shape
        auto shape = tensor.sizes();
        size_t numDims = shape.size();
        destData.append(&numDims, sizeof(numDims));
        for (const auto& dim : shape) {
            int64_t dimSize = dim;
            destData.append(&dimSize, sizeof(dimSize));
        }

        // Serialize tensor data
        size_t tensorSize = tensor.numel() * tensor.element_size();
        destData.append(tensor.data_ptr(), tensorSize);
    }
}

void NeuralMidiFXPluginProcessor::setStateInformation(const void *data, int sizeInBytes) {
    // Extract XML state
    auto xmlData = static_cast<const char*>(data);
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(xmlData, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));

    const char* tensorData = xmlData + sizeInBytes;
    size_t numTensors;
    memcpy(&numTensors, tensorData, sizeof(numTensors));
    tensorData += sizeof(numTensors);

    std::vector<torch::Tensor> myTensors;
    myTensors.reserve(numTensors);

    for (size_t i = 0; i < numTensors; ++i) {
        // Deserialize tensor type
        int tensorTypeId;
        memcpy(&tensorTypeId, tensorData, sizeof(tensorTypeId));
        tensorData += sizeof(tensorTypeId);
        auto tensorType = static_cast<torch::ScalarType>(tensorTypeId);

        // Deserialize tensor shape
        size_t numDims;
        memcpy(&numDims, tensorData, sizeof(numDims));
        tensorData += sizeof(numDims);

        std::vector<int64_t> shape(numDims);
        for (size_t j = 0; j < numDims; ++j) {
            int64_t dimSize;
            memcpy(&dimSize, tensorData, sizeof(dimSize));
            tensorData += sizeof(dimSize);
            shape[j] = dimSize;
        }

        // Deserialize tensor data
        size_t tensorSize = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<>()) * torch::elementSize(tensorType);
        torch::Tensor tensor = torch::from_blob((void*)tensorData, shape, tensorType);
        tensorData += tensorSize;

        myTensors.push_back(tensor);
    }

    std::cout << "Deserialized tensors:" << std::endl;
    // print myTensors
    for (const auto& tensor : myTensors) {
        std::cout << tensor << std::endl;
    }

}


