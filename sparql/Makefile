
CXXFLAGS += -I/usr/include/raptor2 -I/usr/include/rasqal -g

all: sparql

install:
	sudo cp sparql /usr/local/bin/
#	sudo cp sparql.service /usr/lib/systemd/system/sparql.service
	sudo cp sparql@.service /usr/lib/systemd/system/sparql@.service
	sudo cp sparql.socket /usr/lib/systemd/system/sparql.socket
	sudo systemctl daemon-reload

sparql: sparql.o
	${CXX} ${CXXFLAGS} sparql.o -o sparql -lrdf -lmicrohttpd

