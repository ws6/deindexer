CC=gcc 
CFLAGS= -ggdb3  -Wall -std=gnu99 -DPOST_PROCESS=1 
LINKERS= -lz 
SOURCES= parse_file.c  read_cfg.c collision_handler.c  deindex.c  main.c
	
OBJECTS=${SOURCES:.c=.o}
Executable=deindexer
TEST_DIR=test
-include $(OBJS:.o=.d)


all: ${Executable}

$(Executable): ${OBJECTS}
	${CC} -o ${Executable} ${OBJECTS} ${CFLAGS} ${LINKERS}
	@rm -f  *.d
tests: ${TEST_DIR}  gitax_class
$(TEST_DIR):
	test -d ${TEST_DIR} || mkdir -p ${TEST_DIR}
%.o: %.c
	${CC} -c $(CFLAGS) $(D)  $*.c -o $*.o
parse_file: parse_file.h parse_file.c
	${CC}  ${CFLAGS} ${LINKERS}   parse_file.c  -DPARSE_FILE_MAIN=1          -o ${TEST_DIR}/parse_file
gitax_class: parse_file.h parse_file.c  gitax_class.c gitax_class.h
	${CC}  ${CFLAGS} ${LINKERS}   parse_file.c gitax_class.c  -D_GITAX_CLASS_MAIN=1         -o ${TEST_DIR}/gitax_class

gitax_class.c: gitax_class.h
	
clean:
	rm -f ${Executable}  *.o  *.d  *~  a.out
	@#rm -rf ${TEST_DIR}
