#ifndef THREADMANAGER_H
#define	THREADMANAGER_H

#include "defaults.h"

void thr_mgr_downloadallfiles(prgstx *);

int thr_mgr_downloadallchunks(const char *resultdir,
		lnk *link, lnk_http_header *linkh);
int create_chunk_bounds(chunk_bounds **bounds, lnk *link,
		lnk_http_header *lnkh, file_fd fd, maptbl **mptbl);



#endif /* THREADMANAGER_H */
