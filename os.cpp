#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>

/* Structs */

struct OSProcess {
   int pid;
   int parent;
   char name[32];
   short priviledge;
   char *entryPoint;
};

struct MemoryArea {
    char *first;
    uint16_t size;
    int pid;
    uint64_t id;
};

/* Global */

const uint16_t memoryAreaLimit = 32;
const uint16_t heapLimit = 32 * 1024;
const uint16_t heapSystemLimit = 2048;
const uint8_t  processLimit = 16;


char *heap;
char *nextHeapPos;

char *heapSystem;

uint16_t *memoryAreaIndex;
uint64_t *memoryIdCounter;
MemoryArea *memoryAreas;

uint8_t *processIndex;
uint64_t *processPidCounter;
OSProcess *processes;

/* Declarations */

int main();
void os_initMemory();
void os_uninitMemory();
uint16_t os_getMemoryStart();
uint16_t os_getMemoryUsed();

OSProcess* os_createProcess();
OSProcess* os_createProcess(char *name, OSProcess *parent);
OSProcess* os_findProcessByPid(uint16_t pid);
OSProcess* os_findProcessSlot();
void os_removeProcess(uint16_t pid);
uint16_t os_getProcessCount();

MemoryArea* os_alloc(uint16_t size);
void os_wipe(MemoryArea *area);
void os_free(MemoryArea *area);

void util_clear(char *memory, uint32_t size);
void util_shiftHeap();
void util_shiftHeapSystem();
void util_write(const char *text, char *target, int offset, int length);

MemoryArea* sys_memoryBlocks(OSProcess *process);
MemoryArea* sys_processes(OSProcess *process);
MemoryArea* sys_whoami(OSProcess *process);
MemoryArea* sys_findsequence(OSProcess *process, MemoryArea *data, MemoryArea *sequence);
MemoryArea* sys_kill(OSProcess *process, MemoryArea *pid);

/* Main */

int main() {
   os_initMemory();
   std::cout << "Heap Address: " << reinterpret_cast<long>(heap) << std::endl;
   
   OSProcess *process = os_createProcess();
   MemoryArea *result = sys_memoryBlocks(process);
   MemoryArea *result2 = sys_processes(process);
   std::cout << result->first << std::endl;
   std::cout << result2->first << std::endl;

   std::cout << "Memory Used: " << os_getMemoryUsed() << std::endl;
   std::cout << "Memory Start: " << os_getMemoryStart() << std::endl;
   std::cout << "Process Count: " << os_getProcessCount() << std::endl;
   std::cout << os_findProcessByPid(1)->priviledge << std::endl;

   os_uninitMemory();
   return 0;
}

/* OS Memory Management */

void os_initMemory() {
   heap = reinterpret_cast<char*>(malloc(heapLimit));
   nextHeapPos = heap;
   heapSystem = reinterpret_cast<char*>(malloc(heapSystemLimit));
   
   util_clear(heap, heapLimit);
   util_clear(heapSystem, heapSystemLimit);

   char *p = heapSystem;
   memoryAreaIndex = reinterpret_cast<uint16_t*>(p);
   p += 2;
   memoryIdCounter = reinterpret_cast<uint64_t*>(p);
   p += 4;
   memoryAreas = reinterpret_cast<MemoryArea*>(p);
   p += memoryAreaLimit * sizeof(MemoryArea) + 1;  
   processIndex = reinterpret_cast<uint8_t*>(p);
   *processIndex = 0;
   p += 1;
   processPidCounter = reinterpret_cast<uint64_t*>(p);
   *processPidCounter = 0;
   p += 4;
   processes = reinterpret_cast<OSProcess*>(p);
}

void os_uninitMemory() {
   free(heap);
   free(heapSystem);
}

uint16_t os_getMemoryUsed() {
   uint16_t count = 0;
   for(uint16_t i = 0; i < *memoryAreaIndex; i++) {
      count += memoryAreas[i].size;
   }
   return count;
}

uint16_t os_getMemoryStart() {
   MemoryArea area;
   area.first = heap;
   area.size = 0;
   
   int index = 0;

   for(int i = 0; i < *memoryAreaIndex; i++) {
      if(memoryAreas[i].first > area.first) {
          area = memoryAreas[i];
	  index = (area.first - heap) + area.size;
      }
   }
   return static_cast<uint16_t>(index);
}

OSProcess* os_createProcess() {
   return os_createProcess(nullptr, nullptr);
}

OSProcess* os_createProcess(char* name, OSProcess *parent) {
   OSProcess *process = os_findProcessSlot();
   
   if(process == nullptr) {
      if(os_getProcessCount() >= processLimit) {
         std::cerr << "Cannot create new process: Process limit reached!" << std::endl;
         exit(1);
      } else {
	 std::cerr << "Cannot create new prcoess: Unknown error occurred: NULLPTR";
      }
   }

   (*processPidCounter)++;
   (*processIndex)++;
   process->pid = *processPidCounter;

   if(name != nullptr) {
      util_write(name, process->name, 0, 31);
      process->name[32] = 0;
   }

   if(parent != nullptr) {
      process->parent = parent->pid;
      process->priviledge = parent->priviledge;
   }

   return process;  
}

OSProcess* os_findProcessSlot() {
   for(int i = 0; i < processLimit; i++) {
      if(processes[i].pid == 0) {
         return &processes[i];
      }
   }
   return nullptr;
}

