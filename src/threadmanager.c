/*!
 * \file
 * \brief Simple threaded manager of files and chunks.
 */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>	// memory mapping
#include "threadmanager.h"
#include "httpclient.h"
#include "linkparser.h"

// extern long long int MAX_MAP_SIZE;
// long long int MAX_MAP_SIZE = 0x7FFFFFFF;
static long long int MAX_MAP_SIZE = (long long int) 0x80000000;

typedef struct
{
	const char *resultdir;
	lnk *link;
} downinfo;

/**
 * Downloads one link (function for one thread).
 * param data of type (downinfo *).
 */
void *
run_download(void *data)
{
	downinfo *dinfo = (downinfo *) data;
	//  lnk* link = dinfo->link;// (lnk *) data;
	lnk_http_header *linkh; // = malloc(sizeof(lnk_http_header));

	if (http_link_header(dinfo->link, &linkh) == -1)
		return (void *) (-1);
	if (thr_mgr_downloadallchunks(dinfo->resultdir, dinfo->link, linkh)
			== -1)
		return (void *) (-1);

	//  sleep(rand() % 5);
//	printf("I AM THREAD rquri: %s with filesize:%lld and type:%s\n",
//			dinfo->link->rquri, linkh->clen, linkh->ctype);

	return (void *) (0);
}

/**
 * Downloads all links specified in prgstx structure by creating thread for
 * every link.
 */
void
thr_mgr_downloadallfiles(prgstx *stx)
{
	int lnkidx = 0;
	int *retval;


	pthread_t *downloaders = malloc(sizeof (pthread_t) * stx->numlinks);
	lnk *links = malloc(sizeof (lnk) * stx->numlinks);
	int *activepthr = malloc(sizeof (int) * stx->numlinks);
	downinfo *dinfo = malloc(sizeof (downinfo) * stx->numlinks);

	// set chunk num
//	links[lnkidx].chunknum = stx->chunks;
//
//	dinfo[lnkidx].link = &links[lnkidx];
//	dinfo[lnkidx].resultdir = stx->resultdir;

//	if (link_parse(stx->links[lnkidx], &links[lnkidx]) != -1) {
//		run_download(&dinfo[lnkidx]);
//	}

	for (lnkidx = 0; lnkidx != stx->numlinks; ++lnkidx) {
		links[lnkidx].chunknum = stx->chunks;
		if ((activepthr[lnkidx] = link_parse(stx->links[lnkidx],
				&links[lnkidx])) != -1) {
			links[lnkidx].chunknum = stx->chunks;
			printf("downloading link %s%s\n",
					links[lnkidx].hostname,
					links[lnkidx].filename);
			dinfo[lnkidx].link = &links[lnkidx];
			dinfo[lnkidx].resultdir = stx->resultdir;
			if (pthread_create(&downloaders[lnkidx], NULL,
					run_download, &dinfo[lnkidx]) != 0) {
				fprintf(stdlog, log_ERROR
						"thread for link %s couldn't "
						"be created\n",
						links[lnkidx].rquri);
				activepthr[lnkidx] = -1;
			}
		}
	}

	printf("\nWait please for downloading all links...\n\n");

	for (lnkidx = 0; lnkidx != stx->numlinks; ++lnkidx) {
		if (activepthr[lnkidx] != -1) {
			pthread_join(downloaders[lnkidx], (void **) &retval);
//			printf("RETVAL: %d\n", (int) retval);
			if (retval == 0) {
				printf("%s successfully downloaded! "
						"(http://%s%s)\n",
						links[lnkidx].filename,
						links[lnkidx].hostname,
						links[lnkidx].rquri);
			}
		}
	}

	free(links);
	free(downloaders);
	free(activepthr);
}

void *
run_download_chunk(void *data)
{
	chunk_bounds *bound = (chunk_bounds *) data;
//	char *memory;
//	// int newfd = dup(bound->fd);
//	if ((memory = mmap(0, bound->memlen, PROT_READ| PROT_WRITE, MAP_SHARED,
//			bound->fd, bound->startpos)) == MAP_FAILED) {
//		perror("mmap");
//		return (void *) (0);
//	}
//	bound->memory = memory;

	// TODO LAST complete http requests and results
	if (http_link_write_chunk(bound) == -1)
		return (void *) (-1);
	//  http_chunk_res();

//	TODO munmap let to file_fdtbl
//	if (munmap(bound->memory, bound->memlen) == -1) {
//		close(bound->fd);
//		return (void *)(-1);
//	}

	//  close(bound->fd);
	return (void *) (0);
}

