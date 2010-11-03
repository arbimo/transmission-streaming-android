/*
 * This file Copyright (C) 2008-2010 Mnemosyne LLC
 *
 * This file is licensed by the GPL version 2.  Works owned by the
 * Transmission project are granted a special exemption to clause 2(b)
 * so that the bulk of its code can remain under the MIT license.
 * This exemption does not extend to derived works not owned by
 * the Transmission project.
 *
 * $Id: web.h 10535 2010-04-28 00:32:43Z charles $
 */

#ifndef TR_HTTP_H
#define TR_HTTP_H

struct tr_address;

void tr_webInit( tr_session * session );

typedef enum
{
    TR_WEB_CLOSE_WHEN_IDLE,
    TR_WEB_CLOSE_NOW
}
tr_web_close_mode;

void tr_webClose( tr_session * session, tr_web_close_mode close_mode );

typedef void ( tr_web_done_func )( tr_session       * session,
                                   long               response_code,
                                   const void       * response,
                                   size_t             response_byte_count,
                                   void             * user_data );

const char * tr_webGetResponseStr( long response_code );

void         tr_webRun( tr_session        * session,
                        const char        * url,
                        const char        * range,
                        tr_web_done_func    done_func,
                        void              * done_func_user_data );

struct evbuffer;

void tr_http_escape( struct evbuffer *out, const char *str, int len, tr_bool escape_slashes );

char* tr_http_unescape( const char * str, int len );

#endif
