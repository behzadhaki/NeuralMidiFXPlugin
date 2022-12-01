//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../settings.h"
#include <torch/torch.h>

#include <utility>


using namespace std;

// ============================================================================================================
// ==========          LockFreeQueue used for general data such as arrays, ints, floats and...   =============
// ==========          !!!! NOT TO BE USED WITH Vectors or                  !!!!
// ==========          !!!!  datatypes where size can change Dynamically    !!!!
// ============================================================================================================

/***
 * a Juce::abstractfifo implementation of a Lockfree FIFO
 *  to be used for T (template) datatypes
 * @tparam T    template datatype
 * @tparam queue_size
 */
template <typename T, int queue_size> class LockFreeQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<T> data;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    T latest_written_data;
    bool writingActive = false;

public:
    LockFreeQueue()
    {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

    }

    void WriteTo (const T* writeData, int numTowrite)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;

        lockFreeFifo ->prepareToWrite(
            numTowrite, start1, blockSize1,
            start2, blockSize2);

        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;

            for (int i=0; i < blockSize1; i++)
            {
                *(start_data_ptr + i) = *(writeData+i);
            }
        }

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            for (int i=0; i < blockSize2; i++)
            {
                *(start_data_ptr + i) = *(writeData+blockSize1+i);
            }
        }

        latest_written_data = writeData[numTowrite-1];
        num_writes = num_writes + numTowrite;
        lockFreeFifo->finishedWrite(numTowrite);
        writingActive = false;

    }

    void ReadFrom(T* readData, int numToRead)
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            numToRead, start1, blockSize1,
            start2, blockSize2);

        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            for (int i=0; i < blockSize1; i++)
            {
                *(readData + i) = *(start_data_ptr+i);
            }
        }

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            for (int i=0; i < blockSize2; i++)
            {
                *(readData+blockSize1+i) = *(start_data_ptr + i);
            }
        }

        lockFreeFifo -> finishedRead(blockSize1+blockSize2);
        num_reads += numToRead;
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    void push ( T writeData)
    {
        int start1, start2, blockSize1, blockSize2;
        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;

        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);

        writingActive = false;

    }

    T pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;
    }

    T getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;
        T readData;

        lockFreeFifo ->prepareToRead(
            getNumReady(), start1, blockSize1,
            start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            readData = *(start_data_ptr+blockSize2-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;

        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            readData = *(start_data_ptr+blockSize1-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;
        }

    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the ReadFrom() or pop() methods!!!
    T getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
    }
};


// ============================================================================================================
// ==========          BasicNote Structure                                                        =============
// ==========
// ============================================================================================================

/**the onset of midi message has two attributes: (1) the ppq of the beginning of the frame
     * (2) the number of AUDIO samples from the beginning of the frame
     * hence we need to use the sample rate and the qpm from the daw to calculate
     * absolute ppq of the onset. Look into NppqPosB structure
     *
     * \param ppq (double): time in ppq
     *
     * Used in BasicNote Structure (see for instantiation example)
     */
struct onset_time{

    // exact position of onset with respect to ppq
    double ppq;

    // default constructor for empty instantiation
    onset_time() = default;

    // constructor for desired timing of event in terms of absolute ppq
    onset_time(double ppq_): ppq(ppq_) {}

    // constructor for received timing of midi messages
    // Automatically calculates absolute timing of event in terms of ppq
    onset_time(double frameStartPpq, double audioSamplePos, double qpm, double sample_rate):ppq()
    {
        ppq = calculate_absolute_ppq(frameStartPpq, audioSamplePos, qpm, sample_rate); // calculate ppq
    }

    //constructor for calculating ppq using grid_line index and offset
    onset_time(int grid_line, double offset)
    {
        ppq = (offset / HVO_params::_max_offset * HVO_params::_32_note_ppq) + (grid_line * HVO_params::_16_note_ppq);
        if (ppq < 0)
        {
            ppq = ppq + HVO_params::_16_note_ppq * HVO_params::_n_16_notes;
        }
    }

