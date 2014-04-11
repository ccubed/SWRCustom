/*
 * Copyright (c) 2008-2010 Kai Braaten
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef _SWR2_OS_AMIGA_H_
#define _SWR2_OS_AMIGA_H_

#ifdef __STORMGCC__
typedef long clock_t;
typedef int ssize_t;
#ifdef __cplusplus
extern "C" {
#endif
int isascii( int );
#ifdef __cplusplus
}
#endif
#endif /* __STORMGCC__ */

#ifdef __MORPHOS__
#include <proto/socket.h>
#endif

#include <sys/filio.h>

#if defined(SWR2_USE_DLSYM) && defined(__MORPHOS__)
#include <proto/dynload.h>
#endif

#include <utility/tagitem.h>
#include <exec/exec.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef AMIGA
#include <lineread.h>
#endif

#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <db.h>

typedef LONG socklen_t;
static const int INVALID_SOCKET = -1;
static const int SOCKET_ERROR = -1;
#define GETERROR Errno()
#define closesocket CloseSocket
typedef int SOCKET;
static const int MSG_NOSIGNAL = 0;
typedef unsigned char sockbuf_t;

#endif /* include guard */

