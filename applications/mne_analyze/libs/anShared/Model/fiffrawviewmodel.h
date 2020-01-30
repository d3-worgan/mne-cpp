//=============================================================================================================
/**
 * @file     fiffrawviewmodel.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @version  dev
 * @date     October, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief     FiffRawViewModel class declaration.
 *
 */

#ifndef ANSHAREDLIB_FIFFRAWVIEWMODEL_H
#define ANSHAREDLIB_FIFFRAWVIEWMODEL_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"
#include "abstractmodel.h"

#include <list>

#include <fiff/fiff.h>
#include <fiff/fiff_io.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QFutureWatcher>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
    class FiffChInfo;
}

class QFile;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB {


//*************************************************************************************************************
//=============================================================================================================
// ANSHAREDLIB FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
 *
 * @brief Model that holds and manages raw fiff data.
 */
class ANSHAREDSHARED_EXPORT FiffRawViewModel : public AbstractModel
{
    Q_OBJECT

public:
    typedef QSharedPointer<FiffRawViewModel> SPtr;              /**< Shared pointer type for FiffRawViewModel. */
    typedef QSharedPointer<const FiffRawViewModel> ConstSPtr;   /**< Const shared pointer type for FiffRawViewModel. */

    //=========================================================================================================
    /**
     * Constructs a FiffRawViewModel object.
     */
    FiffRawViewModel(QObject *pParent = nullptr);

    //=========================================================================================================
    /**
     * Constructs a FiffRawViewModel object.
     */
    FiffRawViewModel(const QString &sFilePath,
                     qint32 iVisibleWindowSize = 10,
                     qint32 iPreloadBufferSize = 1,
                     QObject *pParent = nullptr);

    //=========================================================================================================
    /**
     * Destructs a FiffRawViewModel.
     */
    ~FiffRawViewModel();

    //=========================================================================================================
    /**
     * Helper function for initialization
     */
    void initFiffData();

    //=========================================================================================================
    /**
     * Returns the data stored under the given role for the index.
     * Currently only Qt::DisplayRole is supported.
     * Index rows reflect channels, first column is channel names, second is raw data.
     *
     * @param[in] index   The index that referres to the requested item.
     * @param[in] role    The requested role.
     */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the data for the given role and section in the header with the specified orientation.
     *
     * @param[in] section        For horizontal headers, the section number corresponds to the column number. Similarly, for vertical headers, the section number corresponds to the row number.
     * @param[in] orientation    Qt::Horizontal or Qt::Vertical
     * @param[in] role           role to show
     *
     * @return accessed eader data
     */
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    //=========================================================================================================
    /**
     * Returns the item flags for the given index.
     *
     * @param[in] index   The index that referres to the requested item.
     */
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Returns the index for the item in the model specified by the given row, column and parent index.
     * Currently only Qt::DisplayRole is supported.
     * Index rows reflect channels, first column is channel names, second is raw data.
     *
     * @param[in] row      The specified row.
     * @param[in] column   The specified column.
     * @param[in] parent   The parent index.
     */
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the parent index of the given index.
     * In this Model the parent index in always QModelIndex().
     *
     * @param[in] index   The index that referres to the child.
     */
    QModelIndex parent(const QModelIndex &index) const override;

    //=========================================================================================================
    /**
     * Returns the number of childeren for the parent node.
     *
     * @param[in] parent     The parent index.
     */
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns the number of objects stored in the node.
     *
     * @param[in] parent     The index of the requested node.
     */
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * Returns true if parent has any children; otherwise returns false.
     *
     * @param[in] parent     The index of the parent node.
     */
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    //=========================================================================================================
    /**
     * The type of this model (FiffRawViewModel)
     *
     * @return The type of this model (FiffRawViewModel)
     */
    inline MODEL_TYPE getType() const override;

    //=========================================================================================================
    /**
     * Return the first sample of the currently loaded blocks
     *
     * @return The first sample of the currently loaded blocks
     */
    inline qint32 currentFirstSample() const;

