/*open*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <strings.h>

#include "../lib/json2txt.h"
#define ALLOC_SIZE 56
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
	int warning;
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

int main(int argc, char **argv){
	struct args a = { 1, 1, 0, 1, 1, STDIN_FILENO };
	struct json_parser p = INIT_JSON(BUFSIZE, ALLOC_SIZE, CHARS, NULL);
	struct json *j = NULL, *pj;
	char buffer[BUFSIZE];
	argp_parse(&argp, argc, argv, 0, 0, &a);
	p.buffer = buffer;
	p.file = a.fd;
	if(!a.tojson && !a.totext && !a.test)
		warnx("Parsing only is done");
	while(read_fn(&p))
		do{
			readchar(&p.offset, &p.buf, p.chars);
			switch(*p.buf){
				case '{':
					pj = json_create(&j, NEW, PAIR);
					pair(&p, &j);
					break;
				case '[':
					pj = json_create(&j, NEW, ARRAY);
					array(&p, &j);
					break;
				case 0:
					break;
				default:
					errx(255, "Unexpected character at offset %lu.", p.offset);
			}
		}while(*p.buf);
	if(p.file != STDIN_FILENO && p.file != STDERR_FILENO)
		close(p.file);
	if(a.test){
		if(duplicate_keys(pj, (a.warning > -1) ? a.warning : 1) == 1){
			json_destroy(&pj);
			exit(255);
		}
	}
	if(a.tojson)
		json_print(pj, 1, 0, ' ', 3, a.warning);
	if(a.tojson && a.totext)
		json_reset_flags(pj);
	if(a.totext)
		json2txt(pj, a.sort, NULL, a.warning);
	json_destroy(&pj);
	return 0;
}

