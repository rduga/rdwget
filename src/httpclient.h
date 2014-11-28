#ifndef HTTPCLIENT_H
#define	HTTPCLIENT_H
#include "defaults.h"

http_sockfd http_socket(void);

int http_connect(http_sockfd *sockfd, const lnk *link);
int http_close(http_sockfd sockfd);

statcode link_header_parse(char *buff, lnk_http_header* linkh);
int http_header_req(http_sockfd sockfd, lnk *link);
int http_header_res(http_sockfd sockfd, lnk_http_header *linkh);
int http_header_read(http_sockfd sockfd, headerbufs *hbufs);

int http_link_header(lnk *link, lnk_http_header **linkhp);

int http_chunk_req(http_sockfd sockfd, chunk_bounds* bounds);
int http_chunk_res(http_sockfd sockfd, char *memory, size_t memlen,
		chunk_bounds* bounds);
int http_link_write_chunk(chunk_bounds* bounds);

http_statcode_grp http_str2statuscode_grp(char *code);

#endif /* HTTPCLIENT_H */
