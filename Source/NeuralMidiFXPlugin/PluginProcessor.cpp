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

    // Populate Pianoroll Data
    // ----------------------------------------------------------------------------------
    auto tabList = UIObjects::Tabs::tabList;

    // populate midiVisualizersData
    {
        int count = 0;
        std::vector<std::string> param_ids;

        for (auto tab_list: tabList)
        {
            auto midiVisualizers = std::get<6>(tab_list);
            for (auto midiVisualizerJson: midiVisualizers)
            {
                param_ids.push_back(
                    label2ParamID(midiVisualizerJson["label"].get<std::string>()));
                count++;
            }
        }

        midiVisualizersData = std::make_unique<MidiVisualizersData>(param_ids);
    }

    // populate audioVisualizersData
    {
        int count = 0;
        std::vector<std::string> param_ids;

        for (auto tab_list: tabList)
        {
            auto audioVisualizers = std::get<7>(tab_list);
            for (auto audioVisualizerJson: audioVisualizers)
            {
                param_ids.push_back(
                    label2ParamID(audioVisualizerJson["label"].get<std::string>()));
                count++;
            }
        }

        audioVisualizersData = std::make_unique<AudioVisualizersData>(param_ids);
    }


    //       Make_unique pointers for Queues
    // ----------------------------------------------------------------------------------
    // used for NMP2DPL_Event_Que
    NMP2DPL_Event_Que =
        make_unique<LockFreeQueue<EventFromHost, queue_settings::NMP2DPL_que_size>>();
    // used for DPL2NMP_GenerationEvent_Que
    DPL2NMP_GenerationEvent_Que = make_unique<
        LockFreeQueue<GenerationEvent, queue_settings::DPL2NMP_que_size>>();

    //     Make_unique pointers for APVM Queues
    // ----------------------------------------------------------------------------------
    APVM2DPL_GuiParams_Que =
        make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();

    // Queues used in both single and three thread mode
    GUI2DPL_DroppedMidiFile_Que =
        make_unique<LockFreeQueue<juce::MidiFile, 4>>();
    DPL2GUI_GenerationMidiFile_Que =
        make_unique<LockFreeQueue<juce::MidiFile, 4>>();
    NMP2GUI_IncomingMessageSequence =
        make_unique<LockFreeQueue<juce::MidiMessageSequence, 32>>() ;

    // ----------------------------------------------------------------------------------
    deploymentThread = make_shared<PluginDeploymentThread>();
    apvtsMediatorThread =
        make_unique<APVTSMediatorThread>(deploymentThread->CustomPresetData.get());


    //       give access to resources && run threads
    // ----------------------------------------------------------------------------------
    deploymentThread->startThreadUsingProvidedResources(
        NMP2DPL_Event_Que.get(),
        APVM2DPL_GuiParams_Que.get(),
        DPL2NMP_GenerationEvent_Que.get(),
        GUI2DPL_DroppedMidiFile_Que.get(),
        realtimePlaybackInfo.get(),
        midiVisualizersData.get(),
        audioVisualizersData.get());


    // give access to resources && run threads
    apvtsMediatorThread->startThreadUsingProvidedResources(
        &apvts,
        APVM2DPL_GuiParams_Que.get());

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

    if (!deploymentThread->readyToStop) {
        deploymentThread->prepareToStop();
    }
    if (!apvtsMediatorThread->readyToStop) {
        apvtsMediatorThread->prepareToStop();
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
                NMP2DPL_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;
                incoming_messages_sequence = juce::MidiMessageSequence();
                NMP2GUI_IncomingMessageSequence->push(incoming_messages_sequence);
            } else {
                // if just stopped, register the playhead stopping position
                auto frame_meta_data = EventFromHost {Pinfo, fs,
                                                      buffSize, false};
                if (print_start_stop_times) { PrintMessage("Stopped playing"); }
                frame_meta_data.setPlaybackStoppedEvent();
                NMP2DPL_Event_Que->push(frame_meta_data);
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
                            NMP2DPL_Event_Que->push(frame_meta_data);
                        }
                    } else {
                        NMP2DPL_Event_Que->push(frame_meta_data);
                    }
                }

                // if playhead moved manually backward clear all events after now
                if (frame_meta_data.Time().inQuarterNotes() < last_frame_meta_data.Time().inQuarterNotes())
                {
                    if (UIObjects::MidiInVisualizer::deletePreviousIncomingMidiMessagesOnBackwardPlayhead) {
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
                        NMP2DPL_Event_Que->push(*NewBarEvent);
                        NewBarEvent = std::nullopt;
                    }
                }

                // check if a specified number of whole notes has passed
                if (NewTimeShiftEvent.has_value() && SendTimeShiftEvents_FLAG) {
                    if (midiEvent.Time().inSamples() >= NewTimeShiftEvent->Time().inSamples()) {
                        NMP2DPL_Event_Que->push(*NewTimeShiftEvent);
                        NewTimeShiftEvent = std::nullopt;
                    }
                }

                if (midiEvent.isMidiMessageEvent()) {
                    if (midiEvent.isNoteOnEvent()) {
                        if (!FilterNoteOnEvents_FLAG) {
                            NMP2DPL_Event_Que->push(midiEvent);
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
                            NMP2DPL_Event_Que->push(midiEvent);
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
                        if (!FilterCCEvents_FLAG) { NMP2DPL_Event_Que->push(midiEvent); }
                    }
                }
            }
        }

        // if there is a new bar event, && hasn't been sent yet, send it
        if (NewBarEvent.has_value() && SendNewBarEvents_FLAG) {
            NMP2DPL_Event_Que->push(*NewBarEvent);
            NewBarEvent = std::nullopt;
        }
        if (NewTimeShiftEvent.has_value() && SendTimeShiftEvents_FLAG) {
            NMP2DPL_Event_Que->push(*NewTimeShiftEvent);
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

    std::vector<std::string> param_ids_so_far;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    int version_hint = 1;

    size_t numTabs = UIObjects::Tabs::tabList.size();
    size_t numSliders;
    size_t numRotaries;
    size_t numButtons;
    size_t numhsliders;
    size_t numComboBoxes;
    size_t numTriangleSliders;

    tab_tuple tabTuple;

    slider_list sliderList;
    rotary_list rotaryList;
    button_list buttonList;
    hslider_list hsliderList;
    comboBox_list comboboxList;
    triangleSliders_list triangleSlidersList;

    for (size_t j = 0; j < numTabs; j++) {
        tabTuple = UIObjects::Tabs::tabList[j];

        sliderList = std::get<1>(tabTuple);
        rotaryList = std::get<2>(tabTuple);
        buttonList = std::get<3>(tabTuple);
        hsliderList = std::get<4>(tabTuple);
        comboboxList = std::get<5>(tabTuple);
        triangleSlidersList = std::get<10>(tabTuple);

        numSliders = sliderList.size();
        numRotaries = rotaryList.size();
        numButtons = buttonList.size();
        numhsliders = hsliderList.size();
        numComboBoxes = comboboxList.size();
        numTriangleSliders = triangleSlidersList.size();

        // Sliders
        for (size_t i = 0; i < numSliders; ++i) {
            auto sliderJson = sliderList[i];

            auto name = sliderJson["label"].get<std::string>();
            auto minValue = sliderJson["min"].get<float>();
            auto maxValue = sliderJson["max"].get<float>();
            auto initValue = sliderJson["default"].get<float>();


            // Param ID will read "Slider" + [tab, item] i.e. 'Slider_13"
            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            bool alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (param_id == paramIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterFloat> (paramID, name, minValue, maxValue, initValue));
                param_ids_so_far.push_back(paramIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramIDstr << ". \tCheck if intentional!" << endl;
            }

        }

        // Rotaries
        for (size_t i = 0; i < numRotaries; ++i) {
            auto rotaryJson = rotaryList[i];

            auto name = rotaryJson["label"].get<std::string>();
            auto minValue = rotaryJson["min"].get<float>();
            auto maxValue = rotaryJson["max"].get<float>();
            auto initValue = rotaryJson["default"].get<float>();

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            bool alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (param_id == paramIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterFloat> (paramID, name, minValue, maxValue, initValue));
                param_ids_so_far.push_back(paramIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramIDstr << ". Check if intentional!" << endl;
            }
        }

        // Buttons
        for (size_t i = 0; i < numButtons; ++i) {
            auto buttonJson = buttonList[i];

            auto name = buttonJson["label"].get<std::string>();

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            bool alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (param_id == paramIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterInt> (paramID, name, 0, 1, 0));
                param_ids_so_far.push_back(paramIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramIDstr << ". Check if intentional!" << endl;
            }
        }

        // Vertical Sliders
        for (size_t i = 0; i < numhsliders; ++i) {
            auto sliderJson = hsliderList[i];

            auto name = sliderJson["label"].get<std::string>();
            auto minValue = sliderJson["min"].get<float>();
            auto maxValue = sliderJson["max"].get<float>();
            auto initValue = sliderJson["default"].get<float>();

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            bool alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (param_id == paramIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterFloat> (paramID, name, minValue, maxValue, initValue));
                param_ids_so_far.push_back(paramIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramIDstr << ". Check if intentional!" << endl;
            }
        }

        // Combo Boxes
        for (size_t i = 0; i < numComboBoxes; ++i) {
            auto comboboxJson = comboboxList[i];

            auto name = comboboxJson["label"].get<std::string>();
            auto items = comboboxJson["items"].get<std::vector<std::string>>();
            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            bool alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (param_id == paramIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterInt> (paramID, name, 0, items.size() - 1, 0));
                param_ids_so_far.push_back(paramIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramIDstr << ". Check if intentional!" << endl;
            }
        }

        // Triangle Sliders
        for (size_t i = 0; i < numTriangleSliders; ++i) {
            auto json = triangleSlidersList[i];

            if (!json.contains("DistanceFromBottomLeftCornerSlider") || !json.contains("HeightSlider")) {
                cout << "ERROR: Triangle Slider missing DistanceFromBottomLeftCornerSlider or HeightSlider" << endl;
                continue;
            }

            auto DistanceFromBottomLeftCornerSlider = json["DistanceFromBottomLeftCornerSlider"].get<std::string>();
            auto HeightSlider = json["HeightSlider"].get<std::string>();

            auto paramAIDstr = label2ParamID(DistanceFromBottomLeftCornerSlider);
            auto paramBIDstr = label2ParamID(HeightSlider);

            juce::ParameterID paramAID = juce::ParameterID(paramAIDstr, version_hint);
            juce::ParameterID paramBID = juce::ParameterID(paramBIDstr, version_hint);

            bool alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (label2ParamID(param_id) == paramAIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterFloat> (paramAID, paramAIDstr, 0, 1, 0));
                param_ids_so_far.push_back(paramAIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramAIDstr << ". Check if intentional!" << endl;
            }

            alreadyExists = false;
            for (const auto& param_id : param_ids_so_far) {
                if (label2ParamID(param_id) == paramBIDstr) {
                    alreadyExists = true;
                    break;
                }
            }

            if (!alreadyExists) {
                layout.add (std::make_unique<juce::AudioParameterFloat> (paramBID, paramBIDstr, 0, 1, 0));
                param_ids_so_far.push_back(paramBIDstr);
            } else {
                cout << "[settings.json] WARNING: Duplicate paramID: " << paramBIDstr << ". Check if intentional!" << endl;
            }
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

    // add preset parameter
    layout.add(
        std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID(label2ParamID("Preset"), version_hint),
            "Preset", 1, 100, 0));

    return layout;
}

inline std::string generateTempFileName() {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;

    oss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");
    auto time_str = oss.str();
    auto temp_file_name = time_str + ".preset";

    return temp_file_name;
}

void NeuralMidiFXPluginProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    // Generate and save the file path
    auto filePath = generateTempFileName();
    xml->setAttribute("filePath", filePath); // Add the file path as an attribute
    copyXmlToBinary(*xml, destData);
}

