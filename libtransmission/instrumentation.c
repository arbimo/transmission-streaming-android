/******************************************************************************
 * $Id: instrumenation.c 
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

#include "event.h"
#include "instrumentation.h"
#include "fdlimit.h"
#include "session.h"
#include "transmission.h"
#include "utils.h"



/**
***    Instrumentation
**/

void
tr_instruInit( tr_session * session )
{
    assert( tr_isSession( session ) );
    
    if( session->isInstruEnabled )
    {
        /* building file name */
        const char * configDir;
        char  timeStr[30];
        char * logfile;
        
        configDir = tr_sessionGetConfigDir( session );
        tr_getLogTimeStr( timeStr, 29 );
        logfile = tr_malloc( strlen(configDir) + 35 );
        strcpy( logfile, configDir );
        if( logfile[strlen(logfile) - 1] != '/')
            strcat(logfile, "/");
        strcat( logfile, timeStr );
        strcat( logfile, ".log" );
        
        /* opening log file */
        session->fd_instru = tr_open_file_for_writing( logfile );
        if( session->fd_instru == -1)
        {
            /* Can't open file, disabling instrumentation */
            session->fd_instru = 0;
            session->isInstruEnabled = FALSE;
            tr_err( "Cannot open instrumentation file, turning instrumentation off" );
        }
        else
        {
            tr_dbg( "Starting instrumentation in file %s", logfile );
            tr_instruMsg( session, "This is an instrumentation file !!!" );
        }
        tr_free( logfile );
    }
    else
    {
        /* Instrumentation is disabled */
        session->fd_instru = 0;
        tr_inf("No instrumentation");
    }
}

void
tr_instruUninit( tr_session * session )
{
    assert( tr_isSession( session ) );
    
    if( session->isInstruEnabled && session->fd_instru != 0 )
    {
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


/**
***    Instrumentation 
**/

void
tr_instruMsg( tr_session * session, const char * fmt, ... )
{
    
    assert( tr_isSession( session ) );
    
    if( session->isInstruEnabled )
    {
        va_list           args;
        char              timestr[64];
        struct evbuffer * buf = evbuffer_new( );
        
        assert( session->fd_instru > 0 );
        
        evbuffer_add_printf( buf, "[%s] ",
                            tr_getLogTimeStr( timestr, sizeof( timestr ) ) );
                            
        va_start( args, fmt );
        evbuffer_add_vprintf( buf, fmt, args );
        evbuffer_add_printf( buf, "\n" );
        
        evbuffer_write( buf, session->fd_instru );
    }
    
}
