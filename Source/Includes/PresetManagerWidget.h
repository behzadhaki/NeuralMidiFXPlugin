#pragma once

#include <array>
#include "shared_plugin_helpers/shared_plugin_helpers.h"

#pragma once

class PresetTableComponent : public juce::Component,
                             public juce::TableListBoxModel,
                             private juce::Slider::Listener
{
public:
    explicit PresetTableComponent(juce::AudioProcessorValueTreeState& apvts_) : apvts(apvts_)
    {
        // Initialize the tables
        for (auto& table : presetTables) {
            initializeTable(table);
            addAndMakeVisible(table);
        }
        currentPresetSlider.setValue(0);
        currentPresetSliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            apvts, label2ParamID("presetParam"), currentPresetSlider);

        // Add and configure Save and Rename buttons
        saveButton.setButtonText("Save");
        saveButton.onClick = [this] { savePreset(); };
        addAndMakeVisible(saveButton);

        renameButton.setButtonText("Rename");
        renameButton.onClick = [this] { renamePreset(); };
        addAndMakeVisible(renameButton);

        // Add and configure the Text Editor
        presetNameEditor.setMultiLine(false);
        presetNameEditor.setReturnKeyStartsNewLine(false);
        presetNameEditor.setReadOnly(false);
        presetNameEditor.setScrollbarsShown(true);
        presetNameEditor.setCaretVisible(true);
        presetNameEditor.setPopupMenuEnabled(true);
        addAndMakeVisible(presetNameEditor);

        loadPresetNames();

        renameButton.onClick = [this] { renamePreset(); };

        // attach the slider listener
        currentPresetSlider.addListener(this);
    }

    void paint(juce::Graphics& g) override
    {
        // make background light grey
        presetTables[0].setColour(juce::ListBox::backgroundColourId, juce::Colours::lightgrey);
    }

    ~PresetTableComponent() override = default;

    void resized() override
    {   auto gap = proportionOfHeight(0.05f);

        auto area = getLocalBounds();
        area.removeFromTop(gap);

        auto tableArea = area.removeFromTop(proportionOfHeight(0.7f)).removeFromLeft(proportionOfWidth(1));

        for (auto& table : presetTables) {
            table.setBounds(tableArea);
            tableArea = tableArea.translated(proportionOfWidth(1), 0);
        }
        area.removeFromTop(gap);
        presetNameEditor.setBounds(area.removeFromTop(proportionOfHeight(0.05f)));
        saveButton.setBounds(area.removeFromBottom(proportionOfHeight(0.05f)));
        renameButton.setBounds(area.removeFromTop(proportionOfHeight(0.05f)));
        area.removeFromTop(gap);

    }

    // Override to handle row selection changes
    void selectedRowsChanged(int lastRowSelected) override
    {
        cout << "Selected row changed to " << lastRowSelected << endl;
        if (lastRowSelected >= 0 && lastRowSelected < presetNames.size())
        {
            if (lastRowSelected != (currentPresetSlider.getValue() - 1)) {
                currentPresetSlider.setValue(lastRowSelected);
            }
            presetNameEditor.setText(presetNames[lastRowSelected], juce::dontSendNotification);
            currentPresetSlider.setValue(lastRowSelected + 1);
        }

        savePresetNames();

    }

    // save preset names to a file
    void savePresetNames()
    {
        juce::File presetNamesFile(
            stripQuotes(std::string(default_preset_dir)) + path_separator + "preset.names");
        presetNamesFile.deleteFile();
        presetNamesFile.create();
        presetNamesFile.appendText(presetNames.joinIntoString("\n"));
    }

    void loadPresetNames()
    {
        juce::File presetNamesFile(
            stripQuotes(std::string(default_preset_dir)) + path_separator + "preset.names");

        if (presetNamesFile.existsAsFile())
        {
            cout << "Loading preset names from file" << endl;
            presetNames.clear();
            presetNamesFile.readLines(presetNames);
        }
    }

    void initializeTable(juce::TableListBox& table)
    {
        // Add a single column
        table.getHeader().addColumn("Preset", 1, 100);

        // Set up table properties
        table.setModel(this);
        table.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
        table.setOutlineThickness(1);
        table.setMultipleSelectionEnabled(false);

        // Populate the table with presets
        for (int i = 0; i < 100; ++i) {
            presetNames.add("Preset " + juce::String(i + 1));
        }
    }

    // TableListBoxModel overrides
    int getNumRows() override
    {
        return presetNames.size();
    }

    void paintRowBackground(juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        if (rowIsSelected)
            g.fillAll(juce::Colours::lightblue);
    }

    void paintCell(juce::Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
    {
        g.setColour(juce::Colours::black);
        g.drawText(presetNames[rowNumber], 2, 0, width - 4, height, juce::Justification::centredLeft, true);
    }

    void savePreset()
    {
        // Implement the logic to save the selected preset
    }

    void getSelectedPreset() {
        // Implement the logic to get the selected preset

    }

    void renamePreset()
    {
        for (auto& table : presetTables) {
            if (table.getSelectedRow() >= 0) {
                auto selectedRow = table.getSelectedRow();
                presetNames.set(selectedRow, presetNameEditor.getText());
                table.updateContent();
                break;
            }
        }
    }

    // listen to slider changes
    void sliderValueChanged(juce::Slider* slider) override {
        if (slider == &currentPresetSlider) {
            cout << "currentPresetSlider value changed from " << currentPresetSlider.getValue() -1 << " to " << slider->getValue() << endl;
            auto selectedRow = (int) currentPresetSlider.getValue() -1 ;
            if (true) {
                presetNameEditor.setText(presetNames[selectedRow], juce::dontSendNotification);
                for (auto& table : presetTables) {
                    selectedRowsChanged(selectedRow);
                    table.selectRow(selectedRow);
                }
            }
        }
    }

private:
    std::array<juce::TableListBox, 1> presetTables;
    juce::StringArray presetNames;
    juce::TextButton saveButton;
    juce::TextButton renameButton;
    juce::TextEditor presetNameEditor; // Added text editor for preset names
    juce::AudioProcessorValueTreeState& apvts;
    juce::Slider currentPresetSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> currentPresetSliderAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetTableComponent)
};
