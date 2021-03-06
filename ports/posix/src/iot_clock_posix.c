/*
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * @file iot_clock_posix.c
 * @brief Implementation of the functions in iot_clock.h for POSIX systems.
 */

/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdlib.h>

/* POSIX includes. Allow the default POSIX headers to be overridden. */
#ifdef POSIX_ERRNO_HEADER
    #include POSIX_ERRNO_HEADER
#else
    #include <errno.h>
#endif
#ifdef POSIX_PTHREAD_HEADER
    #include POSIX_PTHREAD_HEADER
#else
    #include <pthread.h>
#endif
#ifdef POSIX_SIGNAL_HEADER
    #include POSIX_SIGNAL_HEADER
#else
    #include <signal.h>
#endif
#ifdef POSIX_TIME_HEADER
    #include POSIX_TIME_HEADER
#else
    #include <time.h>
#endif

/* Platform clock include. */
#include "platform/iot_clock.h"

/* Configure logs for the functions in this file. */
#ifdef IOT_LOG_LEVEL_PLATFORM
    #define LIBRARY_LOG_LEVEL        IOT_LOG_LEVEL_PLATFORM
#else
    #ifdef IOT_LOG_LEVEL_GLOBAL
        #define LIBRARY_LOG_LEVEL    IOT_LOG_LEVEL_GLOBAL
    #else
        #define LIBRARY_LOG_LEVEL    IOT_LOG_NONE
    #endif
#endif

#define LIBRARY_LOG_NAME    ( "CLOCK" )
#include "iot_logging_setup.h"

/* When building tests, the Unity framework's malloc overrides are used to track
 * calls to platform resource creation and destruction. This ensures that all
 * platform resources are destroyed before the tests finish. When not testing,
 * define the Unity malloc functions to nothing. */
#if IOT_BUILD_TESTS != 1
    #define UnityMalloc_AllocateResource()    true
    #define UnityMalloc_FreeResource()
#endif

/*-----------------------------------------------------------*/

/**
 * @brief The format of timestrings printed in logs.
 *
 * For more information on timestring formats, see [this link.]
 * (http://pubs.opengroup.org/onlinepubs/9699919799/functions/strftime.html)
 */
#define TIMESTRING_FORMAT              ( "%F %R:%S" )

/*
 * Time conversion constants.
 */
#define NANOSECONDS_PER_SECOND         ( 1000000000 ) /**< @brief Nanoseconds per second. */
#define NANOSECONDS_PER_MILLISECOND    ( 1000000 )    /**< @brief Nanoseconds per millisecond. */
#define MILLISECONDS_PER_SECOND        ( 1000 )       /**< @brief Milliseconds per second. */

/*-----------------------------------------------------------*/

/**
 * @brief Wraps an #IotThreadRoutine_t with a POSIX-compliant one.
 *
 * @param[in] argument The value passed as `sigevent.sigev_value`.
 */
static void _timerExpirationWrapper( union sigval argument );

/**
 * @brief Convert a relative timeout in milliseconds to an absolute timeout
 * represented as a struct timespec.
 *
 * This function is not included in iot_clock.h because it's platform-specific.
 * But it may be called by other POSIX platform files.
 * @param[in] timeoutMs The relative timeout.
 * @param[out] pOutput Where to store the resulting `timespec`.
 *
 * @return `true` if `timeoutMs` was successfully converted; `false` otherwise.
 */
bool IotClock_TimeoutToTimespec( uint32_t timeoutMs,
                                 struct timespec * pOutput );

/*-----------------------------------------------------------*/

static void _timerExpirationWrapper( union sigval argument )
{
    IotTimer_t * pTimer = ( IotTimer_t * ) argument.sival_ptr;

    /* Call the wrapped thread routine. */
    pTimer->threadRoutine( pTimer->pArgument );
}

/*-----------------------------------------------------------*/

bool IotClock_TimeoutToTimespec( uint32_t timeoutMs,
                                 struct timespec * pOutput )
{
    bool status = true;
    struct timespec systemTime = { 0 };

    if( clock_gettime( CLOCK_REALTIME, &systemTime ) == 0 )
    {
        /* Add the nanoseconds value to the time. */
        systemTime.tv_nsec += ( long ) ( ( timeoutMs % MILLISECONDS_PER_SECOND ) * NANOSECONDS_PER_MILLISECOND );

        /* Check for overflow of nanoseconds value. */
        if( systemTime.tv_nsec >= NANOSECONDS_PER_SECOND )
        {
            systemTime.tv_nsec -= NANOSECONDS_PER_SECOND;
            systemTime.tv_sec++;
        }

        /* Add the seconds value to the timeout. */
        systemTime.tv_sec += ( time_t ) ( timeoutMs / MILLISECONDS_PER_SECOND );

        /* Set the output parameter. */
        *pOutput = systemTime;
    }
    else
    {
        IotLogError( "Failed to read system time. errno=%d", errno );

        status = false;
    }

    return status;
}

/*-----------------------------------------------------------*/

