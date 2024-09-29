/*read(), STD*_FILENO*/
#include <unistd.h>
/*memset, memcpy, strcpy, strcat, strcmp*/
#include <string.h>
/*err, errx, warnx*/
#include <err.h>
/*sprintf, printf*/
#include <stdio.h>
/*malloc, calloc(), realloc()*/
/*#ifdef  __USE_XOPEN_EXTENDED
	#undef __USE_XOPEN_EXTENDED
#endif
#ifdef __USE_XOPEN2K8
	#undef __USE_XOPEN2K8
#endif*/
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
	VOID = 2
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
ssize_t read_fn(struct json_parser *p);
void readchar(ssize_t *offset, char **str, const char *not);
void allocstr(char **buffer, size_t lentoadd);
void *json_create(struct json **j, enum KIND kind, enum TYPE type);
void *getint(struct json_parser *p);
void *getstr(struct json_parser *p);
void *array(struct json_parser *p, struct json **j);
void *pair(struct json_parser *p, struct json **j);
ssize_t json_sort(struct json *j, char ***order, int sort, int warn_only);
int duplicate_keys(struct json *j, int warning_only);
void json_print(struct json *j, int sort, size_t space, char c_sp, size_t count, int warn_only);
void json2txt(struct json *j, int sort, char *string, int warn_only);
void json_reset_flags(struct json *j);
void json_destroy(struct json **j);

