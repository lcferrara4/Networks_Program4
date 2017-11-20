CXX=		gcc
CXXFLAGS=	-g
LDFLAGS=	-pthread
PROGRAMS=	chatserver

all:		chatserver

myftpd:		chatserver.c
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PROGRAMS)
