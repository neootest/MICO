/**
  ******************************************************************************
  * @file    MICORTOS.h 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   This file provides all the headers of RTOS operation provided by MICO.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
  ******************************************************************************
  */ 


#ifndef __MICORTOS_H__
#define __MICORTOS_H__

#include "Common.h"

#define MICO_NEVER_TIMEOUT   (0xFFFFFFFF)
#define MICO_WAIT_FOREVER    (0xFFFFFFFF)
#define MICO_NO_WAIT         (0)

/************************************************************************
 *   MICO thread priority table
 *
 * +----------+-----------------+
 * | Priority |      Thread     |
 * |----------|-----------------|
 * |     0    |      MICO       |   Highest priority
 * |     1    |     Network     |
 * |     2    |                 |
 * |     3    | Network worker  |
 * |     4    |                 |
 * |     5    | Default Library |
 * |          | Default worker  |
 * |     6    |                 |
 * |     7    |   Application   |
 * |     8    |                 |
 * |     9    |      Idle       |   Lowest priority
 * +----------+-----------------+
 */
#define MICO_NETWORK_WORKER_PRIORITY      (3)
#define MICO_DEFAULT_WORKER_PRIORITY      (5)
#define MICO_DEFAULT_LIBRARY_PRIORITY     (5)
#define MICO_APPLICATION_PRIORITY         (7)

#define mico_thread_sleep                 sleep
#define mico_thread_msleep                msleep

/* RTOS APIs*/
typedef void (*mico_thread_function_t)( void* arg );
typedef void* mico_semaphore_t;
typedef void* mico_mutex_t;
typedef void* mico_thread_t;
typedef void* mico_queue_t;
typedef void (*timer_handler_t)( void* arg );

typedef struct
{
    void *          handle;
    timer_handler_t function;
    void*           arg;
}mico_timer_t;

/** Creates and starts a new thread
 *
 * @param thread     : Pointer to variable that will receive the thread handle
 * @param priority   : A priority number.
 * @param name       : a text name for the thread (can be null)
 * @param function   : the main thread function
 * @param stack_size : stack size for this thread
 * @param arg        : argument which will be passed to thread function
 *
 * @return    kNoErr          : on success.
 * @return    MXCHIP_FAILED   : if an error occurred
 */
OSStatus mico_rtos_create_thread( mico_thread_t* thread, uint8_t priority, const char* name, mico_thread_function_t function, uint32_t stack_size, void* arg );


/** Deletes a terminated thread
 *
 * @param thread     : the handle of the thread to delete, , NULL is the current thread
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_delete_thread( mico_thread_t* thread );

/** Suspend a thread
 *
 * @param thread     : the handle of the thread to suspend, NULL is the current thread
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
void mico_rtos_suspend_thread(mico_thread_t* thread);


/** Sleeps until another thread has terminated
 *
 * Causes the current thread to sleep until the specified other thread
 * has terminated. If the processor is heavily loaded
 * with higher priority tasks, this thread may not wake until significantly
 * after the thread termination.
 * Causes the specified thread to wake from suspension. This will usually
 * cause an error or timeout in that thread, since the task it was waiting on
 * is not complete.
 *
 * @param thread : the handle of the other thread which will terminate
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_thread_join( mico_thread_t* thread );


/** Forcibly wakes another thread
 *
 * Causes the specified thread to wake from suspension. This will usually
 * cause an error or timeout in that thread, since the task it was waiting on
 * is not complete.
 *
 * @param thread : the handle of the other thread which will be woken
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_thread_force_awake( mico_thread_t* thread );


/** Checks if a thread is the current thread
 *
 * Checks if a specified thread is the currently running thread
 *
 * @param thread : the handle of the other thread against which the current thread will be compared
 *
 * @return    true        : specified thread is the current thread
 * @return    false       : specified thread is not currently running
 */
bool mico_rtos_is_current_thread( mico_thread_t* thread );


/** Initialises a semaphore
 *
 * Initialises a counting semaphore
 *
 * @param semaphore : a pointer to the semaphore handle to be initialised
 * @param count         : the max count number of this semaphore
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_init_semaphore( mico_semaphore_t* semaphore, int count );


/** Set (post/put/increment) a semaphore
 *
 * Set (post/put/increment) a semaphore
 *
 * @param semaphore : a pointer to the semaphore handle to be set
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_set_semaphore( mico_semaphore_t* semaphore );


/** Get (wait/decrement) a semaphore
 *
 * Attempts to get (wait/decrement) a semaphore. If semaphore is at zero already,
 * then the calling thread will be suspended until another thread sets the
 * semaphore with @ref mico_rtos_set_semaphore
 *
 * @param semaphore : a pointer to the semaphore handle
 * @param timeout_ms: the number of milliseconds to wait before returning
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_get_semaphore( mico_semaphore_t* semaphore, uint32_t timeout_ms );



/** De-initialise a semaphore
 *
 * Deletes a semaphore created with @ref mico_rtos_init_semaphore
 *
 * @param semaphore : a pointer to the semaphore handle
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_deinit_semaphore( mico_semaphore_t* semaphore );


/** Initialises a mutex
 *
 * Initialises a mutex
 * A mutex is different to a semaphore in that a thread that already holds
 * the lock on the mutex can request the lock again (nested) without causing
 * it to be suspended.
 *
 * @param mutex : a pointer to the mutex handle to be initialised
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_init_mutex( mico_mutex_t* mutex );


/** Obtains the lock on a mutex
 *
 * Attempts to obtain the lock on a mutex. If the lock is already held
 * by another thead, the calling thread will be suspended until
 * the mutex lock is released by the other thread.
 *
 * @param mutex : a pointer to the mutex handle to be locked
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_lock_mutex( mico_mutex_t* mutex );


/** Releases the lock on a mutex
 *
 * Releases a currently held lock on a mutex. If another thread
 * is waiting on the mutex lock, then it will be resumed.
 *
 * @param mutex : a pointer to the mutex handle to be unlocked
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_unlock_mutex( mico_mutex_t* mutex );


/** De-initialise a mutex
 *
 * Deletes a mutex created with @ref mico_rtos_init_mutex
 *
 * @param mutex : a pointer to the mutex handle
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_deinit_mutex( mico_mutex_t* mutex );


/** Initialises a queue
 *
 * Initialises a FIFO queue
 *
 * @param queue : a pointer to the queue handle to be initialised
 * @param name  : a text string name for the queue (NULL is allowed)
 * @param message_size : size in bytes of objects that will be held in the queue
 * @param number_of_messages : depth of the queue - i.e. max number of objects in the queue
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_init_queue( mico_queue_t* queue, const char* name, uint32_t message_size, uint32_t number_of_messages );


/** Pushes an object onto a queue
 *
 * Pushes an object onto a queue
 *
 * @param queue : a pointer to the queue handle
 * @param message : the object to be added to the queue. Size is assumed to be
 *                  the size specified in @ref mico_rtos_init_queue
 * @param timeout_ms: the number of milliseconds to wait before returning
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error or timeout occurred
 */
