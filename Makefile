# Compiler settings
CXX = g++
CXXFLAGS = -Wall -Wextra -O2

# Source files
SERVER_SRC = server.cpp
CLIENT_SRC = client.cpp

# Executable names
SERVER_EXE = server
CLIENT_EXE = client

# Default target
all: $(SERVER_EXE) $(CLIENT_EXE)

# Compile the server executable
$(SERVER_EXE): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $(SERVER_SRC) -o $(SERVER_EXE)

# Compile the client executable
$(CLIENT_EXE): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $(CLIENT_SRC) -o $(CLIENT_EXE)

# Clean up the generated files
clean:
	rm -f $(SERVER_EXE) $(CLIENT_EXE)

# Phony targets
.PHONY: all clean