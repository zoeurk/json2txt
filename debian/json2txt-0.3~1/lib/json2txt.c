#include "json2txt.h"
/*functions*/
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
			/*#ifndef EXPLICIT_SIGN
				errx(255, "Invalid Number");
			#endif*/
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
			if(*p->buf == '.'){
				switch(dot){
					case 0:
						if(zero == 0 && start == 0){
							warnx("Value number: start by '(+|-)?.num', valid value should be '-?0.num' (offset: %lu)", p->offset);
							/*#ifdef STRICT_NUM
							errx("Invalid number.");
							return p;
							#endif*/
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
	warnx("Unexpected EOF\n\t'\"' at offset %lu.", start);
	return NULL;
}
#define DESTROY_ALL(j, p) \
	*j = go_first(*j); \
	if(p->stock) \
		free(p->stock); \
	json_destroy(j);
#define NEW_PJ(pj) \
	if(*pj == NULL){ \
		if((*pj = calloc(1, sizeof(struct json_new_lst))) == NULL) \
			err(255, "calloc()"); \
	}else{ \
		if(((*pj)->next = calloc(1, sizeof(struct json_new_lst))) == NULL) \
			err(255, "calloc()"); \
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
#define DESTROY_fn(ptr, reader) \
	while(ptr){ \
		reader = ptr->prev; \
		free(ptr); \
		ptr = reader; \
	}
/*enum CHARS{
	'{',
	'[',
	',',
	':',
	'}',
	']',
};*/
void *starting(struct json_parser *p, struct json **j, struct json_new_lst **pj, struct fn **f){
	static struct json *spj;
	struct fn *rf;
	struct json_new_lst *lst;
	switch(*p->buf){
		case '{':
			NEW_PJ(pj);
			spj = (*pj)->j = json_create(j, NEW, PAIR);
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				err(255, "calloc()");
			}
			(*f)->offset = p->offset;
			(*f)->err = '{';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '{';
			(*f)->key = 1;
			(*f)->value = 0;
			(*f)->do_it = &pair;
			p->buf++;
			p->offset++;
			break;
		case '[':
			NEW_PJ(pj);
			spj = (*pj)->j = json_create(j, NEW, ARRAY);
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				err(255, "calloc()");
			}
			(*f)->err = '[';
			(*f)->offset = p->offset;
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '[';
			(*f)->do_it = &array;
			p->buf++;
			p->offset++;
			break;
		case 0:
			break;
		default:
			DESTROY_fn((*f), rf);
			DESTROY_fn((*pj), lst);
			errx(255, "Unexpected character at offset %lu.", p->offset);
	}
	*j = spj;
	return p;
}
void *array(struct json_parser *p, struct json **j, struct json_new_lst **pj, struct fn **f){
	struct fn *rf;
	struct json_new_lst *lst;
	char c_char;
	switch(*p->buf){
		case '"':
			if((*f)->c_end != '[' &&(*f)->c_end != ','){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			(*f)->c_end = 0;
			p->buf++;
			p->offset++;
			if(!getstr(p)){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				_exit(255);
			}
			(*j)->value.name.index = (*f)->index;
			(*j)->value.value = (p->stock) ? p->stock : "";
			(*j)->t_val = STRING;
			p->stock = NULL;
			p->stock_size = 0;
			p->buf++;
			p->offset++;
			(*f)->offset = p->offset;
			(*f)->index++;
			break;
		case ',':
			(*f)->offset = p->offset;
			(*f)->err = ',';
			json_create(j, NEXT, ARRAY);
			if((*f)->c_end != 0 && (*f)->c_end != ']' && (*f)->c_end != '}'){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected ',' at offset %lu.", p->offset);
			}
			(*f)->c_end = ',';
			p->offset++;
			p->buf++;
			break;
		case ']':
			if((*f)->c_end != 0 && (*f)->c_end != '[' && (*f)->c_end != '}' && (*f)->c_end != ']'){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			*f = (*f)->prev;
			free((*f)->next);
			(*f)->next = NULL;
			(*f)->key = 0;
			(*f)->value = 0;
			(*f)->c_end = ']';
			*j = (*pj)->j;
			DEL_PJ(pj);
			p->offset++;
			p->buf++;
			return p;
		case '[':
			NEW_PJ(pj);
			(*f)->offset = p->offset;
			(*f)->err = '[';
			(*j)->value.name.index = (*f)->index;
			(*f)->index++;
			(*pj)->j = json_create(j, SUB, ARRAY);
			if((*f)->c_end != '{' && (*f)->c_end != '[' && (*f)->c_end != ','){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected '[' at offset %lu.", p->offset);
			}
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				err(255, "calloc()");
			}
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '[';
			(*f)->offset = p->offset;
			(*f)->do_it = &array;
			(*f)->offset = p->offset;
			p->buf++;
			p->offset++;
			break;
		case '{':
			(*f)->offset = p->offset;
			(*f)->err = '{';
			if((*f)->c_end != '[' && (*f)->c_end != ','){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected '{' at offset %lu.", p->offset);
			}
			NEW_PJ(pj);
			(*j)->value.name.index = (*f)->index;
			(*f)->index++;
			(*pj)->j = json_create(j, SUB, PAIR);
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				err(255, "calloc()");
			}
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '{';
			(*f)->do_it = &pair;
			(*f)->key = 1;
			(*f)->value = 0;
			p->buf++;
			p->offset++;
			break;
		case '}':
			warnx("Unexpected '}' at offset %lu.", p->offset);
			DESTROY_fn((*pj), lst);
			DESTROY_fn((*f), rf);
			DESTROY_ALL(j, p);
			_exit(255);
		case 0:
			break;
		default:
			if((*f)->c_end != '[' && (*f)->c_end != ','){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			getint(p);
			if(((c_char = *(p->buf)) < '0' || c_char > '9'  || c_char == '+' || c_char == '-' ) &&
				(!p->stock || *(p->pstock) != 0)){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter (bad number) near offset %lu: %c", p->offset, c_char);
			}
			(*f)->c_end = 0;
			c_char = *p->stock;
			if(c_char == '+' || *(p->stock + (c_char == '+' || c_char == '-')) == '.'|| *(p->pstock -1) == '.'){
				warnx("Invalid number before offset: %lu",p->offset -1);
				(*j)->t_val = INT | WARN;
			}else
				(*j)->t_val = INT;
			(*j)->value.name.index = (*f)->index;
			(*j)->value.value = p->stock;
			(*f)->index++;
			p->stock = NULL;
			p->stock_size = 0;
			break;
	}
	return p;
}
void *pair(struct json_parser *p, struct json **j, struct json_new_lst **pj, struct fn **f){
	struct fn *rf;
	struct json_new_lst *lst;
	int c_char;
	switch(*p->buf){
		case '"':
			if((*f)->key == 1)
				(*f)->key = 0;
			if((*f)->c_end != '{' && (*f)->c_end != ':' && (*f)->c_end != ','){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			if((*f)->value == 0)
				(*f)->value = 1;
			else
				if((*f)->value == 2)
					(*f)->value = 0;
				else{
					DESTROY_fn((*pj), lst);
					DESTROY_fn((*f), rf);
					DESTROY_ALL(j, p);
					errx(255, "Unexpected chararcter at offset %lu.", p->offset);
				}
			(*f)->c_end = 0;
			p->buf++;
			p->offset++;
			if(!getstr(p)){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				_exit(0);
			}
			if((*j)->value.name.key == NULL)
				(*j)->value.name.key = (p->stock) ? p->stock : "";
			else{
				(*j)->value.value = (p->stock) ? p->stock : "";
				(*j)->t_val = STRING;
			}
			p->stock = NULL;
			p->stock_size = 0;
			p->buf++;
			p->offset++;
			(*f)->offset = p->offset;
			break;
		case ':':
			if((*f)->value == 1)
				(*f)->value = 2;
			else{
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			(*f)->offset = p->offset;
			(*f)->err = ':';
			(*f)->c_end = ':';
			(*f)->key = 0;
			p->offset++;
			p->buf++;
			break;
		case '{':
			if((*f)->c_end != '{' && (*f)->c_end != ',' && (*f)->c_end != ':'){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected '{' at offset %lu.", p->offset);
			}
			NEW_PJ(pj);
			(*pj)->j = json_create(j, SUB, PAIR);
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				err(255, "calloc()");
			}
			(*f)->offset = p->offset;
			(*f)->err = '{';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '{';
			(*f)->do_it = &pair;
			(*f)->key = 1;
			(*f)->value = 0;
			p->buf++;
			p->offset++;
			break;
		case '[':
			/*pj = *j;*/
			if((*f)->c_end != ',' && (*f)->c_end != ':'){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected '[' at offset %lu.", p->offset);
			}
			NEW_PJ(pj);
			(*pj)->j = json_create(j, SUB, ARRAY);
			if(((*f)->next = calloc(1, sizeof(struct fn))) == NULL){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				err(255, "calloc()");
			}
			(*f)->offset = p->offset;
			(*f)->err = '[';
			(*f)->next->prev = *f;
			*f = (*f)->next;
			(*f)->c_end = '[';
			(*f)->do_it = &array;
			(*f)->key = 1;
			(*f)->value = 0;
			p->buf++;
			p->offset++;
			break;
		case ',':
			json_create(j, NEXT, PAIR);
			if((*f)->c_end != 0 && (*f)->c_end != ']' && (*f)->c_end != '}'){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected ',' at offset %lu.", p->offset);
			}
			(*f)->offset = p->offset;
			(*f)->err = ',';
			(*f)->c_end = ',';
			(*f)->key = 1;
			p->offset++;
			p->buf++;
			break;
		case '}':
			if(((*f)->c_end != 0 && (*f)->c_end != ']' && (*f)->c_end != '{') || (*f)->value){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			*f = (*f)->prev;
			free((*f)->next);
			(*f)->next = NULL;
			(*f)->key = 0;
			(*f)->value = 0;
			*j = (*pj)->j;
			DEL_PJ(pj);
			(*f)->c_end = '}';
			p->offset++;
			p->buf++;
			return p;
		case ']':
			warnx("Unexpected ']' at offset %lu.", p->offset);
			DESTROY_fn((*pj), lst);
			DESTROY_fn((*f), rf);
			DESTROY_ALL(j, p);
			_exit(255);
		case 0:
			break;
		default:
			if((*f)->value != 2 || (*f)->key == 1){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter at offset %lu.", p->offset);
			}
			getint(p);
			if(((c_char = *(p->buf)) < '0' || c_char > '9'|| c_char == '+' || c_char == '-') &&
				(!p->stock || *(p->pstock) != 0)){
				DESTROY_fn((*pj), lst);
				DESTROY_fn((*f), rf);
				DESTROY_ALL(j, p);
				errx(255, "Unexpected chararcter (bad number) near offset %lu: %c", p->offset, c_char);
			}
			(*f)->value = 0;
			(*f)->key = 1;
			(*f)->c_end = 0;
			c_char = *p->stock;
			if(c_char == '+' || *(p->stock + (c_char == '+' || c_char == '-')) == '.' || *(p->pstock -1) == '.'){
				warnx("Invalid number before offset: %lu",p->offset -1);
				(*j)->t_val = INT | WARN;
			}else
				(*j)->t_val = INT;
			(*j)->value.value = p->stock;
			p->stock = NULL;
			p->stock_size = 0;
			break;
	}
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
				if(!pj->value.name.key)
					continue;
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
	if(space == 0)
		putchar('\n');
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
				if(pj->value.value && strcmp(pj->value.value, "") != 0)
					free(pj->value.value);
				break;
			case PAIR:
				if(pj->value.name.key && strcmp(pj->value.name.key, "") != 0)
					free(pj->value.name.key);
				if(pj->value.value && strcmp(pj->value.value, "") != 0){
					free(pj->value.value);
				}
				break;
			}
		ppj = pj->next;
		free(pj);
		pj = ppj;
	}
}
