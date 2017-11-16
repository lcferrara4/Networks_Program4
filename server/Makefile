CXX=		gcc
CXXFLAGS=	-g -lpthread
PROGRAMS=	chatserver

all:		chatserver

myftpd:		chatserver.c
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAMS)