    // used usually for finding absolute timing of messages received in processor MIDIBuffer
    double calculate_absolute_ppq(double frameStartPpq, double audioSamplePos, double qpm, double sample_rate)
    {
        auto tmp_ppq = frameStartPpq + audioSamplePos * qpm / (60 * sample_rate);
        return tmp_ppq;
    }

    // used usually for preparing for playback through the processor MIDI buffer
    juce::int64 calculate_num_samples_from_frame_ppq(double frameStartPpq, double qpm, double sample_rate){
        auto tmp_audioSamplePos = (ppq - frameStartPpq) * 60.0 * sample_rate / qpm;
        return juce::int64(tmp_audioSamplePos);
    }

};

/**
     * A note structure holding the note number for an onset along with
     * ppq position -->  defined as the ration of quarter note
     * onset_time (struct see above) --> structure holding absolute ppq with implemented
     * utilities for quick conversions for buffer to processsor OR processor to buffer  cases
     * \n\n
     *
     * can create object three ways:
     *          \n\t\t 1. default empty note using Note()
     *          \n\t\t 2. notes received in AudioProcessorBlock using second constructor
     *                  (see place_note_in_queue() in Includes/UtilityMethods.h)
     *          \n\t\t 3. creating notes obtained from hvo (i.e. using ppq, note number and vel)
     *                  (see HVO::getNotes() below)
     *
     * \param note (int): midi note number of the Note object
     * \param velocity (float): velocity of the note
     * \param time (onset_time): structure for onset time of the note (see onset_time)
     *
     */
struct BasicNote{

    int note =0 ;
    float velocity = 0;
    onset_time time = 0;

    bool capturedInLoop = false;        // useful for overdubbing calculations in
                                        // HVO::addNote(note_, shouldOverdub)
    bool capturedInPlaying = false;     // useful for checking if note was played while
                                        // playhead was playing
    float captureWithBpm = -100;

    // default constructor for empty instantiation
    BasicNote() = default;

    // constructor for placing notes received in the processor from the MIDIBuffer
    BasicNote(int note_number, float velocity_Value, double frameStartPpq, double audioSamplePos, double qpm, double sample_rate):
        note(note_number), velocity(velocity_Value), time(frameStartPpq, audioSamplePos, qpm, sample_rate){
    }

    // constructor for placing notes generated
    BasicNote(int voice_index, float velocity_Value, int grid_line, double offset,
         std::array<int, HVO_params::num_voices> voice_to_midi_map = nine_voice_kit_default_midi_numbers):
        note(voice_to_midi_map[(unsigned long)voice_index]),
        velocity(velocity_Value), time(grid_line, offset) {}

    // < operator to check which note happens earlier (used for sorting)
    bool operator<( const BasicNote& rhs ) const
    { return time.ppq < rhs.time.ppq; }

    bool isLouderThan ( const BasicNote& otherNote ) const
    {
        return velocity >= otherNote.velocity;
    }

    bool occursEarlierThan (const BasicNote& otherNote ) const
    {
        return time.ppq < otherNote.time.ppq;
    }

    // converts the data to a string message to be printed/displayed
    string getStringDescription() const
    {
        std::ostringstream msg_stream;
        msg_stream << "N: " << note
                   << ", vel " << velocity
                   << ", time " << time.ppq
                   << ", capturedInPlaying "<< capturedInPlaying
                   << ", capturedInLoop "<< capturedInLoop
                   << ", captured at bpm of "<< captureWithBpm
                   << endl;
        return msg_stream.str();
    }
};


// ============================================================================================================
// ==========          GeneratedData Structure and Corresponding Queue                            =============
// ==========           (Used for sending data from model thread to processBlock()
// ============================================================================================================

/***
 * A structre used for pre-generating all possible notes (as juce::MidiMessage objects) generated to be sent to processblock() for playback.
 * NOTE!!!! the initialization reserves space for maximum number of events to be generated
 * (i.e. time_steps_*num_voices_ events). So, only the first numberOfGenerations()
 * should be read from the midiMessages.
 * @tparam time_steps_
 * @tparam num_voices_
 */
