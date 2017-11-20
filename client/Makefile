CXX=		gcc
CXXFLAGS=	-g
PROGRAMS=	chatclient

all:		chatclient

myftp:		chatclient.c
	$(CXX) $(CXXFLAGS) -o $@ $^ -lpthread

clean:
	rm -f $(PROGRAMS)

