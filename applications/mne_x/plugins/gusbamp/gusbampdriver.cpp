//=============================================================================================================
/**
* @file     gusbampdriver.cpp
* @author   Viktor Klüber <viktor.klueber@tu-ilmenau.de>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Viktor Klüber, Lorenz Esch and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the GUSBAmpDriver class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QtCore>
#include <QtCore/QcoreApplication>
#include <QDebug>
#include "gusbampdriver.h"
#include "gusbampproducer.h"
#include <iostream>
#include <Windows.h>
#include <deque>
#include <stdarg.h>




//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace GUSBAmpPlugin;
using namespace std;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmpDriver::GUSBAmpDriver(GUSBAmpProducer* pGUSBAmpProducer)
: m_pGUSBAmpProducer(pGUSBAmpProducer)
,m_SLAVE_SERIALS_SIZE(0)
,m_SAMPLE_RATE_HZ(1200)
,m_NUMBER_OF_SCANS(256)
,m_NUMBER_OF_CHANNELS(5)
,m_TRIGGER(FALSE)
,m_QUEUE_SIZE(4)
,m_mode(M_NORMAL)
,m_commonReference({ FALSE, FALSE, FALSE, FALSE })
,m_commonGround({ FALSE, FALSE, FALSE, FALSE })
,m_nPoints(m_NUMBER_OF_SCANS * (m_NUMBER_OF_CHANNELS + m_TRIGGER))
,m_bufferSizeBytes(HEADER_SIZE + m_nPoints * sizeof(float))
,m_numBytesReceived(0)
,m_isRunning(false)
{

    //Linking the specific API-library to the project
    #ifdef _WIN64
        #pragma comment(lib, __FILE__"\\..\\gUSBamp_x64.lib")
    #else
        #pragma comment(lib, __FILE__"\\..\\gUSBamp_x86.lib")
    #endif


    //initializing a deque-list of the serial numbers to be called (LPSTR)
    m_masterSerial       = LPSTR("UB-2015.05.16");
    m_slaveSerials[0]    = LPSTR("UB-2010.03.43");
    m_slaveSerials[1]    = LPSTR("UB-2010.03.44");
    m_slaveSerials[2]    = LPSTR("UB-2010.03.47");
    for (int i=0; i<m_SLAVE_SERIALS_SIZE; i++)
        m_callSequenceSerials.push_back(m_slaveSerials[i]);
    //add the master device at the end of the list!
    m_callSequenceSerials.push_back(m_masterSerial);


    //initializing UCHAR-list of channels to acquire
    for(int i = 0; i < m_NUMBER_OF_CHANNELS; i++)
        m_channelsToAcquire[i] = UCHAR(i+1);

//    float version = float(GT_GetDriverVersion());

//    qDebug() << "gUSBamp-driver version " << version << "was found";
//    qDebug() <<endl<< "gUSBampdriver-contructor build successfully " << endl;

    //define Name and place of written data-file
     m_file.setFileName("d:/Clouds/OneDrive/Studium/Master/Masterarbeit/testing/gUSBamp/driver/data.txt");

}


//*************************************************************************************************************

GUSBAmpDriver::~GUSBAmpDriver()
{

    //qDebug() << "GUSBAmpDriver::~GUSBAmpDriver()" << endl;
}


//*************************************************************************************************************

bool GUSBAmpDriver::initDevice()
{


    /*Space for setting Main-variables
    *
    *
    *
    */


    m_isRunning =true;

    try
    {
        for (deque<LPSTR>::iterator serialNumber = m_callSequenceSerials.begin(); serialNumber != m_callSequenceSerials.end(); serialNumber++)
        {
            //open the device
            HANDLE hDevice = GT_OpenDeviceEx(*serialNumber);

            if (hDevice == NULL)
                throw string("Error on GT_OpenDeviceEx: Couldn't open device ").append(*serialNumber);

            //add the device handle to the list of opened devices
            m_openedDevicesHandles.push_back(hDevice);

            //set the channels from that data should be acquired
            if (!GT_SetChannels(hDevice, m_channelsToAcquire, m_NUMBER_OF_CHANNELS))
                throw string("Error on GT_SetChannels: Couldn't set channels to acquire for device ").append(*serialNumber);

            //set the sample rate
            if (!GT_SetSampleRate(hDevice, m_SAMPLE_RATE_HZ))
                throw string("Error on GT_SetSampleRate: Couldn't set sample rate for device ").append(*serialNumber);

            //disable the trigger line
            if (!GT_EnableTriggerLine(hDevice, m_TRIGGER))
                throw string("Error on GT_EnableTriggerLine: Couldn't enable/disable trigger line for device ").append(*serialNumber);

            //set the number of scans that should be received simultaneously
            if (!GT_SetBufferSize(hDevice, m_NUMBER_OF_SCANS))
                throw string("Error on GT_SetBufferSize: Couldn't set the buffer size for device ").append(*serialNumber);

            //don't use bandpass and notch for each channel
            for (int i=0; i<m_NUMBER_OF_CHANNELS; i++)
            {
                //don't use a bandpass filter for any channel
                if (!GT_SetBandPass(hDevice, m_channelsToAcquire[i], -1))
                    throw string("Error on GT_SetBandPass: Couldn't set no bandpass filter for device ").append(*serialNumber);

                //don't use a notch filter for any channel
                if (!GT_SetNotch(hDevice, m_channelsToAcquire[i], -1))
                    throw string("Error on GT_SetNotch: Couldn't set no notch filter for device ").append(*serialNumber);
            }

            //determine master device as the last device in the list
            bool isSlave = (*serialNumber != m_callSequenceSerials.back());

            //set slave/master mode of the device
            if (!GT_SetSlave(hDevice, isSlave))
                throw string("Error on GT_SetSlave: Couldn't set slave/master mode for device ").append(*serialNumber);

            //disable shortcut function
            if (!GT_EnableSC(hDevice, false))
                throw string("Error on GT_EnableSC: Couldn't disable shortcut function for device ").append(*serialNumber);

            //set unipolar derivation
            m_bipolarSettings.Channel1 = 0;
            m_bipolarSettings.Channel2 = 0;
            m_bipolarSettings.Channel3 = 0;
            m_bipolarSettings.Channel4 = 0;
            m_bipolarSettings.Channel5 = 0;
            m_bipolarSettings.Channel6 = 0;
            m_bipolarSettings.Channel7 = 0;
            m_bipolarSettings.Channel8 = 0;
            m_bipolarSettings.Channel9 = 0;
            m_bipolarSettings.Channel10 = 0;
            m_bipolarSettings.Channel11 = 0;
            m_bipolarSettings.Channel12 = 0;
            m_bipolarSettings.Channel13 = 0;
            m_bipolarSettings.Channel14 = 0;
            m_bipolarSettings.Channel15 = 0;
            m_bipolarSettings.Channel16 = 0;

            if (!GT_SetBipolar(hDevice, m_bipolarSettings))
                throw string("Error on GT_SetBipolar: Couldn't set unipolar derivation for device ").append(*serialNumber);

            if (m_mode == M_COUNTER)
                if (!GT_SetMode(hDevice, M_NORMAL))
                    throw string("Error on GT_SetMode: Couldn't set mode M_NORMAL (before mode M_COUNTER) for device ").append(*serialNumber);

            //set acquisition mode
            if (!GT_SetMode(hDevice, m_mode))
                throw string("Error on GT_SetMode: Couldn't set mode for device ").append(*serialNumber);

            //for g.USBamp devices set common ground and common reference
            if (strncmp(*serialNumber, "U", 1) == 0 && (m_mode == M_NORMAL || m_mode == M_COUNTER))
            {
                //don't connect the 4 groups to common reference
                if (!GT_SetReference(hDevice, m_commonReference))
                    throw string("Error on GT_SetReference: Couldn't set common reference for device ").append(*serialNumber);

                //don't connect the 4 groups to common ground
                if (!GT_SetGround(hDevice, m_commonGround))
                    throw string("Error on GT_SetGround: Couldn't set common ground for device ").append(*serialNumber);
            }

            printf("\tg.USBamp %s initialized as %s (#%d in the call sequence)!\n", *serialNumber, (isSlave) ? "slave" : "master", m_openedDevicesHandles.size());

        }

        //define the buffer variables and start the device:
        //create _callSequenceHandles for the sequence of calling the devices (Master has to be the last device to be called!)
        m_callSequenceHandles = m_openedDevicesHandles;
        m_numDevices = (int) m_callSequenceHandles.size();

        m_buffers     = new BYTE**[m_numDevices];
        m_overlapped  = new OVERLAPPED*[m_numDevices];

        //for each device create a number of QUEUE_SIZE data buffers
        for (int deviceIndex=0; deviceIndex<m_numDevices; deviceIndex++)
        {
            m_buffers[deviceIndex] = new BYTE*[m_QUEUE_SIZE];
            m_overlapped[deviceIndex] = new OVERLAPPED[m_QUEUE_SIZE];

            //for each data buffer allocate a number of bufferSizeBytes bytes
            for (int queueIndex=0; queueIndex<m_QUEUE_SIZE; queueIndex++)
            {
                m_buffers[deviceIndex][queueIndex] = new BYTE[m_bufferSizeBytes];
                memset(&(m_overlapped[deviceIndex][queueIndex]), 0, sizeof(OVERLAPPED));

                //create a windows event handle that will be signalled when new data from the device has been received for each data buffer
                m_overlapped[deviceIndex][queueIndex].hEvent = CreateEvent(NULL, false, false, NULL);
            }
        }

        //opening the file for data writing and establish data-stream
        m_file.open(QIODevice::WriteOnly | QIODevice::Text );
        m_stream.setDevice(&m_file);

        //start the devices (master device must be started at last)
        for (int deviceIndex=0; deviceIndex<m_numDevices; deviceIndex++)
        {
            HANDLE hDevice = m_callSequenceHandles[deviceIndex];

            if (!GT_Start(hDevice))
            {
                //throw string("Error on GT_Start: Couldn't start data acquisition of device.");
                cout << "\tError on GT_Start: Couldn't start data acquisition of device.\n";
                return 0;

            }
            //queue-up the first batch of transfer requests
            for (int queueIndex=0; queueIndex<m_QUEUE_SIZE; queueIndex++)
            {
                if (!GT_GetData(hDevice, m_buffers[deviceIndex][queueIndex], m_bufferSizeBytes, &m_overlapped[deviceIndex][queueIndex]))
                {
                    cout << "\tError on GT_GetData.\n";
                    return 0;
                }
            }
        }

        qDebug() << "Plugin GUSBAmp - INFO - initDevice() - The device has been connected and initialised successfully" << endl;

        return true;

    }
    catch (string& exception)
    {

        //in case an exception occurred, close all opened devices...
        while(!m_openedDevicesHandles.empty())
        {
            GT_CloseDevice(&m_openedDevicesHandles.front());
            qDebug() << "error occurred - Device " << &m_openedDevicesHandles.front()  << "was closed" << endl;
            m_openedDevicesHandles.pop_front();
        }

        cout << exception << '\n';

        return false;

    }
}


