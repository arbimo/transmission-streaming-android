/******************************************************************************
 * $Id: playback.c
 *
 * Copyright (c) 2011 Arthur Bit-Monnot
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

#include "playback.h"

#include "tcp-server.h"
#include "trust_update.h"

#include "transmission.h"
#include "completion.h"
#include "event.h"
#include "torrent.h"


#define UPDATE_TIME_MSEC 500

/** specific to manip.mp4
 * size : 19010333
 *  duration : 5*60 + 14 = 314
 *  B/s : 60543
 */
#define BUFFER_SIZE 3 * 60543


/* opaque structure to keep informations about the playback */
struct playback
{
    struct peer_update_agent pua;
    struct event * timer;
    tr_torrent * tor;
    uint64_t writtenBytes;
};

static struct playback pb;



/**
 * Return a timer to call the callback function after msec millisecondes
 * cbdata is passed as the third argument
 * this timer can be incremented/relaunched by calling tr_timerAddMsec( )
 */
static struct event *
createTimer( int msec, void (*callback)(int, short, void *), void * cbdata )
{
    struct event * timer = tr_new0( struct event, 1 );
    evtimer_set( timer, callback, cbdata );
    tr_timerAddMsec( timer, msec );
    return timer;
}

static void
SigHandler( int signum UNUSED )
{

}

/**
 * This function is called at list once per second
 * It notifies the player if
 * it should start playing or stop playing
 */
static void
updatePlaybackStatus( int foo UNUSED, short bar UNUSED, void * vsession UNUSED )
{
    if( is_movi_download_finished() )
    {
        /* make sure we are playing and don't call that function anymore */
        start_play();
    }
    else if( pb.writtenBytes > player_get_read_bytes() + BUFFER_SIZE)
    {
        start_play();
        tr_timerAddMsec( pb.timer, UPDATE_TIME_MSEC );
    }
    else
    {
        if( get_play_status() == 1)
            pause_play();
        tr_timerAddMsec( pb.timer, UPDATE_TIME_MSEC );
    }
}

void tr_playbackInit( tr_session * session )
{
    uint8_t mac_addr[MAC_ADDR_LEN];

    tr_sessionSetIncompleteFileNamingEnabled( session, FALSE );

    /* bullshit parameters */
    init_peer_update_agent( &pb.pua, NULL, 7, mac_addr, SigHandler );
    init_cmd_server( );
    init_tcp_serv( );

    pb.writtenBytes = 0;
    pb.tor = NULL;
    pb.timer = NULL;
}

void tr_playbackFinish( tr_session * session UNUSED)
{
    tcp_serv_finish();
    cmd_server_finish();
    stop_peer_update_agent( &pb.pua );

    if( pb.timer != NULL )
        tr_free( pb.timer );
}

/** Right now we only support one playback per session.
 * This function should be called to decide which torrent to use (no multi-file torrents) */
void tr_playbackSetTorrent( const tr_torrent * tor )
{
    if( pb.tor == NULL )
    {
        if( tr_torrentHasMetadata( tor ) && tr_torrentInfo( tor )->fileCount == 1)
        {
            pb.tor = (tr_torrent *) tor;
            player_set_file( tr_torrentFindFile( tor, 0 ) );
            pb.timer = createTimer( UPDATE_TIME_MSEC, updatePlaybackStatus, tor->session );
        }
        else
        {
            fprintf( stderr, "Either no metadata or more than one file \n" );
        }
    }
    else if( pb.tor != tor )
    {
        fprintf( stderr, "Another torrent is already being played \n" );
    }
}

void tr_playbackSetWrittenBytes( const tr_torrent * tor, uint64_t bytes )
{
    if( pb.tor == tor && pb.writtenBytes != bytes )
    {
        /* Inform the server about new bytes  */
        assert( bytes > pb.writtenBytes );

        player_add_write_bytes( (int) ( bytes - pb.writtenBytes ) );
        pb.writtenBytes = bytes;
    }

    /* Is the download finished ? */
    if( pb.writtenBytes == tr_torrentInfo( tor )->totalSize )
        set_movi_finish_download();
}

