// All rights reserved ADENEO EMBEDDED 2010
/*
===============================================================================
*             Texas Instruments OMAP(TM) Platform Software
* (c) Copyright Texas Instruments, Incorporated. All Rights Reserved.
*
* Use of this software is controlled by the terms and conditions found
* in the license agreement under which this software has been supplied.
*
===============================================================================
*/
//
//  File:  indexlist.h
//

#ifndef __INDEXLIST_H__
#define __INDEXLIST_H__

//-----------------------------------------------------------------------------
#define INDEXED_CHUNK_SIZE              (32)

//-----------------------------------------------------------------------------
//
//  Class:  IndexList
//
template <typename dataType>
class IndexList 
{
protected:    
    //-------------------------------------------------------------------------
    // typedefs and structs
    //
    struct IndexData
    {
        IndexData      *pNext;
        DWORD           ffMask;
        dataType        rgData[INDEXED_CHUNK_SIZE]; 
    };

protected:
    //-------------------------------------------------------------------------
    // member variables
    //
    IndexData           m_Head;
    DWORD               m_maxIndex;

public:
    //-------------------------------------------------------------------------
    // constructor/destructor
    //
    IndexList()
    {        
        memset(&m_Head, 0, sizeof(IndexData));
        m_maxIndex = INDEXED_CHUNK_SIZE - 1;
    }

    ~IndexList()
    {
        IndexData *pCurrent;
        IndexData *pDelete;

        pCurrent = m_Head.pNext;
        while (pCurrent != NULL)
            {
            pDelete = pCurrent;
            pCurrent = pCurrent->pNext;
            delete pDelete;
            }
    }

public:
    //-------------------------------------------------------------------------
    // methods
    //
    DWORD MaxIndex()
    {
        return m_maxIndex;
    }
    
    BOOL NewIndex(dataType **ppType, DWORD *pId)
    {
        DWORD ffMask;
        INT index = 0;        
        BOOL rc = FALSE;        
        IndexData *pCurrent;

        // find emtpy chunk
        pCurrent = &m_Head;
        while (pCurrent->ffMask == (-1))
            {
            if (pCurrent->pNext == NULL)
                {
                IndexData *pNew = new IndexData;
                if (pNew == NULL) goto cleanUp;

                memset(pNew, 0, sizeof(IndexData));
                pCurrent->pNext = pNew;
                m_maxIndex += INDEXED_CHUNK_SIZE;
                }

            index += INDEXED_CHUNK_SIZE;
            pCurrent = pCurrent->pNext;
            }

        // get index within chunk
        ffMask = pCurrent->ffMask;
        while ((ffMask & 1) == 1)
            {
            index++;
            ffMask >>= 1;
            }

        // copy empty slot info and mark as reserved
        *pId = index;
        *ppType = &(pCurrent->rgData[index & 0x1F]);
        pCurrent->ffMask |= (1 << (index & 0x1F));

        rc = TRUE;
    cleanUp:
        return rc;
    }

    void DeleteIndex(DWORD id)
    {
        IndexData *pCurrent;
        DWORD chunkId = id >> 5;

        // find chunk
        pCurrent = &m_Head;
        while (chunkId)
            {
            --chunkId;
            pCurrent = pCurrent->pNext;
            if (pCurrent == NULL) return;            
            }

        // clear mask
        pCurrent->ffMask &= ~(1 << (id & 0x1F));
    }

    dataType* GetIndex(DWORD id)
    {
        IndexData *pCurrent;
        DWORD chunkId = id >> 5;

        // find chunk
        pCurrent = &m_Head;
        while (chunkId)
            {
            --chunkId;
            pCurrent = pCurrent->pNext;
            if (pCurrent == NULL) return NULL;   
            }

        // clear mask
        return (pCurrent->ffMask & (1 << (id & 0x1F))) ? 
                    &(pCurrent->rgData[id & 0x1F]) : NULL;
    }
};

//-----------------------------------------------------------------------------
#endif //__INDEXLIST_H__