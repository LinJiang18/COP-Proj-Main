CFLAGS = -g -std=c++11 -Wall
TARGETS = server client
all: $(TARGETS)

client : client.cpp endpoint.cpp message.cpp logging.cpp endpoint_list.cpp gomoku.cpp
	g++ $^ -o $@ $(CFLAGS) -pthread
server : server.cpp endpoint.cpp message.cpp logging.cpp endpoint_list.cpp
	g++ $^ -o $@ $(CFLAGS)
gomoku : gomoku.cpp
	g++ $^ -o $@ $(CFLAGS)

clean: 
	rm -f $(TARGETS) *.o
.PHONY: clean
