/*
 * Amazon FreeRTOS+POSIX V1.0.2
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
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file FreeRTOS_POSIX_utils.c
 * @brief Implementation of utility functions in utils.h
 */

/* C standard library includes. */
#include <stddef.h>

/* FreeRTOS+POSIX includes. */
#include "FreeRTOS_POSIX.h"
#include "FreeRTOS_POSIX/errno.h"
#include "FreeRTOS_POSIX/utils.h"

/*-----------------------------------------------------------*/

size_t UTILS_strnlen( const char * const pcString,
                      size_t xMaxLength )
{
    const char * pcCharPointer = pcString;
    size_t xLength = 0;

    if( pcString != NULL )
    {
        while( ( *pcCharPointer != '\0' ) && ( xLength < xMaxLength ) )
        {
            xLength++;
            pcCharPointer++;
        }
    }

    return xLength;
}

/*-----------------------------------------------------------*/

int UTILS_AbsoluteTimespecToDeltaTicks( const struct timespec * const pxAbsoluteTime,
                                   const struct timespec * const pxCurrentTime,
                                   TickType_t * const pxResult )
{
    int iStatus = 0;
    struct timespec xDifference = { 0 };

    /* Check parameters. */
    if( ( pxAbsoluteTime == NULL ) || ( pxCurrentTime == NULL ) || ( pxResult == NULL ) )
    {
        iStatus = EINVAL;
    }

    /* Calculate the difference between the current time and absolute time. */
    if( iStatus == 0 )
    {
        iStatus = UTILS_TimespecSubtract( pxAbsoluteTime, pxCurrentTime, &xDifference );
        if( iStatus == 1 )
        {
            /* pxAbsoluteTime was in the past. */
            iStatus = ETIMEDOUT;
        }
        else if( iStatus == -1 )
        {
            /* error */
            iStatus = EINVAL;
        }
    }

    /* Convert the time difference to ticks. */
    if( iStatus == 0 )
    {
        iStatus = UTILS_TimespecToTicks( &xDifference, pxResult );
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

int UTILS_TimespecToTicks( const struct timespec * const pxTimespec,
                           TickType_t * const pxResult )
{
    int iStatus = 0;
    uint64_t ullTotalTicks = 0;
    long lNanoseconds = 0;

    /* Check parameters. */
    if( ( pxTimespec == NULL ) || ( pxResult == NULL ) )
    {
        iStatus = EINVAL;
    }
    else if( ( pxTimespec != NULL ) && ( UTILS_ValidateTimespec( pxTimespec ) == false ) )
    {
        iStatus = EINVAL;
    }

    if( iStatus == 0 )
    {
        /* Convert timespec.tv_sec to ticks. */
        ullTotalTicks = ( uint64_t ) configTICK_RATE_HZ * ( uint64_t ) ( pxTimespec->tv_sec );

        /* Convert timespec.tv_nsec to ticks. This value does not have to be checked
         * for overflow because a valid timespec has 0 <= tv_nsec < 1000000000 and
         * NANOSECONDS_PER_TICK > 1. */
        lNanoseconds = pxTimespec->tv_nsec / ( long ) NANOSECONDS_PER_TICK +                  /* Whole nanoseconds. */
                       ( long ) ( pxTimespec->tv_nsec % ( long ) NANOSECONDS_PER_TICK != 0 ); /* Add 1 to round up if needed. */

        /* Add the nanoseconds to the total ticks. */
        ullTotalTicks += ( uint64_t ) lNanoseconds;

        /* Write result. */
        *pxResult = ( TickType_t ) ullTotalTicks;
    }

    return iStatus;
}

/*-----------------------------------------------------------*/

void UTILS_NanosecondsToTimespec( int64_t llSource,
                                  struct timespec * const pxDestination )
{
    long lCarrySec = 0;

    /* Convert to timespec. */
    pxDestination->tv_sec = ( time_t ) ( llSource / NANOSECONDS_PER_SECOND );
    pxDestination->tv_nsec = ( long ) ( llSource % NANOSECONDS_PER_SECOND );

    /* Subtract from tv_sec if tv_nsec < 0. */
    if( pxDestination->tv_nsec < 0L )
    {
        /* Compute the number of seconds to carry. */
        lCarrySec = ( pxDestination->tv_nsec / ( long ) NANOSECONDS_PER_SECOND ) + 1L;

        pxDestination->tv_sec -= ( time_t ) ( lCarrySec );
        pxDestination->tv_nsec += lCarrySec * ( long ) NANOSECONDS_PER_SECOND;
    }
}

/*-----------------------------------------------------------*/

int UTILS_TimespecAdd( const struct timespec * const x,
                       const struct timespec * const y,
                       struct timespec * const pxResult )
{
    int64_t llResult64 = 0;

    /* Check parameters. */
    if( ( pxResult == NULL ) || ( x == NULL ) || ( y == NULL ) )
    {
        return -1;
    }

    /* Perform addition. */
    llResult64 = ( ( ( int64_t ) ( x->tv_sec ) * NANOSECONDS_PER_SECOND ) + ( int64_t ) ( x->tv_nsec ) )
                 + ( ( ( int64_t ) ( y->tv_sec ) * NANOSECONDS_PER_SECOND ) + ( int64_t ) ( y->tv_nsec ) );

    /* Convert result to timespec. */
    UTILS_NanosecondsToTimespec( llResult64, pxResult );

    return ( int ) ( llResult64 < 0LL );
}

/*-----------------------------------------------------------*/

int UTILS_TimespecAddNanoseconds( const struct timespec * const x,
                                  int64_t llNanoseconds,
                                  struct timespec * const pxResult )
{
    struct timespec y = { .tv_sec = ( time_t ) 0, .tv_nsec = 0L };

    /* Check parameters. */
    if( ( pxResult == NULL ) || ( x == NULL ) )
    {
        return -1;
    }

    /* Convert llNanoseconds to a timespec. */
    UTILS_NanosecondsToTimespec( llNanoseconds, &y );

    /* Perform addition. */
    return UTILS_TimespecAdd( x, &y, pxResult );
}

/*-----------------------------------------------------------*/

int UTILS_TimespecSubtract( const struct timespec * const x,
                            const struct timespec * const y,
                            struct timespec * const pxResult )
{

    int iCompareResult = 0;

    /* Check parameters. */
    if( ( pxResult == NULL ) || ( x == NULL ) || ( y == NULL ) )
    {
        return -1;
    }

    iCompareResult = UTILS_TimespecCompare( x, y );

    /* if x < y then result would be negative, return 1 */
    if( iCompareResult == -1 )
    {
        return 1;
    }

    /* if times are the same return zero */
    if( iCompareResult == 0 )
    {
        pxResult->tv_sec = 0;
        pxResult->tv_nsec = 0;
        return 0;
    }

    /* If x > y Perform subtraction. */
    pxResult->tv_sec = x->tv_sec - y->tv_sec;
    pxResult->tv_nsec = x->tv_nsec - y->tv_nsec;

    /* check if nano seconds value needs to borrow */
    if( pxResult->tv_nsec < 0 )
    {
        /* Based on comparison, tv_sec > 0 */
        pxResult->tv_sec--;
        pxResult->tv_nsec += (long) NANOSECONDS_PER_SECOND;
    }

    /* if nano second is negative after borrow, it is an overflow error */
    if( pxResult->tv_nsec < 0 )
    {
        return -1;
    }
    return 0;
}

/*-----------------------------------------------------------*/

int UTILS_TimespecCompare( const struct timespec * const x,
                               const struct timespec * const y)
{
    /* Check parameters */
    if( ( x == NULL ) && ( y == NULL ) )
    {
        return 0;
    }
    else if( y == NULL )
    {
        return 1;
    }
    else if( x == NULL )
    {
        return -1;
    }

    /* Compare seconds and then nano seconds */
    if( x->tv_sec > y->tv_sec )
    {
        return 1;
    }
    else if( x->tv_sec < y->tv_sec )
    {
        return -1;
    }
    else
    {
        if( x->tv_nsec > y->tv_nsec )
        {
            return 1;
        }
        else if(x->tv_nsec < y->tv_nsec)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
}

/*-----------------------------------------------------------*/

bool UTILS_ValidateTimespec( const struct timespec * const pxTimespec )
{
    bool xReturn = false;

    if( pxTimespec != NULL )
    {
        /* Verify 0 <= tv_nsec < 1000000000. */
        if( ( pxTimespec->tv_nsec >= 0 ) &&
            ( pxTimespec->tv_nsec < NANOSECONDS_PER_SECOND ) )
        {
            xReturn = true;
        }
    }

    return xReturn;
}

/*-----------------------------------------------------------*/
