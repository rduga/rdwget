#ifndef LINKPARSER_H
#define	LINKPARSER_H

#include "defaults.h"

#define	LINK_REGEXP "^(http://)?([^/]+)(/(([^/]+/)*([^/]+))?)?$"

void strtoprot(char *str, protocols* prots);
char *_strtok(char **holder, char *s, const char *delim);
size_t _sprintf(int num, char **filledstr, const char *format, ...);
char *_trim(char *str);
size_t _strnlen(const char *str, size_t maxlen);
char *_strndup(char *str, size_t num);
char *_strtr(char *str, char from, char to);
int match(const char *string, char *pattern);
int link_parse(char *linkstr, lnk *link);

void create_rand_filename(lnk *link);
void mk_filename(const char *resultdir, lnk *link);



#endif /* LINKPARSER_H */
