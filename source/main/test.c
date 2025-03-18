/*read(), STD*_FILENO*/
#include <unistd.h>
/*memset, memcpy, strcpy, strcat, strcmp*/
#include <string.h>
/*err, errx, warnx*/
#include <err.h>
/*sprintf, printf*/
#include <stdio.h>
/*malloc, calloc(), realloc()*/
#include <stdlib.h>

#define INIT_JSON(BUFSIZE, ALLOC_SIZE, CHARS, buffer) { 0, BUFSIZE, ALLOC_SIZE, CHARS, buffer, NULL, 0, 0, 1, NULL, NULL, 0 }
enum TYPE{
	ARRAY = 1,
	PAIR = 2,
	SET = 4
};
enum VAL{
	INT = 0,
	STRING = 1,
	VOID = 2,
	WARN = 4
};
enum KIND{
	NEW,
	SUB,
	NEXT
};
struct json_parser{
	int file;
	unsigned int buflen;
	size_t stock_buf;
	char *chars;
	char *buffer;
	char *buf;
	size_t len;
	ssize_t r_len;
	ssize_t offset;
	char *pstock;
	char *stock;
	size_t stock_size;
};
struct json{
	enum TYPE type;
	enum VAL t_val;
	struct{
		union{
			size_t index;
			char *key;
		}name;
		char *value;
	}value;
	struct json *next;
	struct json *prev;
	struct json *sub;
	struct json *up;
};
static ssize_t read_fn(struct json_parser *p);
static void readchar(ssize_t *offset, char **str, const char *not);
static void allocstr(char **buffer, size_t lentoadd);
static void *json_create(struct json **j, enum KIND kind, enum TYPE type);
static void *getint(struct json_parser *p);
static void *getstr(struct json_parser *p);
static void *array(struct json_parser *p, struct json **j);
static void *pair(struct json_parser *p, struct json **j);
static ssize_t json_sort(struct json *j, char ***order, int sort, int warn_only);
static int duplicate_keys(struct json *j, int warning_only);
static void json_print(struct json *j, int sort, size_t space, char c_sp, size_t count, int warn_only);
static void json2txt(struct json *j, int sort, char *string, int warn_only);
static void json_reset_flags(struct json *j);
static void json_destroy(struct json **j);

