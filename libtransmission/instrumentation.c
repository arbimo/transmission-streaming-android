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
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "event.h"
#include "instrumentation.h"
#include "fdlimit.h"
#include "session.h"
#include "transmission.h"
#include "utils.h"

char* getDateStr( char * dateStr, int buflen );
char* getFileName( tr_session * session, char * logfile, int buflen );
char* getInstruDir( tr_session * session, char * instruDir, int buflen );



void
tr_instruInit( tr_session * session )
{
    assert( tr_isSession( session ) );

    if( session->isInstruEnabled )
    {
        /* building file name */
        char logfile[512];
        char filename[256];
        struct stat sb;

        getInstruDir( session, logfile, 256 );
        if( stat( logfile, &sb ) )
        {
            /* Folder doesn't exist yet, we create it */
            tr_mkdirp( logfile, 0777 );
        }

        strcat( logfile, getFileName( session, filename, 256 ) );

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


/**** Utils ****/


char*
getDateStr( char * dateStr, int buflen )
{
    struct tm      now_tm;
    struct timeval tv;
    time_t         seconds;
    char buff[buflen -5];

    gettimeofday( &tv, NULL );

    seconds = tv.tv_sec;
    tr_localtime_r( &seconds, &now_tm );
    strftime( buff, buflen-5, "%m-%d_%H-%M-%S", &now_tm );
    snprintf( dateStr, buflen, "%s.%03d", buff, (int) (tv.tv_usec / 1000) );
    return dateStr;
}

char*
getFileName( tr_session * session, char * logfile, int buflen )
{
    char dateStr[64];
    char hostname[100];

    getDateStr( dateStr, 64 );
    gethostname( hostname, 100 );
    snprintf( logfile, buflen, "%s__%s:%d.log", dateStr, hostname, session->peerPort );

    return logfile;
}

char *
getInstruDir( tr_session * session, char * instruDir, int buflen )
{
    const char * configDir = tr_sessionGetConfigDir( session );
    strncpy( instruDir, configDir, buflen );
    if( instruDir[strlen(instruDir) - 1] != '/' )
        strncat( instruDir, "/", buflen );

    strncat(instruDir, "instrumentation/", buflen );

    return instruDir;
}
