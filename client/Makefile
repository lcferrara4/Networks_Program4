CXX=		gcc
CXXFLAGS=	-g
PROGRAMS=	chatclient

all:		chatclient

myftp:		chatclient.c
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(PROGRAMS)

