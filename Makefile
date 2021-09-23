NAME=sfs
COMMON_FLAGS=-Wall -Wextra -Wpedantic -Wconversion -Wcast-qual -Wwrite-strings -Werror
LDFLAGS=-lncurses
RFLAGS=-O2 -g -flto $(COMMON_FLAGS)
DFLAGS=-g -fanalyzer $(COMMON_FLAGS)

SRC = $(wildcard src/*.cpp)
HDR = $(wildcard src/*.hpp)

ROBJ = $(SRC:src/%.cpp=release/obj/%.o)
DOBJ = $(SRC:src/%.cpp=debug/obj/%.o)


release: release/$(NAME)

release/$(NAME): $(ROBJ)
	$(CXX) $(RFLAGS) $(LDFLAGS) $^ -o $@

release/obj/%.o: src/%.cpp | release/obj
	$(CXX) $(RFLAGS) $^ -c -o $@

release/obj:
	mkdir -p $@


debug: debug/$(NAME)

debug/$(NAME): $(DOBJ)
	$(CXX) $(DFLAGS) $(LDFLAGS) $^ -o $@

debug/obj/%.o: src/%.cpp | debug/obj
	$(CXX) $(DFLAGS) $^ -c -o $@

debug/obj:
	mkdir -p $@


format: $(SRC) $(HDR)
	clang-format -i --style=file $^

clean:
	rm -rf $(NAME) release debug

install: release
	cp -f release/$(NAME) /usr/local/bin/$(NAME)

uninstall:
	rm -f /usr/local/bin/$(NAME)

.PHONY: release debug format clean install uninstall
