CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
SANITIZE = -g -fsanitize=address,undefined

ipv4: src/main.cpp src/ipv4.cpp src/ipv4.hpp
	$(CXX) $(CXXFLAGS) -o $@ src/main.cpp src/ipv4.cpp

test_runner: tests/test_ipv4.cpp src/ipv4.cpp src/ipv4.hpp
	$(CXX) $(CXXFLAGS) $(SANITIZE) -Isrc -o $@ tests/test_ipv4.cpp src/ipv4.cpp

# Unit tests, then compare the program's output on the assignment's sample run.
test: test_runner ipv4
	./test_runner
	./ipv4 < tests/sample_input.txt | diff tests/sample_expected.txt -
	@echo "Sample run output matches."

clean:
	rm -rf ipv4 test_runner *.dSYM

.PHONY: test clean
