#include "pcre_split.h"

/* initialise the regex */
split_t *pcre_split(char *pattern, char *string) {
	pcre *re;
	const char *error;
	int erroffset;
	split_t_int *ret_int;
	split_t *curr = NULL;
	split_t *head = NULL;

	/* setup regex */
	re = pcre_compile(
		pattern,
		0,
		&error,
		&erroffset,
		NULL);

	/* check if compilation was successful */
	if(re == NULL) {
		printf("Error: PCRE compilation failed at offset %d: %s\n", erroffset, error);
		return (split_t *)NULL;
	}


	/* loop thorough string */
	do {
		if(head == NULL) {
			head = (split_t *)calloc(sizeof(split_t), 1);
			curr = head;
		}
		else {
			curr->next = (split_t *)calloc(sizeof(split_t), 1);
			curr = curr->next;
		}
		ret_int = pcre_split_int(re, string);
		curr->string = ret_int->string;
		curr->match = ret_int->match;
		string = ret_int->back;
		free(ret_int);
	} while(curr->match);

	pcre_free(re);

	return head;
}

split_t_int *pcre_split_int(pcre *re, char *string) {

	int rc;
	int ovector[OVECCOUNT];
	int length;
	split_t_int *s = (split_t_int *)calloc(sizeof(split_t_int), 1);

	length = (int)strlen(string);

	rc = pcre_exec(
		re,
		NULL,
		string,
		length,
		0,
		0,
		ovector,
		OVECCOUNT);

	/* check for matches */
	if(rc < 0) {
		switch(rc) {
			case PCRE_ERROR_NOMATCH:
				s->string = string;
				s->match = NULL;
				return s;
				break;
			default:
				printf("Error: Matching error: %d\n", rc);
				return (split_t_int *)NULL;
				break;
		}
		pcre_free(re);
	}

	/* check if output vector was large enough */
	if(rc == 0) {
		rc = OVECCOUNT/3;
		printf("Warning: ovector only has room for %d captured substrings\n", rc-1);
	}

	s->string = calloc(sizeof(char) * (ovector[0] + 1), 1);
	strncpy(s->string, string, ovector[0]);

	s->match = calloc(sizeof(char) * ((ovector[1] - ovector[0]) + 1), 1);
	strncpy(s->match, (char *)(string + ovector[0]), (ovector[1] - ovector[0]));

	s->back = calloc(sizeof(char) * ((length - ovector[1]) + 1), 1);
	strncpy(s->back, (char *)(string + ovector[1]), (length - ovector[1]));

	return (split_t_int *)s;
}

int pcre_split_free(split_t *split) {
	split_t *mine;

	while(split->next != NULL) {
		mine = split;
		split = split->next;

		free(mine->string);
		free(mine->match);
		free(mine);
	}

	free(split->string);
	free(split);
	return 0;
}

int pcre_split_print(split_t *split) {

	if(split == NULL)
		printf("Error: Nothing to print\n");
	else {
		do {
			printf("string: %s\n", split->string);
			printf("match : %s\n", split->match);
		} while((split = split->next) != NULL);
	}

	return 0;
}
