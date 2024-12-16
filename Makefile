CXX = g++
CXXFLAGS = -Wall -Wextra -O2

# Object files
OBJS = common.o client.o server.o

# Executables
TARGETS = client server

all: $(TARGETS)

# Linking the client and server
client: client.o common.o
	$(CXX) $(CXXFLAGS) -o $@ $^

server: server.o common.o
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compiling source files
client.o: client.cpp common.h
	$(CXX) $(CXXFLAGS) -c $<

server.o: server.cpp common.h
	$(CXX) $(CXXFLAGS) -c $<

common.o: common.cpp common.h
	$(CXX) $(CXXFLAGS) -c $<

# Clean up
clean:
	rm -f $(TARGETS) $(OBJS)

.PHONY: all clean