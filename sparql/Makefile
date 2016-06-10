
CXXFLAGS += -I/usr/include/raptor2 -I/usr/include/rasqal

sparql: sparql.o
	${CXX} ${CXXFLAGS} sparql.o -o sparql -lrdf -lmicrohttpd