//*************************************************************************************************************

bool GUSBAmpDriver::uninitDevice()
{

    cout << "Stopping devices and cleaning up..." << "\n";

    //clean up allocated resources for each device
    for (int i=0; i<m_numDevices; i++)
    {
        HANDLE hDevice = m_callSequenceHandles[i];

        //clean up allocated resources for each queue per device
        for (int j=0; j<m_QUEUE_SIZE; j++)
        {
            WaitForSingleObject(m_overlapped[i][j].hEvent, 1000);
            CloseHandle(m_overlapped[i][j].hEvent);
            delete [] m_buffers[i][j];
            qDebug()<< "deleted queue buffer" << j << "successfully";
        }

        //stop device
        GT_Stop(hDevice);
        qDebug() << "stopped " << QString(m_callSequenceSerials[i])<< " successfully" << endl;

        //reset device
        GT_ResetTransfer(hDevice);
        qDebug() << "reseted Transfer of " << QString(m_callSequenceSerials[i])<< " successfully" << endl;

        delete [] m_overlapped[i];
        delete [] m_buffers[i];
    }

    delete [] m_buffers;
    delete [] m_overlapped;

    //closing all devices from the Call-Sequence-Handle
    while (!m_callSequenceHandles.empty())
    {
        //closes each opened device and removes it from the call sequence
        GT_CloseDevice(&m_callSequenceHandles.front());
        m_callSequenceHandles.pop_front();
    }

    //closes all openend Device-Handles
    while(!m_openedDevicesHandles.empty())
    {
        GT_CloseDevice(&m_openedDevicesHandles.front());
        m_openedDevicesHandles.pop_front();
    }

    //close the data file
    m_file.close();

    m_isRunning = false;

    qDebug() << "Plugin GUSBAmp - INFO - uninitDevice() - Successfully uninitialised the device" << endl;

    return true;
}