OSStatus mico_rtos_push_to_queue( mico_queue_t* queue, void* message, uint32_t timeout_ms );


/** Pops an object off a queue
 *
 * Pops an object off a queue
 *
 * @param queue : a pointer to the queue handle
 * @param message : pointer to a buffer that will receive the object being
 *                  popped off the queue. Size is assumed to be
 *                  the size specified in @ref mico_rtos_init_queue , hence
 *                  you must ensure the buffer is long enough or memory
 *                  corruption will result
 * @param timeout_ms: the number of milliseconds to wait before returning
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error or timeout occurred
 */
OSStatus mico_rtos_pop_from_queue( mico_queue_t* queue, void* message, uint32_t timeout_ms );


/** De-initialise a queue
 *
 * Deletes a queue created with @ref mico_rtos_init_queue
 *
 * @param queue : a pointer to the queue handle
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_rtos_deinit_queue( mico_queue_t* queue );


/** Check if a queue is empty
 *
 * @param queue : a pointer to the queue handle
 *
 * @return    true        : queue is empty.
 * @return    false       : queue is not empty.
 */
bool mico_rtos_is_queue_empty( mico_queue_t* queue );


/** Check if a queue is full
 *
 * @param queue : a pointer to the queue handle
 *
 * @return    kNoErr        : queue is full.
 * @return    kGeneralErr   : queue is not full.
 */
OSStatus mico_rtos_is_queue_full( mico_queue_t* queue );

/** Enables the MCU to enter powersave mode.
*
* @param enable : 1=enable MCU powersave, 0=disable MCU powersave
* @return    void
*/
void mico_mcu_powersave_config( int enable );


/**
 * Gets time in miiliseconds since RTOS start
 *
 * @Note: since this is only 32 bits, it will roll over every 49 days, 17 hours.
 *
 * @returns Time in milliseconds since RTOS started.
 */
uint32_t mico_get_time(void);


/** Initialises a RTOS timer
 *
 * Initialises a RTOS timer
 * Timer does not start running until @ref mico_start_timer is called
 *
 * @param timer    : a pointer to the timer handle to be initialised
 * @param time_ms  : Timer period in milliseconds
 * @param function : the callback handler function that is called each
 *                   time the timer expires
 * @param arg      : an argument that will be passed to the callback
 *                   function
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_init_timer( mico_timer_t* timer, uint32_t time_ms, timer_handler_t function, void* arg );


/** Starts a RTOS timer running
 *
 * Starts a RTOS timer running. Timer must have been previously
 * initialised with @ref mico_init_timer
 *
 * @param timer    : a pointer to the timer handle to start
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_start_timer( mico_timer_t* timer );


/** Stops a running RTOS timer
 *
 * Stops a running RTOS timer. Timer must have been previously
 * started with @ref mico_start_timer
 *
 * @param timer    : a pointer to the timer handle to stop
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_stop_timer( mico_timer_t* timer );


/** Reloads a RTOS timer that has expired
 *
 * This is usually called in the timer callback handler, to
 * reschedule the timer for the next period.
 *
 * @param timer    : a pointer to the timer handle to reload
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_reload_timer( mico_timer_t* timer );


/** De-initialise a RTOS timer
 *
 * Deletes a RTOS timer created with @ref mico_init_timer
 *
 * @param timer : a pointer to the RTOS timer handle
 *
 * @return    kNoErr        : on success.
 * @return    kGeneralErr   : if an error occurred
 */
OSStatus mico_deinit_timer( mico_timer_t* timer );


/** Check if an RTOS timer is running
 *
 * @param timer : a pointer to the RTOS timer handle
 *
 * @return    true        : if running.
 * @return    false       : if not running
 */
bool mico_is_timer_running( mico_timer_t* timer );


/** Suspend current thread for a specific time
 *
 * @param timer : A time interval (Unit: seconds)
 *
 * @return    None.
 */
void mico_thread_sleep(int seconds);

/** Suspend current thread for a specific time
 *
 * @param timer : A time interval (Unit: millisecond)
 *
 * @return    None.
 */

void mico_thread_msleep(int mseconds);

#endif