// allocate file
// resolve filename
// map into memory (mmap)
// create chunk threads, wait for every success, if unsuccessfull
// -> delete file, report error
/**
 * Function creates chunks due to link->chunknum parameter and creates one
 * thread for every chunk and downloads the whole file into resultdir.
 \return 0 on success, -1 on fail.
 */
int
thr_mgr_downloadallchunks(const char *resultdir, lnk *link,
		lnk_http_header *linkh)
{
	file_fd fd;
	char buf = '\0';
//	int idx = 0;
	int chidx = 0;
	int *retval;
	int mgrretval;
	chunk_bounds *bounds;
	maptbl *maptable;
	pthread_t *chunkthrs;

	mk_filename(resultdir, link);

	if ((fd = open(link->filename, O_CREAT| O_EXCL | O_RDWR, S_IRUSR
			| S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
		fprintf(stdlog, log_ERROR
				"Couldn't create file %s ", link->filename);
		perror("open");
		return (-1);
	}

//	if (ftruncate(fd, linkh->clen) != 0)
//	{
//		fprintf(stdlog, log_ERROR
//				"file couldn't be truncated"
//				" to specified size\n");
//	}

	// set filesize
	lseek(fd, linkh->clen - 1, SEEK_SET);
	write(fd, &buf, 1);

	create_chunk_bounds(&bounds, link, linkh, fd, &maptable);
	//  create_chunk_bounds(&bounds, link, linkh, fd);
	chunkthrs = malloc(sizeof (pthread_t) * link->chunknum);


	for (chidx = 0; chidx != link->chunknum; ++chidx) {
		pthread_create(&chunkthrs[chidx], NULL,
				run_download_chunk, &bounds[chidx]);
	}

	mgrretval = 0;
	for (chidx = 0; chidx != link->chunknum; ++chidx) {
		pthread_join(chunkthrs[chidx], (void **) &retval);
		if (retval != 0) {
			mgrretval = (int)retval;
		}
	}

	free(chunkthrs);

	if (maptable->memlen != -1) {
		if (munmap(maptable->memory, maptable->memlen) == -1) {
			perror("munmap");
		}
	}

	free(maptable);

//	fprintf(stdlog, "created:%s\n", link->filename);
	if (close(fd) == -1) {
		fprintf(stdlog, log_ERROR "File descriptor for filename %s "
				"couldn't be closed.\n", link->filename);
		return (-1);
	}

	return (mgrretval);
}

/**
 * Creates chunk bounds and maps file into memory or assignes '\\0' to memory.
 * Creates chunk bounds due to link, linkh parameters, maps file into memory
 * and creates maptable containing list of mapping memory witch its sizes to
 * enable use munmap after chunks will be downloaded.
 * Mapping into memory is processed for last chunks (which have ranges at the
 * end modulo MAX_MAP_SIZE). The first chunks will be assigned '\\0' into
 * chunk memory.
 * \return 0 on success, -1 on fail.
 */
int
create_chunk_bounds(chunk_bounds **bounds, lnk *link, lnk_http_header *lnkh,
		file_fd fd, maptbl **maptable)
{
	int map_times = (int) (lnkh->clen / MAX_MAP_SIZE);
	// modulo size of file moduling MAX_MAP_SIZE
	int map_mod = (int) (lnkh->clen % MAX_MAP_SIZE);

	int mpidx, chidx, challidx; // , idx;
	// piece = size of one piece
	// size = number of items in one mapping memory or res.chunk memory
	long long int actualpos, filepos;
	long long int chunkpiece = lnkh->clen
			/ ((long long int) link->chunknum);

	int chunkmp_size = MAX_MAP_SIZE / chunkpiece;
	long long int chunkmp_piece = MAX_MAP_SIZE
			/ (long long int) chunkmp_size;

	int chunk_res_size;
	long long int chunk_res_piece;
	char *memory;

	*maptable = malloc(sizeof (maptbl));
	(*maptable)->memlen = 0;
	if (map_mod)
		++((*maptable)->memlen);
//	(*maptable)->items = malloc(sizeof (mapitem) * (*maptable)->size);

	if (link->chunknum < map_times + (*maptable)->memlen) {
		fprintf(
		stdlog, log_ERROR "Number of chunks too small for downloading"
			" a big file from link %s, setting chunk num to"
			" %lli\n", link->rquri,
			map_times + (*maptable)->memlen);
		link->chunknum = (int)(map_times + (*maptable)->memlen);
	}

	*bounds = malloc(sizeof (chunk_bounds) * link->chunknum);

	challidx = 0;
	filepos = 0;
	mpidx = 0;

	// chunks which are to mmap MAX_MAP_SIZE replace with NULL memory
	for (; mpidx < map_times; ++mpidx) {
	// allocaate memory and assign it chunks
	// which are smaller than max map size

//		if ((memory = mmap(0, (size_t)MAX_MAP_SIZE, PROT_READ |
//				PROT_WRITE, MAP_SHARED, fd,
//				(off_t) filepos)) == MAP_FAILED) {
//			perror("mmap");
//			memory = '\0';
// //			return (-1);
//		}
//		memory = '\0';

//		(*maptable)->items[mpidx].memlen = MAX_MAP_SIZE;
//		(*maptable)->items[mpidx].memory = memory;

		actualpos = 0;
		for (chidx = 0; chidx < chunkmp_size - 1; ++chidx) {
		// for (chidx = 0; chidx < link->chunknum -1; ++chidx)
			(*bounds)[challidx].memory = NULL;
			(*bounds)[challidx].startpos = actualpos;
			actualpos += chunkmp_piece;
			(*bounds)[challidx].endpos = actualpos -1;
			(*bounds)[challidx].memlen = (size_t) (actualpos -
					(*bounds)[challidx].startpos);
			(*bounds)[challidx].lnk = link;
			(*bounds)[challidx].lnk_header = lnkh;
			(*bounds)[challidx].fd = fd;
			(*bounds)[challidx].mpidx = mpidx;

			++challidx;
		}

		(*bounds)[challidx].memory = NULL;
		(*bounds)[challidx].startpos = actualpos;
		(*bounds)[challidx].endpos = MAX_MAP_SIZE - 1;
		(*bounds)[challidx].memlen = (size_t) MAX_MAP_SIZE - actualpos;
		(*bounds)[challidx].lnk = link;
		(*bounds)[challidx].lnk_header = lnkh;
		(*bounds)[challidx].fd = fd;
		(*bounds)[challidx].mpidx = mpidx;
		++challidx;

		filepos += MAX_MAP_SIZE;
	}

	chunk_res_size = link->chunknum - challidx;
	if (map_mod) { // if modulo of page mapping is greater than 0
		if (map_mod / chunk_res_size == 0) {
			fprintf(stdlog, log_ERROR
					"number of chunks of link %s will be "
					"diminished (-%d chunks) (large file "
					"doesn't fit memory map size)",
					link->rquri, map_times);
			link->chunknum = link->chunknum - map_times;
			chunk_res_size = link->chunknum - challidx;
		}

		if ((memory = mmap(0, (size_t)(lnkh->clen - filepos),
				PROT_READ | PROT_WRITE, MAP_SHARED, fd,
				(off_t) filepos)) == MAP_FAILED) {
			perror("mmap");
			memory = '\0';
//			return (-1);
		}

		(*maptable)->memlen =
			(size_t)(lnkh->clen - filepos);
		(*maptable)->memory = memory;

		assert(map_mod / chunk_res_size > 0);

		chunk_res_piece = map_mod / chunk_res_size;

		actualpos = 0;
		for (chidx = 0; chidx < chunk_res_size - 1; ++chidx) {
		// for (chidx = 0; chidx < link->chunknum -1; ++chidx)
			(*bounds)[challidx].memory = (memory == NULL) ? memory :
					memory + actualpos;
			(*bounds)[challidx].startpos = actualpos;
			actualpos += chunk_res_piece;
			(*bounds)[challidx].endpos = actualpos -1;
			(*bounds)[challidx].memlen = (size_t) (actualpos -
					(*bounds)[challidx].startpos);
			(*bounds)[challidx].lnk = link;
			(*bounds)[challidx].lnk_header = lnkh;
			(*bounds)[challidx].fd = fd;
			(*bounds)[challidx].mpidx = mpidx;
			++challidx;
		}

		(*bounds)[challidx].memory = (memory == NULL) ? memory :
				memory + actualpos;
		(*bounds)[challidx].startpos = actualpos;
		(*bounds)[challidx].endpos = map_mod - 1;
		(*bounds)[challidx].memlen = (size_t) map_mod - actualpos;
		(*bounds)[challidx].lnk = link;
		(*bounds)[challidx].lnk_header = lnkh;
		(*bounds)[challidx].fd = fd;
		(*bounds)[challidx].mpidx = mpidx;
		++challidx;
	}

	assert(challidx == link->chunknum);

	return (0);
}
