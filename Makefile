CFLAGS=-Wall -ggdb --std=gnu99

makepicture: makepicture.c font.c gif.c

solved.gif: makepicture
	./makepicture

www: solved.gif
	cp solved.gif ~/www/

solved.txt: solved.gif
	xxd -c1 -b -s43 solved.gif > solved.txt
