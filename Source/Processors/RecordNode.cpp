/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2013 Open Ephys

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

#include "RecordNode.h"
#include "ProcessorGraph.h"
#include "../UI/EditorViewport.h"
#include "../UI/ControlPanel.h"

#define EVERY_ENGINE for(int eng = 0; eng < engineArray.size(); eng++) engineArray[eng]



#include "Channel.h"

RecordNode::RecordNode()
    : GenericProcessor("Record Node"),
      newDirectoryNeeded(true),  timestamp(0)
{

    isProcessing = false;
    isRecording = false;
    allFilesOpened = false;
    signalFilesShouldClose = false;

    signalFilesShouldClose = false;

    settings.numInputs = 2048;
    settings.numOutputs = 0;

    eventChannel = new Channel(this, 0);
    eventChannel->setType(EVENT_CHANNEL);
    recordingNumber = 0;


    spikeElectrodeIndex = 0;

    experimentNumber = 0;
    hasRecorded = false;

    // 128 inputs, 0 outputs
    setPlayConfigDetails(getNumInputs(),getNumOutputs(),44100.0,128);

}


RecordNode::~RecordNode()
{

}

void RecordNode::setChannel(Channel* ch)
{

    int channelNum = channelPointers.indexOf(ch);

    std::cout << "Record node setting channel to " << channelNum << std::endl;

    setCurrentChannel(channelNum);

    // for (int i = 0; i < con.size(); i++)
    // {

    // 	if (continuousChannels[i].nodeId == id &&
    // 		continuousChannels[i].chan == chan)
    // 	{
    // 		std::cout << "Found channel " << i << std::endl;
    // 		setCurrentChannel(i);
    // 		break;
    // 	}

    // }
}

void RecordNode::setChannelStatus(Channel* ch, bool status)
{

    //std::cout << "Setting channel status!" << std::endl;
    setChannel(ch);

    if (status)
        setParameter(2, 1.0f);
    else
        setParameter(2, 0.0f);

}

// void RecordNode::enableCurrentChannel(bool state)
// {
// 	continuousChannels[nextAvailableChannel].isRecording = state;
// }

void RecordNode::resetConnections()
{
    //std::cout << "Resetting connections" << std::endl;
    nextAvailableChannel = 0;
    wasConnected = false;
    spikeElectrodeIndex = 0;

    channelPointers.clear();
    eventChannelPointers.clear();
    spikeElectrodePointers.clear();

    EVERY_ENGINE->resetChannels();

}

void RecordNode::filenameComponentChanged(FilenameComponent* fnc)
{

    //std::cout << "Got a new file" << std::endl;
    dataDirectory = fnc->getCurrentFile();
    // std::cout << "File name: " << dataDirectory.getFullPathName();
    // if (dataDirectory.isDirectory())
    //     std::cout << " is a directory." << std::endl;
    // else
    //     std::cout << " is NOT a directory." << std::endl;

    //createNewDirectory();



}

void RecordNode::updateChannelName(int channelIndex, String newname)
{
/*  if (channelPointers[channelIndex] != nullptr && channelIndex < channelPointers.size())
    {
        channelPointers[channelIndex]->name = newname;
        updateFileName(channelPointers[channelIndex]);
    } else*/
    {
        // keep name and do the change when the pointer actually points to something... ?
        modifiedChannelNames.add(newname);
        modifiedChannelInd.add(channelIndex);
    }
}

void RecordNode::getChannelNamesAndRecordingStatus(StringArray &names, Array<bool> &recording)
{
    names.clear();
    recording.clear();

    for (int k = 0; k < channelPointers.size(); k++)
    {
        if (channelPointers[k] == nullptr)
        {
            names.add("not allocated");
            recording.add(false);

        }
        else
        {
            Channel *ch = channelPointers[k];
            String n = ch->name;
            names.add(n);
            recording.add(channelPointers[k]->getRecordState());
        }
    }
}

void RecordNode::addInputChannel(GenericProcessor* sourceNode, int chan)
{

    if (chan != getProcessorGraph()->midiChannelIndex)
    {

        int channelIndex = getNextChannel(false);

        channelPointers.add(sourceNode->channels[chan]);
        setPlayConfigDetails(channelIndex+1,0,44100.0,128);

        //   std::cout << channelIndex << std::endl;

        channelPointers[channelIndex]->recordIndex = channelIndex;

        EVERY_ENGINE->addChannel(channelIndex,channelPointers[channelIndex]);

    }
    else
    {

        for (int n = 0; n < sourceNode->eventChannels.size(); n++)
        {

            eventChannelPointers.add(sourceNode->eventChannels[n]);

        }

    }

}

