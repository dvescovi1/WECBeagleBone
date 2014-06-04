//
// Copyright (c) Texas Instruments Incorporated 2010. All Rights Reserved.
//
//------------------------------------------------------------------------------

//! \file Emac_Queue.h
//! \brief Miniport required Queue data structure related functions.
//! 
//! This header File contains the singly linked list implementation of Queue.
//! It has various macros for basic queue functions like insertion,deletion etc.
//! 
//! \version  1.00 Aug 22nd 2006 File Created 

#ifndef __EMAC_QUEUE_H_INCLUDED__
#define __EMAC_QUEUE_H_INCLUDED__

#include <windows.h>


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
    (PVOID)(Element) = (PVOID) NULL;\
    if(((List) != (PQUEUE_T) NULL) && ((PQUEUE_T)(List)->m_pHead != NULL))\
    {\
        (PSLINK_T)(Element) = (PSLINK_T)((PQUEUE_T)(List))->m_pHead;\
        ((PQUEUE_T)(List))->m_pHead = (PSLINK_T)(((PQUEUE_T)(List))->m_pHead)->m_pLink;\
        if(((PQUEUE_T)(List))->m_pHead == NULL)\
            ((PQUEUE_T)(List))->m_pTail = NULL;\
        ((PQUEUE_T)(List))->m_Count --;\
        ((PSLINK_T)(Element))->m_pLink =(PSLINK_T)NULL;\
    }\
}

                                
#define QUEUE_IS_EMPTY(List) (((List)->m_pHead == 0) ? 1 : 0 )
#define QUEUE_COUNT(List)    ((List)->m_Count)

#endif /* #ifndef __EMAC_QUEUE_H_INCLUDED__*/