    //=========================================================================================================
    /**
     * Return the first sample of the loaded Fiff file
     *
     * @return The first sample of the loaded Fiff file
     */
    inline qint32 absoluteFirstSample() const;

    //=========================================================================================================
    /**
     * Return the last sample of the currently loaded blocks (inclusive)
     *
     * @return The last sample of the currently loaded blocks (inclusive)
     */
    inline qint32 currentLastSample() const;

    //=========================================================================================================
    /**
     * Returns the last sample of the loaded Fiff file
     *
     * @return The last sample of the loaded Fiff file
     */
    inline qint32 absoluteLastSample() const;

    //=========================================================================================================
    /**
     * Returns the the number of samples that can be loaded in the window.
     *
     * @return The the number of samples that can be loaded in the window
     */
    inline qint32 sampleWindowSize() const;

    inline void setDataColumnWidth(int iWidth);

    inline double pixelDifference() const;

public slots:

    /**
     * This tells the model where the view currently is.
     *
     * @param newScrollPosition Absolute sample number.
     */
    void updateScrollPosition(qint32 newScrollPosition);

private:

    //=========================================================================================================
    /**
     * This is a helper method thats is meant to correctly set the endOfFile / startOfFile flags whenever needed
     */
    void updateEndStartFlags();

    //=========================================================================================================
    /**
     * This is helper function, its main purpose is to avoid code redundancy.
     *
     * @param[in] loadFunction The function that will be run in the background. Should be either loadEarlierBlocks or loadLaterBlocks.
     * @param[in] numBlocks The Number of blocks to load.
     */
    void startBackgroundOperation(int (FiffRawViewModel::*loadFunction)(int), int iBlocksToLoad);

    //=========================================================================================================
    /**
     * This is run concurrently
     *
     * @param[in] iCursorRequested Cursor that points to the requested sample
     */
    int loadEarlierBlocks(qint32 numBlocks);

    //=========================================================================================================
    /**
     * This is run concurrently
     *
     * @param[in] iCursorRequested Cursor that points to the requested sample
     */
    int loadLaterBlocks(qint32 numBlocks);

    //=========================================================================================================
    /**
     * This is run by the FutureWatcher when its finished
     *
     * @param[in] result Code value for the result
     */
    void postBlockLoad(int result);

signals:
     void newBlocksLoaded();

private:

    std::list<QSharedPointer<QPair<MatrixXd, MatrixXd>>> m_lData;    /**< Data */
    std::list<QSharedPointer<QPair<MatrixXd, MatrixXd>>> m_lNewData; /**< Data that is to be appended or prepended */

    // Display studd
    double      m_dDx;              /**< pixel difference to the next sample. */

    // model config
    qint32 m_iSamplesPerBlock;      /**< Number of samples per block */
    qint32 m_iVisibleWindowSize;    /**< Number of blocks per visible window */
    qint32 m_iPreloadBufferSize;    /**< Number of blocks that are preloaded left and right */
    qint32 m_iTotalBlockCount;      /**< Total block count ( =  m_iVisibleWindowSize + 2 * m_iPreloadBufferSize) */

    // management
    qint32 m_iFiffCursorBegin;      /**< This always points to the very first sample that is currently held (in the earliest block) */
    bool m_bStartOfFileReached;     /**< Flag for having reached the start of the file */
    bool m_bEndOfFileReached;       /**< Flag for having reached the end of the file */

    // concurrent reloading
    QFutureWatcher<int> m_blockLoadFutureWatcher;   /**< QFutureWatcher for watching process of reloading fiff data. */
    bool m_bCurrentlyLoading;                       /**< Flag to indicate whether or not a background operation is going on. */
    mutable QMutex m_dataMutex;                     /**< Using mutable is not a pretty solution */
    QFile m_file;

