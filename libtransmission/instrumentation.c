/******************************************************************************
 * $Id: instrumentation.c 
 *
 * Copyright (c) 2010 Arthur Bit-Monnot
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#include <assert.h>
#include <time.h>
#include <sys/stat.h>

#include "event.h"
#include "instrumentation.h"
#include "fdlimit.h"
#include "session.h"
#include "transmission.h"
#include "utils.h"

char*
tr_getDateStr( char * buf, int buflen )
{
    struct tm      now_tm;
    struct timeval tv;
    time_t         seconds;

    gettimeofday( &tv, NULL );

    seconds = tv.tv_sec;
    tr_localtime_r( &seconds, &now_tm );
    strftime( buf, buflen, "%m-%d_%H-%M-%S", &now_tm );

    return buf;
}


void
tr_instruInit( tr_session * session )
{
    assert( tr_isSession( session ) );

    if( session->isInstruEnabled )
    {
        /* building file name */
        const char * configDir;
        char instruDir[256];
        char  dateStr[64];
        char * logfile;
        struct stat sb;

        configDir = tr_sessionGetConfigDir( session );
        strcpy( instruDir, configDir );
        if( instruDir[strlen(instruDir) - 1] != '/' )
            strcat( instruDir, "/" );

        strcat(instruDir, "instrumentation/");

        if( stat( instruDir, &sb ) )
        {
            /* Folder doesn't exist yet, we create it */
            tr_mkdirp( instruDir, 0777 );
        }

        tr_getDateStr( dateStr, 64 );
        logfile = tr_malloc( strlen(instruDir) + 70 );
        strcpy( logfile, instruDir );
        strcat( logfile, dateStr );
        strcat( logfile, ".log" );

        /* opening log file */
        session->fd_instru = tr_open_file_for_writing( logfile );
        if( session->fd_instru == -1)
        {
            /* Can't open file, disabling instrumentation */
            session->fd_instru = 0;
            session->isInstruEnabled = FALSE;
            tr_err( "Cannot open instrumentation file, turning instrumentation off - Error : %s"
                    , strerror(errno) );
        }
        else
        {
            tr_dbg( "Starting instrumentation in file %s", logfile );
            tr_instruMsg( session, "# This is an instrumentation for transmission daemon" );
        }
        tr_free( logfile );
    }
    else
    {
        /* Instrumentation is disabled */
        session->fd_instru = 0;
    }
}


void
tr_instruUninit( tr_session * session )
{
    assert( tr_isSession( session ) );

    if( session->isInstruEnabled && session->fd_instru != 0 )
    {
        tr_instruMsg( session, "# ending instrumentation");
        session->isInstruEnabled = FALSE;
        tr_dbg( "Ending instrumentation" );
        tr_close_file( session->fd_instru );
        session->fd_instru = 0;
    }
    else if( session->isInstruEnabled )
    {
        session->isInstruEnabled = FALSE;
    }
}


void
tr_instruMsg( tr_session * session, const char * fmt, ... )
{
    
    assert( tr_isSession( session ) );
    
    if( session->isInstruEnabled )
    {
        va_list           args;
        struct evbuffer * buf = evbuffer_new( );
        struct timeval    tv;

        assert( session->fd_instru > 0 );

        gettimeofday( &tv, NULL );
        evbuffer_add_printf( buf, "%lu.%03d ", tv.tv_sec, (int) tv.tv_usec / 1000 );

        va_start( args, fmt );
        evbuffer_add_vprintf( buf, fmt, args );
        evbuffer_add_printf( buf, " \n" );

        evbuffer_write( buf, session->fd_instru );

        evbuffer_free( buf );
    }
    
}
