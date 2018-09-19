EXENAME = main
OBJS = main.o vision.o

CXX = g++
CXXFLAGS = -c -g -O0 -Wall -Wextra -pedantic
LD = g++
LDFLAGS = -lpng -lpthread -lm
OPENCV = $(shell pkg-config --libs --static opencv)

all : $(EXENAME)

$(EXENAME) : $(OBJS)
	$(LD) $(OPENCV) $(OBJS) $(LDFLAGS) -o $(EXENAME)

main.o : main.cpp include/vision.h 
	$(CXX) $(CXXFLAGS) main.cpp

vision.o : include/vision.cpp include/vision.h
	$(CXX) $(CXXFLAGS) include/vision.cpp

test : tests.o vision.o 
	$(LD) tests.o $(LDFLAGS) -o test

tests.o : tests/tests.cpp include/vision.h
	$(CXX) $(CXXFLAGS) tests/tests.cpp

clean :
	-rm -f *.o $(EXENAME) test
