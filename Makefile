CC=gcc
OPTIONS=-Wall -Wpadded -g
all:
	$(CC) $(OPTIONS) json2txt.c -o json2txt

clean:
	rm -v json2txt
