//========================================================================
//   Copyright 2006 Mistral Software Pvt Ltd.
//   Copyright (c) Texas Instruments Incorporated 2008-2009
//
//   Use of this software is controlled by the terms and conditions found
//   in the license agreement under which this software has been supplied
//   or provided.
//========================================================================
#ifndef __CPSW3G_QUEUE_H_INCLUDED__
#define __CPSW3G_QUEUE_H_INCLUDED__

#include <windows.h>
#include "constants.h"


/* Queue data strucures */

typedef struct SingleLink
{
    struct SingleLink* m_pLink;
} SLINK_T, *PSLINK_T;


/*
 * The following structure is a definition of a Queue structure ( FIFO )
 * which is defined based on the Singly Link data structure.
 */
typedef struct Queue 
{
    PSLINK_T m_pHead;
    PSLINK_T m_pTail;
    ULONG    m_Count;
} QUEUE_T, *PQUEUE_T;




/* 
 * The following macro must be used for initializing a Queue data structure
 * before any of the insert/remove/count operations can be performed.
 */
#define QUEUE_INIT(List)     {\
    (List)->m_pHead = (PSLINK_T) NULL;\
    (List)->m_pTail = (PSLINK_T) NULL;\
    (List)->m_Count = 0;\
}

/*
 * The following is the MACRO definition which is used for inserting new
 * elements into the tail of a Queue.
 */
#define QUEUE_INSERT(List, Element) {\
    if((((PQUEUE_T)(List)) != (PQUEUE_T) NULL) &&  \
            ((PSLINK_T)(Element) != (PSLINK_T) NULL)) \
    {\
        if((PSLINK_T)(((PQUEUE_T)(List))->m_pHead) == (PSLINK_T) NULL)\
            ((PQUEUE_T)(List))->m_pHead = (PSLINK_T)(Element);\
        else\
            (PSLINK_T)((((PQUEUE_T)(List))->m_pTail)->m_pLink) = (PSLINK_T)(Element);\
        ((PQUEUE_T)(List))->m_pTail = (PSLINK_T)(Element);\
        ((PQUEUE_T)(List))->m_Count ++;\
        ((PSLINK_T)(Element))->m_pLink =(PSLINK_T)NULL;\
    }\
}

/*
 * The following is the MACRO definition which is used for inserting new
 * elements into the head of a Queue.
 */
#define QUEUE_INSERT_HEAD(List, Element) {\
    if(((PQUEUE_T)(List) != (PQUEUE_T) NULL) &&  \
            ((PSLINK_T)(Element) != (PSLINK_T) NULL)) \
    {\
        if((PSLINK_T)((PQUEUE_T)(List)->m_pTail) == (PSLINK_T) NULL)\
            ((PQUEUE_T)(List))->m_pTail = (PSLINK_T)(Element);\
        else\
            ((PSLINK_T)(Element))->m_pLink =(PSLINK_T)(List)->m_pHead;\
        ((PQUEUE_T)(List))->m_pHead = (PSLINK_T)(Element);\
        ((PQUEUE_T)(List))->m_Count ++;\
    }\
}

/*
 * The following is the MACRO definition which is used for inserting a queue
 * to the tail of another queue.
 */
#define QUEUE_INSERT_QUEUE(List1, List2) {\
    if(((List1) != (PQUEUE_T) NULL) &&  \
            ((List2) != (PQUEUE_T) NULL))\
    {\
        if(((PQUEUE_T)(List1))->m_pTail != NULL)\
            ((PQUEUE_T)(List1))->m_pTail->m_pLink = (PSLINK_T)(List2)->m_pHead;\
        else\
            ((PQUEUE_T)(List1))->m_pHead = (PSLINK_T)((PQUEUE_T)(List2))->m_pHead;\
        ((PQUEUE_T)(List1))->m_pTail = ((PQUEUE_T)(List2))->m_pTail;\
        ((PQUEUE_T)(List1))->m_Count += ((PQUEUE_T)(List2))->m_Count;\
     }\
}

/*
 * The following is the MACRO definition which is used for removing an element
 * from the head of the queue.
 */
#define QUEUE_REMOVE(List, Element)  {\
    Element = NULL;\
    if(((List) != (PQUEUE_T) NULL) && ((PQUEUE_T)(List)->m_pHead != NULL))\
    {\
        PVOID *pElem = &((PVOID)Element); \
        *pElem = (PVOID)((PQUEUE_T)(List)->m_pHead);\
        ((PQUEUE_T)(List))->m_pHead = (PSLINK_T)(((PQUEUE_T)(List))->m_pHead)->m_pLink;\
        if(((PQUEUE_T)(List))->m_pHead == NULL)\
            ((PQUEUE_T)(List))->m_pTail = NULL;\
        ((PQUEUE_T)(List))->m_Count --;\
        ((PSLINK_T)(Element))->m_pLink =(PSLINK_T)NULL;\
    }\
}

                                
#define QUEUE_IS_EMPTY(List) (((List)->m_pHead == 0) ? 1 : 0 )
#define QUEUE_COUNT(List)    ((List)->m_Count)

#endif /* #ifndef __CPSW3G_QUEUE_H_INCLUDED__*/
