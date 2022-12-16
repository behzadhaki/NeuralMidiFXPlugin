//
// Created by Behzad Haki on 2022-02-11.
//

#include "../settings.h"
#include <torch/torch.h>
#include "CustomStructsAndLockFreeQueue.h"

// can be used in processor to pass the messages received in a MidiBuffer as is,
// sequentially in a queue to be shared with other threads

using namespace std;


/**
 * converts a note_on message received in the processor into a BasicNote instance
 * then sends it to receiver thread using the provided note_que
 * @param midiMessages (juce::MidiBuffer&)
 * @param playheadP (juce::AudioPlayHead*)
 * @param note_que  (LockFreeQueue<Note, que_size>*)
 *
 */
template<int que_size>
inline void place_BasicNote_in_queue(
    juce::MidiBuffer& midiMessages,
    juce::AudioPlayHead* playheadP,
    LockFreeQueue<BasicNote, que_size>* note_que,
    double sample_rate)
{
    double frameStartPpq;
    double qpm;
    bool isPlaying;
    bool isLooping;
    float captureWithBpm;

    if (playheadP)
    {

        auto pinfo = playheadP->getPosition();
        if (pinfo)
        {

            // https://forum.juce.com/t/messagemanagerlock-and-thread-shutdown/353/4
            // read from midi_message_que
            frameStartPpq = *pinfo->getPpqPosition();
            qpm = *pinfo->getBpm();
            isPlaying = pinfo->getIsPlaying();
            isLooping = pinfo->getIsLooping();
            captureWithBpm = (float) *pinfo->getBpm();

            for (auto m: midiMessages)
            {
                auto message = m.getMessage();
                if (message.isNoteOn())
                {
                    BasicNote note(message.getNoteNumber(),
                                   message.getFloatVelocity(),
                                   frameStartPpq,
                                   message.getTimeStamp(),
                                   qpm,
                                   sample_rate);

                    note.capturedInPlaying = isPlaying;
                    note.capturedInLoop = isLooping;
                    note.captureWithBpm = captureWithBpm;
                    note_que->WriteTo(&note, 1);
                }
            }

        }
    }
}


/**
 * converts a note_on message received in the processor into a BasicNote instance
 * then sends it to receiver thread using the provided note_que
 * @param midiMessages (juce::MidiBuffer&)
 * @param pinfo using playheadP->getPosition() (juce::Optional<juce::AudioPlayHead::PositionInfo>)
 * @param note_que  (LockFreeQueue<Note, que_size>*)
 *
 */
inline void place_BasicNote_in_queue(
    juce::MidiBuffer& midiMessages,
    juce::Optional<juce::AudioPlayHead::PositionInfo > pinfo,
    LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>* ProcessBlockToGrooveThreadQue,
    double sample_rate)
{
    double frameStartPpq;
    double qpm;
    bool isPlaying;
    bool isLooping;
    float captureWithBpm;


    if (pinfo)
    {

        // https://forum.juce.com/t/messagemanagerlock-and-thread-shutdown/353/4
        // read from midi_message_que
        frameStartPpq = *pinfo->getPpqPosition();
        qpm = *pinfo->getBpm();
        isPlaying = pinfo->getIsPlaying();
        isLooping = pinfo->getIsLooping();
        captureWithBpm = (float) *pinfo->getBpm();

        for (auto m: midiMessages)
        {
            auto message = m.getMessage();
            if (message.isNoteOn())
            {
                BasicNote note(message.getNoteNumber(),
                               message.getFloatVelocity(),
                               frameStartPpq,
                               message.getTimeStamp(),
                               qpm,
                               sample_rate);

                note.capturedInPlaying = isPlaying;
                note.capturedInLoop = isLooping;
                note.captureWithBpm = captureWithBpm;
                ProcessBlockToGrooveThreadQue->WriteTo(&note, 1);
            }
        }

    }
}


inline string stream2string(std::ostringstream msg_stream)
{
    return msg_stream.str();
}

/**
 * Sends a string along with header to a logger thread using the specified queue
 *
 * @param text_message_queue (StringLockFreeQueue<GeneralSettings::text_message_queue_size>*):
 *                          queue for communicating with message receiver thread
 * @param message (string): main message to display
 * @param header  (string): message to be printed as a header before showing message
 * @param clearFirst (bool): if true, empties receiving gui thread before display
 */
inline void showMessageinEditor(StringLockFreeQueue<GeneralSettings::gui_io_queue_size>* text_message_queue,
                                string message, string header, bool clearFirst)
{
    if (text_message_queue!=nullptr)
    {
        if (clearFirst)
        {
            text_message_queue->addText((char*) "clear");
        }

        text_message_queue->addText("<<<<<  " + header + "  >>>>>>");

        /*char* c_message = const_cast<char*>(message.c_str());
        text_message_queue->addText(c_message);*/

        text_message_queue->addText(message);
    }

}


// converts a tensor to string to be used with DBG for debugging
inline std::string tensor2string (torch::Tensor tensor)
{
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    return tensor_string;
}



/**
 * Converts a torch tensor to a string
 * @param torch::tensor
 * @return std::string
 */
inline std::string torch2string (const torch::Tensor& tensor)
{
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    return tensor_string;
}


