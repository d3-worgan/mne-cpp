//=============================================================================================================
/**
 * @file     rtfwd.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RtFwd class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwd.h"

#include <fwd/computeFwd/compute_fwd.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

#include <mne/mne_forwardsolution.h>

#include <scMeas/realtimehpiresult.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace FWDLIB;
using namespace FIFFLIB;
using namespace INVERSELIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwd::RtFwd()
    : m_bUpdateHeadPos(false)
    , m_pFiffInfo(new FiffInfo)
    , m_pCircularBuffer(CircularBuffer_Matrix_double::SPtr::create(40))
    , m_pFwdSettings(new ComputeFwdSettings)
{
    // set init values
    m_pFwdSettings->solname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/Result/sample_audvis-meg-eeg-oct-6-fwd.fif";
    m_pFwdSettings->mriname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/MEG/sample/all-trans.fif";
    m_pFwdSettings->bemname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-1280-1280-1280-bem.fif";
    m_pFwdSettings->srcname = QCoreApplication::applicationDirPath() + "/mne-cpp-test-data/subjects/sample/bem/sample-oct-6-src.fif";
    m_pFwdSettings->include_meg = true;
    m_pFwdSettings->include_eeg = true;
    m_pFwdSettings->accurate = true;
    m_pFwdSettings->mindist = 5.0f/1000.0f;
}

//=============================================================================================================

RtFwd::~RtFwd()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> RtFwd::clone() const
{
    QSharedPointer<RtFwd> pRtFwdClone(new RtFwd);
    return pRtFwdClone;
}

//=============================================================================================================

void RtFwd::init()
{
    // Inits


    // Input
    m_pHpiInput = PluginInputData<RealTimeHpiResult>::create(this, "rtFwdIn", "rtFwd input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &RtFwd::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pHpiInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "rtFwdOut", "rtFwd output data");
    m_pOutput->data()->setName(this->getName());
    m_outputConnectors.append(m_pOutput);
}

//=============================================================================================================

void RtFwd::unload()
{
}

//=============================================================================================================

bool RtFwd::start()
{
    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool RtFwd::stop()
{
    requestInterruption();
    wait(500);

    // Clear all data in the buffer connected to displays and other plugins
    m_pOutput->data()->clear();
    m_pCircularBuffer->clear();

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

IPlugin::PluginType RtFwd::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString RtFwd::getName() const
{
    return "Real-Time Forward Solution";
}

//=============================================================================================================

QWidget* RtFwd::setupWidget()
{
    RtFwdSetupWidget* setupWidget = new RtFwdSetupWidget(this);
    return setupWidget;
}

//=============================================================================================================

void RtFwd::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeHpiResult> pRTHPI = pMeasurement.dynamicCast<RealTimeHpiResult>()) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTHPI->getFiffInfo();
            m_transDevHead = pRTHPI->getValue()->devHeadTrans;

//            m_mutex.lock();
//            m_bUpdateHeadPos = true;
//            m_mutex.unlock();
        }

        m_mutex.lock();
        m_pHpiFitResult = pRTHPI->getValue();
        m_mutex.unlock();

        checkHeadDisplacement();

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

//        for(unsigned char i = 0; i < pRTHPI->getMultiArraySize(); ++i) {
//            // Please note that we do not need a copy here since this function will block until
//            // the buffer accepts new data again. Hence, the data is not deleted in the actual
//            // Mesaurement function after it emitted the notify signal.
//            while(!m_pCircularBuffer->push(pRTHPI->getMultiSampleArray()[i])) {
//                //Do nothing until the circular buffer is ready to accept new data again
//            }
//        }
    }
}

//=============================================================================================================

void RtFwd::checkHeadDisplacement()
{
    m_mutex.lock();
    float fThreshMove = m_fThreshMove;      // threshold for movement
    float fThreshRot = m_fThreshRot;        // threshold for rotation
    float fAngle = m_transDevHead.angleTo(m_pHpiFitResult->devHeadTrans.trans);
    float fMove = m_transDevHead.translationTo(m_pHpiFitResult->devHeadTrans.trans);
    m_mutex.unlock();

    if(fAngle > fThreshRot || fMove > fThreshMove) {
        m_mutex.lock();
        m_bUpdateHeadPos = true;
        m_transDevHead = m_pHpiFitResult->devHeadTrans;
        m_mutex.unlock();
    }
}

//=============================================================================================================

void RtFwd::onThresholdRotationChanged(double dThreshRot)
{
    m_mutex.lock();
    m_fThreshRot = dThreshRot;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::onThresholdMovementChanged(double dThreshMove)
{
    m_mutex.lock();
    m_fThreshMove = dThreshMove;
    m_mutex.unlock();
}

//=============================================================================================================

void RtFwd::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Quick control widget here ?

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void RtFwd::run()
{
    MatrixXd matData;

    // Wait for fiff info
    while(true) {
        m_mutex.lock();
        if(m_pFiffInfo) {
            m_mutex.unlock();
            break;
        }
        m_mutex.unlock();
        msleep(100);
    }

    m_pFwdSettings->pFiffInfo = m_pFiffInfo;

    m_mutex.lock();
    FiffCoordTransOld transMegHeadOld = m_transDevHead.toOld();
    m_mutex.unlock();

    // Compute initial Forward solution
    ComputeFwd::SPtr pComputeFwd = ComputeFwd::SPtr(new ComputeFwd(m_pFwdSettings));
    pComputeFwd->calculateFwd();
    pComputeFwd->storeFwd();

    // get Mne Forward Solution (in future this is not necessary, ComputeForward will have this as member)
    QFile t_fSolution(m_pFwdSettings->solname);
    m_pFwdSolution = MNEForwardSolution::SPtr(new MNEForwardSolution(t_fSolution));

    while(!isInterruptionRequested()) {
        // Get the current data
        m_mutex.lock();
        if(m_bUpdateHeadPos) {
            transMegHeadOld = m_transDevHead.toOld();
            m_mutex.unlock();
            pComputeFwd->updateHeadPos(&transMegHeadOld);
            m_pFwdSolution->sol = pComputeFwd->sol;
            m_pFwdSolution->sol_grad = pComputeFwd->sol_grad;
        }
        m_mutex.lock();
        m_bUpdateHeadPos = false;
        m_mutex.unlock();

        //Send the data to the connected plugins and the online display
        //Unocmment this if you also uncommented the m_pOutput in the constructor above
        if(!isInterruptionRequested()) {
            m_pOutput->data()->setValue(matData);
        }

    }
}