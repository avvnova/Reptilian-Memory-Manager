#include "MemoryManager.h"

#include <iostream>
#include <cmath>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_WORDS 65536

MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator){
    word_size = wordSize;
    curr_allocator = allocator; 
}
MemoryManager::~MemoryManager(){
    //std::cout << "In MemoryManager Destructor:)";
    if(!(mem.empty() && holes.empty() && blocks.empty()))
        shutdown();
}
void MemoryManager::initialize(size_t sizeInWords){
    if(sizeInWords > MAX_WORDS)
    {  
        std::cout << "\n!!! ERROR !!! : sizeInWords parameter value was greater than 65536. Please use a smaller word count!" << std::endl;
        return; 
    }
    if(!(mem.empty() && holes.empty() && blocks.empty()))
        shutdown();
    
    word_count = sizeInWords;
    //std::cout << "Num Words: " <<word_count << " | ";
    byte_count = word_count * word_size;
    //std::cout << "Mem limit " << byte_count <<std::endl;
    mem.resize(byte_count);
    holes.emplace_front(0, word_count);
    init = true;

}
void MemoryManager::shutdown(){
    /* This should only include memory created for long term use,
    * not those that returned such as getList() or getBitmap() 
    * as whatever is calling those should delete it instead.
    */
   //std::cout << "In Shutdown: Calling mem.clear() | ";
   init = false;
   mem.clear();
   mem.shrink_to_fit();
   //std::cout << "Calling holes.clear() | ";
   holes.clear();
   //std::cout << "Calling blocks.clear() | ";
   blocks.clear();

   //std::cout << "Success!" <<std::endl;
    // std::function allocator?
}
void* MemoryManager::allocate(size_t sizeInBytes){
    if(!checkInit()){
        exit;
    }
    if (sizeInBytes > byte_count){
        std::cout << "\n!!! ERROR !!! : sizeInBytes parameter exceeded max byte size in mem manager." << std::endl;
        return nullptr;
    }
    if (sizeInBytes <= 0){
        std::cout << "\n!!! ERROR !!! : sizeInBytes parameter invalid. Please ensure sizeInBytes is a positive integer." << std::endl;
        return nullptr;
    }
    std::list<std::pair<uint16_t, uint16_t>>::iterator it;
    uint8_t* ret;
    uint16_t word_count = ceil(sizeInBytes/(double)word_size);
    uint16_t* list = static_cast<uint16_t*>(getList());
    int loc = curr_allocator(word_count, list);   
    delete [] list;
    //print();
    //std::cout << "Allocating Now";
    if(loc == -1){
        std::cout << "No Space Available in the Memory Block." << std::endl;
        return nullptr;
    }
    else{
        for(it = holes.begin(); it != holes.end(); it++)
        {
            // Need to get address of memory block a right location and return it
            if (it->first == loc){
                if(it->second > word_count){
                    uint16_t first = it->first;
                    uint16_t second = it->second;
                    holes.emplace(it, first + word_count, second - word_count);
                    holes.erase(it);  
                }
                else if(it->second == word_count){
                    holes.erase(it);
                }
                else{
                    std::cout << "\n!!! ERROR !!! Hole given can not fit this memory block! Our allocators are wack.";
                }
            break;
            }
        }
        if(blocks.empty()){
            blocks.emplace(blocks.begin(),loc, word_count);
            ret = &mem[0];
            ////print();
            return ret;
        }
        else{   
            bool success = false; 
            for(it = blocks.begin(); it != blocks.end(); it++){   
                if(it->first >= loc){
                    blocks.emplace(it, loc, word_count);
                    success = true;
                    break;
                }
                else if(is_last(it, blocks)){
                    blocks.emplace_back(loc, word_count);
                    success = true;
                    break;
                }
            }
            if(success){
                ret = &mem[0] + (loc * word_size);
                //print();
                return ret;
            }
            else{
                std::cout << "\n!!! ERROR !!! Insertion failed. Check allocate()" <<std::endl;
                return nullptr;
            }
        }
    }
}
void MemoryManager::free(void *address){
    if(!checkInit()){
        exit;
    }
    std::list<std::pair<uint16_t, uint16_t>>::iterator it;
    uint8_t* add = static_cast<uint8_t*>(address);
    uint16_t offset = 0;
    uint16_t length = 0;
    
    int distance = add - &mem[0];
    offset = distance/(double)word_size;
    //std::cout << "Freeing block at offset " << offset;
    for(it = blocks.begin(); it != blocks.end(); it++){
        if (it->first == offset){
            length = it->second;
            blocks.erase(it);
            break;
        }
    }
    for(it = holes.begin(); it != holes.end(); it++){
        if(it->first >= offset){
            holes.emplace(it, offset, length);
            it = prev(it);
            break;
        }
        else if(is_last(it, holes)){
            holes.emplace_back(offset, length);
            break;
        }
    }
    combine_holes(it);
    //print();
}
void MemoryManager::setAllocator(std::function<int(int, void *)> allocator){
    curr_allocator = allocator;
}
int MemoryManager::dumpMemoryMap(char *filename){
    // Format: "[START, LENGTH] - [START, LENGTH] …", e.g., "[0, 10] - [12, 2] - [20, 6]"
    if(!checkInit()){
        exit;
    }
    int fd = open(filename, O_WRONLY | O_CREAT);
    if (fd == -1) {
        return -1;
    }
    if(ftruncate(fd, 0) == -1){
        perror("Could not truncate");
    }
    //make into a complete list
    uint16_t offset;
    uint16_t length;
    char space[4]= " - ";
    uint16_t* holelist = static_cast<uint16_t*>(getList());
    uint16_t* entry_point = holelist;
    uint16_t holelistlength = *holelist++;

    if (holelistlength == 0){
        delete[] entry_point;
        return 0;
    }
    std::string mem_map;
    for(uint16_t i = 1; i < (holelistlength) * 2; i += 2) {
        offset = holelist[i - 1];
        length = holelist[i];
        std::string entry = makeEntry(offset, length);
        mem_map += entry;
        if(!((i + 2) >= (holelistlength * 2))){
            mem_map += space;
        }
        
    }
    write(fd, mem_map.c_str(), strlen(mem_map.c_str()));
        
    delete[] entry_point;
    return 0;
}
bool MemoryManager::checkInit(){
    if(init)
        return true;
    else{
        std::cout << "\n!!! ERROR !!! : MemoryManager not initialized!"
        << "\nMemoryManager will not perform functionality without a fully initialized memory block."
        << "\nPlease ensure initialize(sizeInBytes) is called before calling any other functions." << std::endl;
        return false;
    }
}
std::string MemoryManager::makeEntry(uint16_t off, uint16_t len){
    std::string hole_entry = "[";
    hole_entry.append(std::to_string(off));
    hole_entry.append(", ");
    hole_entry.append(std::to_string(len));
    hole_entry.append("]");
    return hole_entry;
}
void* MemoryManager::getList(){
    // Offset and length are in words. If no memory has been allocated, the function should return a NULL pointer.
    if(!checkInit()){
        exit;
    }
    uint16_t num_holes = static_cast<uint16_t>(holes.size());
    int size = (num_holes *2) + 1;
    hole_list = new uint16_t[size];
    //if(hole_list.empty())
    hole_list[0] = num_holes;
    //else 
      //  hole_list.at(0) = num_holes;
    int i = 1;
    for(auto x : holes){
        hole_list[i] = x.first;
        hole_list[i + 1] = x.second;
        i += 2;
        //if(i + 2 >= (size))
        //    break;
    }   
    return hole_list;
}
void* MemoryManager::getBitmap(){
    // first two bytes are the size of the bitmap (little-Endian){} the rest is the bitmap, word-wise.
    if(!checkInit()){
        exit;
    }
    size_t by = 8;
    int size = ceil((double)word_count/(double)by);
    bit_map = new uint8_t[2 + size];
    for(int i = 0; i < size + 2; i++){
        bit_map[i] = 0x00;
    }
    std::string bitstream (word_count, '1');
    // std::cout << "Initialized bitstream: " << bitstream << std::endl;
    uint16_t* holelist = static_cast<uint16_t*>(getList());
    uint16_t* entry_point = holelist;
    uint16_t holelistlength = *holelist++;

    for(int i = 1; i < holelistlength * 2; i += 2){
        int block_start = holelist[i] + holelist[i - 1];
        for(int k = holelist[i-1]; k < block_start; k++){
            bitstream[k] = '0';
        }   
    }
    // std::cout << "bitstream: " << bitstream << std::endl;
    int j = 0;
    bit_map[1] = static_cast<uint8_t>((size & 0xFF00) >> 8);
    bit_map[0] = static_cast<uint8_t>(size & 0x00FF);

    for(int i = 0; i < size; i++){
        std::string byte;
        if((j + by) < bitstream.size())
            byte = bitstream.substr(j,by);
        else
            byte = bitstream.substr(j);

        int precision = by - std::min(by, byte.size());
        byte.append(precision, '0');

        std::string buf(by, '0');
        for(int k = 0; k < by; k++){
            buf[k] = byte[by - (k + 1)];
        }
        byte = buf;
        // std::cout << "byte processed: " << byte <<std::endl;
        int st = stoi(byte, 0, 2);
        bit_map[i+2] = static_cast<uint8_t>(st);
        j += by;
    }
    delete [] entry_point;
    return bit_map;
}
unsigned MemoryManager::getWordSize(){
    return word_size;
}
void* MemoryManager::getMemoryStart(){
    return &mem[0];
}
unsigned MemoryManager::getMemoryLimit(){
    return byte_count;
}