void RecordNode::updateFileName(Channel* ch)
{
    // String filename = rootFolder.getFullPathName();
    // filename += rootFolder.separatorString;

    // if (!ch->getType() == EVENT_CHANNEL)
    // {
    //     filename += ch->nodeId;
    //     filename += "_";
    //     filename += ch->name;

    //     if (appendTrialNum)
    //     {
    //         filename += "_";
    //         filename += trialNum;
    //     }

    //     filename += ".continuous";
    // }
    // else
    // {
    //     filename += "all_channels.events";
    // }


    // ch->filename = filename;
    // ch->file = 0;


    //std::cout << "Updating " << filename << std::endl;

}

void RecordNode::updateTrialNumber()
{
    trialNum++;
}

void RecordNode::appendTrialNumber(bool t)
{
    appendTrialNum = t;
}


void RecordNode::createNewDirectory()
{
    std::cout << "Creating new directory." << std::endl;

    rootFolder = File(dataDirectory.getFullPathName() + File::separator + generateDirectoryName());
    newDirectoryNeeded = false;


}

String RecordNode::generateDirectoryName()
{
    Time calendar = Time::getCurrentTime();

    Array<int> t;
    t.add(calendar.getYear());
    t.add(calendar.getMonth()+1); // January = 0
    t.add(calendar.getDayOfMonth());
    t.add(calendar.getHours());
    t.add(calendar.getMinutes());
    t.add(calendar.getSeconds());

    String filename = getControlPanel()->getTextToPrepend();

    String datestring = "";

    for (int n = 0; n < t.size(); n++)
    {
        if (t[n] < 10)
            datestring += "0";

        datestring += t[n];

        if (n == 2)
            datestring += "_";
        else if (n < 5)
            datestring += "-";
    }

    getControlPanel()->setDateText(datestring);

    filename += datestring;
    filename += getControlPanel()->getTextToAppend();

    return filename;

}

String RecordNode::generateDateString()
{
    Time calendar = Time::getCurrentTime();

    String datestring;

    datestring += String(calendar.getDayOfMonth());
    datestring += "-";
    datestring += calendar.getMonthName(true);
    datestring += "-";
    datestring += String(calendar.getYear());
    datestring += " ";

    int hrs, mins, secs;
    hrs = calendar.getHours();
    mins = calendar.getMinutes();
    secs = calendar.getSeconds();

    datestring += hrs;

    if (mins < 10)
        datestring += 0;

    datestring += mins;

    if (secs < 0)
        datestring += 0;

    datestring += secs;

    return datestring;

}


void RecordNode::setParameter(int parameterIndex, float newValue)
{
    //editor->updateParameterButtons(parameterIndex);

    // 0 = stop recording
    // 1 = start recording
    // 2 = toggle individual channel (0.0f = OFF, anything else = ON)

    if (parameterIndex == 1)
    {

        isRecording = true;
        hasRecorded = true;
        // std::cout << "START RECORDING." << std::endl;

        if (newDirectoryNeeded)
        {
            createNewDirectory();
            recordingNumber = 0;
            experimentNumber = 1;
            EVERY_ENGINE->directoryChanged();
        }
        else
        {
            recordingNumber++; // increment recording number within this directory
        }

        if (!rootFolder.exists())
        {
            rootFolder.createDirectory();
            String settingsFileName = rootFolder.getFullPathName() + File::separator + "settings.xml";
            getEditorViewport()->saveState(File(settingsFileName));
        }

        EVERY_ENGINE->openFiles(rootFolder, experimentNumber, recordingNumber);

        allFilesOpened = true;

    }
    else if (parameterIndex == 0)
    {


        // std::cout << "STOP RECORDING." << std::endl;

        if (isRecording)
        {

            // close necessary files
            signalFilesShouldClose = true;

        }

        isRecording = false;


    }
    else if (parameterIndex == 2)
    {

        if (isProcessing)
        {

            std::cout << "Toggling channel " << currentChannel << std::endl;

            if (isRecording)
            {
                //Toggling channels while recording isn't allowed. Code shouldn't reach here.
                //In case it does, display an error and exit.
                sendActionMessage("Toggling record channels while recording is not allowed");
                std::cout << "ERROR: Wrong code section reached\n Toggling record channels while recording is not allowed." << std::endl;
                return;
            }

            if (newValue == 0.0f)
            {
                channelPointers[currentChannel]->setRecordState(false);
            }
            else
            {
                channelPointers[currentChannel]->setRecordState(true);
            }
        }
    }
}