OSProcess* os_findProcessByPid(uint16_t pid) {
   std::cout << "os_findProcessByPid()" << std::endl;
   for(int i = 0; i < *processIndex; i++) {
      OSProcess *current = &processes[i];
      if(current->pid == pid) {
         return current;
	 break;
      }
   }
   return nullptr;
}

void os_removeProcess(uint16_t pid) {
   std::cout << "os_removeProcess()" << std::endl;
   OSProcess *process = os_findProcessByPid(pid);
   if(process == nullptr) {
       return;
   }
   
   for(int i = 0; i < processLimit; i++) {
      if(processes[i].parent == process->pid) {
         os_removeProcess(processes[i].pid);
      }
   }
  
   for(int i = 0; i < memoryAreaLimit; i++) {
      if(memoryAreas[i].pid == process->pid) {
         os_free(&memoryAreas[i]);
      }
   }

   process->priviledge = 0;
   process->parent = 0;
   process->pid = 0;
   
   (*processIndex)--;
}

uint16_t os_getProcessCount() {
   return (uint16_t) *processIndex;
}

MemoryArea* os_alloc(OSProcess *process, uint16_t size) {
   if(*memoryAreaIndex >= memoryAreaLimit) {
      std::cerr << "Cannot allocate more memory areas!" << std::endl;
      exit(1);
   }

   if(os_getMemoryStart() + size > heapLimit) {
      std::cerr << "Memory allocation would fail: Out of memory!" << std::endl;
      exit(1);
   }

   MemoryArea *alloc = &memoryAreas[*memoryAreaIndex];
   alloc->first = nextHeapPos;
   alloc->size = size;
   alloc->pid = process->pid; 
   
   nextHeapPos += alloc->size;
   (*memoryAreaIndex)++;
   return alloc;
}

void os_wipe(MemoryArea *area) {
   char* first = area->first;
   util_clear(first, area->size);
}

void os_free(MemoryArea *area) {
   os_wipe(area);
   util_clear(reinterpret_cast<char*>(area), sizeof(MemoryArea));
}

void util_clear(char* memory, uint32_t size) {
   for(int i = 0; i < size; i++) {
       memory[i] = 0;
   }
}

void util_shiftHeap() {
   int index = *memoryAreaIndex;
   
   MemoryArea *lastMoved = nullptr;
   char       *lastMovedFirst = nullptr;

   char *freePlace = heap;
   
   for(int i = 0; i < index; i++) {
      MemoryArea *next = &memoryAreas[0];
      for(int j = 0; j < index; j++) {
	 MemoryArea *m = &memoryAreas[j];
         if(m->first < next->first && m->first > lastMovedFirst) {
	    next = m;
         }
      }

      if(next->first > freePlace) {
         for(int i = 0; i < next->size; i++) {
            freePlace[i] = next->first[i];
	 }
         next->first = freePlace;
	 freePlace = next->first + next->size;
      }

      lastMoved = next;
      lastMovedFirst = next->first;
   }
}

void util_shiftHeapSystem() {
   int indexOfEmpty = 0;

   for(int i = memoryAreaLimit - 1; i > 0; i++) {
      while(indexOfEmpty < i && memoryAreas[indexOfEmpty].pid != 0) {
          indexOfEmpty++;
      }
      if(i == indexOfEmpty) {
	 break;
      }
      
      MemoryArea *selected = &memoryAreas[i];
      memoryAreas[indexOfEmpty] = *selected;
      util_clear(reinterpret_cast<char*>(selected), sizeof(MemoryArea));
   }
}

void util_write(const char *text, char *target, int offset, int length) {
   for(int i = 0; i < length; i++) {
      target[i + offset] = text[i];
   } 
}

MemoryArea* sys_memoryBlocks(OSProcess *process) {
   MemoryArea *textBuffer = os_alloc(process, (uint16_t)(32 * memoryAreaLimit + 1));
   char *text = textBuffer->first;
   int index = 0;

   std::stringstream stream;

   stream << "Memory blocks: ";
   stream << (int) *memoryAreaIndex;
   stream << "\n";

   for(int i = 0; i < memoryAreaLimit; i++) {
      MemoryArea *area = &memoryAreas[i];
      if(area->size != 0) {
         stream << i;
         stream << (int) area->size;
      }
   }
   
   std::string result;
   stream >> result;

   util_write(result.c_str(), text, 0, result.length());
   text[result.length()] = 0;
   return textBuffer;
}

MemoryArea* sys_processes(OSProcess *process) {
   MemoryArea *textBuffer = os_alloc(process, (uint16_t)(32 * processLimit + 1));
   char *text = textBuffer->first;
   int index = 0;

   for(int i = 0; i < processLimit; i++) {
      OSProcess *process = &processes[i];
      if(process->pid == 0) {
         continue;
      }
      std::string indexAsStr = std::to_string(i);
      std::string pidAsStr = std::to_string(process->pid);
      util_write(indexAsStr.c_str(), text, index, indexAsStr.length());
      index += indexAsStr.length();
      util_write(": ", text, index, 2);
      index += 2;
      util_write(pidAsStr.c_str(), text, index, pidAsStr.length());
      index += pidAsStr.length();
      text[index] = 10;
      index++;
   }
   
   text[index] = 0;
   return textBuffer;
}
