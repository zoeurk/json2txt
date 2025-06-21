/*open*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <strings.h>

#include "../lib/json2txt.h"
#define ALLOC_SIZE 16
#define BUFSIZE 56
#define CHARS " \t\n\r"

char *const sort[] = {
	"asc",
	"dsc",
	"none",
	NULL
};
char *const duplicate[] = {
	"true",
	"false",
	"none",
	NULL
};
int subopt(char *subopt, char *const *tokens){
	int i = 0;
	for(;*tokens && strcasecmp(subopt, *tokens); tokens++, i++);
	return i;
}

#include <argp.h>
const char *argp_program_version = "json2txt version 0.3";
const char *argp_program_bug_address = "<zoeurk@gmail.com>";
static char doc[] = "Parse, test and convert to text json stream";
static char args_doc[] = "[file]";
static struct argp_option options[] = {
	{ "json", 'J', NULL, 0, "Inhibit output to json", 0 },
	{ "text", 'T', NULL, 0, "Inhibit output to text", 0 },
	{ "sort", 's', "asc|dsc|none", 0, "Sort keys, ", 1},
	{ "test", 't', NULL, 0, "No test", 2},
	{ "warn", 'w', "true|false|none", 0, "Exit on duplicate key", 2},
	{ "rfc", 'r', NULL, 0, "try to be RFC 8259 compliant", 2},
	{ NULL, 0, NULL, 0, "Help", 3},
	{ NULL, '?', NULL, 0, "Alias for --usage", 4},
	{ NULL, 'h', NULL, 0, "Alias for --help", 4},
	{ 0 }
};
struct args{
	int tojson;
	int totext;
	int sort;
	int test;
	short int warning;
	short int rfc;
	int fd;
}args;
static error_t parse_opt(int key, char *arg, struct argp_state *state){
	struct args *a = state->input;
	state->name = state->argv[0];
	switch(key){
		case '?':
			argp_state_help(state, stdout, ARGP_HELP_USAGE);
			_exit(EXIT_SUCCESS);
		case 'h':
			argp_state_help(state, stdout, ARGP_HELP_SHORT_USAGE | ARGP_HELP_DOC | ARGP_HELP_LONG | ARGP_HELP_BUG_ADDR);
			_exit(EXIT_SUCCESS);
		case 'J':
			a->tojson = 0;
			break;
		case 'T':
			a->totext = 0;
			break;
		case 's':
			switch(subopt(arg,sort)){
				case 0:
					a->sort = 1;
					break;
				case 1:
					a->sort = -1;
					break;
				case 2:
					a->sort = 0;
					break;
				default:
					errx(255, "Invalid argument for '-s(/--sort)'");
			}
			break;
		case 't':
			a->test = 0;
			break;
		case 'w':
			switch(subopt(arg, duplicate)){
				case 0:
					a->warning = 1;
					break;
				case 1:
					a->warning = 0;
					break;
				case 2:
					a->warning = -1;
					break;
				default:
					errx(255, "Invalid argument for '-w(/--warning)'");
			}
			break;
		case 'r':
			a->rfc = 1;
			break;
		case ARGP_KEY_ARG:
			if((a->fd = open(arg, O_RDONLY)) < 0)
				err(255, "open()");
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}
static struct argp argp = { options, parse_opt, args_doc, doc, NULL, NULL, NULL };
#define DESTROY \
	if(j){ \
		j = go_first(j); \
		json_destroy(&j); \
	} \
	if(p.stock) \
		free(p.stock); \
	while(f){ \
		pf = f->prev; \
		free(f); \
		f = pf; \
	} \
	while(pj_lst){ \
		lst = pj_lst->prev; \
		free(pj_lst); \
		pj_lst = lst; \
	}
/*
goto =>
	DATA (number)
	PARSE (json chars)
	ERRORS (if a fatal error)
*/
union test{
	int (*test)(char *, ...);
	int (*test_int)(char *, char *);
	int (*test_bool)(char *);
};
int main(int argc, char **argv){
	struct args a = { 1, 1, 0, 1, 1, 0, STDIN_FILENO };
	struct json_parser p = INIT_JSON(BUFSIZE, ALLOC_SIZE, CHARS, NULL);
	struct json *j = NULL;
	struct fn *f, *pf;
	struct json_new_lst *pj_lst = NULL,*lst;
	union test t;
	int ret, init = 0;
	char buffer[BUFSIZE];
	void *a1, *a2; 
	argp_parse(&argp, argc, argv, 0, 0, &a);
	if(!a.tojson && !a.totext && !a.test)
		warnx("Parsing only will be do.");
	p.buffer = buffer;
	p.file = a.fd;
	if((f = calloc(1, sizeof(struct fn))) == NULL){
		err(255, "calloc()");
	}
	f->fns.do_it.read_with_this = &starting;
	while(read_fn(&p))
		do{
			if(f->fns.get.get_it){
				/* DATA:
					Start of number don't increment p.offset and p.buf the first time,
					it was not analyzed
				*/
				DATA:
				if((ret = (*f->fns.get.get_it)(&p, init)) == 0){
					if(j->t_val&INT && (ret = t.test(p.stock, p.pstock))){
						/* ERRORS
							Same for all errors: at the end of loop
							Don't increment again
						*/
						if(json_err != -6 && json_err != -7)
							goto ERRORS;
						else{
							if(a.rfc || a.warning == 0)
								goto ERRORS;
							if(a.warning == 1)
								json_errors(&p, &j, &f);
							json_err = 0;
						}
					}
					init = 0;
					(*f->fns.up.up_it)(&j, &p, f);
					(*f->fns.structure.up_int)(&p, a1, a2);
					f->fns.get.get_it = NULL;
					if((j->t_val&WARN_NBR) && a.warning != -1){
						warnx("Invalid number at offset: %lu",p.offset -1);
						if(a.warning == 0){
							DESTROY;
							return -255;
						}
					}
					if((j->t_val&INT)){
						/* PARSE:
							End of number, don't increment p.buf and p.offset.
							Maybe an errors, we need to re-analyze it differently.
						*/
							goto PARSE;
					}
				}else
					if(init && (j->t_val&INT))
						switch(ret){
							case 1:
								t.test_bool = &json_bool_test;
								f->fns.up.up_int = &up_from_bool;
								break;
							case 2:
								t.test_int = &json_int_test;
								break;
						}

				if(json_err > 0){
					DESTROY;
					return json_err;
				}
				if(ret < 0 && a.warning == 1){
					switch(ret){
						case -1:
							warnx("Value number: start by '+',\n\tthis value is not valid (offset: %lu)",
								p.offset);
							break;
						case -2:
							warnx(
						"Value number: start by '(+|-)?.num',\n\tValid value should be '-?0.num' (offset: %lu)"
								,p.offset);
							break;
						/*case -3:
							if(bool_err != 1)
								warnx("Invalid boolean start at(/before) offset: %lu", p.offset);
							break;*/
					}
				}/*else
				printf("%i\n", ret);
				*/
				init = 0;
			}else{
				PARSE:
				readchar(&p.offset, &p.buf, p.chars);
				if(!*p.buf)
					continue;
				if((ret = (*f->fns.do_it.do_it)(&p , &j, &f, &pj_lst))){
					if(json_err > 0){
						/* 
							"[m|c|re]alloc() json_err = errno"
							malloc() used by json_sort()
						*/
						DESTROY;
						return json_err;
					}
					/* ERRORS:
						Same for all errors: at the end of loop
						Don't increment again.
						(It's the same that recall this function)
					*/
					goto ERRORS;
				}else
					if(f->fns.get.get_it){
						/*Next char is INT or STRING */
						if(j->t_val&INT){
							t.test_int = &json_int_test;
							init = 1;
							a1 = j;
							a2 = f;
							/* Don't increment p.buf and p.offset */
							goto DATA;
						}else{
							/* '"' while be read: start of string prepare for next char */
							a1 = f;
							a2 = j;
						}
					}
			}
		}while(*p.buf && *(++p.buf) && ++p.offset > 0);
	ERRORS:
	if(p.file != STDIN_FILENO && p.file != STDERR_FILENO)
		close(p.file);
	if(p.r_len < 0){
		DESTROY;
		return json_err;
	}
	/* show errors, return json_err */
	/* Compute error for a missing '}' or ']'*/
	if(!json_err)
		json_err = -1*(f->prev != NULL || f->err == 0);
	if((json_err || a.warning == 1) && json_errors(&p, &j, &f) < 0){
		DESTROY;
		return json_err;
	}
	free(f);
	if(p.offset <= 0){
		DESTROY;
		errx(-6, "File too long !!!");
	}
	if(a.test)
		if(json_test_sort(	&j,
					(a.sort == 0) ? &json_test 
						: (a.sort == -1) ? &json_sort_dsc
						: &json_sort_asc, (a.rfc) ? 0
						: a.warning)
		){
			json_errors(&p, &j, &f);
			json_destroy(&j);
			return json_err;
		}
	if(a.tojson)
		json_print(j, 0, " : ", ' ', 3);
	if(a.totext)
		(void)json2txt(j, NULL);
	json_destroy(&j);
	return json_err;
}

