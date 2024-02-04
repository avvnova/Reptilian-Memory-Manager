GXX := g++
GXXFLAGS := -g
DEPS = MemoryManager.h


%.o: &.c -g $(DEPS)
	$(GXX) $(GXXFLAGS) -c -o $@ $<

mem_manager_make: MemoryManager.o
	$(GXX) $(GXXFLAGS) -c MemoryManager.cpp
	$ ar cr libMemoryManager.a MemoryManager.o
	$ rm MemoryManager.o

