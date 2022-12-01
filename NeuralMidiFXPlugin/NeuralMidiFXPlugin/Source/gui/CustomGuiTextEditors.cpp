//
// Created by Behzad Haki on 2022-08-03.
//

#include "CustomGuiTextEditors.h"



LoggerTextEditorTemplate::LoggerTextEditorTemplate():juce::TextEditor(), juce::Thread("Thread")
{
    this->setFont(juce::Font(8));

    this->setMultiLine (true);
    this->setReturnKeyStartsNewLine (true);
    this->setReadOnly (false);
    this->setScrollbarsShown (true);
    this->setCaretVisible (true);

}

void LoggerTextEditorTemplate::prepareToStop() { stopThread(1200) ;}

// Override this with the task to be done on received data from queue
// See examples of the child classes below

void LoggerTextEditorTemplate::QueueDataProcessor() {}

// No need to override in the children classes
void LoggerTextEditorTemplate::run()
{
    bool bExit = threadShouldExit();
    while (!bExit)
    {
        QueueDataProcessor();
        bExit = threadShouldExit();
        sleep (10); // avoid burning CPU, if reading is returning immediately
    }
}


BasicNoteStructLoggerTextEditor::BasicNoteStructLoggerTextEditor(
    LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* note_quePntr): LoggerTextEditorTemplate()
{
    note_queP = note_quePntr;

    TextEditorLabel.setText ("Midi#/Vel/Actual Onset ppq", juce::dontSendNotification);
    TextEditorLabel.attachToComponent (this, juce::Justification::top);
    TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    TextEditorLabel.setJustificationType (juce::Justification::top);
    addAndMakeVisible (TextEditorLabel);

    this->setCurrentThreadName("BasicNoteStructureLoggerThread");

    numNotesPrintedOnLine = 0;

    this->startThread(5);

}

BasicNoteStructLoggerTextEditor::~BasicNoteStructLoggerTextEditor()
{
    this->prepareToStop();

}

void BasicNoteStructLoggerTextEditor::QueueDataProcessor()
{
    if (note_queP != nullptr)
    {
        while (note_queP->getNumReady() > 0 and not this->threadShouldExit())
        {
            BasicNote note;
            note_queP->ReadFrom(&note, 1); // here cnt result is 3
            juce::MessageManagerLock mmlock;

            if(this->getTotalNumChars()>gui_settings::BasicNoteStructLoggerTextEditor::maxChars)
            {
                if (not threadShouldExit())
                    this->clear();
                if (not threadShouldExit())
                    this->numNotesPrintedOnLine = 0;
            }
            if (not threadShouldExit())
                insertTextAtCaret(
                    note.getStringDescription());

            if (not threadShouldExit())
                this->numNotesPrintedOnLine += 1;

            if(this->numNotesPrintedOnLine > 0 and
                this->numNotesPrintedOnLine % gui_settings::BasicNoteStructLoggerTextEditor::
                            nNotesPerLine == 0)
                if (not threadShouldExit())
                    insertTextAtCaret(juce::newLine);

        }
    }

}

TextMessageLoggerTextEditor::TextMessageLoggerTextEditor(
    string const label, StringLockFreeQueue<GeneralSettings::gui_io_queue_size>* text_message_quePntr):LoggerTextEditorTemplate()
{
    text_message_queue = text_message_quePntr;

    TextEditorLabel.setText (label, juce::dontSendNotification);
    TextEditorLabel.attachToComponent (this, juce::Justification::top);
    TextEditorLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    TextEditorLabel.setJustificationType (juce::Justification::top);
    addAndMakeVisible (TextEditorLabel);
    setCurrentThreadName("TextMessageLoggerThread_"+label);
    startThread(5);
}

TextMessageLoggerTextEditor::~TextMessageLoggerTextEditor()
{
    this->prepareToStop();

}

void TextMessageLoggerTextEditor::QueueDataProcessor()
{
    if (text_message_queue != nullptr)
    {
        string msg_received;
        while (text_message_queue->getNumReady() > 0 and not threadShouldExit())
        {
            msg_received = text_message_queue->getText();

            juce::MessageManagerLock mmlock;

            if(this->getTotalNumChars()>gui_settings::TextMessageLoggerTextEditor::maxChars)
            {
                if (not threadShouldExit())
                    this->clear();
            }

            if (msg_received == "clear" or msg_received == "Clear") {
                if (not threadShouldExit())
                    this->clear();
            }
            else
            {
                if (not threadShouldExit())
                    insertTextAtCaret(msg_received);
                if (not threadShouldExit())
                    insertTextAtCaret(juce::newLine);
            }
        }
    }
}

