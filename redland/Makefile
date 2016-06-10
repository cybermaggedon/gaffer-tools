
CFLAGS=-I/usr/include/raptor2 -I/usr/include/rasqal -fPIC -I. -Iredland -g
CXXFLAGS=-I/usr/include/raptor2 -I/usr/include/rasqal -fPIC -I. -Iredland -g
LIBS=-lrasqal -lrdf

SQLITE_FLAGS=-DSTORE=\"sqlite\" -DSTORE_NAME=\"STORE.db\"

GAFFER_FLAGS=-DSTORE=\"gaffer\" -DSTORE_NAME=\"http://localhost:8080/example-rest/v1\"

all: test-sqlite test-gaffer librdf_storage_gaffer.so

test-sqlite: test-sqlite.o
	${CXX} ${CXXFLAGS} test-sqlite.o -o $@ ${LIBS}

test-gaffer: test-gaffer.o
	${CXX} ${CXXFLAGS} test-gaffer.o -o $@ ${LIBS}

test-sqlite.o: test.C
	${CXX} ${CXXFLAGS} -c $< -o $@  ${SQLITE_FLAGS}

test-gaffer.o: test.C
	${CXX} ${CXXFLAGS} -c $< -o $@ ${GAFFER_FLAGS}

mytest: mytest.o gaffer_comms.o gaffer_query.o
	${CC} ${CFLAGS} mytest.o gaffer_comms.o gaffer_query.o \
		-ljson-c -lcurl -o $@

GAFFER_OBJECTS=gaffer.o gaffer_comms.o gaffer_query.o

librdf_storage_gaffer.so: ${GAFFER_OBJECTS}
	${CXX} ${CXXFLAGS} -shared -o $@ ${GAFFER_OBJECTS} -lsqlite3 \
		-ljson-c -lrdf

gaffer.o: CFLAGS += -DHAVE_CONFIG_H -DLIBRDF_INTERNAL=1 

install: all
	sudo cp librdf_storage_gaffer.so /usr/lib64/redland

depend:
	makedepend -Y -I. *.c *.C

# DO NOT DELETE

gaffer.o: ./gaffer_comms.h ./gaffer_query.h
gaffer_comms.o: ./gaffer_comms.h ./gaffer_query.h
gaffer-comms-test.o: ./gaffer_comms.h ./gaffer_query.h
gaffer_query.o: ./gaffer_query.h
mytest.o: ./gaffer_comms.h ./gaffer_query.h
