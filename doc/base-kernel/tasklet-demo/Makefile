CXXFLAGS += -Wall -std=c++11
LDFLAGS += -pthread

all: clean main

main: main.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o main main.o

clean:
	rm -f main.o main
