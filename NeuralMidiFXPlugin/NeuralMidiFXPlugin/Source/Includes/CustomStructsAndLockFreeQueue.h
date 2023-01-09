//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../settings.h"
#include <torch/script.h> // One-stop header.

#include <utility>


using namespace std;

template <typename T> struct MultiTimedStructure {
    T absolute_time_in_ppq;
    T absolute_time_in_seconds;
    T relative_time_in_ppq;
    T relative_time_in_seconds;
};

struct Note {
    int channel {0};
    int number {0};
    int velocity {0};
    double onset_time {-1};
    double duration {0};

    Note() = default;

    Note(int channel_, int number_, int velocity_, double onset_time_, double duration_) {
        this->channel = channel_;
        this->number = number_;
        this->velocity = velocity_;
        this->onset_time = onset_time_;
        this->duration = duration_;
    }

    string getDescription() const {
        stringstream ss;
        ss << "NoteNumber: " + to_string(number) + " Velocity: " + to_string(velocity) + " Start: " +
        to_string(onset_time) + " Duration: " + to_string(duration);
        return ss.str();
    }

    bool operator==(const Note& rhs) const {
        return channel == rhs.channel &&
               number == rhs.number &&
               velocity == rhs.velocity &&
               onset_time == rhs.onset_time &&
               duration == rhs.duration;
    }
};

// Structs for passing note on, note off, cc, tempo and time sig data to other threads (no time info embedded)
struct NoteOn {
    int channel {0};
    int number {0};
    int velocity {0};
    double time_ppq_absolute {0.0};
    double time_sec_absolute {0.0};
    double time_ppq_relative {0.0};
    double time_sec_relative {0.0};

    NoteOn() = default;

    NoteOn(int channel_, int number_, int velocity_, double time_ppq_absolute_, double time_sec_absolute_,
           double time_ppq_relative_, double time_sec_relative_) :
            channel(channel_), number(number_), velocity(velocity_), time_ppq_absolute(time_ppq_absolute_),
            time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
            time_sec_relative(time_sec_relative_){}


    // extracts note on data from a midi message if it is a note on message
    NoteOn(juce::MidiMessage m, double time_ppq_absolute_, double time_sec_absolute_,
           double time_ppq_relative_, double time_sec_relative_) : time_ppq_absolute(time_ppq_absolute_),
           time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
           time_sec_relative(time_sec_relative_){
        {
            channel = m.getChannel();
            number = m.getNoteNumber();
            velocity = m.getVelocity();
        }
    }
};


struct NoteOff {
    int channel {0};
    int number {0};
    int velocity {0};
    double time_ppq_absolute {0.0};
    double time_sec_absolute {0.0};
    double time_ppq_relative {0.0};
    double time_sec_relative {0.0};

    NoteOff() = default;

    NoteOff(int channel_, int number_, int velocity_, double time_ppq_absolute_, double time_sec_absolute_,
            double time_ppq_relative_, double time_sec_relative_) :
            channel(channel_), number(number_), velocity(velocity_), time_ppq_absolute(time_ppq_absolute_),
            time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
            time_sec_relative(time_sec_relative_){}

    // extracts note off data from a midi message if it is a note off message
    NoteOff(juce::MidiMessage m, double time_ppq_absolute_, double time_sec_absolute_,
            double time_ppq_relative_, double time_sec_relative_) : time_ppq_absolute(time_ppq_absolute_),
            time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
            time_sec_relative(time_sec_relative_){
        {
            channel = m.getChannel();
            number = m.getNoteNumber();
            velocity = m.getVelocity();
        }
    }
};


struct CC {
    int channel {0};
    int number {0};
    int value {0};
    double time_ppq_absolute {0.0};
    double time_sec_absolute {0.0};
    double time_ppq_relative {0.0};
    double time_sec_relative {0.0};

    CC() = default;

    CC(int channel_, int number_, int value_, double time_ppq_absolute_, double time_sec_absolute_,
       double time_ppq_relative_, double time_sec_relative_) :
            channel(channel_), number(number_), value(value_), time_ppq_absolute(time_ppq_absolute_),
            time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
            time_sec_relative(time_sec_relative_){}

    // extracts cc data from a midi message if it is a cc message
    CC(juce::MidiMessage m, double time_ppq_absolute_, double time_sec_absolute_,
         double time_ppq_relative_, double time_sec_relative_) : time_ppq_absolute(time_ppq_absolute_),
         time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
         time_sec_relative(time_sec_relative_){
          {
                channel = m.getChannel();
                number = m.getControllerNumber();
                value = m.getControllerValue();
          }
     }
};


struct TempoTimeSignature
{
    double qpm {0.0f};
    int microseconds_per_quarter {0};

    int numerator {0};
    int denominator {0};

    double time_ppq_absolute {0.0};
    double time_sec_absolute {0.0};
    double time_ppq_relative {0.0};
    double time_sec_relative {0.0};

    TempoTimeSignature() = default;

    TempoTimeSignature(double qpm_, int microseconds_per_quarter_, int numerator_, int denominator_,
                       double time_ppq_absolute_, double time_sec_absolute_,
                       double time_ppq_relative_, double time_sec_relative_) :
            qpm(qpm_), microseconds_per_quarter(microseconds_per_quarter_), numerator(numerator_), denominator(denominator_),
            time_ppq_absolute(time_ppq_absolute_), time_sec_absolute(time_sec_absolute_),
            time_ppq_relative(time_ppq_relative_),
            time_sec_relative(time_sec_relative_){}

    TempoTimeSignature(double qpm_, int numerator_, int denominator_, double time_ppq_absolute_, double time_sec_absolute_,
                       double time_ppq_relative_, double time_sec_relative_) :
            qpm(qpm_), numerator(numerator_), denominator(denominator_), time_ppq_absolute(time_ppq_absolute_),
            time_sec_absolute(time_sec_absolute_), time_ppq_relative(time_ppq_relative_),
            time_sec_relative(time_sec_relative_){
        microseconds_per_quarter = int(60000000 / qpm);
    }

    TempoTimeSignature(int microseconds_per_quarter_, int numerator_, int denominator_,
                       double time_ppq_absolute_, double time_sec_absolute_,
                       double time_ppq_relative_, double time_sec_relative_) :
            microseconds_per_quarter(microseconds_per_quarter_), numerator(numerator_), denominator(denominator_),
            time_ppq_absolute(time_ppq_absolute_), time_sec_absolute(time_sec_absolute_),
            time_ppq_relative(time_ppq_relative_), time_sec_relative(time_sec_relative_){
        qpm = 60000000 / microseconds_per_quarter;
    }

};

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
    T latest_written_data {};
    bool writingActive = false;

public:
    LockFreeQueue()
    {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo> (
            new juce::AbstractFifo(queue_size));

        // data.ensureStorageAllocated(queue_size);

        while (data.size() < queue_size)
        {
            // check if T is a tuple
            auto empty_exp = T();
            data.add(empty_exp);
        }
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