//*************************************************************************************************************


bool GUSBAmpDriver::getSampleMatrixValue(MatrixXf& sampleMatrix)
{
    sampleMatrix.setZero(); // Clear matrix - set all elements to zero

    for(int queueIndex=0; queueIndex<m_QUEUE_SIZE; queueIndex++)
    {

        //receive data from each device
        for (int deviceIndex = 0; deviceIndex < m_numDevices; deviceIndex++)
        {
            HANDLE hDevice = m_callSequenceHandles[deviceIndex];

            //wait for notification from the system telling that new data is available
            if (WaitForSingleObject(m_overlapped[deviceIndex][queueIndex].hEvent, 1000) == WAIT_TIMEOUT)
            {
                //throw string("Error on data transfer: timeout occurred.");
                cout << "Error on data transfer: timeout occurred." << "\n";
                return 0;
            }

            //get number of received bytes...
            GetOverlappedResult(hDevice, &m_overlapped[deviceIndex][queueIndex], &m_numBytesReceived, false);

            //...and check if we lost something (number of received bytes must be equal to the previously allocated buffer size)
            if (m_numBytesReceived != m_bufferSizeBytes)
            {
                //throw string("Error on data transfer: samples lost.");
                cout << "Error on data transfer: samples lost." << "\n";
                return 0;
            }
        }

        //store received data from each device in the correct order (that is scan-wise, where one scan includes all channels of all devices) ignoring the header
        //Data is aligned as follows: element at position destBuffer[scanIndex * (numberOfChannelsPerDevice * numDevices) + channelIndex] is sample of channel channelIndex (zero-based) of the scan with zero-based scanIndex.
        //channelIndex ranges from 0..numDevices*numChannelsPerDevices where numDevices equals the number of recorded devices and numChannelsPerDevice the number of channels from each of those devices.
        //It is assumed that all devices provide the same number of channels.
        for (int scanIndex = 0; scanIndex < m_NUMBER_OF_SCANS; scanIndex++)
        {
            for (int deviceIndex = 0; deviceIndex < m_numDevices; deviceIndex++)
            {
                for(int channelIndex = 0; channelIndex<m_NUMBER_OF_CHANNELS; channelIndex++)
                {
                    BYTE ByteValue[sizeof(float)];
                    float   FloatValue;

                    for(int i=0;i<sizeof(float);i++)
                    {
                        ByteValue[i] = m_buffers[deviceIndex][queueIndex][(scanIndex * (m_NUMBER_OF_CHANNELS + m_TRIGGER) + channelIndex) * sizeof(float) + HEADER_SIZE + i];
                    }
                    memcpy(&FloatValue, &ByteValue, sizeof(float));

                    //FloatValue im stream ablegen
                    m_stream << FloatValue<< "\t";
                    //FloatValue in Matrix abspeichern
                    sampleMatrix(channelIndex*(1+deviceIndex),scanIndex*(1+queueIndex)) = FloatValue;
                }
            }
            m_stream << "\n";
        }

        //add new GetData call to the queue replacing the currently received one
        for (int deviceIndex = 0; deviceIndex < m_numDevices; deviceIndex++)
            if (!GT_GetData(m_callSequenceHandles[deviceIndex], m_buffers[deviceIndex][queueIndex], m_bufferSizeBytes, &m_overlapped[deviceIndex][queueIndex]))
            {
                cout << "\tError on GT_GetData.\n";
                return 0;
            }
    }


    return true;
}


