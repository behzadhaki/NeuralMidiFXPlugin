#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/GenerationEvent.h"
#include "Includes/APVTSMediatorThread.h"

using namespace std;
using namespace debugging_settings::ProcessorThread;

NeuralMidiFXPluginProcessor::NeuralMidiFXPluginProcessor() : apvts(
        *this, nullptr, "PARAMETERS", createParameterLayout()) {
    //       Make_unique pointers for Queues
    // --------------------------------------------------------------------------------------
    NMP2ITP_Event_Que = make_unique<LockFreeQueue<Event, queue_settings::NMP2ITP_que_size>>();
    ITP2MDL_ModelInput_Que = make_unique<LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size>>();
    MDL2PPP_ModelOutput_Que = make_unique<LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size>>();
    PPP2NMP_GenerationEvent_Que = make_unique<LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size>>();

    //     Make_unique pointers for APVM Queues
    // --------------------------------------------------------------------------------------
    APVM2ITP_GuiParams_Que = make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();
    APVM2MDL_GuiParams_Que = make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();
    APVM2PPP_GuiParams_Que = make_unique<LockFreeQueue<GuiParams, queue_settings::APVM_que_size>>();

    //       Create shared pointers for Threads (shared with APVTSMediator)
    // --------------------------------------------------------------------------------------
    inputTensorPreparatorThread = make_shared<InputTensorPreparatorThread>();
    modelThread = make_shared<ModelThread>();
    playbackPreparatorThread = make_shared<PlaybackPreparatorThread>();
    apvtsMediatorThread = make_unique<APVTSMediatorThread>();

    //       give access to resources and run threads
    // --------------------------------------------------------------------------------------
    inputTensorPreparatorThread->startThreadUsingProvidedResources(NMP2ITP_Event_Que.get(),
                                                                   ITP2MDL_ModelInput_Que.get(),
                                                                   APVM2ITP_GuiParams_Que.get());
    modelThread->startThreadUsingProvidedResources(ITP2MDL_ModelInput_Que.get(),
                                                   MDL2PPP_ModelOutput_Que.get(),
                                                   APVM2MDL_GuiParams_Que.get());
    playbackPreparatorThread->startThreadUsingProvidedResources(MDL2PPP_ModelOutput_Que.get(),
                                                                PPP2NMP_GenerationEvent_Que.get(),
                                                                APVM2PPP_GuiParams_Que.get());
    apvtsMediatorThread->startThreadUsingProvidedResources(&apvts,
                                                           APVM2ITP_GuiParams_Que.get(),
                                                           APVM2MDL_GuiParams_Que.get(),
                                                           APVM2PPP_GuiParams_Que.get());
}

NeuralMidiFXPluginProcessor::~NeuralMidiFXPluginProcessor() {
    if (!modelThread->readyToStop) {
        modelThread->prepareToStop();
    }
    if (!inputTensorPreparatorThread->readyToStop) {
        inputTensorPreparatorThread->prepareToStop();
    }
    if (!playbackPreparatorThread->readyToStop) {
        playbackPreparatorThread->prepareToStop();
    }
}

void NeuralMidiFXPluginProcessor::PrintMessage(const std::string& input) {
    using namespace debugging_settings::ModelThread;
    if (disable_user_print_requests) { return; }

    // if input is multiline, split it into lines and print each line separately
    std::stringstream ss(input);
    std::string line;

    // get string from now
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::string now_str = std::ctime(&now_c);
    // remove newline
    now_str.erase(std::remove(now_str.begin(), now_str.end(), '\n'), now_str.end());

    while (std::getline(ss, line)) {
        std::cout << clr::cyan << "[NMP] " << now_str << "|" << line << clr::reset << std::endl;
    }
}

void NeuralMidiFXPluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
    tempBuffer.clear();



    // get Playhead info and buffer size and sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();

    // register current time for later use
    auto frame_now = time_{*Pinfo->getTimeInSamples(),
                     *Pinfo->getTimeInSeconds(),
                     *Pinfo->getPpqPosition()};

    // Send received events from host to ITP thread
    sendReceivedInputsAsEvents(midiMessages, Pinfo, fs, buffSize);

    // see if any generations are ready
    if (PPP2NMP_GenerationEvent_Que->getNumReady() > 0) {
        auto event = PPP2NMP_GenerationEvent_Que->pop();
        if (event.IsNewPlaybackPolicyEvent() and print_generation_policy_reception) {

            // set anchor time relative to which timing information of generations should be interpreted
            playbackPolicies = event.getNewPlaybackPolicyEvent();
            if (playbackPolicies.IsPlaybackPolicy_RelativeToNow()) {
                time_anchor_for_playback = frame_now;
            } else if (playbackPolicies.IsPlaybackPolicy_RelativeToAbsoluteZero()) {
                time_anchor_for_playback = time_{0, 0.0f, 0.0f};
            } else {
                time_anchor_for_playback = playhead_start_time;
            }
            PrintMessage(" New Generation Policy Received" );

            // check overwrite policy
            if (playbackPolicies.IsOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream()) {
                playbackMessageSequence.clear();
            } else if (playbackPolicies.IsOverwritePolicy_DeleteAllEventsAfterNow()) {
                auto delete_start_time = frame_now.getTimeWithUnitType(playbackPolicies.getTimeUnitIndex());
                for (size_t i=0; i++; i<playbackMessageSequence.getNumEvents()) {
                    auto msg = playbackMessageSequence.getEventPointer(i);
                    if (msg->message.getTimeStamp() >= delete_start_time) {
                        playbackMessageSequence.deleteEvent(i, false);
                    }
                }

            } else if (playbackPolicies.IsOverwritePolicy_KeepAllPreviousEvents()) { /* do nothing */ } else {
                assert ("PlaybackPolicies Overwrite Action not Specified!");
            }
        }

        // update midi message sequence if new one arrived
        if (event.IsNewPlaybackSequence()) {
            if (print_generation_stream_reception) { PrintMessage(" New Sequence of Generations Received"); }
            // update according to policy (clearing already taken care of above)
            playbackMessageSequence.addSequence(event.getNewPlaybackSequence().getAsJuceMidMessageSequence(), 0);
            playbackMessageSequence.updateMatchedPairs();
        }
    }

    // start playback if any
    if (Pinfo->getIsPlaying()){
        for (auto &msg: playbackMessageSequence) {
            // PrintMessage(std::to_string(msg->message.getTimeStamp()));
            // PrintMessage(std::to_string(playbackPolicies.getTimeUnitIndex()));
            auto msg_to_play = getMessageIfToBePlayed(
                    frame_now, msg->message, buffSize, fs, *Pinfo->getBpm());
            if (msg_to_play.has_value()) {
                std::stringstream ss;
                ss << "Playing: " << msg_to_play->getDescription() << " at time: " << msg_to_play->getTimeStamp();
                PrintMessage(ss.str());
                tempBuffer.addEvent(*msg_to_play, 0);
            }
        }
    }
    auto now_in_user_unit = frame_now.getTimeWithUnitType(playbackPolicies.getTimeUnitIndex());

    midiMessages.swapWith(tempBuffer);

    buffer.clear(); //
}

