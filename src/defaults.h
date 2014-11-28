/*!
 * \file
 * \brief Main definitions of constants, enums and other types.
 */

#ifndef DEFAULTS_H
#define	DEFAULTS_H

#include <stddef.h>
#include <stdio.h> // stdlog (errors for logging)
#include <errno.h>

// BIG FILES SETTINGS
// #define _LARGEFILE64_SOURCE
#if _FILE_OFFSET_BITS - 0 == 64
#define	STRTOOFF_T strtoll
#else
#define	STRTOOFF_T strtol
#endif

// # define MAX_MAP_SIZE 0x7FFFFFFF;
// long long int MAX_MAP_SIZE = 0x7FFFFFFF;

#define	stdlog stderr
#define	log_ERROR "ERROR:"

typedef enum
{
	HTTP, FTP, UNKNOWN
} protocols;

#define	D_CHUNKS 1
#define	D_RESULT_DIR "./"

#define	D_NUMLINKS 0
#define	D_LINKS NULL

typedef struct
{
	int chunks;
	const char *resultdir;

	int numlinks;
	char **links;
	int ipv6;
} prgstx;

typedef struct
{
	protocols prot;
	char *hostname;
	char *rquri;
	char *filename;
	int chunknum;

} lnk;

#define	CHUNK_NUM_DEF 1
#define	URI_BASENAME "/"

// HTTPCLIENT.H-----------------------------------------------------------------
#ifdef HTTP_IPV6_SOCKS
#undef HTTP_IPV6_SOCKS // sockets on ipv6 protocol
#endif

#define	PROTOCOL_HTTP "http://"
#define	PROTOCOL_FTP "ftp://"
#define	HTTP_VERSION "HTTP/1.1"
#define	HTTP_METHOD_GET "GET"
#define	HTTP_METHOD_HEAD "HEAD"
#define	HTTP_HEAD_CONTLEN "Content-Length:"
#define	HTTP_HEAD_CONTTYPE "Content-Type:"

#define	HTTP_CONTTYPE_DEF "text/plain"

#define	HTTP_STATUSCODE_OK 200
#define	HTTP_STATUSCODE_PARTIAL 206
#define	HTTP_STATUSCODE_BAD_RANGE 416
#define	HTTP_PORT 80
#define	HTTP_RQ_HOST "Host:"
#define	HTTP_RQ_RANGE_BYTES "Range: bytes="
#define	RANGE_BYTES_MAX_LEN 12
#define	CRLF "\r\n"
#define	WS " "

typedef enum
{
	STAT_UNKNOWN, INFORM, SUCCESS, REDIRECT, CLIENT_ERR, SERVER_ERR
} http_statcode_grp;

typedef int statcode;

typedef int http_sockfd;
typedef int file_fd;

// typedef struct
// {
//	char *memory;
//	long long int memlen;
// } mapitem;

typedef struct
{
	char *memory;
	long long int memlen;
} maptbl;

typedef struct
{
	off_t clen;
	char *ctype;
	http_statcode_grp statcodegrp;
} lnk_http_header;

// -----------------------------------------------------------------------------

typedef struct
{
	long long int startpos, endpos;
	size_t memlen;
	lnk_http_header *lnk_header;
	lnk *lnk;
	file_fd fd;
	int mpidx;
	char *memory;
} chunk_bounds;

typedef struct
{
	char *hdata;
	size_t hsize;
	char *remain;
	size_t rlen; // without \0 character
} headerbufs;

extern int errno;

#endif /* DEFAULTS_H */
