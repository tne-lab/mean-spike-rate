/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2018 Translational NeuroEngineering Laboratory, MGH

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "MeanSpikeRateEditor.h"
#include <string> // stof
#include <cfloat> // FLT_MAX

MeanSpikeRateEditor::MeanSpikeRateEditor(MeanSpikeRate* parentNode)
    : GenericEditor(parentNode, false)
{
    desiredWidth = WIDTH;
    const int HEADER_HEIGHT = 22;

    auto processor = static_cast<MeanSpikeRate*>(getProcessor());

    // spike channels
    spikeChannelViewport = new Viewport();
    spikeChannelViewport->setScrollBarsShown(false, false, true, false);
    spikeChannelViewport->setBounds(0, HEADER_HEIGHT, CONTENT_WIDTH, BUTTON_VIEWPORT_HEIGHT);

    spikeChannelCanvas = new Component();
    spikeChannelViewport->setViewedComponent(spikeChannelCanvas);

    addAndMakeVisible(spikeChannelViewport);

    // other controls
    int xPos = 10;
    int yPos = HEADER_HEIGHT + BUTTON_VIEWPORT_HEIGHT + 5;
    const int TEXT_HEIGHT = 20;

    outputLabel = new Label("outputL", "Output:");
    outputLabel->setBounds(xPos, yPos + 1, 70, TEXT_HEIGHT);
    outputLabel->setFont(Font("Small Text", 12, Font::plain));
    outputLabel->setColour(Label::textColourId, Colours::darkgrey);
    outputLabel->setTooltip(OUTPUT_TOOLTIP);
    addAndMakeVisible(outputLabel);

    outputBox = new ComboBox("outputB");
    outputBox->setBounds(xPos + 75, yPos, 50, TEXT_HEIGHT);
    outputBox->setTooltip(OUTPUT_TOOLTIP);
    outputBox->addListener(this);
    addAndMakeVisible(outputBox);

    yPos += TEXT_HEIGHT + 5;

    timeConstLabel = new Label("timeConstL", "Time const:");
    timeConstLabel->setBounds(xPos, yPos + 1, 80, TEXT_HEIGHT);
    timeConstLabel->setFont(Font("Small Text", 12, Font::plain));
    timeConstLabel->setColour(Label::textColourId, Colours::darkgrey);
    timeConstLabel->setTooltip(TIME_CONST_TOOLTIP);
    addAndMakeVisible(timeConstLabel);

    timeConstEditable = new Label("timeConstE");
    timeConstEditable->setEditable(true);
    timeConstEditable->setBounds(xPos += 80, yPos, 45, TEXT_HEIGHT);
    timeConstEditable->setText(String(processor->timeConstMs), dontSendNotification);
    timeConstEditable->setColour(Label::backgroundColourId, Colours::grey);
    timeConstEditable->setColour(Label::textColourId, Colours::white);
    timeConstEditable->setTooltip(TIME_CONST_TOOLTIP);
    timeConstEditable->addListener(this);
    addAndMakeVisible(timeConstEditable);

    timeConstUnit = new Label("timeConstU", "ms");
    timeConstUnit->setBounds(xPos + 45, yPos + 1, 25, TEXT_HEIGHT);
    timeConstUnit->setFont(Font("Small Text", 12, Font::plain));
    timeConstUnit->setColour(Label::textColourId, Colours::darkgrey);
    timeConstUnit->setTooltip(TIME_CONST_TOOLTIP);
    addAndMakeVisible(timeConstUnit);
}

MeanSpikeRateEditor::~MeanSpikeRateEditor() {}

void MeanSpikeRateEditor::updateSettings()
{
    MeanSpikeRate* processor = static_cast<MeanSpikeRate*>(getProcessor());

    // update output channel options
    int oldNumChans = outputBox->getNumItems();
    int newNumChans = processor->getNumInputs();

    if (newNumChans != oldNumChans)
    {
        outputBox->clear(dontSendNotification);
        for (int i = 0; i < newNumChans; ++i)
        {
            outputBox->addItem(String(i + 1), i + 1);
        }

        if (newNumChans > processor->outputChan)
        {
            outputBox->setSelectedId(processor->outputChan + 1, dontSendNotification);
        }
        else if (newNumChans > 0)
        {
            outputBox->setSelectedId(1, sendNotificationAsync);
        }
    }

    // update electrode buttons
    auto& spikeChannelArray = processor->spikeChannelArray;

    // make spikeChannelButtons array match the spikeChannelArray
    int numSpikeChans = spikeChannelArray.size();
    int numButtons;
    for (int kChan = 0; kChan < numSpikeChans; ++kChan)
    {
        numButtons = spikeChannelButtons.size();
        jassert(numButtons >= kChan);

        if (numButtons > kChan)
        {
            // check whether this or a later button matches the channel
            String name = spikeChannelArray[kChan]->getName();
            if (spikeChannelButtons[kChan]->getTooltip() == name)
            {
                continue; // already in the right place
            }

            bool found = false;
            for (int kButton = kChan + 1; kButton < numButtons; ++kButton)
            {
                if (spikeChannelButtons[kButton]->getTooltip() == name)
                {
                    found = true;
                    spikeChannelButtons.swap(kChan, kButton);
                    break;
                }
            }

            if (found)
            {
                continue; // button found and swapped to right place
            }
        }

        // have to add a new button
        spikeChannelButtons.insert(kChan, makeNewChannelButton(spikeChannelArray[kChan]));
    }

    // remove extra buttons
    numButtons = spikeChannelButtons.size();
    if (numButtons > numSpikeChans)
    {
        spikeChannelButtons.removeLast(numButtons - numSpikeChans);
    }

    // position the buttons
    layoutChannelButtons();
}

