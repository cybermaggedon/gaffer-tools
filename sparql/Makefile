
CXXFLAGS += -I/usr/include/raptor2 -I/usr/include/rasqal

all: sparql

install:
	sudo cp sparql /usr/local/bin/
	sudo cp sparql.service /usr/lib/systemd/system/sparql.service

sparql: sparql.o
	${CXX} ${CXXFLAGS} sparql.o -o sparql -lrdf -lmicrohttpd

