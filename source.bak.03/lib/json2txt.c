#include "json2txt.h"
/*functions*/
int json_err = 0;
ssize_t read_fn(struct json_parser *p){
	if((p->r_len = read(p->file, p->buffer, p->buflen)) < 0){
		warn("read()");
		json_err = errno;
		return 0;
	}
	p->buffer[p->r_len] = 0;
	p->buf = p->buffer;
	return p->r_len;
}
void readchar(ssize_t *offset, char **str, const char *not){
	static const char *n;
	for(;**str;(*str)++,(*offset)++)
		for(n = not;*n != **str;n++)
			if(*n == 0)
				return;
}
int json_errors(struct json_parser *p, struct json **j, struct fn **f){
	switch(json_err){
		case 0:
			warnx("Parse is success");
			break;
		case -1:
			if(*p->buf && (*f)->err == 0 && (*f)->c_end == (*f)->err)
				warnx("Invalid JSON data: at offset %lu unexpected '%c'.\n\tJSON data not starting by '{' or '['",
					p->offset, *p->buf);
			else{
				/*if((*f)->err != '[' && (*f)->err != '{'){
					if((*f)->prev->err == '{'){
						warnx("OBJECT: Unexpected end, expected '}' after offset %lu", p->offset);
						json_err = -3;
					}else{
						warnx("ARRAY: Unexpected end, expected ']' after offset %lu", p->offset);
						json_err = -2;
					}
				}else*/
					warnx("Unexpected '%c' at offset %lu.\n\tGarbage in JSON data.", *p->buf, p->offset);
			}
			break;
		case -2:
			if(*p->buf == 0){
				/*if((*f)->c_end != ',')
					warnx("ARRAY: Unexpected end, expected ']' after offset %lu", p->offset);
				else*/
					warnx("ARRAY: Unexpected %s, expected ']' after offset %lu",
							((*f)->c_end == ',') ? "','" : "end", p->offset);
				break;
			}
			if((*f)->c_end == ',')
				warnx("ARRAY: Unexpected '%c' at offset %lu, expected 'number|bool|null|\"string\"'", *p->buf, p->offset);
			else
				warnx("ARRAY: Unexpected '%c' at offset %lu, expected ']' or ',number|bool|null|\"string\"(,...)]'",
						*p->buf, p->offset);
			break;
		case -3:
			if(*p->buf == 0 && (*f)->err != ','){
				warnx("OBJECT: Unexpected end, expected '}' after offset %lu", p->offset);
				break;
			}
			switch((*f)->err){
				case ':':
					if((*j)->value.name.key && !(*j)->value.value)
						warnx(
					"OBECT: Unexpected '%c' at offset %lu, expected 'number|bool|null|\"string\"' for key \"%s\"",
							*p->buf, p->offset, (*j)->value.name.key);
					else
						if(!(*j)->value.name.key)
							warnx("OBJECT: Unexpected ':' at offset %lu, expected '}' no \"key\" detected",
								p->offset);
						else
							warnx("OBJECT: Unexpected '%c' at offset %lu, expected '}' or ','",
								*p->buf, p->offset);
					break;
				case ',':
					if(!(*j)->value.name.key)
						warnx(
					"OBJECT: Unexpected ',' at offset %lu, expected '}' or '\"key\":number|bool|null|\"string\"(,...)}'",
							p->offset
						);
					else
						if((*f)->c_end != ':')
							warnx(
					"OBJECT: Unexpected '%c' at offset %lu, expected ':number|bool|null|\"string\"' for key \"%s\"",
							*p->buf, p->offset, (*j)->value.name.key);
						else
							warnx(
					"OBJECT: Unexpected '%c' at offset %lu, expected 'number|bool|null|\"string\"' for key \"%s\"",
							*p->buf, p->offset, (*j)->value.name.key);
					break;
				default:
					if(*p->buf == '}'){
						if((*j)->value.name.key && !(*j)->value.value){
							warnx(
					"OBJECT: Unexpected '%c' at offset %lu, expected ':number|bool|null|\"string\"' for key \"%s\"",
							*p->buf, p->offset, (*j)->value.name.key);
						}
					}else{
						warnx(
					"OBJECT: Unexpected '%c' at offset %lu, expected '}' or ',\"key\":number|bool|null|\"string\"(,...)}'",
						*p->buf, p->offset);
					}
					break;
			}
			break;
		case -4:
			warnx("Unexpected EOF, string start at offset %lu", (*f)->offset);
			break;
		case -5:
			warnx("Unexpected '%c', invalid number or expected string at offset %lu", *p->buf, p->offset);
			break;
		case -6:
			/*if(p->stock)*/
			warnx("Invalid boolean or expected string at offset %lu : %s (start at %lu)",
				p->offset -1, p->stock, p->offset - strlen(p->stock));
			/*else
				warnx("Unexpected '%c', or expected \".*%c.*\" at offset %lu", *p->buf, *p->buf, p->offset);*/
			break;
		case -7:
			if(p->stock)
				warnx("Invalid number|bool or expected string at offset %lu : %s (start at %lu)",
					p->offset, p->stock, p->offset - strlen(p->stock) +1);
			else{
				if(((*j)->t_val&WARN_NBR))
					warnx("Malformed number: %s", (*j)->value.value);
				else
					warnx("Duplicated key: \"%s\"", (*j)->value.name.key);
			}
			break;
		default:
			warnx("System error");
			break;
	}
	return json_err;
}
int json_bool_test(char *stock){
	static char *b_wanted[3] = { "true" , "false", "null" };
	if(!strcmp(stock, b_wanted[0]) || !strcmp(stock, b_wanted[1]) || !strcmp(stock, b_wanted[2]))
		return 0;
	return json_err = -6;
}
int json_int_test(char *stock, char *pstock){
	if(	(!stock || *pstock != 0)
		|| (*(stock + (*stock == '+' || *stock == '-')) == '\0')
		|| (
			*(pstock-1) == *(stock + (*stock == '+' || *stock == '-'))
			&& ((*(pstock -1) == '.') )
		)
	)	return json_err = -5;
	if(*stock == '+' || *(stock + (*stock == '-')) == '.')
		return json_err = -7;
	return 0;
}
void *allocstr(char **buffer, size_t lentoadd){
	if(!*buffer){
		if((*buffer = calloc(lentoadd, sizeof(char))) == NULL){
			json_err = errno;
			warn("calloc");
			return NULL;
		}
	}else{
		if((*buffer = realloc(*buffer, lentoadd * sizeof(char))) == NULL){
			json_err = errno;
			warn("realloc()");
			return NULL;
		}
	}
	return *buffer;
}
void *json_create(struct json **j, enum KIND kind, enum TYPE type){
	static struct json *rj;
	switch(kind){
		case NEW:
			if(((*j) = calloc(1, sizeof(struct json))) == NULL){
				json_err = errno;
				warn("calloc()");
				return NULL;
			}
			rj = *j;
			break;
		case SUB:
			rj = *j;
			if(((*j)->sub = calloc(1, sizeof(struct json))) == NULL){
				json_err = errno;
				warn("calloc()");
				return NULL;
			}
			(*j)->sub->up = *j;
			(*j) = (*j)->sub;
			break;
		case NEXT:
			rj = *j;
			if(((*j)->next = calloc(1, sizeof(struct json))) == NULL){
				json_err = errno;
				warn("calloc()");
				return NULL;
			}
			(*j)->next->prev = *j;
			(*j) = (*j)->next;
			break;
	}
	(*j)->t_val = VOID;
	(*j)->type = type;
	return rj;
}
struct json *go_first(struct json *j){
	struct json *pj = j;
	while(pj->prev || pj->up){
		if(pj->up)
			pj = pj->up;
		if(pj->prev)
			pj = pj->prev;
	}
	return pj;
}
#define STOCK_BUF(p) \
	if(p->len == 0){ \
		if(allocstr(&p->stock, p->stock_buf) == NULL) \
			return -3; \
		p->stock_size = p->stock_buf; \
		memset(p->stock, 0, p->stock_buf); \
		p->pstock = p->stock; \
	}else{ \
		if(p->len +1 == p->stock_size){ \
			if(allocstr(&p->stock, p->stock_buf + p->stock_size) == NULL) \
				return -3; \
			p->stock_size += p->stock_buf; \
			memset(p->stock + p->len, 0, p->stock_buf+1); \
			p->pstock = p->stock + p->len; \
		} \
	} \
	*p->pstock = *p->buf; \
	p->pstock++;\
	p->len++;

