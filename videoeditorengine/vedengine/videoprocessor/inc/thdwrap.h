/*
* Copyright (c) 2010 Ixonos Plc.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - Initial contribution
*
* Contributors:
* Ixonos Plc
*
* Description:  
* Header for thdwrap.cpp.
*
*/



#ifndef _MTXWRAP_H_
#define _MTXWRAP_H_

#include "nrctyp32.h"

typedef void *thdMutex_t;
typedef void *thdSyncObject_t;
typedef void *array_t;
typedef void *thdParamData_t;


#define THD_INFINITE -1
#define THD_TIMEOUT  -1
#define THD_MESSAGE  -2

/* If THD_DS not defined, define it as nothing */
#ifndef THD_DS
#define THD_DS
#endif

#if defined(__MARM__) && !defined(__stdcall)
#define __stdcall
#endif

/* 
 * Structs and typedefs
 */

/* This type is used in thdBeginThreadSync to pass the address
   of the thread main function. */
typedef unsigned ( __stdcall *thdMainFunction_t )( void * );


/****************************************************************************\
*
* Function:     thdMutex_t thdCreateMutex( void );
*
* Description:  Allocates a mutex handle
*
* Returns:      Newly created mutex
*
\****************************************************************************/

THD_DS thdMutex_t thdCreateMutex( void );

/****************************************************************************\
*
* Function:     int thdDestroyMutex( thdMutex_t handle );
*
* Description:  Deallocates a mutex handle
*
* Input:        thdMutex_t handle      Handle to be freed
*
* Returns:      zero on success, negative on error
*
\****************************************************************************/

THD_DS int thdDestroyMutex( thdMutex_t handle );

/****************************************************************************\
*
* Function:     int thdEnterMutex( thdMutex_t handle );
*
* Description:  Enters a mutex
*               When a mutex has been entered, it can not be entered by
*               any other thread before the first one has left it.
*               One thread can enter mutex many times ("nested"), but it
*               must always leave the mutex the exactly same number of times.
*
* Input:        thdMutex_t handle      Mutex handle to enter
*
* Returns:      zero on success, negative on error
*
\****************************************************************************/

THD_DS int thdEnterMutex( thdMutex_t handle );

#if(_WIN32_WINNT >= 0x0400)
/****************************************************************************\
*
* Function:     int thdTryMutex( thdMutex_t handle );
*
* Description:  Tries to enter a mutex, but does not wait to be able to enter
*               Note: only available with WinNT 4.0 or above
*
* Input:        thdMutex_t handle      Mutex handle to enter
*
* Returns:      1 if entered, 0 if not entered, negative on error
*
\****************************************************************************/

THD_DS int thdTryMutex( thdMutex_t handle );
#endif

/****************************************************************************\
*
* Function:     int thdLeaveMutex( thdMutex_t handle );
*
* Description:  Leaves a mutex
*
* Input:        thdMutex_t handle      Mutex handle to leave
*
* Returns:      zero on success, negative on error
*
\****************************************************************************/

THD_DS int thdLeaveMutex( thdMutex_t handle );

/****************************************************************************\
*
* Function:     thdBeginThreadSync
*               thdCreateThreadSync
*
* Description:  Creates a thread with synchronization; ensures that a message
*               queue exists for the thread immediately after returning from
*               this function. The main function of the created thread MUST
*               include a call to thdEnterThreadSync.
*
*               The difference between thdBeginThreadSync and 
*               thdCreateThreadSync is the type of the routine parameter.
*               If the function is passed as a data pointer (as in
*               thdCreateThreadSync), the compiler should generate
*               a warning since this is against ANSI-C. Thus, the usage of
*               thdBeginThreadSync should be avoided.
*
* Input:        routine             Pointer to thread main function
*               param               Free-form parameter given to thread
*               id                  Pointer to int to receive thread id
*
* Returns:      Handle of the newly created thread, NULL for error
*
\****************************************************************************/

THD_DS void * thdBeginThreadSync( thdMainFunction_t routine, 
   void *param, u_int32 *id );
THD_DS void * thdCreateThreadSync( void *routine, void *param, 
   u_int32 *id );

/****************************************************************************\
*
* Function:     void *thdEnterThreadSync( void *param );
*
* Description:  Creates the message queue and signals the parent thread
*               the completition.
*
* Input:        void *param             The parameter passed to the main
*
* Returns:      The free-form parameter that was given to thdCreateThreadSync
*
\****************************************************************************/

THD_DS void * thdEnterThreadSync( void *param );

/****************************************************************************\
*
* Function:     void thdExitThread( int exitcode );
*
* Description:  Terminates the current thread with given exit code.
*               Note! This function does not return.
*               (WIN32) :: If thread does not call this function, small
*                          memory leaks will result
*
* Input:        int exitcode            Exit code for thread
*
\****************************************************************************/

THD_DS void thdExitThread( int exitcode );

/****************************************************************************\
*
* Function:     void thdSetThreadPriority( void *thread, int priority );
*
* Description:  Sets thread's priority
*
* Input:        void *thread            Thread to set
*               int priority            Priority number, from -3 to 3,
*                                       0 is normal priority.
*
\****************************************************************************/

THD_DS void thdSetThreadPriority( void *thread, int priority );

