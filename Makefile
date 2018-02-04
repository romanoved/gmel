#!/usr/bin/make -f
# clang-format -i -style="{BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 80}" *

CFLAGS += -fPIC -Wall
LDFLAGS += -ldl
all: test;


SO := gmel.so gmel_py2.so gmel_py3.so
DEPS :=  bind util_hashtable util_common util_vector util_strview
OBJS := $(addprefix build/,$(addsuffix .o,$(DEPS)))

build: $(SO)


gmel_py2.so: build/gmel_py2.o $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^ $$(python2-config --ldflags)

gmel_py3.so: build/gmel_py3.o $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^ $$(python3-config --ldflags)

build/gmel_py2.o: srcs/gmel_py.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $< $$(python2-config --cflags)

build/gmel_py3.o: srcs/gmel_py.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $< $$(python3-config --cflags)

%.so: build/%.o $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

build/%.o: srcs/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

test: build
	GMEL_USE_PY2=1 ./test.mk
	./test.mk

clean:
	-rm -rf $(SO) build

install: build
	mkdir -p $(DESTDIR)/usr/lib $(DESTDIR)/usr/include
	cp $(SO) $(DESTDIR)/usr/lib/
	cp gmel gmel_py2 gmel_py3 $(DESTDIR)/usr/include/
