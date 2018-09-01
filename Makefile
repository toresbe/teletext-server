BOOST = /usr/include/boost

CC = clang
CFLAGS = -g -O2
CPPFLAGS = -c -std=c++11 -DBOOST_LOG_DYN_LINK
LIBS = -lstdc++ -lboost_log -pthread -lboost_filesystem -lboost_system
INCLUDE = .

EXEC = ttxserver

SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

$(EXEC): $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(DFLAGS) $(LIBS) -o $(EXEC)

%.o: %.cpp
	$(CC) $(CFLAGS) $(DFLAGS) $(CPPFLAGS) -I $(INCLUDE) -I ${BOOST} $< -o $@

clean:
	rm -f $(EXEC) $(OBJECTS)