template <int time_steps_, int num_voices_> struct GeneratedData{
    vector<float> ppqs;          // default value is
    int lastFilledIndex;   // specifies how many messages are valid from the beginning
    vector<juce::MidiMessage> midiMessages;

    GeneratedData()
    {

        lastFilledIndex = -1;             // no valid messages yet

        // initialize messages and ppqs
        for (int i = 0; i<(time_steps_*num_voices_); i++)
        {
            ppqs.push_back(-1);             // a dummy ppq value
            midiMessages.push_back(juce::MidiMessage::noteOn((int) 1, (int) 1, (float) 0));   // a dummy note
        }

    }

    void addNote(BasicNote note_)
    {
        ppqs[lastFilledIndex + 1] = note_.time.ppq;
        midiMessages[lastFilledIndex + 1].setNoteNumber(note_.note);
        midiMessages[lastFilledIndex + 1].setVelocity(note_.velocity);
        lastFilledIndex++;
    }

    int numberOfGenerations()
    {
        return (int) (lastFilledIndex+1);
    }
};

template <int time_steps_, int num_voices_, int queue_size> class GeneratedDataQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<GeneratedData<time_steps_, num_voices_>> data{};

    int time_steps, num_voices;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    GeneratedData<time_steps_, num_voices_> latest_written_data {};
    bool writingActive = false;

public:
    GeneratedDataQueue(){

        time_steps = time_steps_;
        num_voices = num_voices_;

        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_GenerateData= GeneratedData<time_steps_, num_voices_>();
            data.add(empty_GenerateData);
        }
    }

    void push (const GeneratedData<time_steps_, num_voices_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

    }

    GeneratedData<time_steps_, num_voices_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;
    }

    GeneratedData<time_steps_, num_voices_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
            getNumReady(), start1, blockSize1, start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr + blockSize2 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;

            return readData;
        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr + blockSize1 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;
            return readData;
        }

        return GeneratedData<time_steps_, num_voices_>();
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    GeneratedData<time_steps_, num_voices_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }

    bool isWritingInProgress()
    {
        return writingActive;
    }
};




// ============================================================================================================
// ==========                    HVO Structure and Corresponding Queue                            =============
// ==========
// ============================================================================================================

