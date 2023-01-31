#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/GenerationEvent.h"
#include "Includes/InputEvent.h"

using namespace std;

NeuralMidiFXPluginProcessor::NeuralMidiFXPluginProcessor() : apvts(
        *this, nullptr, "PARAMETERS", createParameterLayout()) {
    //       Make_unique pointers for Queues
    // --------------------------------------------------------------------------------------
    NMP2ITP_Event_Que = make_unique<LockFreeQueue<Event, queue_settings::NMP2ITP_que_size>>();
    ITP2MDL_ModelInput_Que = make_unique<LockFreeQueue<ModelInput, queue_settings::ITP2MDL_que_size>>();
    MDL2PPP_ModelOutput_Que = make_unique<LockFreeQueue<ModelOutput, queue_settings::MDL2PPP_que_size>>();
    PPP2NMP_GenerationEvent_Que = make_unique<LockFreeQueue<GenerationEvent, queue_settings::PPP2NMP_que_size>>();

    //       Create shared pointers for Threads (shared with APVTSMediator)
    // --------------------------------------------------------------------------------------
    inputTensorPreparatorThread = make_shared<InputTensorPreparatorThread>();
    modelThread = make_shared<ModelThread>();
    playbackPreparatorThread = make_shared<PlaybackPreparatorThread>();

    //       give access to resources and run threads
    // --------------------------------------------------------------------------------------
    inputTensorPreparatorThread->startThreadUsingProvidedResources(NMP2ITP_Event_Que.get(),
                                                                   ITP2MDL_ModelInput_Que.get());
//    modelThread->startThreadUsingProvidedResources(ITP2MDL_ModelInput_Que.get(),
//                                                   MDL2PPP_ModelOutput_Que.get());
//    playbackPreparatorThread->startThreadUsingProvidedResources(MDL2PPP_ModelOutput_Que.get(),
//                                                                PPP2NMP_GenerationEvent_Que.get());

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


void NeuralMidiFXPluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
    tempBuffer.clear();

    // STEP 1
    // get Playhead info and buffer size and sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();

    sendReceivedInputsAsEvents(midiMessages, Pinfo, fs, buffSize);

    // step 2 see if any generations are ready
    if (PPP2NMP_GenerationEvent_Que->getNumReady() > 0) {
        auto event = PPP2NMP_GenerationEvent_Que->pop();
        if (event.IsNewPlaybackPolicyEvent()) {
            DBG("**NMP** GenerationEvent received" << event.getNewPlaybackPolicyEvent().getPlaybackPolicyType());
        }
    }
//    if (PPP2NMP_GenerationEvent_Que->getNumReady() > 0) {
//        DBG("PPP2NMP_GenerationEvent_Que->getNumReady() = " << PPP2NMP_GenerationEvent_Que->getNumReady());
//        auto generationEvent = PPP2NMP_GenerationEvent_Que->pop();
//        DBG(*generationEvent.playbackPolicies->PlaybackPolicy);
//    }

    /*while (PPP2NMP_GenerationEvent_Que->getNumReady() > 0) {
        auto generation_event = PPP2NMP_GenerationEvent_Que->pop();
        DBG("**NMP** Received Generation Event in NMP");
        if (generation_event.playbackPolicies.has_value()) {
            DBG("**NMP** Playback Policies are present");
            playbackPolicies = *generation_event.playbackPolicies;
            phead_at_start_of_new_stream = last_frame_meta_data.bufferMetaData;
            // check what time unit is used for timing of messages
            double time;
            if (playbackPolicies->IsTimeUnitIsAudioSamples()) {
                time = double(phead_at_start_of_new_stream.time_in_samples);
                DBG("**NMP** time in samples: " << time);
            } else if (playbackPolicies->IsTimeUnitIsPPQ()) {
                time = phead_at_start_of_new_stream.time_in_ppq;
                DBG("**NMP** time in ppq: " << time);
            } else {
                time = phead_at_start_of_new_stream.time_in_seconds;
                DBG("**NMP** time in seconds: " << time);
            }
            // check what to do with the previous stream
//            if (playbackPolicies->IsOverwritePolicy_DeleteAllEventsAfterNow()) {
//                for (int i = 0; i < playbackSequence.getNumEvents(); i++) {
//                    auto message = playbackSequence.getEventPointer(i)->message;
//                    if (message.getTimeStamp() > time) {
//                        playbackSequence.deleteEvent(i, true);
//                    }
//                }
//                playbackSequence.updateMatchedPairs();
//
//            } else if (playbackPolicies->IsOverwritePolicy_DeleteAllEventsInPreviousStreamAndUseNewStream()) {
//                playbackSequence.clear();
//            }
        }
        if (generation_event.messageSequence.has_value()*//* and playbackPolicies.has_value()*//*) {
            // auto messageSequence = *generation_event.messageSequence;
            DBG("**NMP** Message Sequence is present");
//            for (auto &message : messageSequence) {
//                DBG(message->message.getDescription());
////                if (playbackPolicies->IsPlaybackPolicy_RelativeToAbsoluteZero()) {
////                    playbackSequence.addEvent(message);
////                } else if (playbackPolicies->IsPlaybackPolicy_RelativeToNow()) {
////                    playbackSequence.
////                }
//            }
        }
    }*/

    midiMessages.swapWith(tempBuffer);

    buffer.clear(); //
}