// checks whether the note timing (adjusted by the time anchor) is within the current buffer
// if so, returns the number of samples from the start of the buffer to the note
// if not, returns -1
std::optional<juce::MidiMessage> NeuralMidiFXPluginProcessor::getMessageIfToBePlayed(
        time_ now_, const juce::MidiMessage &msg, int buffSize, double fs,
        double qpm) {

    auto now_in_user_unit = now_.getTimeWithUnitType(playbackPolicies.getTimeUnitIndex());
    auto time_anchor_for_playback_in_user_unit = time_anchor_for_playback.getTimeWithUnitType(
            playbackPolicies.getTimeUnitIndex());
    auto adjusted_time = msg.getTimeStamp() + time_anchor_for_playback_in_user_unit;

    switch (playbackPolicies.getTimeUnitIndex()) {
        case 1: {       // samples
            auto end = now_.inSamples() + buffSize;
            if (now_in_user_unit <= adjusted_time and adjusted_time < end) {
                auto msg_copy = msg;
                msg_copy.setTimeStamp(adjusted_time - now_in_user_unit);
                return msg_copy;
            } else {
                return {};
            }
        }
        case 2: {      // seconds
            auto end = now_.inSeconds() + buffSize / fs;

            if (now_in_user_unit <= adjusted_time and adjusted_time < end) {
                auto msg_copy = msg;
                auto time_diff = adjusted_time - now_in_user_unit;
                msg_copy.setTimeStamp(std::floor(time_diff * fs));
                return msg_copy;
            } else {
                return {};
            }
        }
        case 3: {      // QuarterNotes
            auto end = now_.inQuarterNotes() + buffSize / fs * qpm / 60.0f;
            if (now_in_user_unit <= adjusted_time and adjusted_time < end) {
                auto msg_copy = msg;
                auto time_diff = adjusted_time - now_in_user_unit;
                msg_copy.setTimeStamp(std::floor(time_diff * fs * 60.0f / qpm));
                return msg_copy;
            } else {
                return {};
            }
        }

    }
}

