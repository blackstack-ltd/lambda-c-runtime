CC=gcc
AR=ar
CFLAGS=-O3 -I.

%.o: %.c %.h Makefile
	$(CC) -c $(CFLAGS) -o $@ $<

libclambda.a: cJSON.o http.o clambda.o
	$(AR) rcs $@ $^

bootstrap: bootstrap.o libclambda.a
	$(CC) -o $@ $^ -L. -lcurl -lclambda

install: bootstrap
	cp $^ ../lambda_runtime

all: install

clean:
	rm *.o libclambda.a bootstrap