int get_int(struct json_parser *p, int init){
	static const char *ib[4][2] = { { "true", "TRUE" }, { "false", "FALSE" }, { "null", "NULL" }, { NULL, NULL} };
	static char **pib, *p1, *p2;
	static int start, signe, zero, dot, i, bret;
	if(init){
		start = signe = zero = dot = bret = 0;
		for(pib = (char **)ib[0]; *pib; pib += 2){
			if(*p->buf == **pib || (bret = (*p->buf == **(pib+1)))){
				p1 = *pib;
				p2 = *(pib+1);
				STOCK_BUF(p);
				if(bret){
					warnx("Invalid boolean (Upper case) at offset %lu", p->offset);
				}
				return 1;
			}
		}
	}
	if(*pib){
		p1++;
		p2++;
		if(*p->buf != *p1 && *p->buf != *p2){
			if(*p1 != 0){
				*p->pstock = 1;
			}
			return 0;
		}
		STOCK_BUF(p);
		if(!bret && (bret = (*p->buf == *p2))){
			warnx("Invalid boolean (Upper case) at offset %lu", p->offset);
			return 1;
		}
		if(*p1 == 0){
			*p->pstock = 1;
			return 0;
		}
		return 1;
	}
	if(signe == 0 && (*p->buf == '-' || *p->buf == '+')){
		STOCK_BUF(p);
		signe = 1;
		if(*p->buf == '+')
			return -1;
		return 2;
	}
	signe = 1;
	if(*p->buf == '.'){
		switch(dot){
			case 0:
				i = (zero == 0 && start == 0);
				dot = 1;
				start = 1;
				STOCK_BUF(p);
				if(i)
					return -2;
				return 2;
			default:
				dot = 1;
				STOCK_BUF(p);
				return 2;
			}
	}else{
		if(start == 0 && *p->buf == '0' && zero++ > 0){
			warnx("Invalid number.");
			return 0;
		}
		if(*p->buf != '0')
			start = 1;
		if(*p->buf < '0' || *p->buf > '9'){
			return 0;
		}
		STOCK_BUF(p);
	}
	return 2;
}

