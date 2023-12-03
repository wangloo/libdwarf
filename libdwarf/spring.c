#include "config.h"
#include "dwarf_incl.h"
#include "dwarf_elf_access.h"
#include "spring/ramdisk.h"
#include "spring/elf.h"

#define DWARF_DBG_ERROR(dbg,errval,retval) \
    _dwarf_error(dbg, error, errval); return(retval);

#define FALSE  0
#define TRUE   1


// SpringOS kernel don't have standard file system
// Make a layer using existing function
int
dwarf_init_rd(struct ramdisk_file *file,
    Dwarf_Unsigned access,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg, Dwarf_Debug * ret_dbg, Dwarf_Error * error)
{
  struct elfhdr elf;
  if (access != DW_DLC_READ) {
      DWARF_DBG_ERROR(NULL, DW_DLE_INIT_ACCESS_WRONG, DW_DLV_ERROR);
  }

  if (ramdisk_read(file, &elf, sizeof(elf), 0) != sizeof(elf)) {
    goto bad;
  }
  if (elf.magic != ELF_MAGIC)
      goto bad;

  return DW_DLV_OK;
bad:
  return DW_DLV_ERROR;
}