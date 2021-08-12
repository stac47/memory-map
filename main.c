#define _DEFAULT_SOURCE
#define _LARGEFILE64_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// See https://www.kernel.org/doc/Documentation/vm/pagemap.txt
typedef struct PagemapEntry
{
    uint64_t pfn : 55;
    bool softDirty : 1;
    bool pageExclusivelyMapped : 1;
    uint8_t zero : 4;
    bool filePageOrSharedAnon : 1;
    bool swapped : 1;
    bool present : 1;
} __attribute((packed)) PagemapEntry_t;

static void Usage()
{
    const char* usage = "mm <file>";
    printf("%s\n", usage);
}

static void InfoLog(const char* fmt, ...)
{
    printf("INFO: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

static void GetPhysicalAddress(char* addr, char* buf)
{
    char pagemap[50];
    int fd = 0;
    int pageSize = getpagesize();
    size_t pageIndex = (uint64_t)addr / pageSize;
    size_t pageOffset = (uint64_t)addr % pageSize;
    PagemapEntry_t entry;
    sprintf(pagemap, "/proc/%d/pagemap", getpid());
    fd = open(pagemap, O_RDONLY);
    if (fd < 0)
    {
        perror("Cannot open pagemap file");
        exit(1);
    }
    InfoLog("Open pagemap at: %s", pagemap);
    if (pread(fd, &entry, sizeof(entry), pageIndex * sizeof(entry)) != sizeof(entry)) {
        perror("Cannot read entry");
        exit(1);
    }
    uint64_t phys = 0;
    if (entry.present)
    {
        phys = entry.pfn * pageIndex + pageOffset;
    }
    else
    {
        InfoLog("Cannot find physical address of %p. ", addr);
    }
    sprintf(buf, "0x%016lx", phys);

}

int main(int argc, char *argv[])
{
    struct stat sb;
    char* addr;
    char physAddr[16];
    if (argc != 2) {
        Usage();
        exit(1);
    }
    const char* filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        perror("Cannot open file");
        exit(1);
    }
    InfoLog("File to map: %s", filename);
    if (fstat(fd, &sb) == -1)
    {
        perror("Cannot stat file");
        exit(1);
    }
    if (sb.st_size / getpagesize() > RLIMIT_MEMLOCK) {
        InfoLog("The file you try to map will occupy more than "
                "RLIMIT_MEMLOCK (%d) pages. Locking will fail.",
                RLIMIT_MEMLOCK);
    }
    addr = mmap(NULL,
                sb.st_size,
                PROT_READ,
                MAP_SHARED|MAP_LOCKED,//|MAP_POPULATE,
                fd,
                0);
    if (addr == MAP_FAILED) {
        perror("Cannot mmap");
        exit(1);
    }
    GetPhysicalAddress(addr, physAddr);
    InfoLog("File %s mapped to virtual address %p (physical address %s)",
            filename, addr, physAddr);
    puts("Press a key to exit...");
    getchar();
    puts("Goodbye");

    return 0;
}
