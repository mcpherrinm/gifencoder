CFLAGS=-Wall -ggdb

makepicture: makepicture.c

solved.gif: makepicture
	./makepicture

solved.txt: solved.gif
	xxd -c1 -b -s43 solved.gif > solved.txt