template <typename Iter, typename Cont>
bool MemoryManager::is_last(Iter iter, const Cont& cont){
    return (iter != cont.end()) && (next(iter) == cont.end());
}
template <typename Iter>
bool MemoryManager::combine_prev(Iter& it){
    // If iterator is not at the beginning of the std::list, check backward
    if(it != holes.begin()){
        if(prev(it)->first + prev(it)->second == it->first){
            prev(it)->second += it->second;
            auto it_buffer = prev(it);
            holes.erase(it);
            it = it_buffer;
            return true;
        }
        else
            return false;
    }
    return false;
}
template <typename Iter>
bool MemoryManager::combine_next(Iter& it){
    // If iterator is not at the end of the std::list, check forward
    if(!is_last(it,blocks)){
        if(it->first + it->second == next(it)->first){
            it->second += next(it)->second;
            holes.erase(next(it));
        }
        else
            return false;
    }
    return false;
}
template <typename Iter>
void MemoryManager::combine_holes(Iter& it){
    if(combine_next(it) || combine_prev(it))
        combine_holes(it);
}
void MemoryManager::print(){
    std::cout << "\n------------------------------------------\n";
    std::cout << "HOLES : " << holes.size() << " ";
    for(auto hole: holes){
        std::cout << "[" <<hole.first <<", "<<hole.second<<"] ";
    }

    std::cout << "\nBLOCKS : " << blocks.size() << " ";
    for(auto block: blocks){
        std::cout << "[" <<block.first <<", "<<block.second<<"] ";
    }
    std::cout << "\n------------------------------------------\n";
}

