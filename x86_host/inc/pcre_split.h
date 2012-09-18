#ifndef _GUARD_PCRE_SPLIT
#define _GUARD_PCRE_SPLIT

/*
 */

#include <stdio.h>
#include <string.h>
#include <pcre.h>

#define OVECCOUNT 30

struct split_t {
	char *string;
	char *match;
	struct split_t *next;
};
typedef struct split_t split_t;

typedef struct {
	char *string;
	char *match;
	char *back;
} split_t_int;

split_t *pcre_split(char *, char *);
split_t_int *pcre_split_int(pcre *, char *);
int pcre_split_free(struct split_t *);
int pcre_split_print(struct split_t *);

#endif
