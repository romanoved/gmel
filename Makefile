#!/usr/bin/make -f

CFLAGS += -fPIC -Wall
all: test;


SO := gmel.so gmel_py2.so
DEPS :=  util_hashtable popen util util_common time_fmt bind util_vector util_strview
OBJS := $(addprefix build/,$(addsuffix .o,$(DEPS)))

build: $(SO)

gmel_py2.so: build/gmel_py2.o $(OBJS)
	$(CC) -shared  -Wno-write-strings -o $@ $^ $$(python2-config --ldflags)

%.so: build/%.o $(OBJS)
	$(CC) -shared  -Wno-write-strings -o $@ $^

build/gmel_py2.o: srcs/gmel_py2.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $< $$(python2-config --cflags)

build/%.o: srcs/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

test: build
	./test.mk

clean:
	-rm -rf $(SO) build

install: build
	mkdir -p $(DESTDIR)/usr/lib $(DESTDIR)/usr/include
	cp gmel.so gmel_py2.so $(DESTDIR)/usr/lib/
	cp gmel gmel_py2 $(DESTDIR)/usr/include/
