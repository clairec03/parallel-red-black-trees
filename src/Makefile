# Compiler and flags
CXX = g++
CXXFLAGS = -O3

# Target for sequential
sequential: red-black-sequential-test.cpp red-black-sequential.h red-black-sequential.cpp
	$(CXX) $(CXXFLAGS) -o red-black-sequential red-black-sequential-test.cpp red-black-sequential.h red-black-sequential.cpp

# Target for parallel
parallel: red-black-lock-free-test.cpp red-black-lock-free.h red-black-lock-free.cpp
	$(CXX) $(CXXFLAGS) -o red-black-parallel red-black-lock-free-test.cpp red-black-lock-free.h red-black-lock-free.cpp

# Clean target
clean:
	rm -f red-black-sequential
	rm -f red-black-parallel
