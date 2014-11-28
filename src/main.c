#include <stdio.h>
#include <stdlib.h> /* NULL... */
#include <unistd.h>
#include <getopt.h>
#include <string.h>

// #define	NDEBUG // uncomment if not debugging mode
#include <assert.h>

#include "defaults.h"
#include "utils.h"
#include "threadmanager.h"

/**
 * \mainpage
 * Welcome to RDWGET utility for downloading files from http servers by chunks.
 *
 * \section Download-links-and-others Download links and others
 * -	<a href="http://www.assembla.com/spaces/rdwget/new_items">
 * 	Project news</a>
 * -	<a href="http://my-svn.assembla.com/svn/rdwget">
 * 	SVN</a>
 * -	<a href="http://my-trac.assembla.com/rdwget/">
 * 	TRAC</a>
 * -	<a href="http://www.w3.org/Protocols/rfc2616/rfc2616">HTTP
 * 	protocol rfc2616</a>
 *
 * \section Author-and-program-licence Author and program licence
 * - Author:	<a href="mailto:radooo.php5@gmail.com">Radovan Duga</a>
 * - License:	<a href="http://www.gnu.org/licenses/gpl.html">GNU GPL</a>
 * - Web page:	<a href="http://www.lookatthis.ic.cz">www.lookatthis.ic.cz</a>
 *
 * \section NAME
 * <b>rdwget</b> - Threaded wget like file download manager which downloads file
 * specified in http link by chunks.
 *
 * \section SYNOPSIS
 * rdwget [options] http_links...
 *
 * \section DESCRIPTION
 * rdwget is threaded wget like file download manager with simultaneous download
 * chunks for every link (HTTP server must send <b>Content-Length</b> and
 * accept ranges of bytes(<b>Accept-ranges: bytes</b>)).
 *
 * This manager runs chunk-manager thread for every http link. Chunk-manager is
 * thread which creates threads for downloading chunks (number of chunks is set
 * by option -c or -chunks).
 * First Chunk-manager sends request for head of link to server, then the file
 * is created of size specified in header, calculated chunk bounds (with mapping
 * into memory), consenquently
 * created threads (every thread downloads its own chunk)sends request
 * to server for specific range of link, then recevied data written into memory
 * and at the end munmapped memory. If memory is unable to map, it writes into
 * file directly by write function.
 *
 * Downloaded filename consists of hostname and request uri where all slashes
 * were replaced by _ character.
 *
 * \subsection basic-parts-of-program basic parts of program
 * - thread manager for downloading files
 * - thread manager for downloading chunks
 * - simple http client
 * - simple link parser and some supporting functions
 *
 * \subsection Used-UNIX-parts Used UNIX parts
 * -	<b>file operations</b> (we mapping file into mermory and writing chunk
 * 	into memory instead of using write for time optimalization.
 * -	<b>threads</b> (multithreaded downloading of files and chunks of file)
 * -	<b>network communication</b> (http client communicates with remote http
 * 	server)
 *
 *  \section OPTIONS
 *  - <b>-c or --chunks=num</b> (if not specified, default is set to 1
 *  Downloads every http link in num chunks. (default is one chunk)
 *  - <b>-result-dir or -R</b>
 *  Result directory (where files will be downloaded)
 *
 * \section COMPILATION
 * requirements:
 * - linux
 * - gcc comliper
 * - make utility
 *
 * Program is able to compile under LINUX with GNU gcc compiler.
 * Move into "Release" subdirectory. Type "make all".
 * After successful compilation run file "rdwget" with specified options.
 *
 */

/*
 * NECESSARY:
 * 1TODO thread manager for downloading	DONE
 * 1TODO http client			DONE
 * 1TODO simple link parser		DONE
 * 1TODO options			DONE
 *
 * COMPILATION WITH ENABLED DOWNLOADING BIG FILES USE OPTION:
 * -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1
 *
 * NOT INCLUDING:
 * 1TODO Transfer-Encoding: chunked
 *
 * IF LOTS OF TIME REMAINS:
 * 1TODO process bar
 * 1TODO log
 *
 */

// PROGRAM NAME
char *prgname;

// opt args
extern char *optarg;
extern int optind, opterr, optopt;

void
usage(void)
{
	fprintf(stderr,
	"USAGE: %s [options] http_links...\n"
	"OPTIONS:\n"
	"-c or --chunks=num\n"
	"     Downloads every http link in num chunks. (default is one chunk)\n"
	"-R or --resultdir=dir\n"
	"     Result directory (where files will be downloaded,"
	"default is current directory).\n", prgname);
	exit(1);
}

static struct option longopts[] =
	{
		{ "chunks", required_argument, NULL, 'c' },
		{ "result-dir", required_argument, NULL, 'R' },
//		{ "sock-ipv6", no_argument, NULL, '6' }
	};

// application local settings
prgstx programsettings;

void
proc_opts(int argc, char **argv)
{
	int optlen = 2 * length(longopts) + 1;
	char optstring[optlen];

	int ridx, idx;
	int opt;

	// number of files
	int linknum, linkidx;

	memset(optstring, 0, optlen);

	// programsettings init
	programsettings.chunks = D_CHUNKS;
	programsettings.resultdir = D_RESULT_DIR;
	programsettings.numlinks = 0;
	programsettings.links = NULL;
	programsettings.ipv6 = 0;

	// optstring init
	for (idx = 0, ridx = 0; idx != length(longopts); ++idx, ++ridx) {
		assert(ridx < optlen);

		optstring[ridx] = longopts[idx].val;
		if (longopts[idx].has_arg == required_argument)
			optstring[++ridx] = ':';
	}

	while ((opt =
		getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
		switch (opt) {
		case 'c':
			if ((programsettings.chunks = atoi(optarg)) <= 0) {
				fprintf(
				stderr, "number of chunks must be a number\n");
				exit(1);
			}
			break;
		case 'R':
			programsettings.resultdir = optarg;
			break;
//		case '6':
			// # define HTTP_IPV6_SOCKS
			// programsettings.ipv6 = 1;
//			break;
		case '?':
			fprintf(stderr, "unrecognized option: -%c\n", optopt);
			usage();
			break;
		default:
			break;
		}
	}

	argv += optind;

	linknum = argc - optind;

	if (linknum == 0) {
		fprintf(stderr, "there was no link in parameters");
		usage();
	}

//	printf("num of http_links: %d\n", linknum);

	programsettings.numlinks = linknum;
	programsettings.links = malloc(linknum * sizeof (char *) + 1);

	linkidx = 0;

	while (*(argv)) {
//		printf("link nr%d: %s\n", linkidx, *argv);
		programsettings.links[linkidx++] = *argv;
		++argv;
	}
	programsettings.links[linkidx] = NULL;
}

int
main(int argc, char **argv)
{
	prgname = argv[0];

	if (argc < 2)
		usage();

	proc_opts(argc, argv);

	thr_mgr_downloadallfiles(&programsettings);

	return (0);
}
