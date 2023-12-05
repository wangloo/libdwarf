#include "config.h"
#include "dwarf_incl.h"
#include "dwarf_elf_access.h"
#include "spring/ramdisk.h"
#include "spring/elf.h"


typedef struct {
    struct ramdisk_file *elf_fp;
    int              is_64bit;
    Dwarf_Small      length_size;
    Dwarf_Small      pointer_size;
    Dwarf_Unsigned   section_count;
    Dwarf_Endianness endianness;
    Dwarf_Small      machine;
    int              libdwarf_owns_elf;
    
    // struct elfhdr *ehdr32; // Only support Elf 64bit now
    struct elfhdr *ehdr;

    /*  Elf symtab and its strtab.  Initialized at first
        call to do relocations, the actual data is in the Dwarf_Debug
        struct, not allocated locally here. */
    struct Dwarf_Section_s *symtab;
    struct Dwarf_Section_s *strtab;

} dwarf_elf_object_access_internals_t;

struct Dwarf_Elf_Rela {
    Dwarf_ufixed64 r_offset;
    /*Dwarf_ufixed64 r_info; */
    Dwarf_ufixed64 r_type;
    Dwarf_ufixed64 r_symidx;
    Dwarf_ufixed64 r_addend;
};


static int
spr_elf_get_secthdr(struct ramdisk_file *file, struct elfhdr *elf, 
                    Dwarf_Half section_index, struct secthdr *sh)
{
  unsigned long i, off;

  // load and map all loadable segments
  for (i = 0, off = elf->shoff; i < elf->shnum; i++, off += sizeof(*sh)) {
    if (ramdisk_read(file, sh, sizeof(*sh), off) != sizeof(*sh))
      goto bad;

    if (i == section_index)
      return DW_DLV_OK;
  }

bad:
  return DW_DLV_ERROR;
}

// [strp]    Allocated internelly, freed outside
// [section] Index of strtbl in elf header
static int
spr_elf_get_strptr(struct ramdisk_file *file, struct elfhdr *elf,
                    unsigned long section, unsigned long offset, char **strp)
{
  struct secthdr sh;

  if (spr_elf_get_secthdr(file, elf, section, &sh) != DW_DLV_OK)
    return DW_DLV_ERROR;
  
  if (sh.type != 3 || offset >= sh.size) 
    return DW_DLV_ERROR;

  *strp = malloc(128); // For simple implementation

  // names in strtbl is null-terminated, so it's safe
  if (ramdisk_read(file, *strp, 256, sh.offset+offset) <= 0) {
    free(*strp);
    return DW_DLV_ERROR;
  }

  return DW_DLV_OK;
}


/*  dwarf_elf_object_access_get_section()

    If writing a function vaguely like this for a non-elf object,
    be sure that when section-index is passed in as zero that
    you set the fields in *ret_scn to reflect an empty section
    with an empty string as the section name.  Adjust your
    section indexes of your non-elf-reading-code
    for all the necessary functions in Dwarf_Obj_Access_Methods_s
    accordingly.

    Should have gotten sh_flags, sh_addralign too.
    But Dwarf_Obj_Access_Section is publically defined so changing
    it is quite painful for everyone.
*/
static int
spr_dwarf_elf_object_access_get_section_info(
    void* obj_in,
    Dwarf_Half section_index,
    Dwarf_Obj_Access_Section* ret_scn,
    int* error)
{
  dwarf_elf_object_access_internals_t*obj =
      (dwarf_elf_object_access_internals_t*)obj_in;
  struct secthdr sh;

  if (spr_elf_get_secthdr(obj->elf_fp, obj->ehdr, section_index, &sh) != DW_DLV_OK) {
    *error = DW_DLE_ELF_GETSHDR_ERROR;
    return DW_DLV_ERROR;
  }

  /*  Get also section 'sh_type' and sh_info' fields, so the caller
      can use it for additional tasks that require that info. */
  ret_scn->type = sh.type;
  ret_scn->size = sh.size;
  ret_scn->addr = sh.addr;
  ret_scn->link = sh.link;
  ret_scn->info = sh.info;
  ret_scn->entrysize = sh.entsize;
  if (spr_elf_get_strptr(obj->elf_fp, obj->ehdr, obj->ehdr->shstrndx, 
                          sh.name, (char **)&(ret_scn->name)) != DW_DLV_OK) {
    *error = DW_DLE_ELF_STRPTR_ERROR;
    return DW_DLV_ERROR;
  }
  return DW_DLV_OK;
}

/* dwarf_elf_object_access_get_byte_order */
static
Dwarf_Endianness
spr_dwarf_elf_object_access_get_byte_order(void* obj_in)
{
    dwarf_elf_object_access_internals_t*obj =
        (dwarf_elf_object_access_internals_t*)obj_in;
    return obj->endianness;
}

static Dwarf_Small
spr_dwarf_elf_object_access_get_length_size(void* obj_in)
{
    dwarf_elf_object_access_internals_t*obj =
        (dwarf_elf_object_access_internals_t*)obj_in;
    return obj->length_size;
}

static Dwarf_Small
spr_dwarf_elf_object_access_get_pointer_size(void* obj_in)
{
    dwarf_elf_object_access_internals_t*obj =
        (dwarf_elf_object_access_internals_t*)obj_in;
    return obj->pointer_size;
}