/*open*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <strings.h>

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
#define STOCK_BUF(p) \
	if(p->len == 0){ \
		allocstr(&p->stock, p->stock_buf); \
		p->stock_size = p->stock_buf; \
		memset(p->stock, 0, p->stock_buf); \
		p->pstock = p->stock; \
	}else{ \
		if(p->len +1 == p->stock_size){ \
			allocstr(&p->stock, p->stock_buf + p->stock_size); \
			p->stock_size += p->stock_buf; \
			memset(p->stock + p->len, 0, p->stock_buf+1); \
			p->pstock = p->stock + p->len; \
		} \
	} \
	*p->pstock = *p->buf; \
	p->pstock++;
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
ssize_t read_fn(struct json_parser *p){
	if((p->r_len = read(p->file, p->buffer, p->buflen)) < 0)
		err(255, "read()");
	p->buffer[p->r_len] = 0;
	p->buf = p->buffer;
	return p->r_len;
}
void readchar(ssize_t *offset, char **str, const char *not){
	const char *n;
	for(;**str;(*str)++,(*offset)++)
		for(n = not;*n != **str;n++)
			if(*n == 0)
				return;
}
void allocstr(char **buffer, size_t lentoadd){
	if(!*buffer){
		if((*buffer = calloc(lentoadd, sizeof(char))) == NULL)
			err(255, "calloc");
	}else{
		if((*buffer = realloc(*buffer, lentoadd * sizeof(char))) == NULL)
			err(255, "realloc()");
	}
}
void *json_create(struct json **j, enum KIND kind, enum TYPE type){
	struct json *rj;
	switch(kind){
		case NEW:
			if(((*j) = calloc(1, sizeof(struct json))) == NULL)
				err(255, "calloc()");
			rj = *j;
			break;
		case SUB:
			rj = *j;
			if(((*j)->sub = calloc(1, sizeof(struct json))) == NULL)
				err(255, "calloc()");
			(*j)->sub->up = *j;
			(*j) = (*j)->sub;
			break;
		case NEXT:
			rj = *j;
			if(((*j)->next = calloc(1, sizeof(struct json))) == NULL)
				err(255, "calloc()");
			(*j)->next->prev = *j;
			(*j) = (*j)->next;
			break;
	}
	(*j)->t_val = VOID;
	(*j)->type = type;
	return rj;
}
#define STOCK_BUF(p) \
	if(p->len == 0){ \
		allocstr(&p->stock, p->stock_buf); \
		p->stock_size = p->stock_buf; \
		memset(p->stock, 0, p->stock_buf); \
		p->pstock = p->stock; \
	}else{ \
		if(p->len +1 == p->stock_size){ \
			allocstr(&p->stock, p->stock_buf + p->stock_size); \
			p->stock_size += p->stock_buf; \
			memset(p->stock + p->len, 0, p->stock_buf+1); \
			p->pstock = p->stock + p->len; \
		} \
	} \
	*p->pstock = *p->buf; \
	p->pstock++;
void *getint(struct json_parser *p){
	int dot  = 0, start = 0, zero = 0;
	char *fboolean[2] = { "false", "FALSE" },
		*tboolean[2] = { "true", "TRUE" },
		*tnull[2] = { "null", "NULL" },
		*pbool = NULL, *Pbool = NULL, last;
	p->len = 0;
	printf("%s\n", p->buf);
	if(*p->buf == 'n' || *p->buf == 'N'){
		pbool = tnull[0];
		Pbool = tnull[1];
		STOCK_BUF(p);
		p->len++;
		p->buf++;
		p->offset++;
		pbool++;
		Pbool++;
		do{
			for(;*p->buf;p->buf++, p->len++, p->offset++, pbool++, Pbool++){
				if(*p->buf != *pbool && *p->buf != *Pbool){
					if(*pbool != 0 && *Pbool != 0)
						*p->pstock = 1;
					return p;
				}
				STOCK_BUF(p);
			}
			if(*pbool == 0 || *Pbool == 0)
				return p;
		}while(read_fn(p));
		return p;
	}
	if(*p->buf == 't' || *p->buf == 'T'){
		pbool = tboolean[0];
		Pbool = tboolean[1];
		STOCK_BUF(p);
		p->len++;
		p->buf++;
		p->offset++;
		pbool++;
		Pbool++;
		do{
			for(;*p->buf;p->buf++, p->len++, p->offset++, pbool++, Pbool++){
				if(*p->buf != *pbool && *p->buf != *Pbool){
					if(*pbool != 0 && *Pbool != 0)
						*p->pstock = 1;
					return p;
				}
				STOCK_BUF(p);
			}
			if(*pbool == 0 || *Pbool == 0)
				return p;
		}while(read_fn(p));
		return p;
	}
	if(*p->buf == 'f' || *p->buf == 'F'){
		pbool = fboolean[0];
		Pbool = fboolean[1];
		STOCK_BUF(p);
		p->len++;
		p->buf++;
		p->offset++;
		pbool++;
		Pbool++;
		do{
			for(;*p->buf;p->buf++, p->len++, p->offset++, pbool++, Pbool++){
				if(/**pbool != 0 && *Pbool != 0 &&*/ *p->buf != *pbool && *p->buf != *Pbool){
					if(*pbool != 0 && *Pbool != 0)
						*p->pstock = 1;
					/**p->pstock = 1;*/
					return p;
				}
				/*if(*p->buf != *pbool && *p->buf != *Pbool){
					return p;
				}*/
				STOCK_BUF(p);
			}
			if(*pbool == 0 || *Pbool == 0)
				return p;
		}while(read_fn(p));
		return p;
	}
		if(*p->buf == '-' || *p->buf == '+'){
			if(*p->buf == '+'){
				warnx("Value number: start by '+', this value is not valid (offset: %lu)", p->offset);
			#ifndef EXPLICIT_SIGN
				errx(255, "Invalid Number");
			#endif
			}
			/*#ifdef EXPLICIT_SIGN*/
				last = *p->buf;
				STOCK_BUF(p);
				p->len++;
				p->buf++;
				p->offset++;
			/*#endif*/
	}
	do
		for(;(last = *p->buf);p->buf++, p->len++, p->offset++)
		{	printf("<%c>\n", *p->buf);
			if(*p->buf == '.'){
				switch(dot){
					case 0:
						if(zero == 0 && start == 0){
							warnx("Value number: start by '(+|-)?.num', valid value should be '-?0.num' (offset: %lu)", p->offset);
							#ifdef STRICT_NUM
							errx("Invalid number.");
							return p;
							#endif
						}
						dot = 1;
						start = 1;
						STOCK_BUF(p);
						break;
					default:
						return p;
				}
			}else{
				/*i = *p->buf - (3 << 4);*/
				if(start == 0 && *p->buf == '0' && zero++ > 0){
					warnx("Invalid number.");
					return p;
				}
				if(*p->buf != '0')
					start = 1;
				if(*p->buf < '0' || *p->buf > '9'){
					return p;
				}
				STOCK_BUF(p);
			}
		}
	while(read_fn(p));
	return p;
}
void *getstr(struct json_parser *p){
	ssize_t start = p->offset -1;
	p->len = 0;
	do
		for(;*p->buf;){
			switch(*p->buf){
				case '\\':
					STOCK_BUF(p);
					p->buf++;
					p->offset++;
					p->len++;
					continue;
				case '"':
					return p;
			}
			STOCK_BUF(p);
			p->buf++;
			p->len++;
			p->offset++;
		}
	while(read_fn(p));
	errx(255, "Unexpected EOF\n\t'\"' at offset %lu.", start);
	return p;
}
void *array(struct json_parser *p, struct json **j){
	struct json *pj;
	size_t index = 0;
	ssize_t offset = p->offset;
	int next = 0, end = 0;
	char c_char;
	p->buf++;
	p->offset++;
	do
		while(*p->buf){
			readchar(&p->offset, &p->buf, p->chars);
			switch(*p->buf){
				case '"':
					if(end)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					end = 1;
					next = 0;
					p->buf++;
					p->offset++;
					getstr(p);
					(*j)->value.name.index = index;
					(*j)->value.value = p->stock;
					(*j)->t_val = STRING;
					p->stock = NULL;
					p->stock_size = 0;
					p->buf++;
					p->offset++;
					index++;
					break;
				case ',':
					json_create(j, NEXT, ARRAY);
					if(next)
						errx(255, "Unexpected ',' at offset %lu.", p->offset);
					end = 0;
					next = 1;
					p->offset++;
					p->buf++;
					break;
				case ']':
					if(next)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					p->offset++;
					p->buf++;
					return p;
				case '[':
					/*pj = *j;*/
					(*j)->value.name.index = index;
					index++;
					pj = json_create(j, SUB, ARRAY);
					if(!next)
						errx(255, "Unexpected '[' at offset %lu.", p->offset);
					array(p, j);
					*j = pj;
					next = 0;
					end = 1;
					break;
				case '{':
					/*pj = *j;*/
					(*j)->value.name.index = index;
					index++;
					pj = json_create(j, SUB, PAIR);
					pair(p, j);
					*j = pj;
					next = 0;
					end = 1;
					break;
				case '}':
					errx(255, "Unexpected '}' at offset %lu.\n\t'[' at offset %lu not close.", p->offset, offset);
				case 0:
					break;
				default:
					getint(p);
					if(((c_char = *(p->buf)) < '0' || c_char > '9'  || c_char == '+' || c_char == '-' ) &&
						(*(p->pstock) != 0)){
						errx(255, "Unexpected chararcter (bad number) near offset %lu.", p->offset);
					}
					/*if(end)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);*/
					end = 1;
					next = 0;
					c_char = *p->stock;
					if(c_char == '+' || *(p->stock + (c_char == '+' || c_char == '-')) == '.')
						(*j)->t_val = INT | WARN;
					else
						(*j)->t_val = INT;
					(*j)->value.name.index = index;
					(*j)->value.value = p->stock;
					index++;
					p->stock = NULL;
					p->stock_size = 0;
					next = 0;
					break;
			}
		} 
	while(read_fn(p));
	errx(255, "Expected ']' at offset %lu.\n\t'[' at offset %lu not close.", p->offset, offset);
	return p;
}
void *pair(struct json_parser *p, struct json **j){
	struct json *pj;
	ssize_t offset = p->offset;
	int need_key = 1, need_value = 0,
		next = 0, end = 0, c_char;
	p->offset++;
	p->buf++;
	do
		while(*p->buf){
			readchar(&p->offset, &p->buf, p->chars);
			switch(*p->buf){
				case '"':
					if(need_key == 1)
						need_key = 0;
					if(end)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					if(need_value == 0)
						need_value = 1;
					else
						if(need_value == 2)
							need_value = 0;
						else
							errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					next = 0;
					end = 0;
					p->buf++;
					p->offset++;
					getstr(p);
					if((*j)->value.name.key == NULL)
						(*j)->value.name.key = p->stock;
					else{
						(*j)->value.value = p->stock;
						(*j)->t_val = STRING;
					}
					p->stock = NULL;
					p->stock_size = 0;
					p->buf++;
					p->offset++;
					next = 0;
					break;
				case ':':
					if(need_value == 1)
						need_value = 2;
					else
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					need_key = 0;
					p->offset++;
					p->buf++;
					break;
				case '{':
					/*pj = *j;*/
					pj = json_create(j, SUB, PAIR);
					pair(p, j);
					*j = pj;
					need_key = 1;
					need_value = 0;
					next = 0;
					end = 1;
					break;
				case '[':
					/*pj = *j;*/
					pj = json_create(j, SUB, ARRAY);
					array(p, j);
					*j = pj;
					need_key = 1;
					need_value = 0;
					next = 0;
					end = 1;
					break;
				case ',':
					json_create(j, NEXT, PAIR);
					if(next)
						errx(255, "Unexpected ',' at offset %lu.", p->offset);
					end = 0;
					next = 1;
					need_key = 1;
					p->offset++;
					p->buf++;
					break;
				case '}':
					if(next || need_value)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					end = 0;
					p->offset++;
					p->buf++;
					return p;
				case ']':
					errx(255, "Unexpected ']' at offset %lu.\n\t'{' at offset %lu not close.", p->offset, offset);
				case 0:
					break;
				default:
					if(need_key == 1)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					if(need_value != 2)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);
					getint(p);
					if(((c_char = *(p->buf)) < '0' || c_char > '9'|| c_char == '+' || c_char == '-') &&
						(*(p->pstock) != 0 )){
						errx(255, "Unexpected chararcter (bad number) near offset %lu.", p->offset);
					}
					/*if(end)
						errx(255, "Unexpected chararcter at offset %lu.", p->offset);*/
					need_value = 0;
					need_key = 1;
					end = 1;
					next = 0;
					/*c_char = *(p->pstock -1);
					if(c_char == '.' || c_char == '+' || c_char == '-'|| c_char < '0' || c_char > '9'){
						errx(255, "Unexpected chararcter (bad number) before offset %lu.", p->offset);
					}*/
					c_char = *p->stock;
					if(c_char == '+' || *(p->stock + (c_char == '+' || c_char == '-')) == '.')
						(*j)->t_val = INT | WARN;
					else
						(*j)->t_val = INT;
					(*j)->value.value = p->stock;
					p->stock = NULL;
					p->stock_size = 0;
					break;
			}
		}
	while(read_fn(p));
	errx(255, "Expected '}' at offset %lu.\n\t'{' at offset %lu not close.", p->offset, offset);
	return p;
}
ssize_t json_sort(struct json *j, char ***order, int sort, int warn_only){
	struct json *pj;
	ssize_t idx, i;
	int ret;
	for(pj = j, idx = 0; pj;pj = pj->next, idx++){
		if(*order == NULL){
			if(((*order) = malloc(idx+1*sizeof(char **))) == NULL)
				err(255, "malloc()");
		}else{
			if(((*order) = realloc((*order), (idx+1)*sizeof(char **))) == NULL)
				err(255, "realloc()");
		}
		(*order)[idx] = NULL;
		for(i = 0, (*order)[idx] = NULL; (*order)[i]; i++){
			if(sort < 0){
				if((ret = strcmp((*order)[i], pj->value.name.key)) <= 0){
					memcpy(&(*order)[i+1], &(*order)[i], (idx-i) * sizeof(char **));
					(*order)[i] = pj->value.name.key;
					if(ret != 0 || warn_only < 0)
						break;
				}
				if(ret == 0){
					warnx("Duplicate key: \"%s\"", pj->value.name.key);
					if(warn_only == 0)
						return -1;
					break;
				}
			}else{
				if(sort > 0){
					if((ret = strcmp((*order)[i], pj->value.name.key)) >= 0){
						memcpy(&(*order)[i+1], &(*order)[i], (idx-i) * sizeof(char **));
						(*order)[i] = pj->value.name.key;
						if(ret != 0 || warn_only < 0)
							break;
					}
					if(ret == 0){
						warnx("Duplicate key: \"%s\"", pj->value.name.key);
						if(warn_only == 0)
							return -1;
						break;
					}
				}else{
					if(warn_only != -1 && strcmp((*order)[i], pj->value.name.key) == 0){
							warnx("Duplicate key: \"%s\"", pj->value.name.key);
						if(warn_only == 0)
							return -1;
						break;
					}
				}
			}
		}
		if(!(*order)[idx]){
			(*order)[idx] = pj->value.name.key;
		}
	}
	return idx;
}
int duplicate_keys(struct json *j, int warning_only){
	struct json *pj = j;
	char **order = NULL;
	for(pj = j; pj;pj = pj->next){
		if(pj->type == ARRAY){
			if(pj->sub){
				if(duplicate_keys(pj->sub, warning_only) == 1)
					return 1;
			}
			if(warning_only >= 0 && (pj->t_val&WARN) == WARN){
				warnx("Mal formed number: %s", pj->value.value);
				if(warning_only == 0)
					return 1;
			}
		}else{
			if(pj->sub)
				if(duplicate_keys(pj->sub, warning_only) == 1){
					free(order);
					return 1;
				}
			if(warning_only >= 0 && (pj->t_val&WARN) == WARN){
				warnx("Mal formed number: %s", pj->value.value);
				if(warning_only == 0)
					return 1;
			}
			if(json_sort(pj, &order, 0, warning_only) == -1){
				free(order);
				return 1;
			}
			free(order);
			order = NULL;
		}
	}
	return 0;
}
void json_print(struct json *j, int sort, size_t space, char c_sp, size_t count, int warn_only){
	struct json *pj = j;
	ssize_t k, idx;
	unsigned long int sp, i;
	int type = j->type;
	char **order = NULL;
	if(c_sp)
		(type == ARRAY) ? puts("[") : puts("{");
	else
		(type == ARRAY) ? putchar('[') : putchar('{');
	if(j->type == PAIR){
		if(sort){
			if((idx = json_sort(j, &order, sort, warn_only)) == -1)
				_exit(255);
		}else{
			if(json_sort(j, &order, sort, warn_only) == -1)
				_exit(255);
			idx = 1;
		}
	}else
		idx = 1;
	for(k = 0; k < idx; k++)
		for(pj = j; pj; pj = pj->next){
			if((pj->type&PAIR) == PAIR && sort){
				if(strcmp(pj->value.name.key, order[k]))
					continue;
				if((pj->type&SET) == SET)
					continue;
				else
					pj->type |= SET;
				if(k > 0){
					if(c_sp)
						puts(",");
					else
						putchar(',');
				}
			}else{
				if(pj->prev){
					if(c_sp)
						puts(",");
					else
						putchar(',');
				}
				pj->type |= SET;
			}
			if(c_sp)
				for(i = 0; i < count; i++)
					for(sp = 0; sp <= space; sp++)
						putchar(c_sp);
			switch(pj->type^SET){
				case ARRAY:
					switch(pj->t_val){
						case INT: case INT|WARN:
							if(warn_only >= 0 && (pj->t_val&WARN) == WARN){
								warnx("Mal formed number: %s", pj->value.value);
									if(warn_only == 0)
										_exit(255);
								}
							printf("%s", pj->value.value);
							break;
						case STRING:
							printf("\"%s\"", pj->value.value);
							break;
						default:
							break;
					}
					break;
				case PAIR:
					printf("\"%s\":", pj->value.name.key);
					switch(pj->t_val){
						case INT: case INT|WARN:
							if(warn_only >= 0 && (pj->t_val&WARN) == WARN){
								warnx("Mal formed number: %s", pj->value.value);
									if(warn_only == 0)
										_exit(255);
								}
							printf("%s", pj->value.value);
							break;
						case STRING:
							printf("\"%s\"", pj->value.value);
							break;
						default:
							break;
					}
					break;
			}
			if(pj->sub)
				json_print(pj->sub, sort, space+1, c_sp, count, warn_only);
			if(sort && (pj->type&PAIR) == PAIR)
				break;
		}
	if(order)
		free(order);
	if(c_sp)
		putchar('\n');
	if(c_sp)
		for(i = 0; i < count; i++)
			for(sp = 0; sp < space; sp++)
				putchar(c_sp);
	(type == ARRAY) ? putchar(']') : putchar('}');
	/*if(space == 0)
		putchar('\n');*/
}
void json2txt(struct json *j, int sort, char *string, int warn_only){
	struct json *pj = j;
	ssize_t k, idx;
	unsigned long int len;
	char *str, ulong[23], **order = NULL;
	if(j->type == PAIR){
		if(sort){
			if((idx = json_sort(j, &order, sort, warn_only)) == -1)
				_exit(255);
		}else{
			if(json_sort(j, &order, sort, warn_only) == -1)
				_exit(255);
			idx = 1;
		}
	}else
		idx = 1;

	for(k = 0; k < idx; k++)
		for(pj = j; pj; pj = pj->next){
			if((pj->type&PAIR) == PAIR && sort){
				if(strcmp(pj->value.name.key, order[k])){
					continue;
				}
				if((pj->type&SET) == SET){
					continue;
				}else
					pj->type |= SET;
			}else
				pj->type |= SET;
			switch(pj->type^SET){
				case ARRAY:
					switch(pj->t_val){
						case INT: case INT|WARN:
							if(warn_only >= 0 && (pj->t_val&WARN) == WARN){
								warnx("Mal formed number: %s", pj->value.value);
									if(warn_only == 0)
										_exit(255);
								}
							if(string)
								printf("%s[%lu]:%s\n",
									string, pj->value.name.index, pj->value.value);
							else
								printf("[%lu]:%s\n", pj->value.name.index, pj->value.value);
							break;
						case STRING:
							if(string)
								printf("%s[%lu]:\"%s\"\n",
										string, pj->value.name.index, pj->value.value);
							else
								printf("[%lu]:\"%s\"\n", pj->value.name.index, pj->value.value);
							break;
						default:
							break;
					}
					break;
				case PAIR:
					switch(pj->t_val){
						case INT:case INT|WARN:
							if(warn_only >= 0 && (pj->t_val&WARN) == WARN){
								warnx("Mal formed number: %s", pj->value.value);
									if(warn_only == 0)
										_exit(255);
								}
							if(string)
								printf("%s.%s:", string, pj->value.name.key);
							else
								printf("%s:", pj->value.name.key);
							printf("%s\n", pj->value.value);
							break;
						case STRING:
							if(string)
								printf("%s.%s:", string, pj->value.name.key);
							else
								printf("%s:", pj->value.name.key);
							printf("\"%s\"\n", pj->value.value);
							break;
						default:
							break;
					}
					break;
				}
			if(pj->sub){
				if(string){
					switch(pj->type^SET){
						case ARRAY:
							sprintf(ulong, "[%lu]", pj->value.name.index);
							len = strlen(string) + strlen(ulong) + 1;
							if((str = calloc(len, sizeof(char))) == NULL){
								err(255, "calloc()");
							}
							strcpy(str, string);
							strcat(str, ulong);
							break;
						case PAIR:
							len = strlen(string) + strlen(pj->value.name.key) +2;
							if((str = calloc(len, sizeof(char))) == NULL){
								err(255, "calloc()");
							}
							strcpy(str, string);
							strcat(str, ".");
							strcat(str, pj->value.name.key);
							break;
					}
				}else{
					switch(pj->type^SET){
						case ARRAY:
							sprintf(ulong, "[%lu]", pj->value.name.index);
							len = strlen(ulong) + 1;
							if((str = calloc(len, sizeof(char))) == NULL){
								err(255, "calloc()");
							}
							strcpy(str, ulong);
							break;
						case PAIR:
							len = strlen(pj->value.name.key) +1;
							if((str = calloc(len, sizeof(char))) == NULL){
								err(255, "calloc()");
							}
							strcat(str, pj->value.name.key);
							break;
					}
				}
				json2txt(pj->sub, sort, str, warn_only);
				free(str);
			}
			if(sort && (pj->type&PAIR) == PAIR)
				break;
		}
	free(order);
}
void json_reset_flags(struct json *j){
	struct json *pj = j;
	for(pj = j; pj;pj = pj->next){
		if(pj->sub)
			json_reset_flags(pj->sub);
		if((pj->type&SET) == SET)
			pj->type -= SET;
	}
}
void json_destroy(struct json **j){
	struct json *pj = *j, *ppj;
	while(pj){
		if(pj->sub)
			json_destroy(&pj->sub);
		switch(pj->type&(ARRAY|PAIR)){
			case ARRAY:
				if(pj->value.value)
					free(pj->value.value);
				break;
			case PAIR:
				if(pj->value.name.key)
					free(pj->value.name.key);
				if(pj->value.value){
					free(pj->value.value);
				}
				break;
			}
		ppj = pj->next;
		free(pj);
		pj = ppj;
	}
}