void NeuralMidiFXPluginProcessor::setStateInformation(const void *data, int sizeInBytes) {

    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

void NeuralMidiFXPluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
    // cout << UIObjects::StandaloneTransportPanel::enable << endl;
    tempBuffer.clear();

    // get Playhead info && buffer size && sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();

    // check if standalone mode
    bool shouldActStandalone = false;
    if (JUCEApplicationBase::isStandaloneApp()) {
        if (UIObjects::StandaloneTransportPanel::enable) {
            shouldActStandalone = true;
        }
    } else {
        if (UIObjects::StandaloneTransportPanel::enable &&
            !UIObjects::StandaloneTransportPanel::disableInPluginMode) {
            shouldActStandalone = true;
        }
    }

    if (shouldActStandalone) {
        standAloneParams->update();
        Pinfo->setTimeInSamples(standAloneParams->TimeInSamples);
        Pinfo->setTimeInSeconds(standAloneParams->TimeInSeconds);
        Pinfo->setPpqPosition(standAloneParams->PpqPosition);
        Pinfo->setBpm(standAloneParams->qpm);
        Pinfo->setIsPlaying(standAloneParams->is_playing);
        Pinfo->setIsLooping(false);
        Pinfo->setIsRecording(standAloneParams->is_recording);
        AudioPlayHead::TimeSignature timeSig;
        timeSig.numerator = standAloneParams->numerator;
        timeSig.denominator = standAloneParams->denominator;
        Pinfo->setTimeSignature(timeSig);

        // prepare playhead for next frame
        standAloneParams->PreparePlayheadForNextFrame(buffSize, fs);

    }

    std::optional<GenerationEvent> event;
    realtimePlaybackInfo->setValues(
        BufferMetaData(
            Pinfo,
            fs,
            buffSize));

    generationsToDisplay.setFs(fs);
    generationsToDisplay.setQpm(   *Pinfo->getBpm());
    generationsToDisplay.setPlayheadPos(*Pinfo->getPpqPosition());

    // check if any events are received from the DPL thread
    if (DPL2NMP_GenerationEvent_Que->getNumReady() > 0)
    {
        event = DPL2NMP_GenerationEvent_Que->pop();
        generationsToDisplay.setPolicy( playbackPolicies);
        // update midi message sequence if new one arrived
        if (event->IsNewPlaybackSequence())
        {
             if (print_generation_stream_reception)
             {
                PrintMessage(" New Sequence of Generations Received");
             }
             double time_adjustment = 0.0;
             if (playbackPolicies.IsPlaybackPolicy_RelativeToAbsoluteZero()) {
                time_adjustment = 0.0;
             }
             else if (playbackPolicies.IsPlaybackPolicy_RelativeToNow()) {
                time_adjustment = time_anchor_for_playback.getTimeWithUnitType(
                    playbackPolicies.getTimeUnitIndex());
             } else if (playbackPolicies.IsPlaybackPolicy_RelativeToPlaybackStart()) {
                time_adjustment = playhead_start_time.getTimeWithUnitType(
                    playbackPolicies.getTimeUnitIndex());
             }
             // update according to policy (clearing already taken care of above)
             playbackMessageSequence.addSequence(
                 event->getNewPlaybackSequence().getAsJuceMidMessageSequence(),
                 time_adjustment);
             playbackMessageSequence.updateMatchedPairs();
             generationsToDisplay.setSequence(playbackMessageSequence);
        }
    } else {
        event = std::nullopt;
    }

    if (Pinfo.hasValue() && Pinfo->getPpqPosition().hasValue()) {

        // register current time for later use
        auto frame_now = time_{*Pinfo->getTimeInSamples(),
                                *Pinfo->getTimeInSeconds(),
                                *Pinfo->getPpqPosition()};

        // Send received events from host to DPL thread
        sendReceivedInputsAsEvents(midiMessages, Pinfo, fs, buffSize);

        // retry sending time anchor to GUI if mutex was locked last time
        if (shouldSendTimeAnchorToGUI) {
             if (playbackAnchorMutex.try_lock())
             {
                TimeAnchor = time_anchor_for_playback;
                playbackAnchorMutex.unlock();
                shouldSendTimeAnchorToGUI = false;
             }
        }

        // see if any generations are ready
        if (event.has_value()) {
             if (event->IsNewPlaybackPolicyEvent()) {
                // set anchor time relative to which timing information
                // of generations should be interpreted
                playbackPolicies = event->getNewPlaybackPolicyEvent();
                generationsToDisplay.policy = playbackPolicies;

                if (playbackPolicies.shouldForceSendNoteOffs()) {
                    for (int i = 0; i < 128; i++) {
                        tempBuffer.addEvent(
                            juce::MidiMessage::noteOff(1, i),
                            0);
                    }
                }
                if (playbackPolicies.IsPlaybackPolicy_RelativeToNow()) {
                    time_anchor_for_playback = frame_now;
                } else if (playbackPolicies.IsPlaybackPolicy_RelativeToAbsoluteZero()) {
                    time_anchor_for_playback = time_{0, 0.0f, 0.0f};
                } else if (playbackPolicies.IsPlaybackPolicy_RelativeToPlaybackStart()) {
                    time_anchor_for_playback = playhead_start_time;
                }

                // update for editor use for looping mode visualization
                shouldSendTimeAnchorToGUI = true;


                if (print_generation_policy_reception) {
                    PrintMessage(" New Generation Policy Received" ); }

                // check overwrite policy. if
                if (playbackPolicies.IsOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream()) {
                    playbackMessageSequence.clear();
                } else if (playbackPolicies.IsOverwritePolicy_DeleteAllEventsAfterNow()) {
                    // print all notes in sequence before deletion
                    std::stringstream ss;
                    for (int i = 0; i < playbackMessageSequence.getNumEvents(); i++) {
                        auto msg = playbackMessageSequence.getEventPointer(
                            i);
                        ss << ", " << std::to_string(i) << " -> " <<
                            msg->message.getTimeStamp() << " " <<
                            msg->message.getDescription();
                    }
                    PrintMessage(
                        "Num messages in sequence: before Delete " +
                        std::to_string(playbackMessageSequence.getNumEvents()) );
                    //                    PrintMessage(ss2.str());
                    // temp sequence to hold messages to be kept

                    juce::MidiMessageSequence tempSequence;

                    auto delete_start_time = frame_now.getTimeWithUnitType(
                        playbackPolicies.getTimeUnitIndex());
                    PrintMessage("Delete Start Time: " + std::to_string(
                                     delete_start_time));
                    for (int i = 0; i < playbackMessageSequence.getNumEvents(); i++) {
                        PrintMessage("Checking To Delete: " + std::to_string(
                                         playbackMessageSequence.getEventPointer(i)->message.getTimeStamp()));
                        auto msg = playbackMessageSequence.getEventPointer(
                            i);
                        if (msg->message.getTimeStamp() < delete_start_time) {
                            // print contents in sequence before deletion
                            tempSequence.addEvent(msg->message, 0);
                        }
                        // check if any of the note ons in the tempSequence
                        // don't have a corresponding note off
                        // if so, add a note off at now
                        for (int j = 0; j < tempSequence.getNumEvents(); j++) {
                            auto msg2 = tempSequence.getEventPointer(j);
                            if (msg2->message.isNoteOn()) {
                                bool hasCorrespondingNoteOff = false;
                                for (int k = 0; k < playbackMessageSequence.getNumEvents(); k++) {
                                    auto msg3 = playbackMessageSequence.getEventPointer(k);
                                    if (msg3->message.isNoteOff() && msg3->message.getNoteNumber() == msg2->message.getNoteNumber()) {
                                        hasCorrespondingNoteOff = true;
                                        break;
                                    }
                                }
                                if (!hasCorrespondingNoteOff) {
                                    tempSequence.addEvent(juce::MidiMessage::noteOff(1, msg2->message.getNoteNumber()),
                                                          frame_now.getTimeWithUnitType(playbackPolicies.getTimeUnitIndex()) - msg2->message.getTimeStamp());
                                }
                            }
                        }
                        // tempSequence.updateMatchedPairs();
                    }

                    // swap temp sequence with playback sequence
                    playbackMessageSequence.clear();
                    playbackMessageSequence.swapWith(tempSequence);


                    // print all notes in sequence after deletion
                    std::stringstream ss2;
                    for (int i = 0; i < playbackMessageSequence.getNumEvents(); i++) {
                        auto msg = playbackMessageSequence.getEventPointer(i);
                        ss2 << ", " << std::to_string(i) << " -> " << msg->message.getTimeStamp() << " " << msg->message.getDescription();
                    }
                    PrintMessage("Num messages in sequence: after Delete " + std::to_string(playbackMessageSequence.getNumEvents()) );
                    // PrintMessage(ss2.str());


                } else if (playbackPolicies.IsOverwritePolicy_KeepAllPreviousEvents()) {
                    /* do nothing */
                } else {
                    assert (false && "PlaybackPolicies Overwrite Action Not Specified!");
                }
             }
        }

        // start playback if any
        if (Pinfo->getIsPlaying()){
             for (auto &msg: playbackMessageSequence) {
                // PrintMessage(std::to_string(msg->message.getTimeStamp()));
                // PrintMessage(std::to_string(playbackPolicies.getTimeUnitIndex()));
                auto msg_to_play = getMessageIfToBePlayed(
                    frame_now, msg->message,
                    buffSize, fs, *Pinfo->getBpm());
                if (msg_to_play.has_value()) {
                    std::stringstream ss;
                    ss << "Playing: " << msg->message.getDescription() << " at time: " << msg->message.getTimeStamp();
                    PrintMessage(ss.str());
                    tempBuffer.addEvent(*msg_to_play, 0);
                }
             }
        }

        if (mVirtualMidiOutput)
        {

             mVirtualMidiOutput->sendBlockOfMessagesNow(tempBuffer);

        }

        midiMessages.swapWith(tempBuffer);

    }

    buffer.clear(); // clear buffer
}

