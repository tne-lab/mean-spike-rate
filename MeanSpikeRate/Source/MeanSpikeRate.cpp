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

#include "MeanSpikeRate.h"
#include "MeanSpikeRateEditor.h"

MeanSpikeRate::MeanSpikeRate()
    : GenericProcessor          ("Mean Spike Rate")
    , outputChan                (0)
    , timeConstMs               (1000.0)
    , currSample                (0)
    , currMean                  (0.0f)
    , wpBuffer                  (nullptr)
{
    setProcessorType(PROCESSOR_TYPE_FILTER);
}

MeanSpikeRate::~MeanSpikeRate() {}

AudioProcessorEditor* MeanSpikeRate::createEditor()
{
    editor = new MeanSpikeRateEditor(this);
    return editor;
}

void MeanSpikeRate::process(AudioSampleBuffer& continuousBuffer)
{
    int numSamples;
    if (getNumInputs() == 0 || (numSamples = getNumSamples(outputChan)) == 0)
    {
        return;
    }

    // update algorithm parameters
    // we assume each spike channel has the same sample rate as the selected channel.
    // if not, this would get a lot more complicated.
    int numActiveElectrodes = getNumActiveElectrodes();
    if (numActiveElectrodes == 0)
    {
        return;
    }
    double timeConstSec = timeConstMs / 1000.0;
    double timeConstSamp = timeConstSec * getDataChannel(outputChan)->getSampleRate();
    decayPerSample = exp(-1 / timeConstSamp);
    
    // the initial amplitude of each spike such that if there is a steady rate of
    // spiking, the average over time of the exponentially weighted mean
    // (at the limit where the process has been continuing forever)
    // equals the actual spike rate in Hz. This is just 1 / (time const in sec).
    spikeAmp = 1 / (timeConstSec * numActiveElectrodes);

    // initialize first sample
    currSample = 0;
    wpBuffer = continuousBuffer.getWritePointer(outputChan);

    // handle each spike, calculating the mean spike rate of samples in between.
    checkForEvents(true);

    // after all spikes are handled, finish writing samples
    for (int samp = currSample; samp < numSamples; ++samp)
    {
        wpBuffer[samp] = currMean;
        currMean *= decayPerSample;
    }
}

void MeanSpikeRate::handleSpike(const SpikeChannel* spikeInfo, const MidiMessage& event, int samplePosition)
{
    if (!channelIsActive(spikeInfo, event))
    {
        return;
    }

    jassert(samplePosition >= currSample); // spike sample must not have already been finished

    // write samples up to the spike position
    for (int samp = currSample; samp < samplePosition; ++samp)
    {
        wpBuffer[samp] = currMean;
        currMean *= decayPerSample;
    }
    currSample = samplePosition;

    // add spike contribution
    currMean += spikeAmp;
}

void MeanSpikeRate::setParameter(int parameterIndex, float newValue)
{
    switch (parameterIndex)
    {
    case OUTPUT_CHAN:
        outputChan = static_cast<int>(newValue);
        break;

    case TIME_CONST:
        timeConstMs = newValue;
        break;

    default:
        jassertfalse;
        break;
    }
}

void MeanSpikeRate::saveCustomChannelParametersToXml(XmlElement* channelElement, int channelNumber, InfoObjectCommon::InfoObjectType channelType)
{
    if (channelType == InfoObjectCommon::SPIKE_CHANNEL)
    {
        auto msrEditor = static_cast<MeanSpikeRateEditor*>(getEditor());
        channelElement->setAttribute("enabled", msrEditor->getSpikeChannelEnabled(channelNumber));
    }
}

void MeanSpikeRate::loadCustomParametersFromXml()
{
    // semi-hack: update signal chain to make sure input spike channels have been created.
    CoreServices::updateSignalChain(nullptr);
}

void MeanSpikeRate::loadCustomChannelParametersFromXml(XmlElement* channelElement, InfoObjectCommon::InfoObjectType channelType)
{
    if (channelType == InfoObjectCommon::SPIKE_CHANNEL)
    {
        int channelNumber = channelElement->getIntAttribute("number", -1);
        bool shouldEnable = channelElement->getBoolAttribute("enabled");
        auto msrEditor = static_cast<MeanSpikeRateEditor*>(getEditor());
        msrEditor->setSpikeChannelEnabled(channelNumber, shouldEnable);
    }
}

// private

int MeanSpikeRate::getNumActiveElectrodes()
{
    auto editor = static_cast<MeanSpikeRateEditor*>(getEditor());
    return editor->getNumActiveElectrodes();
}

bool MeanSpikeRate::channelIsActive(const SpikeChannel* info, const MidiMessage& event)
{
    SpikeEventPtr deserializedEvent = SpikeEvent::deserializeFromMessage(event, info);
    int channelIndex = getSpikeChannelIndex(deserializedEvent);
    
    jassert(getEditor());
    auto msrEditor = static_cast<MeanSpikeRateEditor*>(getEditor());
    return  msrEditor->getSpikeChannelEnabled(channelIndex);
}