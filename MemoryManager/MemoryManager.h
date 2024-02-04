#include <stdlib.h>
#include <functional>
#include <list>
#include <vector>
#include <map>
#include<stdint.h>
#include <string>

class MemoryManager
{
    unsigned short word_size;
    unsigned short word_count;
    unsigned int byte_count;
    bool init = false;
    uint16_t* hole_list;
    uint8_t* bit_map;
    std::vector<uint8_t> mem;
    std::function<int(int,void*)> curr_allocator; //might have to be a function pointer 
    std::list<std::pair<uint16_t,uint16_t>> holes; // <offset(words), size(words)>
    std::list<std::pair<uint16_t,uint16_t>> blocks;
    
    bool checkInit();
    std::string makeEntry(uint16_t off, uint16_t len);
    template <typename Iter>
    void combine_holes(Iter& it);
    template <typename Iter>
    bool combine_prev(Iter& it);
    template <typename Iter>
    bool combine_next(Iter& it);
    template <typename Iter, typename Cont>
    bool is_last(Iter iter, const Cont& cont);
    
    public:
        // Constructor; sets native word size (in bytes, for alignment) and default allocator for finding a memory hole.
        MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
        // Releases all memory allocated by this object without leaking memory.
        ~MemoryManager();
        // Instantiates block of requested size, no larger than 65536 words; cleans up previous block if applicable.
        void initialize(size_t sizeInWords); 
        // Releases memory block acquired during initialization, if any. 
        void shutdown();
        // Allocates a memory using the allocator function. 
        void *allocate(size_t sizeInBytes);
        // Frees the memory block within the memory manager so that it can be reused.
        void free(void *address);
        // Changes the allocation algorithm to identifying the memory hole to use for allocation.
        void setAllocator(std::function<int(int, void *)> allocator);
        // Uses standard POSIX calls to write hole list to filename as text, returning -1 on error and 0 if successful.
        int dumpMemoryMap(char *filename);
        // Returns an array of information (in decimal) about holes for use by the allocator function (little-Endian).
        void *getList();
        // Returns a bit-stream of bits in terms of an array representing whether words are used (1) or free (0). The
        void *getBitmap();
        // Returns the word size used for alignment.
        unsigned getWordSize();
        // Returns the byte-wise memory address of the beginning of the memory block.
        void *getMemoryStart();
        // Returns the byte limit of the current memory block.
        unsigned getMemoryLimit();
    // more functions can go here
        void print();
};

// Returns word offset of hole selected by the best fit memory allocation algorithm, and -1 if there is no fit.
int bestFit(int sizeInWords, void *list);
// Returns word offset of hole selected by the worst fit memory allocation algorithm, and -1 if there is no fit.
int worstFit(int sizeInWords, void *list);
