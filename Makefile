CXXFLAGS = -Wall -Wextra -std=c++11 -g
CXX = clang++
CC = gcc

all: convolution

bmp.o: bmp.hpp bmp.cpp
	$(CXX) -c bmp.cpp -g -Wall -Wextra

convolution: convolution.o bmp.o filter_factory.o filters.o
	$(CXX) -o convolution convolution.o bmp.o filter_factory.o filters.o -lOpenCL -lboost_program_options -lboost_timer -lboost_system

convolution.o: convolution.cpp bmp.hpp filters.hpp
	$(CXX) -c convolution.cpp $(CXXFLAGS)

bmpfuncs.o: bmpfuncs.cpp bmpfuncs.hpp
	$(CXX) -c bmpfuncs.cpp $(CXXFLAGS)

filters.o: filters.hpp filters.cpp
	$(CXX) -c filters.cpp $(CXXFLAGS)

filter_factory.o: filter_factory.cpp filter_factory.hpp filters.hpp
	$(CXX) -c filter_factory.cpp $(CXXFLAGS)

clean:
	rm *.o convolution
