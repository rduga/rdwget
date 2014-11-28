/*!
 * \file
 * \brief Simple HTTP client.
 *
 *  Simple HTTP client able to obtain header information and download files by
 *  range specifiers (if servers enables it.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>		// gethostbyname, h_errno
#include <arpa/inet.h>		// inet_pton
#include <string.h>		// strlen, NULL
#include <unistd.h>		// rite
#include <assert.h>

#include "httpclient.h"
#include "linkparser.h"

#define	HTTP_BUFF_SIZE 100

static long long int MAX_MAP_SIZE = (long long int) 0x80000000;

extern int h_errno;
// extern int errno;


static const char *http_header =
// REQUEST LINE
		HTTP_METHOD_HEAD	// method
		" %s "			// request-uri
		HTTP_VERSION		// http version
		CRLF			// CRLF
		// REQUEST HEADER
		HTTP_RQ_HOST
		" %s "			// hostname
		CRLF CRLF;

static const char *http_chunk =
// REQUEST LINE
		HTTP_METHOD_GET		// method
		" %s "			// request-uri
		HTTP_VERSION		// http version
		CRLF			// CRLF
		// REQUEST HEADER
		HTTP_RQ_HOST
		" %s "			// hostname
		CRLF
		// RANGE RETRIEVAL REQUEST
		HTTP_RQ_RANGE_BYTES
		"%s"			// first byte pos
		"-"
		"%s"			// last byte pos
		CRLF CRLF;


/**
 * Sends request for header information to socket about link specified in lnk.
 * \return 0 on success, -1 on fail.
 */
int
http_header_req(http_sockfd sockfd, lnk *link)
{
	char * hd_rq_str;
	size_t hd_len = _sprintf(2, &hd_rq_str, http_header, link->rquri,
			link->hostname);
	if (write(sockfd, (const void *) hd_rq_str, hd_len) == -1) {
		fprintf(stdlog, log_ERROR
		"header file couldn't be sent in link:%s", link->hostname);
		return (-1);
	}
	free(hd_rq_str);

	return (0);
}

/**
 * Recieves header from http server, parses it and saves found information
 * into linh.
 * \return 0 on success, -1 on fail.
 */
int
http_header_res(http_sockfd sockfd, lnk_http_header *linkh)
{
	headerbufs *hbufs = malloc(sizeof (headerbufs));
	if (http_header_read(sockfd, hbufs) == -1)
		return (-1);
	if (link_header_parse(hbufs->hdata, linkh) != HTTP_STATUSCODE_OK)
		return (-1);

	free(hbufs->hdata);
	free(hbufs->remain);
	free(hbufs);

	return (0);
}

/*
 * Function converts char* status code into http_statcode_grp enum.
 * \return	if code is NULL or unknown returns UNKNOWN
 * 		else returns value from http_statcode_grp enum.
 */
http_statcode_grp
http_str2statuscode_grp(char *code)
{
	char nr;
	if (code == NULL)
		return (UNKNOWN);
	nr = code[0];
	if (nr == '1')
		return (INFORM);
	if (nr == '2')
		return (SUCCESS);
	if (nr == '3')
		return (REDIRECT);
	if (nr == '4')
		return (CLIENT_ERR);
	if (nr == '5')
		return (SERVER_ERR);
	return (UNKNOWN);
}

/**
 * Creates socket for http client.
 * \return http_sockfg (socket filedescriptor)
 */
http_sockfd
http_socket(void)
{
#ifdef HTTP_IPV6_SOCKS
	return (socket(AF_INET6, SOCK_STREAM, 0));
#else
	return (socket(AF_INET, SOCK_STREAM, 0));
#endif
}

/**
 * Connects to socket to hostname specified in link.
 * \return 0 on success, -1 on fail.
 */
