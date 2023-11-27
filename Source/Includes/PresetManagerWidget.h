#pragma once

#include <array>
#include "shared_plugin_helpers/shared_plugin_helpers.h"

#pragma once

class PresetTableComponent : public juce::Component, public juce::TableListBoxModel
{
public:
    PresetTableComponent()
    {
        // Initialize the tables
        for (auto& table : presetTables) {
            initializeTable(table);
            addAndMakeVisible(table);
        }

        // Add and configure Save and Rename buttons
        saveButton.setButtonText("Save");
        saveButton.onClick = [this] { savePreset(); };
        addAndMakeVisible(saveButton);

        renameButton.setButtonText("Rename");
        renameButton.onClick = [this] { renamePreset(); };
        addAndMakeVisible(renameButton);
    }

    ~PresetTableComponent() override = default;

    void resized() override
    {
        auto area = getLocalBounds();
        auto tableArea = area.removeFromTop(proportionOfHeight(0.8f)).removeFromLeft(proportionOfWidth(1));

        for (auto& table : presetTables) {
            table.setBounds(tableArea);
            tableArea = tableArea.translated(proportionOfWidth(1), 0);
        }

        saveButton.setBounds(area.removeFromTop(30));
        renameButton.setBounds(area.removeFromTop(30));
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
            presetNames.add("Empty");
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

    void renamePreset()
    {
        // Implement the logic to rename the selected preset
        for (auto& table : presetTables) {
            if (table.getSelectedRow() >= 0) {
                auto selectedRow = table.getSelectedRow();
                // Logic to rename the preset at selectedRow
                break;
            }
        }
    }

private:
    std::array<juce::TableListBox, 1> presetTables;
    juce::StringArray presetNames;
    juce::TextButton saveButton;
    juce::TextButton renameButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetTableComponent)
};