/****************************************************************************\
*
* Function:     void thdTerminateThread( void *thread, int exitcode );
*
* Description:  Terminates a thread immediately
*
* Input:        void *thread            Thread handle
*               int exitcode            Exit code for the thread
*
\****************************************************************************/

THD_DS void thdTerminateThread( void *thread, int exitcode );

/****************************************************************************\
*
* Function:     thdSyncObject_t thdCreateEvent( void );
*
* Description:  Creates an event
*
* Returns:      Handle for the event
*
\****************************************************************************/

THD_DS thdSyncObject_t thdCreateEvent( void );
THD_DS thdSyncObject_t thdCreateEvent( array_t *array );

/****************************************************************************\
*
* Function:     void thdDestroyEvent( thdSyncObject_t event );
*
* Description:  Destroys an event
*
* Input:        thdSyncObject_t event        Event to be destroyed
*
\****************************************************************************/

THD_DS void thdDestroyEvent( thdSyncObject_t event );

/****************************************************************************\
*
* Function:     void thdSetEvent( thdSyncObject_t event );
*
* Description:  Sets (signals) an event
*
* Input:        thdSyncObject_t event        Event to set
*
\****************************************************************************/

THD_DS void thdSetEvent( thdSyncObject_t event );

/****************************************************************************\
*
* Function:     void thdResetEvent( thdSyncObject_t event );
*
* Description:  Resets (unsignals) an event
*
* Input:        thdSyncObject_t event        Event to reset
*
\****************************************************************************/

THD_DS void thdResetEvent( thdSyncObject_t event );

/****************************************************************************\
*
* Function:     int thdWaitSyncObject( thdSyncObject_t event, int time );
*
* Description:  Waits until the given event is signaled, or time limit elapses
*
* Input:        thdSyncObject_t event        Event handle to wait
*               int time                Time limit in ms, -1 = infinite
*
* Returns:      Zero if event was signaled, -1 if time limit elapsed
*
\****************************************************************************/

THD_DS int thdWaitSyncObject( thdSyncObject_t event, int time );

/****************************************************************************\
*
* Function:     int thdWaitManySyncObjects( thdSyncObject_t *events, int count, int time );
*
* Description:  Waits until at least one of the given events is signaled, or
*               time limit elapses
*
* Input:        thdSyncObject_t *events Pointer to array of event handles
*               int count               Number of event handles in array
*               int time                Time limit in ms, -1 = infinite
*
* Returns:      Index of event signaled (>=0) or
*               THD_TIMEOUT if time limit elapsed
*
\****************************************************************************/

THD_DS int thdWaitManySyncObjects( thdSyncObject_t *events, int count, int time );

/****************************************************************************\
*
* Function:     int thdWaitManySyncObjectsOrMessage( thdSyncObject_t *events, int count, int time )
*
* Description:  Waits until at least one of the given synchronization objects
*               is signaled, or the time limit elapses, or a message is posted
*               to thread's message queue.
*               You can give NULL to events and zero to count to only wait
*               for messages.
*
* Input:        thdSyncObject_t *events Pointer to array of sync. object handles
*               int count               Number of handles in array
*               int time                Time limit in ms, -1 = infinite
*
* Returns:      Index of event signaled (>=0)
*               THD_TIMEOUT if time limit elapsed
*               THD_MESSAGE if a message was posted
*
\****************************************************************************/

THD_DS int thdWaitManySyncObjectsOrMessage( thdSyncObject_t *events, int count, int time );

/****************************************************************************\
*
* Function:     thdSyncObject_t thdThreadToEvent( void *thread );
*
* Description:  Converts a thread handle returned by thdCreateThread into an
*               event handle that can be waited for with thdWaitSyncObject
*               This event handle must not be closed!
*
* Input:        void *thread            Thread handle
*
* Returns:      Event handle
*
\****************************************************************************/

THD_DS thdSyncObject_t thdThreadToEvent( void *thread );

/****************************************************************************\
*
* Function:     thdSyncObject_t thdCreateSemaphore( int initcount, int maxcount );
*
* Description:  Creates a semaphore object.
*
* Input:        int initcount           Initial count for semaphore
*               int maxcount            Maximum count
*
* Returns:      Handle of the created semaphore
*
\****************************************************************************/

THD_DS thdSyncObject_t thdCreateSemaphore( int initcount, int maxcount );
THD_DS thdSyncObject_t thdCreateSemaphore( int initcount, int maxcount, array_t *array );

/****************************************************************************\
*
* Function:     void thdDestroySemaphore( thdSyncObject_t handle );
*
* Description:  Destroys a semaphore object
*
* Input:        thdSyncObject_t handle  Handle to close
*
\****************************************************************************/

THD_DS void thdDestroySemaphore( thdSyncObject_t handle );

/****************************************************************************\
*
* Function:     void thdReleaseSemaphore( thdSyncObject_t handle, int count );
*
* Description:  Releases a semaphore
*
* Input:        thdSyncObject_t handle  Semaphore to release
*               int count               Release count
*
* Returns:      Previous count
*
\****************************************************************************/

THD_DS int thdReleaseSemaphore( thdSyncObject_t handle, int count );

/****************************************************************************\
*
* Function:     void thdSleep( int time );
*
* Description:  Enters an efficient wait state for a specified time
*
* Input:        int time                Time to sleep, in milliseconds
*
\****************************************************************************/

THD_DS void thdSleep( int time );

#endif
// End of File