int
http_connect(http_sockfd *sockfd, const lnk *link)
{
	*sockfd = http_socket();

	struct hostent *hp;
	const char *hstr_err;

	if ((hp = gethostbyname(link->hostname)) == NULL) {
		hstr_err = hstrerror(h_errno);
		fprintf(stdlog,
		log_ERROR "Couldn't resolve host name in link: %s message:%s\n",
		link->hostname, hstr_err);
		return (-1);
	}

#ifdef HTTP_IPV6_SOCKS
	struct sockaddr_in6 sin;
	sin.sin6_family = AF_INET6;
	sin.sin6_port = htons(HTTP_PORT);
	sin.sin6_addr = *((struct in6_addr *)hp->h_addr_list[0]);
#else
	struct sockaddr_in sin;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(HTTP_PORT);
	sin.sin_addr = *((struct in_addr *) hp->h_addr_list[0]);
#endif
	if (connect(*sockfd, (struct sockaddr *) &sin,
					sizeof (sin)) == -1) {
		fprintf(stdlog, log_ERROR "Could't connect to hostname: %s\n",
				link->hostname);
		return (-1);
	}

	return (0);
}

/**
 * Function obtains header data from http sever.
 * Function connects to hostname specified
 * in link, requests and receives header information, parses it and saves it
 * into allocated structure lnk_http_header, saves pointer to this structure
 * into linkhp.
 * \return 0 on success, -1 on fail.
 */
int
http_link_header(lnk *link, lnk_http_header **linkhp)
{
	http_sockfd sockfd;
	if (http_connect(&sockfd, link) == -1)
		return (-1);
	*linkhp = malloc(sizeof (lnk_http_header));

	if ((http_header_req(sockfd, link)) == -1)
		return (-1);
	if ((http_header_res(sockfd, *linkhp)) == -1)
		return (-1);

	return (http_close(sockfd));
}

/**
 * Sends request for range data to socket specified in bounds structure.
 * \return 0 on success, -1 on fail.
 */
int
http_chunk_req(http_sockfd sockfd, chunk_bounds* bounds)
{
	char *ch_rq_str;
	char *sstartpos;
	char *sendpos;

	sstartpos = malloc(RANGE_BYTES_MAX_LEN);
	sendpos = malloc(RANGE_BYTES_MAX_LEN);

	snprintf(sstartpos, RANGE_BYTES_MAX_LEN, "%li", bounds->startpos);
	snprintf(sendpos, RANGE_BYTES_MAX_LEN, "%li", bounds->endpos);

	size_t hd_len = _sprintf(4, &ch_rq_str, http_chunk, bounds->lnk->rquri,
			bounds->lnk->hostname, sstartpos, sendpos);
	if (write(sockfd, (const void *) ch_rq_str, hd_len) == -1) {
		fprintf(stdlog, log_ERROR
				"chunk request couldn't be sent in link:%s",
				bounds->lnk->hostname);
		return (-1);
	}
	return (0);
}

/**
 * Parses http header contained in buffer (ended with '\\0' character).
 * Function fills linkh with specified information and returns
 * status code contained in header.
 * \return	On fail returns -1 (if header doesn't contain status code
 *		(if header is invalid))
 */
statcode
link_header_parse(char *buff, lnk_http_header* linkh)
{
	char *tok, *hd, *hd_protocol, *hd_statuscode; //  *hd_reason_phase;
	char *occur;
	char *slholder, *hdholder;

	size_t head_len_conl = strlen(HTTP_HEAD_CONTLEN);
	size_t head_len_cont = strlen(HTTP_HEAD_CONTTYPE);

	linkh->clen = 0;
	linkh->ctype = HTTP_CONTTYPE_DEF;
	linkh->statcodegrp = UNKNOWN;

	tok = _strtok(&hdholder, buff, CRLF);

	hd = strdup(tok);
	hd_protocol = _strtok(&slholder, hd, WS);
	hd_statuscode = _strtok(&slholder, NULL, WS);
	// d_reason_phase = strdup(slholder);// strtok(&slholder, NULL, WS);

	if (hd_statuscode == NULL) {
		fprintf(stdlog, log_ERROR "Couldn't read link response"
				" status line (in parsing)");
		return (-1);
	}

	linkh->statcodegrp = http_str2statuscode_grp(hd_statuscode);

	while ((tok = _strtok(&hdholder, NULL, CRLF)) != NULL) {
		if ((occur = strstr(tok, HTTP_HEAD_CONTLEN)) != NULL) {
			linkh->clen = STRTOOFF_T(occur+head_len_conl, NULL, 10);
			if (errno == ERANGE) {
				fprintf(stdlog, log_ERROR
						"Unrecognized content "
						"length (value out of range)");
				return (-1);
			}
			continue;
		}
		if ((occur = strstr(tok, HTTP_HEAD_CONTTYPE)) != NULL) {
			linkh->ctype = _trim(strdup(occur+head_len_cont));
		}
	}

	if ((linkh->clen == 0) || (linkh->ctype == '\0')) {
		fprintf(stdlog, log_ERROR "Response header doesn't"
				" contain length or content type");
		return (-1);
	}

	return (atoi(hd_statuscode));
	//  return (0);
}

