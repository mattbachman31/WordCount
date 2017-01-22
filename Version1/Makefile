#Targets:
#  Default target: build project
#  clean:          remove all generated files.
#  submit:         build compress archive all project source files.

PROJECT = 	prj1

TARGET =	word-count

SRC_FILES = \
  main.c \
  word-count.h \
  word-count.c \
  word-store.h \
  word-store.c \
  Makefile \
  README

OBJ_FILES = \
  main.o \
  word-count.o \
  word-store.o

CFLAGS = -g -Wall -std=c11

$(TARGET):  	$(OBJ_FILES)
		$(CC) $(OBJ_FILES) -o $(TARGET)

clean:		
		rm -f $(OBJ_FILES) $(TARGET)


submit:
		tar -cvzf $(PROJECT).tar.gz $(SRC_FILES)