bool IotClock_GetTimestring( char * pBuffer,
                             size_t bufferSize,
                             size_t * pTimestringLength )
{
    bool status = true;
    const time_t unixTime = time( NULL );
    struct tm localTime = { 0 };
    size_t timestringLength = 0;

    /* localtime_r is the thread-safe variant of localtime. Its return value
     * should be the pointer to the localTime struct. */
    if( localtime_r( &unixTime, &localTime ) != &localTime )
    {
        status = false;
    }

    if( status == true )
    {
        /* Convert the localTime struct to a string. */
        timestringLength = strftime( pBuffer, bufferSize, TIMESTRING_FORMAT, &localTime );

        /* Check for error from strftime. */
        if( timestringLength == 0 )
        {
            status = false;
        }
        else
        {
            /* Set the output parameter. */
            *pTimestringLength = timestringLength;
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

uint64_t IotClock_GetTimeMs( void )
{
    struct timespec currentTime = { 0 };

    if( clock_gettime( CLOCK_MONOTONIC, &currentTime ) != 0 )
    {
        /* This block should not be reached; log an error and abort if it is. */
        IotLogError( "Failed to read time from CLOCK_MONOTONIC. errno=%d",
                     errno );

        abort();
    }

    return ( ( uint64_t ) currentTime.tv_sec ) * MILLISECONDS_PER_SECOND +
           ( ( uint64_t ) currentTime.tv_nsec ) / NANOSECONDS_PER_MILLISECOND;
}

/*-----------------------------------------------------------*/

void IotClock_SleepMs( uint32_t sleepTimeMs )
{
    /* Convert parameter to timespec. */
    struct timespec sleepTime =
    {
        .tv_sec = sleepTimeMs / MILLISECONDS_PER_SECOND,
        .tv_nsec = ( sleepTimeMs % MILLISECONDS_PER_SECOND ) * NANOSECONDS_PER_MILLISECOND
    };

    if( nanosleep( &sleepTime, NULL ) == -1 )
    {
        /* This block should not be reached; log an error and abort if it is. */
        IotLogError( "Sleep failed. errno=%d.", errno );

        abort();
    }
}

/*-----------------------------------------------------------*/

bool IotClock_TimerCreate( IotTimer_t * pNewTimer,
                           IotThreadRoutine_t expirationRoutine,
                           void * pArgument )
{
    bool status = UnityMalloc_AllocateResource();
    struct sigevent expirationNotification =
    {
        .sigev_notify            = SIGEV_THREAD,
        .sigev_signo             =                       0,
        .sigev_value.sival_ptr   = pNewTimer,
        .sigev_notify_function   = _timerExpirationWrapper,
        .sigev_notify_attributes = NULL
    };

    if( status == true )
    {
        IotLogDebug( "Creating new timer %p.", pNewTimer );

        /* Set the timer expiration routine and argument. */
        pNewTimer->threadRoutine = expirationRoutine;
        pNewTimer->pArgument = pArgument;

        /* Create the underlying POSIX timer. */
        if( timer_create( CLOCK_REALTIME,
                        &expirationNotification,
                        &( pNewTimer->timer ) ) != 0 )
        {
            IotLogError( "Failed to create new timer %p. errno=%d.", pNewTimer, errno );
            UnityMalloc_FreeResource();
            status = false;
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

void IotClock_TimerDestroy( IotTimer_t * pTimer )
{
    IotLogDebug( "Destroying timer %p.", pTimer );

    /* Decrement the number of platform resources in use. */
    UnityMalloc_FreeResource();

    /* Destroy the underlying POSIX timer. */
    if( timer_delete( pTimer->timer ) != 0 )
    {
        /* This block should not be reached; log an error and abort if it is. */
        IotLogError( "Failed to destroy timer %p. errno=%d.", pTimer, errno );

        abort();
    }
}

/*-----------------------------------------------------------*/

bool IotClock_TimerArm( IotTimer_t * pTimer,
                        uint32_t relativeTimeoutMs,
                        uint32_t periodMs )
{
    bool status = true;
    struct itimerspec timerExpiration =
    {
        .it_value    = { 0 },
        .it_interval = { 0 }
    };

    IotLogDebug( "Arming timer %p with timeout %lu and period %lu.",
                 pTimer,
                 relativeTimeoutMs,
                 periodMs );

    /* Calculate the initial timer expiration. */
    if( IotClock_TimeoutToTimespec( relativeTimeoutMs,
                                    &( timerExpiration.it_value ) ) == false )
    {
        IotLogError( "Invalid relative timeout." );

        status = false;
    }

    if( status == true )
    {
        /* Calculate the timer expiration period. */
        if( periodMs > 0 )
        {
            timerExpiration.it_interval.tv_sec = ( time_t ) ( periodMs / MILLISECONDS_PER_SECOND );
            timerExpiration.it_interval.tv_nsec = ( long ) ( ( periodMs % MILLISECONDS_PER_SECOND ) * NANOSECONDS_PER_MILLISECOND );
        }

        /* Arm the underlying POSIX timer. */
        if( timer_settime( pTimer->timer, TIMER_ABSTIME, &timerExpiration, NULL ) != 0 )
        {
            IotLogError( "Failed to arm timer %p. errno=%d.", pTimer, errno );

            status = false;
        }
    }

    return status;
}

/*-----------------------------------------------------------*/