static Dwarf_Unsigned
spr_dwarf_elf_object_access_get_section_count(void * obj_in)
{
    dwarf_elf_object_access_internals_t*obj =
        (dwarf_elf_object_access_internals_t*)obj_in;
    return obj->section_count;
}

/*  dwarf_elf_object_access_load_section()
    We are only asked to load sections that
    libdwarf really needs. */
// [section_data] Allocate internelly, free outside
static int
spr_dwarf_elf_object_access_load_section(void* obj_in, 
  Dwarf_Half section_index, Dwarf_Small** section_data, int* error)
{
  dwarf_elf_object_access_internals_t *obj =
      (dwarf_elf_object_access_internals_t *)obj_in;
  struct secthdr sh;
  Dwarf_Small *data;

  if (section_index == 0) {
    return DW_DLV_NO_ENTRY;
  }

  if (spr_elf_get_secthdr(obj->elf_fp, obj->ehdr, section_index, &sh) != DW_DLV_OK) {
    *error = DW_DLE_MDE;
    return DW_DLV_ERROR;
  }
  
  if (sh.size <= 0) {
    *error = DW_DLE_MDE;
    return DW_DLV_ERROR;
  }

  data = malloc(sh.size);
  if (ramdisk_read(obj->elf_fp, data, sh.size, sh.offset) != sh.size) {
    free(data);
    *error = DW_DLE_MDE;
    return DW_DLV_ERROR;    
  }

  *section_data = data;
  return DW_DLV_OK;
}

/* dwarf_elf_access method table. */
static const struct Dwarf_Obj_Access_Methods_s dwarf_elf_object_access_methods =
{
    spr_dwarf_elf_object_access_get_section_info,
    spr_dwarf_elf_object_access_get_byte_order,
    spr_dwarf_elf_object_access_get_length_size,
    spr_dwarf_elf_object_access_get_pointer_size,
    spr_dwarf_elf_object_access_get_section_count,
    spr_dwarf_elf_object_access_load_section,
    // spr_dwarf_elf_object_relocate_a_section
};






/*  dwarf_elf_object_access_internals_init()
    On error, set *error with libdwarf error code.
*/
static int
spr_dwarf_elf_object_access_internals_init(struct ramdisk_file *file, 
  void* obj_in, struct elfhdr *elf, int* error)
{
    dwarf_elf_object_access_internals_t*obj =
        (dwarf_elf_object_access_internals_t*)obj_in;
    Dwarf_Half machine = 0;

    obj->elf_fp = file;
    obj->is_64bit = (elf->elf[0] == 2);

    if (elf->elf[1] == 1){
        obj->endianness = DW_OBJECT_LSB;
    } else if (elf->elf[1] == 2){
        obj->endianness = DW_OBJECT_MSB;
    }

    if (obj->is_64bit) {
        obj->ehdr = elf;
        obj->section_count = obj->ehdr->shnum;
        obj->machine = obj->ehdr->machine;
    } else {
        *error = DW_DLE_NO_ELF64_SUPPORT;
        return DW_DLV_ERROR;
    }

    /*  The following length_size is Not Too Significant. Only used
        one calculation, and an approximate one at that. */
    obj->length_size = obj->is_64bit ? 8 : 4;
    obj->pointer_size = obj->is_64bit ? 8 : 4;
    
    return DW_DLV_OK;
}


/*  Interface for the ELF object file implementation.
    On error this should set *err with the
    libdwarf error code.
*/
int
spr_dwarf_elf_object_access_init(struct ramdisk_file *file, struct elfhdr *elf,
    int libdwarf_owns_elf,
    Dwarf_Obj_Access_Interface** ret_obj,
    int *err)
{
    int res = 0;
    dwarf_elf_object_access_internals_t *internals = 0;
    Dwarf_Obj_Access_Interface *intfc = 0;
    
    internals = malloc(sizeof(dwarf_elf_object_access_internals_t));
    if (!internals) {
        *err = DW_DLE_ALLOC_FAIL;
        /* Impossible case, we hope. Give up. */
        return DW_DLV_ERROR;
    }
    memset(internals,0,sizeof(*internals));
    res = spr_dwarf_elf_object_access_internals_init(file, internals, elf, err);
    if (res != DW_DLV_OK){
        /* *err is already set. */
        free(internals);
        return DW_DLV_ERROR;
    }
    internals->libdwarf_owns_elf = libdwarf_owns_elf;

    intfc = malloc(sizeof(Dwarf_Obj_Access_Interface));
    if (!intfc) {
        /* Impossible case, we hope. Give up. */
        *err = DW_DLE_ALLOC_FAIL;
        free(internals);
        return DW_DLV_ERROR;
    }
    /* Initialize the interface struct */
    intfc->object = internals;
    intfc->methods = &dwarf_elf_object_access_methods;


    /*  An access method hidden from non-elf. Needed to
        handle new-ish SHF_COMPRESSED flag in elf.  */
    // _dwarf_get_elf_flags_func_ptr = _dwarf_get_elf_flags_func;


    *ret_obj = intfc;
    return DW_DLV_OK;
}


/* Clean up the Dwarf_Obj_Access_Interface returned by elf_access_init.  */
void
spr_dwarf_elf_object_access_finish(Dwarf_Obj_Access_Interface* obj)
{
  if (!obj)
    return;

  if (obj->object)
      free(((dwarf_elf_object_access_internals_t *)obj->object)->ehdr);
  free(obj->object);
  free(obj);
}