/**
 * Function is dedicated to insert header into buffer obtained from socket.
 * Function inserts header into buffer (hbufs->hdata) and saves
 * the specified lenght of header (hbufs->hsize (incl. end zero character
 * '\\0')).
 * Function reads header into buffer of specified size so if not only header
 * was requested it saves the remain of read buffer (after ending header) into
 * hbufs->remain. hbufs->rlen is length of remain (not ended with \0).
 * If only header was requested hbufs->remain is set to NULL and hbufs->rlen is
 * zero.
 * \return 0 on success, -1 on fail.
 */
int
http_header_read(http_sockfd sockfd, headerbufs *hbufs)
{
//	hbufs->
	size_t size = 1;
	size_t osize;
	size_t curr_buf_size = HTTP_BUFF_SIZE;
	char *rpos = '\0';
	char *buff = malloc(curr_buf_size);

	char http_buf[HTTP_BUFF_SIZE];
	ssize_t sz;

	while ((sz = read(sockfd, http_buf, HTTP_BUFF_SIZE)) > 0) {
		osize = size;
		size += sz;
		while (size > curr_buf_size) {
			curr_buf_size *= 2;
			if ((buff = realloc(buff, curr_buf_size)) == NULL) {
				fprintf(stdlog, log_ERROR "http buffer "
						"couldn't be reallocated");
				return (-1);
			}
		}
		memcpy(buff + osize - 1, http_buf, sz);
		buff[size - 1] = '\0';
		if ((rpos = strstr(buff, CRLF CRLF)) != NULL)
		break;
	}

	if (rpos == NULL) {
		fprintf(stdlog, log_ERROR "Header couldn't be read!");
		return (-1);
	}

	hbufs->rlen = buff + size - rpos - 4 - 1;
	// trlen(rpos) - 4; // + 1 - 4) +1 (NULL) - 4 CRLF CRLF

	if (hbufs->rlen > 0) {
		hbufs->remain = malloc(hbufs->rlen);
		memcpy(hbufs->remain, rpos + 4, hbufs->rlen); // 4 - CR LF CR LF
	} else {
		hbufs->remain = NULL;
	}

	hbufs->hsize = rpos - buff + 1; // +1 (NULL)
	hbufs->hdata = realloc(buff, hbufs->hsize);
	hbufs->hdata[hbufs->hsize-1] = '\0';

	return (0);
}

/**
 * Recieves data of range specified in bounds and writes it into memory buffer.
 * Data were requested by
 * function http_chunk_req(http_sockfd sockfd, chunk_bounds* bounds).
 * \return 0 on success, -1 on fail.
 */