int bestFit(int sizeInWords, void *list){
    int best_hole[2] = {-1, -1};
    uint16_t* holelist = static_cast<uint16_t*>(list);
    uint16_t holelistlength = *holelist++;

    if (holelistlength == 0)
        return -1;
    for(uint16_t i = 1; i < (holelistlength) * 2; i += 2) {
        //std::cout << "holelist[i-1]" << holelist[i - 1] << "___";
        //std::cout << "holelist[i]: " << holelist[i] <<std::endl;
        if(holelist[i] >= sizeInWords){
            if(best_hole[1] == -1 || holelist[i] < best_hole[1]){
                    best_hole[0] = holelist[i-1];
                    best_hole[1] = holelist[i];
            }
        }
    }
    return best_hole[0];
}
int worstFit(int sizeInWords, void *list){
    int max_hole[2] = {-1, -1};  

    uint16_t* holelist = static_cast<uint16_t*>(list);
    uint16_t holelistlength = *holelist++;

     if (holelistlength == 0)
        return -1;
   for(uint16_t i = 1; i < (holelistlength) * 2; i += 2) {
        //std::cout << "holelist[i-1]" << holelist[i - 1] << "___";
        //std::cout << "holelist[i]: " << holelist[i] <<std::endl;
        if(holelist[i] >= sizeInWords){
            if(max_hole[1] == -1 || holelist[i] > max_hole[1]){
                    max_hole[0] = holelist[i-1];
                    max_hole[1] = holelist[i];
            }
        }
    }
    return max_hole[0];
}
