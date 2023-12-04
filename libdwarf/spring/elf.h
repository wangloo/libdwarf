
#include "types.h"

#define ELF_MAGIC 0x464C457FU  // "\x7FELF" in little endian

// Values for Proghdr type
#define ELF_PROG_LOAD           1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC      1
#define ELF_PROG_FLAG_WRITE     2
#define ELF_PROG_FLAG_READ      4


// File header
struct elfhdr {
  uint magic;  // must equal ELF_MAGIC
  uchar elf[12];
  ushort type;
  ushort machine;
  uint version;
  uint64 entry;
  uint64 phoff;
  uint64 shoff;
  uint flags;
  ushort ehsize;
  ushort phentsize;
  ushort phnum;
  ushort shentsize;
  ushort shnum;
  ushort shstrndx;
};


// Program section header
struct proghdr {
  uint32 type;
  uint32 flags;
  uint64 off;
  uint64 vaddr;
  uint64 paddr;
  uint64 filesz;
  uint64 memsz;
  uint64 align;
};

// Section header
struct secthdr {
  uint32 name;   // Section name (string tbl index)
  uint32 type;   // Section type
  uint64 flags;  // Section flags
  uint64 addr;   // Section virtual addr at execution
  uint64 offset; // Section file offset
  uint64 size;   // Section size in bytes
  uint32 link;   // Link to another section
  uint32 info;   // Additional section information
  uint64 addralign; // Section aligment
  uint64 entsize;  // Entry size if section holds table 
};