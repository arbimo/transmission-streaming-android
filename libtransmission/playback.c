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


#define UPDATE_TIME_MSEC 2000

/** manip.mp4
 * size : 19010333
 *  duration : 5*60 + 14 = 314
 *  B/s : 60543
 * buffer : 5s -> 302715
 */
#define BUFFER_SIZE 302715


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

void SigHandler( int signum UNUSED )
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
    tr_piece_index_t it;
    uint32_t sum = 0;


    /* check */
    for( it = 0 ; it < tr_cpNextInOrdrerPiece( &pb.tor->completion ) ; it++)
        sum += tr_torPieceCountBytes( pb.tor, it );

    fprintf( stderr, "\nPeriodicCheck %d \n written bytes : %llu  - sum: %u -   readBytes = %llu \n\n",
             is_player_ready(),
             pb.writtenBytes, sum, player_get_read_bytes() );

    if( !get_play_status() && pb.writtenBytes > player_get_read_bytes() + BUFFER_SIZE )
    {
        if( is_player_ready() )
        {
            fprintf( stderr, "Continue play \n" );
            continue_play();
        }
        else
        {
            start_play();
            fprintf( stderr, "Continue play \n" );
        }
    }
    else
    {
        fprintf( stderr, "Else \n" );
    }

    tr_timerAddMsec( pb.timer, UPDATE_TIME_MSEC );
}

void tr_playbackInit( tr_session * session UNUSED )
{
    uint8_t mac_addr[MAC_ADDR_LEN];

    /* bullshit parameters */
    init_peer_update_agent( &pb.pua, NULL, 7, mac_addr, SigHandler );
    init_cmd_server( );
    init_tcp_serv( );

    pb.writtenBytes = 0;
    pb.tor = NULL;
}

void tr_playbackFinish( tr_session * session UNUSED)
{
    tcp_serv_finish();
    cmd_server_finish();
    stop_peer_update_agent( &pb.pua );
}

/** Right now we only support one playback per session.
 * This function should be called to decide which to use (no multi-file torrents) */
void tr_playbackSetTorrent( const tr_torrent * tor )
{
    if( pb.tor == NULL )
    {
        if( tr_torrentHasMetadata( tor ) && tr_torrentInfo( tor )->fileCount == 1)
        {
            pb.tor = (tr_torrent *) tor;
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
        assert( bytes > pb.writtenBytes );
        player_add_write_bytes( (int) ( bytes - pb.writtenBytes ) );
        pb.writtenBytes = bytes;
    }

    if( pb.writtenBytes == tr_torrentInfo( tor )->totalSize )
        set_movi_finish_download();
}

