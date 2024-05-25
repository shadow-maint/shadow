all: test_nss libsubid_zzz.so

BASE_TEST_DIR ?= $(shell git rev-parse --show-toplevel)
basedir := $(BASE_TEST_DIR)

test_nss: test_nss.c $(basedir)/lib/nss.c
	gcc -c -I$(basedir)/lib/ -I$(basedir)-o test_nss.o test_nss.c
	gcc -o test_nss test_nss.o $(basedir)/lib/.libs/libshadow.a -ldl

libsubid_zzz.so: libsubid_zzz.c
	gcc -c -I$(basedir)/lib/ -I$(basedir) -I$(basedir)/libsubid libsubid_zzz.c
	gcc -L$(basedir)/libsubid -shared -o libsubid_zzz.so libsubid_zzz.o $(basedir)/lib/.libs/libshadow.a -ldl

clean:
	rm -f *.o *.so test_nss