/*/// HVO structure for t time_steps and n num_voices
/// \n . Stores hits, velocities and offsets separately.
/// \n . can also create a random HVO quickly using HVO::Random()
/// \n . can automatically compress the velocity/offset values
/// \n . can automatically prepare and return notes extracted
///     from the HVO score see HVO::getNotes()
/// \tparam time_steps_
/// \tparam num_voices_*/
template <int time_steps_, int num_voices_> struct HVO
{
    int time_steps = time_steps_;
    int num_voices = num_voices_;

    // local variables to keep track of vel_range = (min_vel, max_vel) and offset ranges
    array<float, 2> vel_range = {HVO_params::_min_vel, HVO_params::_max_vel};
    array<float, 2> offset_range = {HVO_params::_min_offset, HVO_params::_max_offset};


    torch::Tensor hits;
    torch::Tensor velocities_unmodified;
    torch::Tensor offsets_unmodified;
    torch::Tensor velocities_modified;
    torch::Tensor offsets_modified;


    /// Default Constructor
    HVO()
    {
        hits = torch::zeros({time_steps, num_voices});
        velocities_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        velocities_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
    }

    /**
     *
     * @param hits_
     * @param velocities_
     * @param offsets_
     */
    HVO(torch::Tensor hits_, torch::Tensor velocities_, torch::Tensor offsets_):
        hits(hits_), time_steps(time_steps_),
        num_voices(num_voices_),
        velocities_unmodified(velocities_),
        offsets_unmodified(offsets_),
        velocities_modified(velocities_),
        offsets_modified(offsets_)
    {
    }

    void update_hvo(
        torch::Tensor hits_, torch::Tensor velocities_, torch::Tensor offsets_, bool autoCompress)
    {
        hits = hits_;
        velocities_unmodified = velocities_;
        offsets_unmodified = offsets_;
        if (autoCompress)
        {
            compressAll();
        }
    }

    void reset()
    {
        hits = torch::zeros({time_steps, num_voices});
        velocities_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_unmodified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        velocities_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets_modified = torch::zeros({time_steps, num_voices}, torch::kFloat32);
    }

    void randomizeAll()
    {
        auto hits_old = torch::rand({time_steps, num_voices});
        hits = torch::zeros({time_steps, num_voices});
        velocities_unmodified = torch::rand({time_steps, num_voices});
        offsets_unmodified = torch::rand({time_steps, num_voices}) - 0.5;

        // convert hits to 0 and 1s by thresholding
        auto row_indices = torch::arange(0, time_steps);
        for (int voice_i=0; voice_i < num_voices; voice_i++){
            // Get probabilities of voice hits at all timesteps
            auto voice_hot_probs = hits_old.index(
                {row_indices, voice_i});
            // Find locations exceeding threshold and set to 1 (hit)
            auto active_time_indices = voice_hot_probs>=0.5;
            hits.index_put_({active_time_indices, voice_i}, 1);
        }

        velocities_unmodified = velocities_unmodified * hits;
        offsets_unmodified = offsets_unmodified * hits;
        compressAll();
    }

    void randomizeExistingVelocities()
    {
        // convert hits to 0 and 1s by thresholding
        auto row_indices = torch::arange(0, time_steps);
        for (int voice_i=0; voice_i < num_voices; voice_i++)
        {
            velocities_unmodified = torch::rand({time_steps, num_voices})  * hits;
        }

        compressAll();
    }

    void randomizeExistingOffsets()
    {
        // convert hits to 0 and 1s by thresholding
        auto row_indices = torch::arange(0, time_steps);
        for (int voice_i=0; voice_i < num_voices; voice_i++)
        {
            offsets_unmodified = (offsets_unmodified = torch::rand({time_steps, num_voices}) - 0.5)  * hits;
        }
        compressAll();
    }

    vector<BasicNote> getUnmodifiedNotes(std::array<int, HVO_params::num_voices> voice_to_midi_map = nine_voice_kit_default_midi_numbers)
    {

        // get hit locations
        auto indices = hits.nonzero();
        auto n_notes = indices.sizes()[0];

        // empty vector for notes
        vector<BasicNote> Notes;

        // for each index create note and add to vector
        for (int i=0; i<n_notes; i++)
        {
            int voice_ix =  indices[i][1].template item<int>();
            int grid_line = indices[i][0].template item<int>();
            auto velocity = velocities_unmodified[indices[i][0]][indices[i][1]].template item<float>();
            auto offset = offsets_unmodified[indices[i][0]][indices[i][1]].template item<double>();
            BasicNote note_(voice_ix, velocity, grid_line, offset, voice_to_midi_map);
            Notes.push_back(note_);
        }

        // sort by time
        std::sort(Notes.begin(), Notes.end());

        return Notes;
    }

    vector<BasicNote> getModifiedNotes(std::array<int, HVO_params::num_voices>  voice_to_midi_map = nine_voice_kit_default_midi_numbers)
    {

        // get hit locations
        auto indices = hits.nonzero();
        auto n_notes = indices.sizes()[0];

        // empty vector for notes
        vector<BasicNote> Notes;

        // for each index create note and add to vector
        for (int i=0; i<n_notes; i++)
        {
            int voice_ix =  indices[i][1].template item<int>();
            int grid_line = indices[i][0].template item<int>();
            auto velocity = velocities_modified[indices[i][0]][indices[i][1]].template item<float>();
            auto offset = offsets_modified[indices[i][0]][indices[i][1]].template item<double>();
            BasicNote note_(voice_ix, velocity, grid_line, offset, voice_to_midi_map);
            Notes.push_back(note_);
        }

        // sort by time
        std::sort(Notes.begin(), Notes.end());

        return Notes;
    }

    GeneratedData<time_steps_, num_voices_> getUnmodifiedGeneratedData(std::array<int, HVO_params::num_voices> voice_to_midi_map = nine_voice_kit_default_midi_numbers)
    {
        GeneratedData<time_steps_, num_voices_> generatedData;

        auto notes = getUnmodifiedNotes(voice_to_midi_map);
        for (int ix = 0; ix < notes.size();ix++)
        {
            generatedData.addNote(notes[ix]);
        }

        return generatedData;
    }

    GeneratedData<time_steps_, num_voices_> getModifiedGeneratedData(std::array<int, HVO_params::num_voices>  voice_to_midi_map = nine_voice_kit_default_midi_numbers)
    {
        GeneratedData<time_steps_, num_voices_> generatedData;

        auto notes = getModifiedNotes(voice_to_midi_map);
        for (int ix = 0; ix < notes.size();ix++)
        {
            generatedData.addNote(notes[ix]);
        }

        return generatedData;
    }

    void compressVelocities(int voice_idx,
                            float min_val, float max_val)
    {
        auto hit_indices = hits > 0;

        for (int i=0; i<time_steps; i++)
        {
            velocities_modified[i][voice_idx]  =
                velocities_unmodified[i][voice_idx] * (max_val - min_val)
                + min_val;
        }
        velocities_modified = velocities_modified.clip(HVO_params::_min_vel, HVO_params::_max_vel);
        velocities_modified = velocities_modified * hits;
    }

    void compressOffsets(int voice_idx,
                            float min_val, float max_val)
    {

        for (int i=0; i<time_steps; i++)
        {
            offsets_modified[i][voice_idx]  =
                (max_val - min_val)/(2*HVO_params::_max_offset)*
                    (offsets_unmodified[i][voice_idx]+HVO_params::_max_offset)
                +min_val;
        }
        offsets_modified = offsets_modified.clip(HVO_params::_min_offset, HVO_params::_max_offset);

        offsets_modified = offsets_modified * hits;

    }


    /***
     * Automatically compresses the velocities and offsets if need be
     */
    void compressAll()
    {
        if (vel_range[0] == HVO_params::_min_vel and vel_range[1] == HVO_params::_max_vel)
        {
            velocities_modified = velocities_unmodified;
        }
        else
        {
            for (int i=0; i<num_voices; i++)
            {
                compressVelocities(i, vel_range[0], vel_range[1]);
            }
        }

        if (offset_range[0] == HVO_params::_min_offset and offset_range[1] == HVO_params::_max_offset)
        {
            offsets_modified = offsets_unmodified;
        }
        else
        {
            for (int i=0; i<num_voices; i++)
            {
                compressOffsets(i, offset_range[0], offset_range[1]);
            }
        }
    }

    /***
     * Updates the compression ranges for vels and offsets
     * Automatically updates the modified hvo if shouldReApplyCompression is true
     * @param newVelOffsetrange (array<float, 4>)
     * @param shouldReApplyCompression  (bool)
     */
    void updateCompressionRanges(array<float, 4> compression_params,
                                 bool shouldReApplyCompression)
    {
        // calculate min/max vel using range and bias
        {
            auto bias = compression_params[0];
            auto range_ratio = compression_params[1] / 100.0f;
            auto vl = float(bias + HVO_params::_min_vel);
            auto vrange = float((HVO_params::_max_vel - HVO_params::_min_vel)
                                * range_ratio);
            if (vrange > 0)
            {
                vel_range [0] = vl;
                vel_range[1] = vl + vrange;
            }
            else // if negative range, swap min and max
            {
                vrange *= -1;
                vel_range[0] = vl + vrange;
                vel_range[1] = vl;
            }
        }

        // calculate min/max offset using range and bias
        {
            auto bias = compression_params[2];
            auto range_ratio = compression_params[3] / 100.0f;
            auto off_range_above_zero = float((HVO_params::_max_offset - HVO_params::_min_offset)
                                              * range_ratio) / 2.0f;
            if (off_range_above_zero > 0)
            {
                offset_range[0] = bias - off_range_above_zero;
                offset_range[1] = bias + off_range_above_zero;
            }
            else // if negative range, swap min and max
            {
                off_range_above_zero *= -1;
                offset_range[0] = bias + off_range_above_zero;
                offset_range[1] = bias - off_range_above_zero;
            }
        }

        if (shouldReApplyCompression)
        {
            compressAll();
        }
    }

    virtual string getStringDescription(bool showHits,
                                        bool showVels,
                                        bool showOffsets,
                                        bool needScaled)
    {
        std::ostringstream msg_stream;
        if (showHits)
        {
            msg_stream     << " ---------        HITS     ----------" << endl << hits;
        }

        if (needScaled)
        {
            if (showVels)
            {
                msg_stream << " ---- Vel (Compressed to Range) ---- " << endl
                           << velocities_modified;
            }
            if (showOffsets)
            {
                msg_stream << " -- Offsets (Compressed to Range) -- " << endl
                           << offsets_modified;
            }
        }
        else
        {
            if (showVels)
            {
                msg_stream << " ---- Vel (Without Compression) ---- " << endl
                           << velocities_unmodified;
            }
            if (showOffsets)
            {
                msg_stream << " -- Offsets (Without Compression) -- " << endl
                           << offsets_unmodified;
            }
        }

        return msg_stream.str();
    }

    torch::Tensor getConcatenatedVersion(bool needScaled)
    {
        if (needScaled)
        {
            return torch::cat({hits, velocities_modified, offsets_modified}, 1);
        }
        else
        {
            return torch::cat({hits, velocities_unmodified, offsets_unmodified}, 1);
        }
    }
};