int get_str(struct json_parser *p){
	static int jump = 0;
	static ssize_t start = 0;
	if(jump){
		STOCK_BUF(p);
		jump = 0;
	}
	if(!start)
		start = p->offset -1;
	switch(*p->buf){
		case '\\':
			STOCK_BUF(p);
			jump = 1;
			break;
		case '"':
			if(!jump){
				start = 0;
				json_err = 0;
				return start;
			}
			STOCK_BUF(p);
			jump = 0;
			break;
		default:
			jump = 0;
			STOCK_BUF(p);
	}
	return start;
}
void up_str_from_array(struct json **j, struct json_parser *p, struct fn *f){
	char *stock = p->stock;
	(*j)->value.name.index = (f->index)++;
	(*j)->value.value = (stock) ? stock : "";
}
void up_str_from_pair(struct json **j, struct json_parser *p){
	char *stock = p->stock;
	if((*j)->value.name.key == NULL)
		(*j)->value.name.key = (stock) ? stock : "";
	else
		(*j)->value.value = (stock) ? stock : "";
}
void up_from_bool(struct json **j, struct json_parser *p, struct fn *f){
	static char *booleans[3] = {"true", "false", "null" };
	f->value = 0;
	f->key = 1;
	f->c_end = 0;
	if(!strcmp(booleans[0], p->stock)
		|| 	!strcmp(booleans[1], p->stock)
		||	!strcmp(booleans[2], p->stock)
	)return;
	(*j)->t_val |= WARN_BOOL;

}
void up_from_int(struct json **j, struct json_parser *p, struct fn *f){
	static char c_char/*, *booleans[3] = {"true", "false", "null" }*/;
	f->value = 0;
	f->key = 1;
	f->c_end = 0;
	if((c_char = *p->stock) == '+' || *(p->stock + (c_char == '+' || c_char == '-')) == '.' || *(p->pstock -1) == '.'){
		(*j)->t_val |= WARN_NBR;
		return;
	}

}
void up_structure_from_int_pair(struct json_parser *p, struct json *j){
	j->value.value = p->stock;
	p->stock = NULL;
	p->len = p->stock_size = 0;
}
void up_structure_from_int_array(struct json_parser *p, struct json *j, struct fn *f){
	j->value.name.index = (f->index)++;
	j->value.value = p->stock;
	p->stock = NULL;
	p->len = p->stock_size = 0;
}
void up_structure_from_str(struct json_parser *p, struct fn *f){
	p->stock = NULL;
	p->len = p->stock_size = 0;
	f->offset = p->offset;
}
#define DESTROY_ALL(j, p) \
	*j = go_first(*j); \
	if(p->stock) \
		free(p->stock); \
	json_destroy(j);
