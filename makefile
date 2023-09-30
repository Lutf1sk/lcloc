OUT = lcloc

OBJS = \
	src/main.o

LT_PATH = lt/
LT_LIB = $(LT_PATH)/bin/lt.a

DEPS = $(patsubst %.o,%.deps,$(OBJS))

CC = cc
CC_FLAGS += -O2 -fmax-errors=3 -I$(LT_PATH)/include/ -std=c11 -Wall -Werror -Wno-strict-aliasing -Wno-error=unused-variable -Wno-unused-function -Wno-pedantic

LNK = cc
LNK_FLAGS += -o $(OUT)
LNK_LIBS += -lpthread -ldl -lm -lasound

all: $(OUT)

install: all
	cp $(OUT) /usr/local/bin/

$(LT_LIB):
	make -C $(LT_PATH)

$(OUT):	$(LT_LIB) $(OBJS)
	$(LNK) $(LNK_FLAGS) $(OBJS) $(LT_LIB) $(LNK_LIBS)

%.o: %.c makefile
	$(CC) $(CC_FLAGS) -MM -MT $@ -MF $(patsubst %.o,%.deps,$@) $<
	$(CC) $(CC_FLAGS) -c $< -o $@

-include $(DEPS)

clean:
	-rm $(OUT) $(OBJS) $(DEPS)

.PHONY: all install clean
