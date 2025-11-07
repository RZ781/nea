CXX = g++
RM = rm -f
CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -Iinclude $(shell pkg-config --cflags gtkmm-4.0)
LDFLAGS = $(shell pkg-config --libs gtkmm-4.0)

SRCS=src/main.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: vm

vm: $(OBJS)
	$(CXX) -o vm $(OBJS) $(LDFLAGS)

src/main.o: src/main.cpp

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) vm
