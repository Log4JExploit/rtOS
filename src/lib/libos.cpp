#include <cstdint>

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
