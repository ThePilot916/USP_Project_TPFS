

tpfs: tpfs.o tpfs_helper.o
	gcc -Wall -w tpfs.o tpfs_helper.o `pkg-config fuse --cflags --libs` -o all

tpfs_helper.o: tpfs_helper.c tpfs.h
	gcc -Wall -c -w tpfs_helper.c `pkg-config fuse --cflags --libs` 	


tpfs.o: tpfs.c tpfs.h
	gcc -Wall -c -w tpfs.c `pkg-config fuse --cflags --libs`



clean:
	rm *.o
