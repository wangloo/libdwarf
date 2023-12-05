#include "config.h"
#include "dwarf_incl.h"
#include "dwarf_elf_access.h"
#include "spring/ramdisk.h"
#include "spring/elf.h"
#include "libdwarf_ukern.h"

#define DWARF_DBG_ERROR(dbg,errval,retval) \
    _dwarf_error(dbg, error, errval); return(retval);

#define FALSE  0
#define TRUE   1






/* Initialize the ELF object access for libdwarf.  */
static int
dwarf_elf_init_file_ownership(struct ramdisk_file *file, struct elfhdr *elf,
    int libdwarf_owns_elf,
    Dwarf_Unsigned access,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg,
    Dwarf_Debug * ret_dbg,
    Dwarf_Error * error)
{
  /* ELF is no longer tied to libdwarf. */
  Dwarf_Obj_Access_Interface *binary_interface = 0;
  int res = DW_DLV_OK;
  int localerrnum = 0;

  if (access != DW_DLC_READ) {
      DWARF_DBG_ERROR(NULL, DW_DLE_INIT_ACCESS_WRONG, DW_DLV_ERROR);
  }

  
  /* This allocates and fills in *binary_interface. */
  res = spr_dwarf_elf_object_access_init(
      file,
      elf,
      libdwarf_owns_elf,
      &binary_interface,
      &localerrnum);
  if (res != DW_DLV_OK) {
    if (res == DW_DLV_NO_ENTRY) {
        return res;
    }
    DWARF_DBG_ERROR(NULL, localerrnum, DW_DLV_ERROR);
  }

  /*  This mallocs space and returns pointer thru ret_dbg,
      saving  the binary interface in 'ret-dbg' */
  res = dwarf_object_init(binary_interface, errhand, errarg,
      ret_dbg, error);
  if (res != DW_DLV_OK){
      dwarf_elf_object_access_finish(binary_interface);
  }
  return res;
}

// SpringOS kernel don't have standard file system
// Make a layer using existing function
int
dwarf_init_rd(struct ramdisk_file *file,
    Dwarf_Unsigned access,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg, Dwarf_Debug * ret_dbg, Dwarf_Error * error)
{
  struct elfhdr *elf;
  if (access != DW_DLC_READ) {
      DWARF_DBG_ERROR(NULL, DW_DLE_INIT_ACCESS_WRONG, DW_DLV_ERROR);
  }

  elf = malloc(sizeof(*elf));

  if (ramdisk_read(file, elf, sizeof(*elf), 0) != sizeof(*elf)) {
    goto bad;
  }
  if (elf->magic != ELF_MAGIC)
      goto bad;

  return dwarf_elf_init_file_ownership(file, elf, TRUE, access, 
                                        errhand, errarg, ret_dbg, error);
bad:
  return DW_DLV_ERROR;
}

/*
    Frees all memory that was not previously freed
    by dwarf_dealloc.
    Aside from certain categories.

    This is only applicable when dwarf_init() or dwarf_elf_init()
    was used to init 'dbg'.
*/
int
dwarf_finish_rd(Dwarf_Debug dbg, Dwarf_Error * error)
{
    if(!dbg) {
        DWARF_DBG_ERROR(NULL, DW_DLE_DBG_NULL, DW_DLV_ERROR);
    }
    spr_dwarf_elf_object_access_finish(dbg->de_obj_file);

    return dwarf_object_finish(dbg, error);
}