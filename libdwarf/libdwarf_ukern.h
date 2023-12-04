int
dwarf_init_rd(struct ramdisk_file *file,
    Dwarf_Unsigned access,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg, Dwarf_Debug * ret_dbg, Dwarf_Error * error);

int
spr_dwarf_elf_object_access_init(struct ramdisk_file *file, struct elfhdr *elf,
    int libdwarf_owns_elf,
    Dwarf_Obj_Access_Interface** ret_obj,
    int *err);