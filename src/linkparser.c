/*!
 * \file
 * \brief Simple libraray for parsing links, validating directories and
 * manipilating strings (char *).
 */

#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>	// ERANGE
#include <ctype.h>	// isspace
#include <stdarg.h>	// _sprintf
#include "linkparser.h"

/**
 * Strips whitespaces from the beginning and end of a string.
 * \return ponter to begining of trimmed string.
 */
char *
_trim(char *str)
{
	char *ibuf, *obuf;

	if (str) {
		for (ibuf = obuf = str; *ibuf; ) {
			while (*ibuf && (isspace (*ibuf)))
				ibuf++;
			if (*ibuf && (obuf != str))
				*(obuf++) = ' ';
			while (*ibuf && (!isspace (*ibuf)))
				*(obuf++) = *(ibuf++);
		}
		*obuf = '\0';
	}
	return (str);
}

/**
 * Extract tokens from strings.
 * Similar to strtok function but added holder to enable tokenize more strings
 * across.
 * \return extraceted token.
 */
char *
_strtok(char **holder, char *s, const char *delim)
{
	if (s)
		*holder = s;

	return (strsep(holder, delim));
}

/*
 * Similar to sprintf but creates and calculates buffer automatically.
 * Formatted string is saved into buffer to which is pointed filledstr.
 * Buffer can be dealocated by free function.
 * @param format must contain not other than "%s" formating strings.
 * \return length of formatted string.
 */
size_t
_sprintf(int num, char **filledstr, const char *format, ...)
{
	va_list ap;
	const char *curarg;
	size_t argln = strlen(format);
	int argcnt = num;

	va_start(ap, format);
	while (num-- > 0) {
		curarg = va_arg(ap, const char *);
		argln += strlen(curarg);
	}
	va_end(ap);

	argln = argln - argcnt * 2;
	(*filledstr) = malloc(argln + 1);

	va_start(ap, format);
	if (vsprintf((*filledstr), format, ap) != argln) {
		fprintf(stdlog, log_ERROR
				"sprintf bad length"
				" in %s:%d\n", __FILE__, __LINE__);
		// return (-1);
	}
	va_end(ap);

	(filledstr)[argln] = '\0';

	return (argln + 1);
}

/**
 * Converts string in str to protocols type contained in prots.
 */
void
strtoprot(char *str, protocols* prots)
{
	if ((strlen(str) == 0) || (strcmp(str, PROTOCOL_HTTP) == 0)) {
		*prots = HTTP;
		return;
	}

	if (strcmp(str, PROTOCOL_FTP) == 0) {
		*prots = FTP;
		return;
	}

	*prots = UNKNOWN;
}

/**
 * GNU like function for determining length of string due to maxlen.
 * \returns size of string.
 */
size_t
_strnlen(const char *str, size_t maxlen)
{
	const char *end = memchr(str, '\0', maxlen);
	return (end ? (size_t) (end - str) : maxlen);
}

/**
 * GNU like function for copying substring of limited length num.
 * \return new allocated sting of slecified length.
 */
char *
_strndup(char *str, size_t num)
{
	size_t len = _strnlen(str, num);

	char *buf = malloc(sizeof (char) * (len + 1));
	if (buf == NULL)
		return (NULL);

	buf[len] = 0;

	return (memcpy(buf, str, num));
}

/**
 * Parses http link and saves parsed info into lnk structure.
 * \return 0 on success, else -1 (if linkstr is not valid http link)
 */
int
link_parse(char *linkstr, lnk *link)
{
	int status;
	regex_t re;
	char *protocol;
	size_t rquriln;
	int matchsize = 8;
	regmatch_t match[matchsize];

	if (regcomp(&re, LINK_REGEXP, REG_EXTENDED) != 0) {
		// error
		fprintf(stdlog, "NOT COMPILED\n");
		return (-1);
	}
	status = regexec(&re, linkstr, (size_t) 8, match, 0);
	regfree(&re);
	if (status != 0) {
		// error
		fprintf(stdlog, log_ERROR "%s not valid http link!!!\n",
				linkstr);
		return (-1);
	}

	protocol = _strndup(linkstr + match[1].rm_so,
			(size_t) (match[1].rm_eo - match[1].rm_so));
	strtoprot(protocol, &link->prot);

	rquriln = (size_t) (match[3].rm_eo - match[3].rm_so);

	link->hostname = _strndup(linkstr + match[2].rm_so,
			(size_t) (match[2].rm_eo - match[2].rm_so));
	if (rquriln == 0)
	link->rquri = URI_BASENAME;
	else
	link->rquri = _strndup(linkstr + match[3].rm_so, rquriln);
	link->filename = _strndup(linkstr + match[6].rm_so,
			(size_t) (match[6].rm_eo - match[6].rm_so));

	free(protocol);

	return (0);
}

/**
 * Function for matching string due to specified pattern (using rexex.h).
 * \return 0 on success, -1 on fail.
 */
int
match(const char *string, char *pattern)
{
	int status;
	regex_t re;

	if (regcomp(&re, pattern, REG_EXTENDED|REG_NOSUB) != 0) {
		// error
		fprintf(stderr, "NOT COMPILED\n");
		return (0);
	}
	status = regexec(&re, string, (size_t) 0, NULL, 0);
	regfree(&re);
	if (status != 0) {
		// error
		return (0);
	}
	return (1);
}

// strcat
//
/**
 * \return	new string consisting from first and second,
 * 		can be deleted with free function
 */
char *
_strcat(const char *first, const char *second)
{
	size_t fln = strlen(first);
	size_t sln = strlen(second);
	char *dest = malloc(fln + sln + 1);

	dest[fln + sln] = '\0';
	memcpy(dest + fln, second, sln);
	return (memcpy(dest, first, fln));
}

/**
 * Translates characters from into to in string str.
 * \return modified string.
 */
char *
_strtr(char *str, char from, char to)
{
	char *idx = str;
	for (; *(idx) != '\0'; ++idx) {
		if (*idx == from)
			*idx = to;
	}
	return (str);
}

/**
 * Checks resultdir if contains trailing slash (if not adds it) and creates
 * full path to filename (consisted from link->hostname and link->rquri) by
 * replacing denied characters (slash) in filename by _.
 */
void
mk_filename(const char *resultdir, lnk *link)
{
	//  char *idx;
	char *newfilename;
	char *resdirs, *oldresdir; // slashed result dir

	// not empty filename, no need to validate

	resdirs = strdup(resultdir);

	if (resdirs[strlen(resultdir) - 1] != '/') {
		oldresdir = resdirs;
		resdirs = _strcat(resdirs, "/");
		free(oldresdir);
	}

	newfilename = _strcat(link->hostname, link->rquri);
	newfilename = _strtr(newfilename, '/', '_');

	// if (strcmp(link->filename, "") == 0) {
	// 	free(link->filename);
	// 	link->filename = strdup(link->rquri);
	//
	//	for (idx = link->filename; *idx != '\0'; ++idx) {
	//	if (*idx == '/')
	//		*idx = '_';
	//	}
	// }

	newfilename = _strcat(resdirs, newfilename);
	free(link->filename);
	link->filename = newfilename;

}
