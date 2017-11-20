CXX=		gcc
CXXFLAGS=	-g -pthread
PROGRAMS=	chatserver

all:		chatserver

myftpd:		chatserver.c
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAMS)
