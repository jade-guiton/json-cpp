CXX = clang++
#CXX = g++

CXXFLAGS = -std=c++20 -Wall -Wextra
#CXXFLAGS += -stdlib=libstdc++ 
#LDLIBS = -lstdc++ -lm

CXXFLAGS += -O3
#CXXFLAGS += -O0 -g

#CXXFLAGS += -fsanitize=address
#LDFLAGS = -fsanitize=address

PYTHON = python3

HEADERS = $(wildcard *.hpp) $(wildcard *.tpp)

.PHONY: clean test

test: out/validator out/formatter
	$(PYTHON) test.py

clean:
	rm -rf out

out:
	mkdir -p out

out/%.o: %.cpp $(HEADERS) | out
	$(CXX) $(CXXFLAGS) -c -o $@ $<

out/validator: out/validator.o out/json.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

out/formatter: out/formatter.o out/json.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

