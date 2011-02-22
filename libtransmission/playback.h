/******************************************************************************
 * $Id: playback.h
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

#ifndef TR_PLAYBACK_H
#define TR_PLAYBACK_H 1

#include <transmission.h>
#include <session.h>




#ifdef SUPPORT_TCP_FEEDER

void tr_playbackInit( tr_session * session );
void tr_playbackFinish( tr_session * session );

/* Set the played torrent to tor, any other torrent will be ignored */
void tr_playbackSetTorrent( const tr_torrent * tor );

/* tell the player that newbytes in order bytes were added to the torrent tor */
void tr_playbackSetWrittenBytes( const tr_torrent * tor, uint64_t newbytes );

#else

#define tr_playbackInit( x )
#define tr_playbackFinish( x )
#define tr_playbackSetWrittenBytes( x, y )
#define tr_playbackSetTorrent( x )

#endif



#endif