//
// Created by u153171 on 11/23/2023.
//

#include "PluginProcessor.h"

using namespace std;
using namespace debugging_settings::ProcessorThread;

void NeuralMidiFXPluginProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
    tempBuffer.clear();

    // get Playhead info && buffer size && sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();


    // check if standalone mode
    if (UIObjects::StandaloneTransportPanel::enable) {
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
    generationsToDisplay.playhead_pos = *Pinfo->getPpqPosition();

    // check if any events are received from the DPL thread
    if (DPL2NMP_GenerationEvent_Que->getNumReady() > 0)
    {
        event = DPL2NMP_GenerationEvent_Que->pop();
        generationsToDisplay.policy = playbackPolicies;
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