/***
  * a Juce::abstractfifo implementation of a Lockfree FIFO
  * to be used for CustomStructs::HVO datatypes
  * @tparam time_steps_
  * @tparam num_voices_
  * @tparam queue_size
  */
template <int time_steps_, int num_voices_, int queue_size> class HVOQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<HVO<time_steps_, num_voices_>> data{};

    int time_steps, num_voices;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    HVO<time_steps_, num_voices_> latest_written_data {};
    bool writingActive = false;


public:
    HVOQueue(){

        time_steps = time_steps_;
        num_voices = num_voices_;

        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_HVO = HVO<time_steps_, num_voices_>();
            data.add(empty_HVO);
        }

    }

    void push (const HVO<time_steps_, num_voices_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

    }

    HVO<time_steps_, num_voices_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;
    }

    HVO<time_steps_, num_voices_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
            getNumReady(), start1, blockSize1, start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr + blockSize2 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;

            return readData;
        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr + blockSize1 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;
            return readData;
        }
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    HVO<time_steps_, num_voices_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
    }
};


// ============================================================================================================
// ==========                    HVOLight Structure and Corresponding Queue                       =============
// ==========   (Used for sending data from model thread to generated drums pianoroll in editor)
// ============================================================================================================

/***
 * A version of HVO which doesn't automatically compress/modify the velocity/offset data.
 * (Only holds the most essential info for displaying on the pianoRoll for generations)
 * @tparam time_steps_
 * @tparam num_voices_
 */
