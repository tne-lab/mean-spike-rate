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

#ifndef MEAN_SPIKE_RATE_H_INCLUDED
#define MEAN_SPIKE_RATE_H_INCLUDED

#include <ProcessorHeaders.h>

/* Estimates the mean spike rate over time and channels. Uses an exponentially
 * weighted moving average to estimate a temporal mean (with adjustable time
 * constant), and averages the rate across selected spike channels (electrodes).
 * Outputs the resulting rate onto a selected continuous channel (overwriting its contents).
 *
 * @see GenericProcessor
 */

class MeanSpikeRateSettings {
public:
    float timeConstMs;
    int outputChan;
};

class MeanSpikeRate : public GenericProcessor
{
    friend class MeanSpikeRateEditor;

public:
    MeanSpikeRate();
    ~MeanSpikeRate();

    bool hasEditor() const { return true; }
    AudioProcessorEditor* createEditor() override;

    void process(AudioSampleBuffer& continuousBuffer) override;
    void handleSpike(SpikePtr spike) override;

    void parameterValueChanged(Parameter* param) override;

    // Stores/loads spike channel selection state. Output channel and time constant are handled automatically
    void loadCustomParametersFromXml(XmlElement* parentElement) override;
    void saveCustomParametersToXml(XmlElement* parentElement) override;

private:
    // functions
    int getNumActiveElectrodes();
    void updateSettings() override;;

    // internals
    StreamSettings<MeanSpikeRateSettings> settings;
    int currSample;          // per-buffer - allows processing samples while handling events
    double spikeAmp;         // updated once per buffer
    double decayPerSample;   // updated once per buffer
    float currMean;
    float* wpBuffer;

    const String OUTPUT_TOOLTIP = "Continuous channel to overwrite with the spike rate (meaned over time and selected electrodes)";
    const String TIME_CONST_TOOLTIP = "Time for the influence of a single spike to decay to 36.8% (1/e) of its initial value (larger = smoother, smaller = faster reaction to changes)";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MeanSpikeRate);
};

#endif // MEAN_SPIKE_RATE_H_INCLUDED