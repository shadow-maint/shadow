all: test_nss test_status libsubid_zzz.so

BUILD_BASE_DIR ?= $(shell git rev-parse --show-toplevel)
basedir := $(BUILD_BASE_DIR)

test_nss: test_nss.c $(basedir)/lib/nss.c
	gcc -c -I$(basedir)/lib/ -I$(basedir) -o test_nss.o test_nss.c
	gcc -o test_nss test_nss.o $(basedir)/lib/.libs/libshadow.a -ldl

test_status: test_status.c
	gcc -I$(basedir)/libsubid -o test_status test_status.c -L$(basedir)/libsubid/.libs -lsubid

libsubid_zzz.so: libsubid_zzz.c
	gcc -c -I$(basedir)/lib/ -I$(basedir) -I$(basedir)/libsubid libsubid_zzz.c
	gcc -L$(basedir)/libsubid -shared -o libsubid_zzz.so libsubid_zzz.o $(basedir)/lib/.libs/libshadow.a -ldl

clean:
	rm -f *.o *.so test_nss test_status