template <int time_steps_, int num_voices_> struct HVOLight
{
    int time_steps = time_steps_;
    int num_voices = num_voices_;

    torch::Tensor hits;
    torch::Tensor hit_probabilities;
    torch::Tensor velocities;
    torch::Tensor offsets;

    // Default Constructor
    HVOLight()
    {
        hits = torch::zeros({time_steps, num_voices});
        hit_probabilities = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        velocities = torch::zeros({time_steps, num_voices}, torch::kFloat32);
        offsets = torch::zeros({time_steps, num_voices}, torch::kFloat32);
    }

    /**
     *
     * @param hits_
     * @param velocities_
     * @param offsets_
     */
    HVOLight(torch::Tensor hits_, torch::Tensor hit_probabilities_, torch::Tensor velocities_, torch::Tensor offsets_):
        hits(hits_), time_steps(time_steps_), num_voices(num_voices_),
        velocities(velocities_),
        offsets(offsets_), hit_probabilities(hit_probabilities_)
    {}
};

template <int time_steps_, int num_voices_, int queue_size> class HVOLightQueue
{
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<HVOLight<time_steps_, num_voices_>> data{};

    int time_steps, num_voices;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    HVOLight<time_steps_, num_voices_> latest_written_data {};
    bool writingActive = false;



public:

    HVOLightQueue(){

        time_steps = time_steps_;
        num_voices = num_voices_;

        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_HVOLight = HVOLight<time_steps_, num_voices_>();
            data.add(empty_HVOLight);
        }

    }

    void push (const HVOLight<time_steps_, num_voices_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

    }

    HVOLight<time_steps_, num_voices_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;
    }

    HVOLight<time_steps_, num_voices_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
            getNumReady(), start1, blockSize1, start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr + blockSize2 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;

            return readData;
        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr + blockSize1 - 1);
            lockFreeFifo->finishedRead(blockSize1 + blockSize2);
            num_reads += 1;
            return readData;
        }
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }

    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    HVOLight<time_steps_, num_voices_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
    }
};


