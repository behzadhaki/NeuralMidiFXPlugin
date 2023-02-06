//
// Created by Behzad Haki on 2022-02-11.
//
#pragma once

// #include <utility>

#include "../settings.h"
#include <torch/script.h> // One-stop header.

#include <utility>


using namespace std;

// ============================================================================================================
// ==========          LockFreeQueue (First In - First Out)          ==========================================
// ============================================================================================================
template<typename T, int queue_size>
class LockFreeQueue {
private:
    //juce::ScopedPointer<juce::AbstractFifo> lockFreeFifo;   depreciated!!
    std::unique_ptr<juce::AbstractFifo> lockFreeFifo;
    juce::Array<std::unique_ptr<T>> data;

    // keep track of number of reads/writes and the latest_value without moving FIFO
    int num_reads = 0;
    int num_writes = 0;
    T latest_written_data{};
    bool writingActive = false;

public:
    LockFreeQueue() {
        // lockFreeFifo = new juce::AbstractFifo(queue_size);   depreciated!!
        lockFreeFifo = std::unique_ptr<juce::AbstractFifo>(
                new juce::AbstractFifo(queue_size));

        // data.ensureStorageAllocated(queue_size);
        while (data.size() < queue_size) {
            // check if T is a tuple
            auto empty_exp = make_shared<T>();
            data.add(nullptr);
        }
    }

    int getNumReady() {
        return lockFreeFifo->getNumReady();
    }

    void push(T writeData) {
        // check if this object is null
        if (this == nullptr) {
            DBG(" [******] You've forgotten to Initialize the LockFreeQueue object! "
                "Double check the processor constructor !!");
            assert (false);
        }
        int start1, start2, blockSize1, blockSize2;
        writingActive = true;
        lockFreeFifo->prepareToWrite(
                1, start1, blockSize1,
                start2, blockSize2);
        auto start_data_ptr = data.getRawDataPointer() + start1;
        *start_data_ptr = make_unique<T>(writeData);
        latest_written_data = writeData;
        num_writes += 1;
        lockFreeFifo->finishedWrite(1);
        writingActive = false;
    }

    T pop() {
        int start1, start2, blockSize1, blockSize2;

        lockFreeFifo->prepareToRead(
                1, start1, blockSize1,
                start2, blockSize2);

        auto start_data_ptr = data.getRawDataPointer() + start1;

        /* This way, when the returned value goes out of scope, it will be automatically deleted. Note that you need to
         * std::move the unique pointer, so the ownership of the dynamically allocated data is transferred to the
         * returned value.*/
        auto res = std::move(*(*(start_data_ptr)));
        lockFreeFifo->finishedRead(1);
        num_reads += 1;

        return res;
    }


    T getLatestOnly() {
        int start1, start2, blockSize1, blockSize2;
        T readData;

        lockFreeFifo ->prepareToRead(
                getNumReady(), start1, blockSize1,
                start2, blockSize2);

        if (blockSize2 > 0) {
            auto start_data_ptr = data.getRawDataPointer() + start2;
            readData = *(*(start_data_ptr+blockSize2-1));
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;

        }
        if (blockSize1 > 0) {
            auto start_data_ptr = data.getRawDataPointer() + start1;
            readData = *(*(start_data_ptr+blockSize1-1));
            lockFreeFifo -> finishedRead(blockSize1+blockSize2);
            num_reads += 1;
            return readData;
        }

    }

    int getNumberOfReads() {
        return num_reads;
    }

    int getNumberOfWrites() {
        return num_writes;
    }

    // This method is useful for keeping track of whether any data has previously
    //      written to Queue regardless of being read or not
    // !! This method should only be used for initialization of GUI objects !!
    // !!! To use the QUEUE for lock free communication use the ReadFrom() or pop() methods!!!
    T getLatestDataWithoutMovingFIFOHeads() {
        return latest_written_data;
    }

    bool isWritingInProgress() {
        return writingActive;
    }
};


// ============================================================================================================
// ==========          GUI PARAM ID STRUCTS                          ==========================================
// ============================================================================================================

static string label2ParamID(const string &label) {
    // capitalize label
    string paramID = label;
    std::transform(paramID.begin(), paramID.end(), paramID.begin(), ::toupper);
    return paramID;
}

// to get the value of a parameter, use the following syntax:
//      auto paramID = GuiParams.getParamValue(LabelOnGUi);
struct GuiParams {

    GuiParams() { construct(); }

    explicit GuiParams(juce::AudioProcessorValueTreeState *apvts) {
        construct();
        for (size_t i = 0; i < paramLabels.size(); i++) {
            paramValues[i] = double(*apvts->getRawParameterValue(paramIDs[i]));
        }
    }

    // checks if any of the parameters have changed, and updates the values
    // returns true if any of the parameters have changed
    bool update(juce::AudioProcessorValueTreeState *apvts) {
        bool hasChanged = false;
        for (size_t i = 0; i < paramLabels.size(); i++) {
            auto newValue = double(*apvts->getRawParameterValue(paramIDs[i]));
            if (newValue != paramValues[i]) {
                paramValues[i] = newValue;
                hasChanged = true;
            }
        }
        return hasChanged;
    }

    void print() {
        for (size_t i = 0; i < paramLabels.size(); i++) {
            DBG("GUI Instance " <<  i << " : " << paramLabels[i] << ", value --> " << paramValues[i]);
        }
    }

    double getParamValue(const string &label) {
        for (size_t i = 0; i < paramLabels.size(); i++) {
            if (paramIDs[i] == label2ParamID(label)) {
                return paramValues[i];
            }
        }
        std::stringstream ss;
        ss << "Label not found. Choose one of ";
        for (auto &paramLabel: paramLabels) {
            ss << paramLabel << ", ";
        }
        DBG(ss.str());
        assert(false);
    }

private:
    vector<string> paramLabels{};
    vector<string> paramIDs{};
    vector<double> paramValues{};

    void construct() {
        auto tabList = UIObjects::Tabs::tabList;

        for (auto tab_list: tabList) {
            auto slidersList = std::get<1>(tab_list);
            auto rotariesList = std::get<2>(tab_list);

            for (const auto &elementsList: {slidersList, rotariesList}) {
                for (auto element: elementsList) {
                    std::string label = std::get<0>(element);
                    double defaultVal = std::get<3>(element);
                    std::string paramID = label2ParamID(label);

                    // check that label doesn't already exist in vector
                    // if it does, assert that it is unique
                    for (const auto &past_labels: paramLabels) {
                        // If you hit this assert, you have a duplicate label, which is not allowed
                        // change the label in Settings.h's UIObjects
                        if (label == past_labels) { DBG("Duplicate label found: " << label); }
                        assert(label != past_labels);
                    }

                    paramLabels.push_back(label);
                    paramIDs.push_back(paramID);
                    paramValues.push_back(defaultVal);

                }
            }
        }
    }
};
