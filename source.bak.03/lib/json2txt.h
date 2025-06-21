/*read(), STD*_FILENO*/
#include <unistd.h>
/*memset, memcpy, strcpy, strcat, strcmp*/
#include <string.h>
#include <strings.h>
/*err, errx, warnx*/
#include <err.h>
#include <errno.h>
/*sprintf, printf*/
#include <stdio.h>
/*malloc, calloc(), realloc()*/
#include <stdlib.h>
extern int json_err;
#define INIT_JSON(BUFSIZE, ALLOC_SIZE, CHARS, buffer) { 0, BUFSIZE, ALLOC_SIZE, CHARS, buffer, NULL, 0, 0, 1, NULL, NULL, 0 }
enum TYPE{
	ARRAY = 1,
	PAIR = 2
};
enum VAL{
	VOID = 0,
	STRING = 1,
	INT = 2,
	WARN = 4,
	WARN_NBR = 8,
	WARN_BOOL = 16,
	SET = 32
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
	int t_val;
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
struct json_new_lst{
	struct json *j;
	struct json_new_lst *prev;
	struct json_new_lst *next;
};
struct fn{
	size_t index;
	ssize_t offset;
	int key;
	int value;
	int c_end;
	int err;
	struct {
		union{
			int (*get_it)(struct json_parser *, ...);
			int (*get_str)(struct json_parser *);
			int (*get_int)(struct json_parser *, int);
		}get;
		union{
			void (*up_it)(struct json **,...);
			void (*up_from_array)(struct json **, struct json_parser *, struct fn *);
			void (*up_from_pair)(struct json **, struct json_parser *);
			void (*up_int)(struct json **, struct json_parser *, struct fn *);
		}up;
		union{
			void (*up_string)(struct json_parser *, ...);
			void (*up_int)(struct json_parser *, ...);
			void (*up_from_string)(struct json_parser *, struct fn *);
			void (*up_int_from_array)(struct json_parser *, struct json *, struct fn *);
			void (*up_int_from_pair)(struct json_parser *, struct json *);
		}structure;
		union{
			int (*do_it)(struct json_parser *, ...);
			int (*read_with_this)(struct json_parser *, struct json **, struct fn **, struct json_new_lst **);
			int (*do_errors)(struct json_parser *, struct json **, struct fn **);
		}do_it;
	}fns;
	struct fn *next;
	struct fn *prev;
};
struct json *go_first(struct json *j);
ssize_t read_fn(struct json_parser *p);
void readchar(ssize_t *offset, char **str, const char *not);
int json_errors(struct json_parser *p, struct json **j, struct fn **f);
int json_int_test(char *stock, char *pstock);
int json_bool_test(char *stock);
void *allocstr(char **buffer, size_t lentoadd);
void *json_create(struct json **j, enum KIND kind, enum TYPE type);

int get_str(struct json_parser *p);
int get_int(struct json_parser *p, int init);
void up_str_from_array(struct json **j, struct json_parser *p, struct fn *f);
void up_str_from_pair(struct json **j, struct json_parser *p);
void up_from_bool(struct json **j, struct json_parser *p, struct fn *f);
void up_from_int(struct json **j, struct json_parser *p, struct fn *f);
void up_structure_from_int_pair(struct json_parser *p, struct json *j);
void up_structure_from_int_array(struct json_parser *p, struct json *j, struct fn *f);
void up_structure_from_str(struct json_parser *p, struct fn *f);

int starting(struct json_parser *p, struct json **j, struct fn **f, struct json_new_lst **nj);
int array(struct json_parser *p, struct json **j, struct fn **f, struct json_new_lst **nj);
int pair(struct json_parser *p, struct json **j, struct fn **f, struct json_new_lst **nj);

int json_sort_asc(struct json **j, int warn_only);
int json_sort_dsc(struct json **j, int warn_only);
int json_test_sort(struct json **j, int (*sorting)(struct json **, int), int warn_only);
int json_test(struct json **j, int warn_only);

void json_print(struct json *j, size_t space, char *sep, char c_sp, size_t count);
int json2txt(struct json *j, char *string);

void json_destroy(struct json **j);