void NeuralMidiFXPluginProcessor::sendReceivedInputsAsEvents(
        MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo,
        double fs, int buffSize) {

    using namespace event_communication_settings;
    if (Pinfo) {

        if (last_frame_meta_data.isPlaying() xor Pinfo->getIsPlaying()) {
            // if just started, register the playhead starting position
            if ((not last_frame_meta_data.isPlaying()) and Pinfo->getIsPlaying()) {
                if (print_start_stop_times) { PrintMessage("Started playing"); }
                playhead_start_time = time_{*Pinfo->getTimeInSamples(),
                                            *Pinfo->getTimeInSeconds(),
                                            *Pinfo->getPpqPosition()};

                time_anchor_for_playback = time_{*Pinfo->getTimeInSamples(),
                                                 *Pinfo->getTimeInSeconds(),
                                                 *Pinfo->getPpqPosition()};

                auto frame_meta_data = Event{Pinfo, fs, buffSize, true};
                NMP2ITP_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;
            } else {
                // if just stopped, register the playhead stopping position
                auto frame_meta_data = Event{Pinfo, fs, buffSize, false};
                if (print_start_stop_times) { PrintMessage("Stopped playing"); }
                frame_meta_data.setPlaybackStoppedEvent();
                NMP2ITP_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;     // reset last frame meta data
            }
        } else {
            // if still playing, register the playhead position
            if (Pinfo->getIsPlaying()) {
                if (print_new_buffer_started) { PrintMessage("New Buffer Arrived"); }
                auto frame_meta_data = Event{Pinfo, fs, buffSize, false};
                if (SendEventAtBeginningOfNewBuffers_FLAG) {
                    if (SendEventForNewBufferIfMetadataChanged_FLAG) {
                        if (frame_meta_data.getBufferMetaData() != last_frame_meta_data.getBufferMetaData()) {
                            NMP2ITP_Event_Que->push(frame_meta_data);
                        }
                    } else {
                        NMP2ITP_Event_Que->push(frame_meta_data);
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
        if (not midiMessages.isEmpty() and Pinfo->getIsPlaying()) {
            // if there are new notes, send them to the groove thread
            for (const auto midiMessage: midiMessages) {
                auto msg = midiMessage.getMessage();
                auto midiEvent = Event{Pinfo, fs, buffSize, msg};

                // check if new bar event exists and it is before the current midi event
                if (NewBarEvent.has_value() and SendNewBarEvents_FLAG) {
                    if (midiEvent.Time().inSamples() >= NewBarEvent->Time().inSamples()) {
                        NMP2ITP_Event_Que->push(*NewBarEvent);
                        NewBarEvent = std::nullopt;
                    }
                }

                // check if a specified number of whole notes has passed
                if (NewTimeShiftEvent.has_value() and SendTimeShiftEvents_FLAG) {
                    if (midiEvent.Time().inSamples() >= NewTimeShiftEvent->Time().inSamples()) {
                        NMP2ITP_Event_Que->push(*NewTimeShiftEvent);
                        NewTimeShiftEvent = std::nullopt;
                    }
                }

                if (midiEvent.isMidiMessageEvent()) {
                    if (midiEvent.isNoteOnEvent()) {
                        if (!FilterNoteOnEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
                    }

                    if (midiEvent.isNoteOffEvent()) {
                        if (!FilterNoteOffEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
                    }

                    if (midiEvent.isCCEvent()) {
                        if (!FilterCCEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
                    }
                }
            }
        }

        // if there is a new bar event, and hasn't been sent yet, send it
        if (NewBarEvent.has_value() and SendNewBarEvents_FLAG) {
            NMP2ITP_Event_Que->push(*NewBarEvent);
            NewBarEvent = std::nullopt;
        }
        if (NewTimeShiftEvent.has_value() and SendTimeShiftEvents_FLAG) {
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

float NeuralMidiFXPluginProcessor::get_playhead_pos() const {
    return playhead_pos;
}

juce::AudioProcessorValueTreeState::ParameterLayout NeuralMidiFXPluginProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    int version_hint = 1;

    const char* name;
    bool isToggleable;
    double minValue, maxValue, initValue;

    size_t numTabs = UIObjects::Tabs::tabList.size();
    size_t numSliders;
    size_t numRotaries;
    size_t numButtons;

    UIObjects::tab_tuple tabTuple;

    UIObjects::slider_list sliderList;
    UIObjects::rotary_list rotaryList;
    UIObjects::button_list buttonList;

    UIObjects::slider_tuple sliderTuple;
    UIObjects::rotary_tuple rotaryTuple;
    UIObjects::button_tuple buttonTuple;


    for (size_t j = 0; j < numTabs; j++) {
        tabTuple = UIObjects::Tabs::tabList[j];

        sliderList = std::get<1>(tabTuple);
        rotaryList = std::get<2>(tabTuple);
        buttonList = std::get<3>(tabTuple);

        numSliders = sliderList.size();
        numRotaries = rotaryList.size();
        numButtons = buttonList.size();

        // Sliders
        for (size_t i = 0; i < numSliders; ++i) {
            sliderTuple = sliderList[i];
            std::tie(name, minValue, maxValue, initValue) = sliderTuple;

            // Param ID will read "Slider" + [tab, item] i.e. 'Slider_13"
            juce::String paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            layout.add(std::make_unique<juce::AudioParameterFloat>(paramID, name, minValue, maxValue, initValue));
        }

        // Rotaries
        for (size_t i = 0; i < numRotaries; ++i) {
            rotaryTuple = rotaryList[i];
            std::tie(name, minValue,maxValue, initValue) = rotaryTuple;

            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);

            layout.add (std::make_unique<juce::AudioParameterFloat> (paramID, name, minValue, maxValue, initValue));
        }

        // Buttons
        for (size_t i = 0; i < numButtons; ++i) {
            std::tie(name, isToggleable) = buttonList[i];
            auto paramIDstr = label2ParamID(name);
            juce::ParameterID paramID = juce::ParameterID(paramIDstr, version_hint);
            layout.add (std::make_unique<juce::AudioParameterInt> (paramID, name, 0, 1, 0));
        }
    }

    return layout;
}

void NeuralMidiFXPluginProcessor::getStateInformation(juce::MemoryBlock &destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NeuralMidiFXPluginProcessor::setStateInformation(const void *data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