// ============================================================================================================
// ==========                    MonotonicGroove Structure and Corresponding Queue                =============
// ==========   (Used for sending data from groove thread to model thread)
// ============================================================================================================
/**
 * A structure holding the monotonic groove
 * @tparam time_steps_
 */
template <int time_steps_> struct MonotonicGroove
{
    torch::Tensor registeration_times;

    int time_steps;

    HVO<time_steps_, 1> hvo;    // holds the groove as is without modification

    MonotonicGroove()
    {
        registeration_times = torch::zeros({time_steps_, 1}, torch::kFloat32);
        time_steps = time_steps_;
    }

    void resetGroove()
    {
        hvo.reset();
        registeration_times = torch::zeros({time_steps_, 1}, torch::kFloat32);
    }

    // if force_overdub is true, places new note in groove regardless of timing vel info
    bool overdubWithNote(BasicNote note_, bool force_overdub = false)
    {

        // 1. find the nearest grid line and calculate offset
        auto ppq = note_.time.ppq;
        auto div = round(ppq / HVO_params::_16_note_ppq);
        auto offset = (ppq - (div * HVO_params::_16_note_ppq))
                      / HVO_params::_32_note_ppq * HVO_params::_max_offset;
        auto grid_index = (long long) fmod(div, HVO_params::_n_16_notes);

        // 2. Place note in groove
        bool shouldAddNote = false;

        if (force_overdub)
        {
            shouldAddNote = true;

        }
        else
        {
            if (note_.capturedInLoop)
            {
                // 2.a. if in loop mode, always adds the latest note received
                shouldAddNote = true;

            }
            else
            {
                auto prev_registration_time_at_grid = registeration_times[grid_index].template item<float>();
                if (abs(ppq - prev_registration_time_at_grid) > HVO_params::_32_note_ppq)
                {   // 2.b. add note if received not in the vicinity of previous note registered at the same position
                    if (note_.velocity != (hvo.velocities_unmodified[grid_index] * hvo.hits[grid_index]).template item<float>() or
                        fmod((ppq - prev_registration_time_at_grid), (time_steps/4))!=0)
                        shouldAddNote = true;
                }
                else
                {   // 2c. if note received in the same vicinity, only add if velocity louder
                    if (note_.velocity >= (hvo.velocities_unmodified[grid_index] * hvo.hits[grid_index]).template item<float>())
                    {
                        shouldAddNote = true;
                    }
                }
            }
        }

        if (shouldAddNote)
        {
            if (note_.velocity > 0)
            {
                hvo.hits[grid_index] = 1;
            }
            else
            {
                hvo.hits[grid_index] = 0;
            }


            hvo.offsets_unmodified[grid_index] = offset;
            hvo.velocities_unmodified[grid_index] = note_.velocity;
            registeration_times[grid_index] = note_.time.ppq;

            return true;
        }
        else
        {
            return false;
        }
    }

    string getStringDescription(bool showHits,
                                bool showVels,
                                bool showOffsets,
                                bool needScaled)
    {
        return hvo.getStringDescription(showHits, showVels, showOffsets, needScaled);
    }

    // gets a full n_voices version of the groove and places the groove parts in the specified
    // VoiceToPlace version.
    // if needScaled is true, the version with modified velocities and offsets will be returned
    // (if this is used to feed groove to model, make sure needScaled is True!)
    torch::Tensor getFullVersionTensor(bool needScaled, int VoiceToPlace, int num_voices)
    {
        auto singleVoiceTensor= hvo.getConcatenatedVersion(needScaled);
        auto FullVoiceTensor = torch::zeros({time_steps, num_voices * 3});
        for (int i=0; i<time_steps; i++)
        {
            FullVoiceTensor[i][VoiceToPlace] = singleVoiceTensor[i][0]; // hit
            FullVoiceTensor[i][VoiceToPlace+num_voices] = singleVoiceTensor[i][1]; //vel
            FullVoiceTensor[i][VoiceToPlace+2*num_voices] = singleVoiceTensor[i][2]; //offset
        }
        return FullVoiceTensor;
    }
};

