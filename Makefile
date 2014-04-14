interpolate:	main.o wav.o send.o recv.o dct.o
	gcc -g -o interpolate main.o wav.o send.o recv.o dct.o

main.o:	wav.h interpolate.h config.h main.c
	gcc -g -c main.c

wav.o:	wav.h interpolate.h config.h wav.c
	gcc -g -c wav.c
send.o:	wav.h interpolate.h config.h send.c
	gcc -g -c send.c
recv.o:	wav.h interpolate.h config.h recv.c
	gcc -g -c recv.c
dct.o:	wav.h interpolate.h config.h dct.c
	gcc -g -c dct.c
	