void RecordNode::closeAllFiles()
{
    if (allFilesOpened)
    {
        EVERY_ENGINE->closeFiles();
        allFilesOpened = false;
    }
}

bool RecordNode::enable()
{
    if (hasRecorded)
    {
        hasRecorded = false;
        experimentNumber++;
    }

    //When starting a recording, if a new directory is needed it gets rewritten. Else is incremented by one.
    recordingNumber = -1;
    EVERY_ENGINE->startAcquisition();
    isProcessing = true;
    return true;
}


bool RecordNode::disable()
{
    // close files if necessary
    setParameter(0, 10.0f);

    if (isProcessing)
        closeAllFiles();

    isProcessing = false;

    return true;
}

float RecordNode::getFreeSpace()
{
    return 1.0f - float(dataDirectory.getBytesFreeOnVolume())/float(dataDirectory.getVolumeTotalSize());
}


void RecordNode::handleEvent(int eventType, MidiMessage& event, int samplePosition)
{
    if (eventType == TTL)
    {
        if (event.getNoteNumber() > 0) // processor ID > 0
        {
            EVERY_ENGINE->writeEvent(eventType, event, samplePosition);
        }
    }
    else if (eventType == TIMESTAMP)
    {
        const uint8* dataptr = event.getRawData();

        // // double-check buffer contents:s
        // std::cout << (int) *(dataptr + 11) << " " <<
        //             (int) *(dataptr + 10) << " " <<
        //             (int) *(dataptr + 9) << " " <<
        //             (int) *(dataptr + 8) << " " <<
        //             (int) *(dataptr + 7) << " " <<
        //             (int) *(dataptr + 6) << " " <<
        //             (int) *(dataptr + 5) << " " <<
        //             (int) *(dataptr + 4) << std::endl;

        memcpy(&timestamp, dataptr + 4, 8); // remember to skip first four bytes
        EVERY_ENGINE->updateTimeStamp(timestamp);
    }

}

void RecordNode::process(AudioSampleBuffer& buffer,
                         MidiBuffer& events,
                         int& nSamples)
{

    if (isRecording && allFilesOpened)
    {

        // FIRST: cycle through events -- extract the TTLs and the timestamps
        checkForEvents(events);

        // SECOND: write channel data
        if (channelPointers.size() > 0)
        {
            EVERY_ENGINE->writeData(buffer,nSamples);
        }

        //  std::cout << nSamples << " " << samplesWritten << " " << blockIndex << std::endl;

        return;

    }

    // this is intended to prevent parameter changes from closing files
    // before recording stops
    if (signalFilesShouldClose)
    {
        closeAllFiles();
        signalFilesShouldClose = false;
    }

}

void RecordNode::registerProcessor(GenericProcessor* sourceNode)
{
    EVERY_ENGINE->registerProcessor(sourceNode);
}

Channel* RecordNode::getDataChannel(int index)
{
    return channelPointers[index];
}

void RecordNode::registerRecordEngine(RecordEngine* engine)
{
    engineArray.add(engine);
}

void RecordNode::registerSpikeSource(GenericProcessor* processor)
{
    EVERY_ENGINE->registerSpikeSource(processor);
}

int RecordNode::addSpikeElectrode(SpikeRecordInfo* elec)
{
    elec->recordIndex=spikeElectrodeIndex;
    spikeElectrodePointers.add(elec);
    EVERY_ENGINE->addSpikeElectrode(spikeElectrodeIndex,elec);
    return spikeElectrodeIndex++;
}

void RecordNode::writeSpike(SpikeObject& spike, int electrodeIndex)
{
    EVERY_ENGINE->writeSpike(spike,electrodeIndex);
}

SpikeRecordInfo* RecordNode::getSpikeElectrode(int index)
{
    return spikeElectrodePointers[index];
}

void RecordNode::clearRecordEngines()
{
	engineArray.clear();
}
