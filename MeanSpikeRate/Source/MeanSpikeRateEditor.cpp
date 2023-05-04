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
    : GenericEditor(parentNode)
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

    addSelectedChannelsParameterEditor("Output", 10, yPos + TEXT_HEIGHT);
    addTextBoxParameterEditor("Time_Const", 100, yPos);
}

MeanSpikeRateEditor::~MeanSpikeRateEditor() {}

void MeanSpikeRateEditor::updateSettings()
{
    MeanSpikeRate* processor = static_cast<MeanSpikeRate*>(getProcessor());

    // update electrode buttons
    auto& spikeChannelArray = processor->spikeChannels;

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

    button->setButtonText(prefix + String(chan->getLocalIndex()));
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
