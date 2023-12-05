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
void
spr_dwarf_elf_object_access_finish(Dwarf_Obj_Access_Interface* obj);
int
dwarf_finish_rd(Dwarf_Debug dbg, Dwarf_Error * error);