int
http_chunk_res(http_sockfd sockfd, char *memory, size_t memlen,
		chunk_bounds* bounds) // ODO bounds pomocna
{
	headerbufs *hbufs = malloc(sizeof (headerbufs));
	lnk_http_header* linkh = malloc(sizeof (lnk_http_header));
	statcode scode;
	size_t toread;
	size_t readed;
	file_fd chfd;
	long long int readsz = 0;
	char *wbuffer;
	int wbuffersize = 10000;

	if (http_header_read(sockfd, hbufs) == -1)
		return (-1);
	if ((scode = link_header_parse(hbufs->hdata, linkh))
			!= HTTP_STATUSCODE_PARTIAL) {
		fprintf(stdlog, log_ERROR
				"Response message not PARTIAL CONTENT:"
				" status code:%i", scode);
		return (-1);
	}

	if (linkh->clen != memlen) {
		fprintf(stdlog,
				log_ERROR "Range in response douesn't"
				" despond to range in request\n");
		return (-1);
	}

	toread = memlen - hbufs->rlen;
	readsz = hbufs->rlen;

	// memory couldn't be mapped, write directly into file
	if (memory == NULL) {
		if ((chfd = dup(bounds->fd)) == -1) {
			fprintf(stdlog, log_ERROR
					"Cannot duplicate fd for chunk %s,"
					"Aborting", bounds->lnk->rquri);
			return (-1);
		}

		if (lseek(chfd, ((long long int)bounds->mpidx) * MAX_MAP_SIZE +
				bounds->startpos, SEEK_SET) == -1) {
			fprintf(stdlog, log_ERROR
					"Cannot lseek file for chunk %s,"
					"Aborting", bounds->lnk->rquri);
			return (-1);
		}
		// TODO
		if (write(chfd, hbufs->remain, hbufs->rlen) != hbufs->rlen) {
			fprintf(stdlog, log_ERROR
					"Cannot write whole buffer into file"
					" for chunk %s, Aborting",
					bounds->lnk->rquri);
			return (-1);
		}

		wbuffer = malloc(wbuffersize);

		if (wbuffersize > toread) {
			wbuffersize = toread;
		}

		while ((toread > 0) && ((readed =
				read(sockfd, wbuffer, wbuffersize)) > 0)) {
			readsz += readed;
			memory += readed;
			toread -= readed;

			write(chfd, wbuffer, wbuffersize);

			if (wbuffersize > toread) {
				wbuffersize = toread;
			}
		}

		free(hbufs->hdata);
		free(hbufs->remain);
		free(hbufs);

		close(chfd);
		free(wbuffer);
		return (0);
	}

	// copy data into mapped memory
	memcpy(memory, hbufs->remain, hbufs->rlen);

	memory += hbufs->rlen;
	while ((toread > 0) && ((readed = read(sockfd, memory, toread)) > 0)) {
		readsz += readed;
		memory += readed;
		toread -= readed;
	}

	if (toread > 0) {
		perror("read");
		fprintf(stdlog,
				log_ERROR
				"Cannot write whole buffer into mapped memory "
				"for chunk %s,"
				"Aborting", bounds->lnk->rquri);
		return (-1);
	}

	free(hbufs->hdata);
	free(hbufs->remain);
	free(hbufs);

	return (0);
}

/**
 * Function obtains chunk data bounds from http sever and writes it into memory.
 * Function connects to hostname specified
 * in link, requests for range data (specified by rfc2616 (Range:
 * bytes=firstbytepos - lastbytepos) receives data and saves it into
 * memory specified in bounds->memory of length bounds->memlen (range size)
 * \return 0 on success, -1 on fail.
 */
int
http_link_write_chunk(chunk_bounds* bounds)
{
	http_sockfd sockfd;
	if (http_connect(&sockfd, bounds->lnk) == -1)
		return (-1);
	if (http_chunk_req(sockfd, bounds) == -1)
		return (-1);
	if ((http_chunk_res(sockfd, bounds->memory,
			bounds->memlen, bounds)) == -1)
		return (-1);

	return (http_close(sockfd));
}

/**
 * Closes http socket.
 * \return 0 on success, -1 on fail.
 */
int
http_close(http_sockfd sockfd)
{
	char *strerr;
	if (close(sockfd) != 0) {
		strerr = strerror(errno);
		fprintf(stdlog, strerr);
		free(strerr);
		return (-1);
	}
	return (0);
}
