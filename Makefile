CFLAGS=-Wall -ggdb --std=gnu99

makepicture: makepicture.c font.c gif.c

sample: sample.c font.c gif.c

sample.gif: sample
	./sample

solved.gif: makepicture
	./makepicture

www: solved.gif
	cp solved.gif ~/www/

check: sample.gif
	diff sample.gif sample_1.gif
