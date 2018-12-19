CXXFLAGS = -std=c++14 -Wall -fsanitize=address,undefined,leak -fuse-ld=gold

all: design_patterns

test:
	./design_patterns

clean:
	rm -f design_patterns