/***
 * a Juce::abstractfifo implementation of a Lockfree FIFO
 * to be used for CustomStructs::MonotonicGroove datatypes
 * @tparam time_steps_
 * @tparam queue_size
 */
template <int time_steps_, int queue_size> class MonotonicGrooveQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<MonotonicGroove<time_steps_>> data{};

    int time_steps;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    MonotonicGroove<time_steps_> latest_written_data {};
    bool writingActive = false;


public:
    MonotonicGrooveQueue(){

        time_steps = time_steps_;

        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            auto empty_groove = MonotonicGroove<time_steps_>();
            data.add(empty_groove);
        }

    }

    void push (const MonotonicGroove<time_steps_> writeData)
    {
        int start1, start2, blockSize1, blockSize2;

        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr =  writeData;

        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;

    }

    MonotonicGroove<time_steps_> pop()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;
        auto res =  *(start_data_ptr);
        lockFreeFifo->finishedRead(1);
        num_reads += 1;
        return res;

    }

    MonotonicGroove<time_steps_>  getLatestOnly()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            getNumReady(), start1, blockSize1,
            start2, blockSize2);

        if (blockSize2 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            auto readData = *(start_data_ptr+blockSize2-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;

        }
        if (blockSize1 > 0)
        {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            auto readData = *(start_data_ptr+blockSize1-1);
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;
        }

    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the pop() or getLatestOnly() methods!!!
    MonotonicGroove<time_steps_> getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }


    bool isWritingInProgress()
    {
        return writingActive;
    }
};



// ============================================================================================================
// ==========                    A queue for String messages                =============
// ==========   (Used for sending messages to a texteditor thread )
// ============================================================================================================
/***
 * a Juce::abstractfifo implementation of a Lockfree FIFO
 * to be used for Strings
 * @tparam queue_size
 */
template<int queue_size> class StringLockFreeQueue
{
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    unique_ptr<juce::AbstractFifo> lockFreeFifo;
    string data[queue_size];

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    bool writingActive = false;


    string latest_written_data = "";


public:
    StringLockFreeQueue()
    {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));
    }

    void addText (string writeText)
    {
        int start1, start2, blockSize1, blockSize2;
        writingActive = true;
        lockFreeFifo ->prepareToWrite(
            1, start1, blockSize1,
            start2, blockSize2);

        data[start1] = writeText;
        latest_written_data = writeText;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;
    }

    string getText()
    {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo ->prepareToRead(
            1, start1, blockSize1,
            start2, blockSize2);

        string retrievedText;
        retrievedText = data[start1];

        lockFreeFifo -> finishedRead(1);
        num_reads += 1;

        return retrievedText;
    }

    int getNumReady()
    {
        return lockFreeFifo -> getNumReady();
    }


    int getNumberOfReads()
    {
        return num_reads;
    }

    int getNumberOfWrites()
    {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the getText() method !!!
    string getLatestDataWithoutMovingFIFOHeads()
    {
        return latest_written_data;
    }

    bool isWritingInProgress()
    {
        return writingActive;
    }


};