#define DESTROY_fn(ptr, reader) \
	while(ptr){ \
		reader = ptr->prev; \
		free(ptr); \
		ptr = reader; \
	}
#define NEW_PJ(pj) \
	if(*pj == NULL){ \
		if((*pj = calloc(1, sizeof(struct json_new_lst))) == NULL){ \
			json_err = errno; \
			warn("calloc()"); \
			return json_err; \
		} \
	}else{ \
		if(((*pj)->next = calloc(1, sizeof(struct json_new_lst))) == NULL){ \
			json_err = errno; \
			warn("calloc()"); \
			return json_err; \
		} \
		(*pj)->next->prev = *pj; \
		*pj = (*pj)->next; \
	}
#define DEL_PJ(pj) \
	if((*pj)->prev){ \
		*pj = (*pj)->prev; \
		free((*pj)->next); \
	}else{ \
		free(*pj); \
		*pj = NULL; \
	}
int starting(struct json_parser *p, struct json **j, struct fn **f, struct json_new_lst **pj){
	static struct json *spj;
	struct json_new_lst *lst;
	struct fn *rf;
	switch(*p->buf){
		case '{':
			NEW_PJ(pj);
			(*pj)->json_err = -3;
			if((spj = (*pj)->j = json_create(j, NEW, PAIR)) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				json_err = errno;
				warn("calloc()");
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->offset = p->offset;
			(*f)->err = '{';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '{';
			(*f)->key = 1;
			(*f)->value = 0;
			(*f)->fns.do_it.read_with_this = &pair;
			break;
		case '[':
			NEW_PJ(pj);
			(*pj)->json_err = -2;
			if((spj = (*pj)->j = json_create(j, NEW, ARRAY)) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				json_err = errno;
				warn("calloc()");
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->err = '[';
			(*f)->offset = p->offset;
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '[';
			(*f)->fns.do_it.read_with_this = &array;
			break;
		default:
			(*f)->fns.do_it.do_errors = &json_errors;
			return json_err = -1;
	}
	*j = spj;
	return 0;
}
int array(struct json_parser *p, struct json **j, struct fn **f, struct json_new_lst **pj){
	static struct fn *rf;
	static struct json_new_lst *lst;
	switch(*p->buf){
		case '"':
			if((*f)->c_end != '[' &&(*f)->c_end != ','){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -2;
			}
			json_err = -4;
			(*f)->c_end = 0;
			(*f)->offset = p->offset;
			(*j)->t_val = STRING;
			(*f)->fns.get.get_str = &get_str;
			(*f)->fns.up.up_from_array = &up_str_from_array;
			(*f)->fns.structure.up_from_string = &up_structure_from_str;
			break;
		case ',':
			(*f)->offset = p->offset;
			(*f)->err = ',';
			if((*f)->c_end != 0 && (*f)->c_end != ']' && (*f)->c_end != '}'){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -2;
			}
			if(json_create(j, NEXT, ARRAY) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->c_end = ',';
			break;
		case ']':
			if((*f)->c_end != 0 && (*f)->c_end != '[' && (*f)->c_end != '}' && (*f)->c_end != ']'){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -2;
			}
			*f = (*f)->prev;
			free((*f)->next);
			(*f)->next = NULL;
			(*f)->key = 0;
			(*f)->value = 0;
			(*f)->c_end = ']';
			*j = (*pj)->j;
			DEL_PJ(pj);
			json_err = (*pj) ? (*pj)->json_err : 0;
			break;
		case '[':
			NEW_PJ(pj);
			(*pj)->json_err = -2;
			(*f)->offset = p->offset;
			(*f)->err = '[';
			(*j)->value.name.index = (*f)->index;
			(*f)->index++;
			if((*f)->c_end != '{' && (*f)->c_end != '[' && (*f)->c_end != ','){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -2;
			}
			if(((*pj)->j = json_create(j, SUB, ARRAY)) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				json_err = errno;
				warn("calloc()");
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '[';
			(*f)->offset = p->offset;
			(*f)->fns.do_it.read_with_this = &array;
			break;
		case '{':
			(*f)->offset = p->offset;
			(*f)->err = '{';
			if((*f)->c_end != '[' && (*f)->c_end != ','){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -2;
			}
			NEW_PJ(pj);
			(*pj)->json_err = -3;
			(*j)->value.name.index = (*f)->index;
			(*f)->index++;
			if(((*pj)->j = json_create(j, SUB, PAIR)) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				json_err = errno;
				warn("calloc()");
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->err = '{';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '{';
			(*f)->fns.do_it.read_with_this = &pair;
			(*f)->key = 1;
			(*f)->value = 0;
			break;
		case '}':
			(*f)->err = '}';
			(*f)->fns.do_it.do_errors = &json_errors;
			return json_err = -2;
		case 0:
			break;
		default:
			if((*f)->c_end != '[' && (*f)->c_end != ','){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -2;
			}
			(*j)->t_val = INT;
			(*f)->c_end = 0;
			(*f)->offset = p->offset;
			(*f)->fns.get.get_int = &get_int;
			(*f)->fns.up.up_int = &up_from_int;
			(*f)->fns.structure.up_int_from_array = &up_structure_from_int_array;
			break;
	}
	return 0;
}
int pair(struct json_parser *p, struct json **j, struct fn **f, struct json_new_lst **pj){
	static struct fn *rf;
	static struct json_new_lst *lst;
	switch(*p->buf){
		case '"':
			if((*f)->key == 1)
				(*f)->key = 0;
			if((*f)->c_end != '{' && (*f)->c_end != ':' && (*f)->c_end != ','){
				(*f)->fns.do_it.do_errors = &json_errors;
				return -3;
			}
			if((*f)->value == 0)
				(*f)->value = 1;
			else
				if((*f)->value == 2){
					(*f)->c_end = 0;
					(*f)->value = 0;
					(*j)->t_val = STRING;
				}else{
					(*f)->fns.do_it.do_errors = &json_errors;
					return json_err = -3;
				}
			json_err = -4;
			(*f)->offset = p->offset;
			(*f)->fns.get.get_str = &get_str;
			(*f)->fns.up.up_from_pair = &up_str_from_pair;
			(*f)->fns.structure.up_from_string = &up_structure_from_str;
			break;
		case ':':
			if((*f)->value == 1 && (*f)->key == 0)
				(*f)->value = 2;
			else{
				(*f)->fns.do_it.do_errors = &json_errors;
				(*f)->err = ':';
				return json_err = -3;
			}
			(*f)->offset = p->offset;
			(*f)->err = ':';
			(*f)->c_end = ':';
			(*f)->key = 0;
			break;
		case '{':
			if((*f)->c_end != '{' && (*f)->c_end != ':'){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -3;
			}
			NEW_PJ(pj);
			(*pj)->json_err = -3;
			if(((*pj)->j = json_create(j, SUB, PAIR)) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				json_err = errno;
				warn("calloc()");
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->offset = p->offset;
			(*f)->err = '{';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '{';
			(*f)->fns.do_it.read_with_this = &pair;
			(*f)->key = 1;
			(*f)->value = 0;
			break;
		case '[':
			/*pj = *j;*/
			if((*f)->c_end != ',' && (*f)->c_end != ':'){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -3;
			}
			NEW_PJ(pj);
			(*pj)->json_err = -2;
			if(((*pj)->j = json_create(j, SUB, ARRAY)) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				json_err = errno;
				warn("calloc()");
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->offset = p->offset;
			(*f)->err = '[';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '[';
			(*f)->fns.do_it.read_with_this = &array;
			(*f)->key = 1;
			(*f)->value = 0;
			break;
		case ',':
			(*f)->err = ',';
			if((*f)->c_end != 0 && (*f)->c_end != ']' && (*f)->c_end != '}'){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -3;
			}
			if(json_create(j, NEXT, PAIR) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				return json_err;
			}
			(*f)->offset = p->offset;
			(*f)->c_end = ',';
			(*f)->key = 1;
			break;
		case '}':
			if(((*f)->c_end != 0 && (*f)->c_end != ']' && (*f)->c_end != '{' && (*f)->c_end != '}') || (*f)->value){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -3;
			}
			*f = (*f)->prev;
			free((*f)->next);
			(*f)->next = NULL;
			(*f)->key = 0;
			(*f)->value = 0;
			*j = (*pj)->j;
			DEL_PJ(pj);
			json_err = (*pj) ? (*pj)->json_err : 0;
			(*f)->c_end = '}';
			break;
		case ']':
			(*f)->fns.do_it.do_errors = &json_errors;
			(*f)->err = ']';
			return json_err = -3;
		case 0:
			break;
		default:
			if((*f)->value != 2 || (*f)->key == 1){
				(*f)->fns.do_it.do_errors = &json_errors;
				return json_err = -3;
			}
			p->len = 0;
			(*j)->t_val = INT;
			(*f)->fns.get.get_int = &get_int;
			(*f)->fns.up.up_int = &up_from_int;
			(*f)->fns.structure.up_int_from_pair = &up_structure_from_int_pair;
			break;
	}
	return 0;
}
#define TEST(j, pj, ppj, warn) \
	for(pj = j; pj; pj = pj->next){ \
		pj->t_val |= SET; \
		for(ppj = pj->next;ppj; ppj = ppj->next){  \
			if((ppj->t_val&WARN) == 0 && strcmp(pj->value.name.key, ppj->value.name.key) == 0){ \
				ppj->t_val |= WARN; \
				DUP \
				if(warn >= 0){ \
					warnx("Duplicate key: \"%s\"", pj->value.name.key); \
					if(warn == 0){ \
						json_err = -7; \
						return 1; \
					} \
				} \
			} \
		} \
	}
int json_sort_asc(struct json **sj, int warn_only){
	struct json *pj, *ppj, *ppprev, *ppnext, *sub, *j = *sj;
	int dup = 0;
	#define DUP dup = 1;
	TEST(j, pj, ppj, warn_only);
	#undef DUP
	for(pj = j; pj;pj = pj->next){
		for(ppj = j; ppj && strcmp(pj->value.name.key, ppj->value.name.key) >= 0; ppj = ppj->next);
		if(!ppj || pj == ppj){
			continue;
		}
		ppnext = ppj->next;
		ppprev = ppj->prev;
		ppj->next = ppj->prev = NULL;
		if(ppprev){
			ppprev->next = ppnext;
			if(ppprev->next)
				ppprev->next->prev = ppprev;
		}else{
			ppnext->prev = ppnext->prev->next = NULL;
			j = ppnext;
		}
		sub = ppj->up;
		ppj->next = pj->next;
		pj->next = ppj;
		if(ppj->next)
			ppj->next->prev = ppj;
		else
			if(dup)
				pj = j;
		pj->next->prev = pj;
		if(sub){
			sub->sub = pj;
			sub->sub->up = sub;
			ppj->up = NULL;
		}
	}
	*sj = j;
	return 0;
}
int json_test(struct json **j, int warn_only){
	struct json *pj, *ppj;
	#define DUP
	TEST(*j, pj, ppj, warn_only);
	#undef DUP
	return 0;
}
int json_sort_dsc(struct json **sj, int warn_only){
	struct json *pj, *ppj, *ppprev, *ppnext, *j = *sj;
	#define DUP
	TEST(j, pj, ppj, warn_only);
	#undef DUP
	for(pj = j; pj;pj = pj->next){
		for(	ppj = j;
			ppj &&
				strcmp(	pj->value.name.key,
					ppj->value.name.key
				) <= 0;
			ppj = ppj->next
		);
		if(!ppj || pj == ppj){
			continue;
		}
		ppnext = ppj->next;
		ppprev = ppj->prev;
		ppj->next = ppj->prev = NULL;
		if(ppprev){
			ppprev->next = ppnext;
			if(ppprev->next)
				ppprev->next->prev = ppprev;
		}else{
			ppnext->prev = ppnext->prev->next = NULL;
			j = ppnext;
		}
		ppj->next = pj->next;
		pj->next = ppj;
		if(ppj->next)
			ppj->next->prev = ppj;
		pj->next->prev = pj;
	}
	*sj = j;
	return 0;
}
int json_test_sort(struct json **j, int (*sorting)(struct json **, int), int warn_only){
	struct json *pj;
	for(pj = *j;pj;pj = pj->next){
		if((pj->type&PAIR) && !(pj->t_val&SET)){
			if((*sorting)(&pj, warn_only)){
				return 1;
			}
			*j = pj;
		}
		if(pj->sub){
			if(json_test_sort(&pj->sub, sorting, warn_only)){
				return 1;
			}
		}
		if(warn_only >= 0 && (((pj->t_val&(INT|WARN_BOOL)) == (INT|WARN_BOOL)) || ((pj->t_val&(INT|WARN_NBR)) == (INT|WARN_NBR)))){
			warnx("Mal formed %s: %s", ((pj->t_val&(INT|WARN_NBR)) == (INT|WARN_NBR)) ? "number" : "boolean" , pj->value.value);
			if(warn_only == 0){
				json_err = -4;
				return 1;
			}
		}
	}
	return 0;
}
void json_print(struct json *j, size_t space, char *sep, char c_sp, size_t count){
	struct json *pj = j;
	unsigned long int sp, i;
	int type = j->type;
	(type == ARRAY) ? putchar('[') : putchar('{');
	if((j->next || j->sub) || (j->type&ARRAY && j->value.value) || ((j->type&PAIR) == PAIR && j->value.name.key))
		putchar('\n');
	for(pj = j; pj; pj = pj->next){
		if(pj->prev){
			if(c_sp)
				puts(",");
			else
				putchar(',');
		}
		if(c_sp && ((j->next || j->sub) || (j->type&ARRAY && j->value.value) || ((j->type&PAIR) == PAIR && j->value.name.key)))
			for(i = 0; i < count; i++)
				for(sp = 0; sp <= space; sp++)
					putchar(c_sp);
		switch(pj->type){
			case ARRAY:
				switch(pj->t_val^(pj->t_val&(SET|WARN|WARN_NBR|WARN_BOOL))){
					case INT:
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
				if(pj->value.name.key){
					printf("\"%s\"%s", pj->value.name.key, sep);
					switch(pj->t_val^(pj->t_val&(SET|WARN|WARN_NBR|WARN_BOOL))){
						case INT: 
							printf("%s", pj->value.value);
							break;
						case STRING:
							printf("\"%s\"", pj->value.value);
							break;
						default:
							break;
					}
				}
				break;
		}
		if(pj->sub)
			json_print(pj->sub, space+1, sep, c_sp, count);
	}
	if(c_sp && ((j->next || j->sub) || (j->type&ARRAY && j->value.value) || ((j->type&PAIR) == PAIR && j->value.name.key))){
		putchar('\n');
		for(i = 0; i < count; i++)
			for(sp = 0; sp < space; sp++)
				putchar(c_sp);
	}
	(type == ARRAY) ? putchar(']') : putchar('}');
	if(space == 0)
		putchar('\n');
}
int json2txt(struct json *j, char *string){
	struct json *pj = j;
	unsigned long int len;
	char *str, ulong[23];
	for(pj = j; pj; pj = pj->next){
		switch(pj->type){
			case ARRAY:

				switch(pj->t_val^(pj->t_val&(SET|WARN|WARN_NBR|WARN_BOOL))){
					case INT:
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
				switch(pj->t_val^(pj->t_val&(SET|WARN|WARN_NBR|WARN_BOOL))){
					case INT:
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
			if(string)
				switch(pj->type){
					case ARRAY:
						sprintf(ulong, "[%lu]", pj->value.name.index);
						len = strlen(string) + strlen(ulong) + 1;
						if((str = calloc(len, sizeof(char))) == NULL){
							json_err = errno;
							warn("calloc()");
							return 1;
						}
						strcpy(str, string);
						strcat(str, ulong);
						break;
					case PAIR:
						len = strlen(string) + strlen(pj->value.name.key) +2;
						if((str = calloc(len, sizeof(char))) == NULL){
							json_err = errno;
							warn("calloc()");
							return 1;
						}
						strcpy(str, string);
						strcat(str, ".");
						strcat(str, pj->value.name.key);
						break;
				}
			else
				switch(pj->type){
					case ARRAY:
						sprintf(ulong, "[%lu]", pj->value.name.index);
						len = strlen(ulong) + 1;
						if((str = calloc(len, sizeof(char))) == NULL){
							json_err = errno;
							warn("calloc()");
							return json_err;
						}
						strcpy(str, ulong);
						break;
					case PAIR:
						len = strlen(pj->value.name.key) +1;
						if((str = calloc(len, sizeof(char))) == NULL){
							json_err = errno;
							warn("calloc()");
							return 1;
						}
						strcat(str, pj->value.name.key);
						break;
				}
			if(json2txt(pj->sub, str))
				return 1;
			free(str);
		}
	}
	return 0;
}
void json_destroy(struct json **j){
	struct json *pj = *j, *ppj;
	while(pj){
		if(pj->sub)
			json_destroy(&pj->sub);
		switch(pj->type&(ARRAY|PAIR)){
			case ARRAY:
				if(pj->value.value && strcmp(pj->value.value, "") != 0)
					free(pj->value.value);
				break;
			case PAIR:
				if(pj->value.name.key && strcmp(pj->value.name.key, "") != 0)
					free(pj->value.name.key);
				if(pj->value.value && strcmp(pj->value.value, "") != 0)
					free(pj->value.value);
				break;
			}
		ppj = pj->next;
		free(pj);
		pj = ppj;
	}
}
