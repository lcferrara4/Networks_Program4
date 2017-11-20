CXX=		gcc
CXXFLAGS=	-g
LDFLAGS=	-pthread
PROGRAMS=	chatclient

all:		chatclient

myftpd:		chatclient.c
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(PROGRAMS)