    // fiff stuff
    QSharedPointer<FIFFLIB::FiffIO> m_pFiffIO;      /**< Fiff IO */
    FIFFLIB::FiffInfo::SPtr m_pFiffInfo;            /**< Fiff info of whole fiff file */
    QList<FIFFLIB::FiffChInfo> m_ChannelInfoList;   /**< List of FiffChInfo objects that holds the corresponding channels information */
};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline MODEL_TYPE FiffRawViewModel::getType() const
{
    return MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL;
}


//*************************************************************************************************************

inline qint32 FiffRawViewModel::currentFirstSample() const {
    return m_iFiffCursorBegin;
}


//*************************************************************************************************************

inline qint32 FiffRawViewModel::absoluteFirstSample() const {
    if(m_pFiffIO->m_qlistRaw.empty() == false)
        return m_pFiffIO->m_qlistRaw[0]->first_samp;
    else
    {
        qDebug() << "[FiffRawViewModel::firstSample] Raw list is empty, returning -1";
        return -1;
    }
}


//*************************************************************************************************************

inline qint32 FiffRawViewModel::currentLastSample() const {
    return m_iFiffCursorBegin + m_iTotalBlockCount * m_iSamplesPerBlock - 1;
}


//*************************************************************************************************************

inline qint32 FiffRawViewModel::absoluteLastSample() const {
    if(m_pFiffIO->m_qlistRaw.empty() == false)
        return m_pFiffIO->m_qlistRaw[0]->last_samp;
    else
    {
        qDebug() << "[FiffRawViewModel::lastSample] Raw list is empty, returning -1";
        return -1;
    }
}


//*************************************************************************************************************

inline qint32 FiffRawViewModel::sampleWindowSize() const {
    return m_iVisibleWindowSize * m_iSamplesPerBlock;
}


//*************************************************************************************************************

inline void FiffRawViewModel::setDataColumnWidth(int iWidth) {
    qDebug() << "FiffRawViewModel::setDataColumnWidth - m_iVisibleWindowSize" << m_iVisibleWindowSize;
    qDebug() << "FiffRawViewModel::setDataColumnWidth - m_iSamplesPerBlock" << m_iSamplesPerBlock;
    m_dDx = (double)iWidth/double(m_iVisibleWindowSize*m_iSamplesPerBlock);
    qDebug() << "FiffRawViewModel::setDataColumnWidth - m_dDx" << m_dDx;
}


//*************************************************************************************************************

inline double FiffRawViewModel::pixelDifference() const {
    return m_dDx;
}


//*************************************************************************************************************
//=============================================================================================================
// CHANNELDATA / CHANNELITERATOR DEFINITION
//=============================================================================================================

/**
 * The ChannelData class is meant to serve as a wrapper / container for more convenient access of channel-row data.
 * It supports range-based looping (for-each), as well as random access of data.
 */
class ChannelData
{

public:
    /**
     * This nested class enables the range-based looping.
     */
    class ChannelIterator : public std::iterator<std::random_access_iterator_tag, const double>
    {

    private:
        const ChannelData* cd;  /**< Pointer to the associated ChannelData container */
        // Remember at which point we are currently (this is NOT the absolute sample number,
        // but the index relative to all stored samples in the associated ChannelData container):
        qint32 currentIndex;

        // Remember which block we are currently in
        std::list<QSharedPointer<QPair<MatrixXd, MatrixXd>>>::const_iterator currentBlockToAccess;
        qint32 currentRelativeIndex; /**< Remember the relative sample in the current block */

    public:
        ChannelIterator(const ChannelData* cd, qint32 index)
        : std::iterator<std::random_access_iterator_tag, const double>()
        , cd(cd)
        , currentIndex(index)
        , currentBlockToAccess(cd->m_lData.begin())
        , currentRelativeIndex(0)
        {
            // calculate current block to access and current relative index
            qint32 temp = currentIndex;

            // comparing temp against 0 to avoid index-out-of bound scenario for ChannelData::end()
            while (temp > 0 && temp >= (*currentBlockToAccess)->first.cols()) {
                temp -= (*currentBlockToAccess)->first.cols();
                currentBlockToAccess++;
            }

            currentRelativeIndex = temp;
        }

        ChannelIterator(const ChannelIterator &other)
        : ChannelIterator(other.cd, other.currentIndex)
        {
        }

