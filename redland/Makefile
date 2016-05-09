
CFLAGS=-I/usr/include/raptor2 -I/usr/include/rasqal -fPIC -I. -Iredland -g
CXXFLAGS=-I/usr/include/raptor2 -I/usr/include/rasqal -fPIC -I. -Iredland -g
LIBS=-lrasqal -lrdf

SQLITE_FLAGS=-DSTORE=\"sqlite\" -DSTORE_NAME=\"STORE.db\"

GAFFER_FLAGS=-DSTORE=\"gaffer\" -DSTORE_NAME=\"STORE.db\"

all: test-sqlite test-gaffer librdf_storage_gaffer.so

test-sqlite: test-sqlite.o
	${CXX} ${CXXFLAGS} test-sqlite.o -o $@ ${LIBS}

test-gaffer: test-gaffer.o
	${CXX} ${CXXFLAGS} test-gaffer.o -o $@ ${LIBS}

test-sqlite.o: test.C
	${CXX} ${CXXFLAGS} -c $< -o $@  ${SQLITE_FLAGS}

test-gaffer.o: test.C
	${CXX} ${CXXFLAGS} -c $< -o $@ ${GAFFER_FLAGS}

test-sqlite.o test-gaffer.o gaffer.o gaffer_comms.o: gaffer_comms.h

librdf_storage_gaffer.so: gaffer.o gaffer_comms.o
	${CXX} ${CXXFLAGS} -shared -o $@ gaffer.o gaffer_comms.o -lsqlite3 \
		-ljson-c -lrdf

gaffer.o: CFLAGS += -DHAVE_CONFIG_H -DLIBRDF_INTERNAL=1 

install: all
	sudo cp librdf_storage_gaffer.so /usr/lib64/redland