void NeuralMidiFXPluginProcessor::sendReceivedInputsAsEvents(
        MidiBuffer &midiMessages, const Optional<AudioPlayHead::PositionInfo> &Pinfo,
        double fs, int buffSize) {

    using namespace event_communication_settings;
    if (Pinfo) {

        if (last_frame_meta_data.bufferMetaData.isPlaying xor Pinfo->getIsPlaying()) {
            // if just started, register the playhead starting position
            if ((not last_frame_meta_data.bufferMetaData.isPlaying) and Pinfo->getIsPlaying()) {
                DBG("**NMP** Started playing");
                auto frame_meta_data = Event{Pinfo, fs, buffSize, true};
                NMP2ITP_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;
            } else {
                // if just stopped, register the playhead stopping position
                auto frame_meta_data = Event{Pinfo, fs, buffSize, false};
                frame_meta_data.setPlaybackStoppedEvent();
                NMP2ITP_Event_Que->push(frame_meta_data);
                last_frame_meta_data = frame_meta_data;     // reset last frame meta data
            }
        } else {
            // if still playing, register the playhead position
            if (Pinfo->getIsPlaying()) {
                auto frame_meta_data = Event{Pinfo, fs, buffSize, false};
                frame_meta_data.bufferMetaData.set_time_shift_compared_to_last_frame(
                        last_frame_meta_data.bufferMetaData);
                if (SendEventAtBeginningOfNewBuffers_FLAG) {
                    if (SendEventForNewBufferIfMetadataChanged_FLAG) {
                        if (frame_meta_data.bufferMetaData != last_frame_meta_data.bufferMetaData) {
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
            last_frame_meta_data.bufferMetaData.isPlaying = false;     // reset last frame meta data
        }

        // Step 4. see if new notes are played on the input side
        if (not midiMessages.isEmpty() and Pinfo->getIsPlaying()) {
            // if there are new notes, send them to the groove thread
            for (const auto midiMessage: midiMessages) {
                auto msg = midiMessage.getMessage();
                auto midiEvent = Event{Pinfo, fs, buffSize, msg};

                // check if new bar event exists and it is before the current midi event
                if (NewBarEvent.has_value() and SendNewBarEvents_FLAG) {
                    if (midiEvent.time_in_samples >= NewBarEvent->time_in_samples) {
                        NMP2ITP_Event_Que->push(*NewBarEvent);
                        NewBarEvent = std::nullopt;
                    }
                }

                // check if a specified number of whole notes has passed
                if (NewTimeShiftEvent.has_value() and SendTimeShiftEvents_FLAG) {
                    if (midiEvent.time_in_samples >= NewTimeShiftEvent->time_in_samples) {
                        NMP2ITP_Event_Que->push(*NewTimeShiftEvent);
                        NewTimeShiftEvent = std::nullopt;
                    }
                }

                if (midiEvent.message.isNoteOn()) {
                    if (!FilterNoteOnEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
                }

                if (midiEvent.message.isNoteOff()) {
                    if (!FilterNoteOffEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
                }

                if (midiEvent.message.isController()) {
                    if (!FilterCCEvents_FLAG) { NMP2ITP_Event_Que->push(midiEvent); }
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

