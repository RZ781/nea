CXX = g++
RM = rm -f
CPPFLAGS = -Wall -Wextra -Wpedantic -std=c++17 -Iinclude -Ilibriscv/lib -Ilibriscv/build/lib $(shell pkg-config --cflags gtkmm-4.0)
LDFLAGS = -Llibriscv/build/lib -lriscv $(shell pkg-config --libs gtkmm-4.0)

SRCS=src/main.cpp src/example_cpu.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: vm

vm: $(OBJS)
	$(CXX) -o vm $(OBJS) $(LDFLAGS)

src/main.o: src/main.cpp include/headers.h.gch

src/example_cpu.o: src/example_cpu.cpp include/cpu.h

include/headers.h.gch: include/headers.h
	$(CXX) $(CPPFLAGS) include/headers.h -o include/headers.h.gch

clean:
	$(RM) $(OBJS) include/headers.h.gch

distclean: clean
	$(RM) vm

