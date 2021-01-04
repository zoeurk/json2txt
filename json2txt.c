#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFFERLEN 65535
#define ALLOC 4096

enum TYPE{
	NONE = 0,
	LIST = 1,
	VALUE = 2,
	ARRAY = 4,
	KEY = 8,
	UNKNOW = 16,
	STR = 32,
};
enum ARGS{
	JSON = 1,
	TXT = 2
};
struct arguments{
	int args;
	int ___;
	char *filename;
};
struct json{
	size_t type;
	union{
		char *name;
		struct {
			char *key;
			char *value;
		};
	};
	struct json *next;
	struct json *prev;
	struct json *sub;
	struct json *up;
};
void print_space(unsigned long int len){
	unsigned long int i;
	for(i = 0; i < len; i++){
		printf(" ");
	}
}
void *___calloc___(unsigned long int nbr, unsigned long int size){
	void *ptr;
	if((ptr = calloc(nbr, size)) == NULL){
		perror("calloc()");
		exit(EXIT_FAILURE);
	}
	return ptr;
}
struct json *add_to_struct_json(struct json **pj, char *type){
	(*pj)->sub = ___calloc___(1, sizeof(struct json));
	(*pj)->sub->up = *pj;
	(*pj) = (*pj)->sub;
	(*pj)->type = ((*type&ARRAY) == ARRAY) ? *type :LIST;
	return *pj;
}
struct json *add_json_entry(struct json **pj){
	(*pj)->next = ___calloc___(1, sizeof(struct json));
	(*pj)->next->prev = (*pj);
	(*pj) = (*pj)->next;
	return *pj;
}
void destroy_json(struct json **j){
	struct json *pj = *j, *ppj;
	while(pj){
		if(pj->sub)
			destroy_json(&pj->sub);
		if(pj->key)
			free(pj->key);

		if(pj->value)
			free(pj->value);
		ppj = pj->next;
		free(pj);
		pj = ppj;
	}
}
struct json *to_json(int fd){
	struct json *j = NULL, *pj = NULL, *ppj;
	char buffer[BUFFERLEN],tampon[ALLOC],___tampon___[ALLOC], *pbuf = buffer, type = 0, quote = 0, quoted = 0, erreur = 0, virgule = 0;
	long int r, i;
	unsigned long int bufsize = BUFFERLEN, len = 0, array = 0, tamp = 0;
	memset(buffer, 0, BUFFERLEN);
	memset(tampon, 0, ALLOC);
	while((r = (long int)read(fd,pbuf,bufsize)) > 0)
	{
		for(i = 0 ,pbuf = buffer; i < r; i++,pbuf++){
			if(erreur == 1 && *pbuf == '/')
				erreur = 2;
			else	if(erreur == 1){
					fprintf(stderr, "Erreur de syntax dans le commentaire vers: %s\n", tampon);
					destroy_json(&j);
					exit(EXIT_FAILURE);
				}
			if(erreur == 2)
				goto end;
			if(*pbuf != '"' && quote == 1){
				goto character;
			}
			if(quote == 0 && (*pbuf == ' ' || *pbuf == '\t' || *pbuf == '\n'))
				continue;
			switch(*pbuf){
				case ',':
					if(tampon[0] != 0){
						if(!pj->name){
							pj->name = ___calloc___(1, strlen(tampon) + 1);
							strcpy(pj->name, tampon);
							strcpy(___tampon___, tampon);
							memset(tampon, 0, ALLOC);
							tamp = 0;
						}else{
							if((pj->type&UNKNOW) == UNKNOW){
								pj->type -= UNKNOW;
								pj->type |= VALUE;
								pj->value = ___calloc___(1, strlen(tampon) + 1);
								strcpy(pj->value, tampon);
								strcpy(___tampon___, tampon);
								memset(tampon, 0, ALLOC);
								tamp = 0;
							}

						}
						virgule = 1;
					}
					pj = add_json_entry(&pj);
					break;
				case '[':
					array++;
					type = ARRAY;
				case '{':
					len++;
					if(j == NULL){
						pj =  j = calloc(1,sizeof(struct json));
						pj->type = (type == ARRAY) ? type : LIST;
					}else
						pj = add_to_struct_json(&pj,&type);
					type = 0;
					virgule = 0;
					break;
				case ']':
					array--;
					type = ARRAY;
				case '}':
					if(tampon[0] != 0){
						if(pj->key == NULL){
							pj->key = ___calloc___(1, strlen(tampon) +1);
							strcpy(pj->key, tampon);
						}else{
							pj->value = ___calloc___(1, strlen(tampon) +1);
							strcpy(pj->value, tampon);
						}
						virgule = 0;
					}
					if(virgule != 0){
						printf("Erreur de syntax trop de ',' vers: %s\n", ___tampon___);
						exit(EXIT_FAILURE);
					}
					memset(___tampon___, 0, ALLOC);
					len--;
					type = (type == ARRAY) ? type : LIST;
					while(pj->prev)
						pj = pj->prev;
					if(pj->up)
						pj = pj->up;
					memset(tampon , 0, ALLOC);
					tamp = 0;
					type = 0;
					break;
				case '"':
					quote = !quote;
					quoted = 1;
					ppj = pj;
					while(ppj->prev)
						ppj = ppj->prev;
					if((ppj->type&ARRAY) == ARRAY){
						if(quote)
							pj->type |= STR;
					}else{
						if(pj->key && quote)
							pj->type |= STR;
					}
					break;
				case ':':
					pj->type |= (KEY|UNKNOW);
					if(pj->key){
						fprintf(stderr, "Erreur de syntax vers: %s\n", tampon);
						destroy_json(&j);
						exit(EXIT_FAILURE);
					}
					if(quoted == 0){
						 fprintf(stderr, "Erreur de syntax vers: %s\n", tampon);
						 destroy_json(&j);
						 exit(EXIT_FAILURE);
					}
					pj->key = ___calloc___(1, strlen(tampon) + 1);
					strcpy(pj->key, tampon);
					memset(tampon, 0, ALLOC);
					quoted = 0;
					tamp = 0;
					break;
				case '/':
					if(quote == 0)
						erreur = 1;
					break;
				default:
					character:
					if(tamp > 4094){
						fprintf(stderr, "Chaine de charactere trop longue: %s...\n", tampon);
						destroy_json(&j);
						exit(EXIT_FAILURE);
					}
					tampon[tamp] = *pbuf;
					tamp++;
					quoted = 0;
					break;
			}
			end:
			if(*pbuf == '\n')
				erreur = 0;
		}
		pbuf = buffer;
	}
	if(r < 0){
		perror("read()");
		exit(EXIT_FAILURE);
	}
	if(len != 0){
		if(array > 0)
			fprintf(stderr, "Trop de '[' ouverts\n");
		else
			if(array < 0)
				fprintf(stderr, "Trop de ']' fermées\n");
			else
				if(len > 0)
					fprintf(stderr, "Trop de '{' ouverts\n");
				else
					if(len < 0)
						fprintf(stderr, "Trop de '}' fermées\n");
					else	/*ne sera jamais vu :)*/
						fprintf(stderr, "Fichier JSON invalide\n");

		destroy_json(&j);
		exit(EXIT_FAILURE);
	}
	return j;
}
void json_to_string(struct json *j,char **string, unsigned long int string_len, unsigned long int *total){
	struct json *pj = j;
	unsigned long int len = 0;
	int inc = 0;
	while(pj){
 		if(pj->sub){
			if(pj->name){
				len = strlen(pj->name);
				if(*total < string_len + len + 2){
					if((*string = realloc(*string, string_len + len + 2)) == NULL){
						perror("realloc()");
						exit(EXIT_FAILURE);
					}
					*total = string_len + len + 2;
					if(string_len == 0)
						**string = 0;
					else{	inc = 1;
						strcat(*string, ".");
					}
					strcat(*string,pj->name);
				}else
					if(string_len > 0){
						inc = 1;
						strcat(*string, ".");
						strcat(*string,pj->name);
					}else{
						strcat((*string),pj->name);
					}
			}
			json_to_string(pj->sub, string, string_len + len + inc, total);
			if(string_len)
				(*string)[string_len] = 0;
			else	**string = 0;
		}else{
			if(pj->name){
				if(pj->value){
					if(*string && strlen(*string) > 0){
						if(**string == '.'){
							if((pj->type&STR) == STR)
								printf("%s.%s:\"%s\"\n",&(*string)[1], pj->name, pj->value);
							else	printf("%s.%s:%s\n",&(*string)[1], pj->name, pj->value);
						}else{	if((pj->type&STR) == STR)
								printf("%s.%s:\"%s\"\n",*string, pj->name, pj->value);
							else	printf("%s.%s:%s\n",*string, pj->name, pj->value);
						}
					}else{
						if((pj->type&STR) == STR)
							printf("%s:\"%s\"\n", pj->name, pj->value);
						else	printf("%s:%s\n", pj->name, pj->value);
					}
				}else{
					if(*string && strlen(*string) > 0){
						if(**string == '.'){
							if((pj->type&STR) == STR)
 	    							printf("%s:\"%s\"\n",&(*string)[1], pj->key);
							else	printf("%s:%s\n",&(*string)[1], pj->key);
						}else{
							if((pj->type&STR) == STR)
								printf("%s:\"%s\"\n",*string, pj->key);
							else	printf("%s:%s\n", *string, pj->key);
						}
					}
					
				}
			}else	if(*string && strlen(*string) > 0)
					printf("%s:\n", *string);
		}
		pj = pj->next;
	}
}
void json_print(struct json *j, unsigned long int space){
	struct json *pj = j;
	unsigned long int type = j->type;
	switch(type&(ARRAY|LIST)){
		case ARRAY:
			printf("[");
			while(pj){
				if(pj->name){
					if(pj->next)
						if((pj->type&STR) == STR)
							printf("\"%s\",", pj->name);
						else
							printf("%s,", pj->name);
					else	if((pj->type&STR) == STR)
							printf("\"%s\"", pj->name);
						else	printf("%s", pj->name);
				}
				if(pj->sub){
					json_print(pj->sub, space+1);
					if(pj->next){
						printf(",\n");
						print_space(space+1);
					}
				}
				 pj = pj->next;
			}
			printf("]");
			break;
		case LIST:
			printf("{\n");
			while(pj){
				if(pj->name){
					print_space(space);
					if(pj->value){
						if((pj->type&STR) == STR)
							printf(" \"%s\":\"%s\"", pj->name, pj->value);
						else	printf(" \"%s\":%s", pj->name, pj->value);
						if(pj->next)
							printf(",\n");
						else	printf("\n");
					}else	printf(" \"%s\":", pj->name);
				}
				if(pj->sub){
					json_print(pj->sub, space+1);
					if(pj->next)
						printf(",\n");
					else	printf("\n");
				}
				 pj = pj->next;
			}
			print_space(space);
			printf("}");	
			break;
	}
	if(space == 0)
		printf("\n");
}
void usage(void){
	printf("Usage: json [options] [filename|-]\n");
	printf("\t-J\tno json\n");
	printf("\t-T\tno text\n");
	printf("\t-[h|?]\tshow this message\n");
	printf("bugs: zoeurk@gmail.com\n");
	exit(EXIT_SUCCESS);
}
void parse_args(int argc, char **argv, struct arguments *args){
	int _argc_, file = 0;
	char **_argv_ =  argv, *a, *a_;
	for(	_argc_ = 1,
		_argv_ = argv;
		_argc_ < argc;
		_argc_++
	){
		for(a = &_argv_[_argc_][0]; file == 0 && *a != 0; a++)
		{	a_ = a;
			if(*a == '-')a = &a[1];
			switch(*a){
				case 'J':
					args->args |= JSON;
					break;
				case 'T':
					args->args |= TXT;
					break;
				case 'h':
				case '?':
					usage();
					break;
				case 0: if(file == 1){
						fprintf(stderr, "Mismatch arguments\n");
						fprintf(stderr, "Regardez l'usage en tapant -[?|h]\n");
						exit(EXIT_FAILURE);
					}
					file = 1;
					break;
				default:
					if(*a_ == '-'){
						fprintf(stderr,"L'option -%c est inconnue\n",*a);
						fprintf(stderr,"essayer %s -[?|h]\n",argv[0]);
						exit(EXIT_FAILURE);
					}else{
						if(file == 1){
							fprintf(stderr, "Mismatch arguments\n");
							fprintf(stderr, "Regardez l'usage en tapant -[?|h]\n");
							exit(EXIT_FAILURE);
						}
						file = 1;
						args->filename = a;
					}
					break;
			}
		}
	}
}
int main(int argc, char **argv){
	struct json *j = NULL;
	struct arguments args = {0, 0, NULL};
	char *string = NULL;
	int fd;
	unsigned long int string_len = 0, total = 0;
	parse_args(argc, argv, &args);
	if(args.filename){
		if((fd = open(args.filename, O_RDONLY)) < 0){
			perror("open()");
			exit(EXIT_FAILURE);	
		}
	}else
		fd = STDIN_FILENO;
	j = to_json(fd);
	if((args.args&JSON) == 0)json_print(j, 0);
	if((args.args&TXT) == 0)json_to_string(j, &string, string_len, &total);
	destroy_json(&j);
	if(string)
		free(string);
	if(fd != STDIN_FILENO)
		close(fd);
	return 0;
}
