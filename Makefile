CXXFLAGS = -std=c++14 -Wall -fsanitize=address,undefined

all: design_patterns

test:
	./design_patterns