//*************************************************************************************************************


void GUSBAmpDriver::setSerials(LPSTR master,
                               LPSTR slave1 = LPSTR("is_empty"),
                               LPSTR slave2 = LPSTR("is_empty"),
                               LPSTR slave3 = LPSTR("is_empty"))
{

    //closes the call-sequence-list
    while (!m_callSequenceSerials.empty())
    {
        m_callSequenceSerials.pop_front();
    }

    //defining the new serials
    m_masterSerial      = master;
    m_slaveSerials[0]   = slave1;
    m_slaveSerials[1]   = slave2;
    m_slaveSerials[2]   = slave3;

    //defining the size of the slave serials
    for (int i = 0; (m_slaveSerials[i] !=LPSTR("is_empty")) && (i<3); i++)
        m_SLAVE_SERIALS_SIZE = i;

    //defining the new deque-list for data acquisition
    for (int i=0; i<m_SLAVE_SERIALS_SIZE; i++)
        m_callSequenceSerials.push_back(m_slaveSerials[i]);
    //add the master device at the end of the list!
    m_callSequenceSerials.push_back(m_masterSerial);

}

//*************************************************************************************************************

bool GUSBAmpDriver::setSampleRate(int sampleRate)
{
    try
    {
        if(m_isRunning)
            throw string("Do not change device-parameters while running the device!\n");


        switch(sampleRate)
        {
        case 32:    m_NUMBER_OF_SCANS = 1;      break;
        case 64:    m_NUMBER_OF_SCANS = 2;      break;
        case 128:   m_NUMBER_OF_SCANS = 4;      break;
        case 256:   m_NUMBER_OF_SCANS = 8;      break;
        case 512:   m_NUMBER_OF_SCANS = 16;     break;
        case 600:   m_NUMBER_OF_SCANS = 32;     break;
        case 1200:  m_NUMBER_OF_SCANS = 64;     break;
        case 2400:  m_NUMBER_OF_SCANS = 128;    break;
        case 4800:  m_NUMBER_OF_SCANS = 256;    break;
        case 9600:  m_NUMBER_OF_SCANS = 512;    break;
        case 19200: m_NUMBER_OF_SCANS = 512;    break;
        case 38400: m_NUMBER_OF_SCANS = 512;    break;
        default: throw string("Error on setSampleRate: please choose between following options:\n32, 64, 128, 256, 512, 600, 1200, 2400, 4800, 9600, 19200 and 38400\n\n:"); break;
        }
        m_SAMPLE_RATE_HZ = sampleRate;
        qDebug()<<"Sample Rate is setted"<<endl;
    }
    catch(string& exception)
    {
        cout << exception << '\n';
        return false;
    }

    return true;

}

//*************************************************************************************************************

bool GUSBAmpDriver::setChannels(vector<int> &list)
{
    if(m_isRunning)
    {
        cout << "Do not change device-parameters while running the device!\n";
        return false;
    }

    int numargin = list.size();
    if (numargin > 16)
    {
        cout << "ERROR: Could not set channels. Size of channel-vector has to be less then 16\n";
        return false;
    }


    //checking if value values of vector are ascending
    int i = 0;
    do
        i++;
    while ((i < numargin) && (list[i] > list[i - 1]));
    if (i != numargin)
    {
        cout << "ERROR: values of the channels in the vector have to be ascending\n";
        return false;
    }

    for(int i = 0; i < numargin; i++)
         m_channelsToAcquire[i] = UCHAR(list[i]);

    m_NUMBER_OF_CHANNELS = numargin;

    return true;

}