        ChannelIterator& operator ++ (int)
        {
            currentIndex++;
            currentRelativeIndex++;
            if (currentRelativeIndex >= (*currentBlockToAccess)->first.cols()) {
                currentRelativeIndex -= (*currentBlockToAccess)->first.cols();
                currentBlockToAccess++;
            }

            return *this;
        }

        ChannelIterator& operator ++ ()
        {
            currentIndex++;
            currentRelativeIndex++;
            if (currentRelativeIndex >= (*currentBlockToAccess)->first.cols()) {
                currentRelativeIndex -= (*currentBlockToAccess)->first.cols();
                currentBlockToAccess++;
            }

            return *this;
        }

        bool operator != (ChannelIterator rhs)
        {
            return currentIndex != rhs.currentIndex;
        }

        double operator * ()
        {
            const double* pointerToMatrix = (*currentBlockToAccess)->first.data();

            // go to row
            pointerToMatrix += cd->m_iRowNumber * (*currentBlockToAccess)->first.cols();

            // go to sample
            pointerToMatrix += currentRelativeIndex;

            return *pointerToMatrix;
        }
    };

    ChannelData(std::list<QSharedPointer<QPair<MatrixXd, MatrixXd>>>::const_iterator it,
                qint32 numBlocks,
                qint32 rowNumber)
    : m_lData()
    , m_iRowNumber(rowNumber)
    , m_iNumSamples(0)
    {
        for (int i = 0; i < numBlocks; ++i) {
            m_lData.push_back(*it);
            it++;
        }

        for (const auto &a : m_lData) {
            m_iNumSamples += a->first.cols();
        }
    }

    ChannelData(const std::list<QSharedPointer<QPair<MatrixXd, MatrixXd>>> data,
                unsigned long rowNumber)
    : ChannelData(data.begin(), data.size(), rowNumber)
    {

    }

    // we need a public copy constructor in order to register this as QMetaType
    ChannelData(const ChannelData& other)
    : ChannelData(other.m_lData, other.m_iRowNumber)
    {

    }

    // we need a public default constructor in order to register this as QMetaType
    ChannelData()
    : m_lData()
    , m_iRowNumber(-1)
    , m_iNumSamples(0)
    {
        qDebug() << "[FiffRawViewModel::ChannelData::ChannelData] WARNING: default constructor called, this is probably wrong ...";
    }

    // we need a public destructor in order to register this as QMetaType
    ~ChannelData() = default;

    // this is comparatively expensive to call, better use the range based for loop
    double operator [] (unsigned long i)
    {
        // see which block we have to access
        std::list<QSharedPointer<QPair<MatrixXd, MatrixXd>>>::const_iterator blockToAccess = m_lData.begin();
        while (i >= (unsigned long)(*blockToAccess)->first.cols())
        {
            i -= (*blockToAccess)->first.cols();
            blockToAccess++;
        }

        // set the pointer to the start of matrix
        const double* pointerToMatrix = (*blockToAccess)->first.data();

        // go to row
        pointerToMatrix += i * (*blockToAccess)->first.rows();

        // go to sample
        pointerToMatrix += m_iRowNumber;

        return *(pointerToMatrix);
    }

    unsigned long size() const
    {
        return m_iNumSamples;
    }

    qint32 getRowNumber() const
    {
        return m_iRowNumber;
    }

    ChannelIterator begin() const
    {
        ChannelIterator begin(this, 0);
        return begin;
    }

    ChannelIterator end() const
    {
        ChannelIterator end(this, m_iNumSamples);
        return end;
    }

private:
    // hold a list of smartpointers to the data that was in the model when the respective instance of ChannelData was created.
    // This prevents that pointers into the Eigen-matrices will become invalid when the background thread returns and changes the matrices.
    std::list<QSharedPointer<QPair<MatrixXd, MatrixXd> > > m_lData;
    qint32 m_iRowNumber;
    qint64 m_iNumSamples;
};


} // namespace ANSHAREDLIB

#endif // ANSHAREDLIB_FIFFRAWVIEWMODEL_H