int MeanSpikeRateEditor::getNumActiveElectrodes()
{
    int numActive = 0;
    for (auto button : spikeChannelButtons)
    {
        if (button->getToggleState())
        {
            numActive++;
        }
    }
    return numActive;
}

void MeanSpikeRateEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    auto processor = static_cast<MeanSpikeRate*>(getProcessor());
    processor->setParameter(OUTPUT_CHAN, comboBoxThatHasChanged->getSelectedId() - 1);
}

void MeanSpikeRateEditor::labelTextChanged(Label* labelThatHasChanged)
{
    if (labelThatHasChanged == timeConstEditable)
    {
        auto processor = static_cast<MeanSpikeRate*>(getProcessor());

        float newVal;
        bool success = updateFloatLabel(labelThatHasChanged, 0.01F, FLT_MAX, static_cast<float>(processor->timeConstMs), &newVal);

        if (success)
        {
            processor->setParameter(TIME_CONST, newVal);
        }
    }
}

bool MeanSpikeRateEditor::getSpikeChannelEnabled(int index)
{
    if (index < 0 || index >= spikeChannelButtons.size())
    {
        jassertfalse;
        return false;
    }
    return spikeChannelButtons[index]->getToggleState();
}

void MeanSpikeRateEditor::setSpikeChannelEnabled(int index, bool enabled)
{
    if (index < 0 || index >= spikeChannelButtons.size())
    {
        jassertfalse;
        return;
    }
    spikeChannelButtons[index]->setToggleState(enabled, sendNotificationSync);
}

void MeanSpikeRateEditor::saveCustomParameters(XmlElement* xml)
{
    xml->setAttribute("Type", "MeanSpikeRateEditor");

    XmlElement* paramValues = xml->createNewChildElement("VALUES");
    paramValues->setAttribute("outputChan", outputBox.get() ? outputBox->getSelectedId() - 1 : -1);
    paramValues->setAttribute("timeConstMs", timeConstEditable.get() ? timeConstEditable->getText() : "1000");
}

void MeanSpikeRateEditor::loadCustomParameters(XmlElement* xml)
{
    forEachXmlChildElementWithTagName(*xml, xmlNode, "VALUES")
    {
        int newOutputChan = xmlNode->getIntAttribute("outputChan", -1);
        if (newOutputChan >= 0 && newOutputChan < outputBox->getNumItems())
        {
            outputBox->setSelectedId(newOutputChan + 1, sendNotificationSync);
        }

        timeConstEditable->setText(xmlNode->getStringAttribute("timeConstMs", timeConstEditable->getText()), sendNotificationSync);
    }
}

/* -------- private ----------- */

ElectrodeButton* MeanSpikeRateEditor::makeNewChannelButton(SpikeChannel* chan)
{
    auto button = new ElectrodeButton(0);
    button->setToggleState(true, dontSendNotification);
    
    String prefix;
    switch (chan->getChannelType())
    {
    case SpikeChannel::SINGLE:
        prefix = "SE";
        break;

    case SpikeChannel::STEREOTRODE:
        prefix = "ST";
        break;

    case SpikeChannel::TETRODE:
        prefix = "TT";
        break;

    default:
        prefix = "IV";
        break;
    }

    button->setButtonText(prefix + String(chan->getSourceTypeIndex()));
    button->setTooltip(chan->getName());

    return button;
}

void MeanSpikeRateEditor::layoutChannelButtons()
{
    int nButtons = spikeChannelButtons.size();
    int nRows = nButtons > 0 ? (nButtons - 1) / ROW_LENGTH + 1 : 0;
    spikeChannelCanvas->setBounds(0, 0, WIDTH, MARGIN * 2 + nRows * BUTTON_HEIGHT);

    for (int kButton = 0; kButton < nButtons; ++kButton)
    {
        int row = kButton / ROW_LENGTH;
        int col = kButton % ROW_LENGTH;

        ElectrodeButton* button = spikeChannelButtons[kButton];
        button->setBounds(MARGIN + col * BUTTON_WIDTH, MARGIN + row * BUTTON_HEIGHT,
            BUTTON_WIDTH, BUTTON_HEIGHT);

        spikeChannelCanvas->addAndMakeVisible(button);
    }    
}

bool MeanSpikeRateEditor::updateFloatLabel(Label* label, float min, float max,
    float defaultValue, float* out)
{
    const String& in = label->getText();
    float parsedFloat;
    try
    {
        parsedFloat = std::stof(in.toRawUTF8());
    }
    catch (const std::logic_error&)
    {
        label->setText(String(defaultValue), dontSendNotification);
        return false;
    }

    *out = jmax(min, jmin(max, parsedFloat));

    label->setText(String(*out), dontSendNotification);
    return true;
}
