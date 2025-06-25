-c
-heap  0x40000
-stack 0x10000


/***************************************************************
* Set -DCORENUM=0 for Core0 build, -DCORENUM=1 for Core1 build!
***************************************************************/

MEMORY
{
    L1PSRAM   (RWX)  : org = 0x00E00000, len = 0x0007FFF
    L1DSRAM   (RWX)  : org = 0x00F00000, len = 0x0007FFF

    MSMCSRAM0 (RWX)  : org = 0x0C001000, len = 0x0007F000   /* Core0 RAM */
    MSMCSRAM1 (RWX)  : org = 0x0C090000, len = 0x0007F000   /* Core1 RAM */
    MSMCSRAM_SH (RWX): org = 0x0C080000, len = 0x00010000   /* Shared RAM */

    /* No program sections go into NOR flash! Only hex conversion uses it. */
    // NORFLASH (R) : org = 0x70000000, len = 0x20000
}

SECTIONS
{
#if (CORENUM == 0)
    .text         : load > MSMCSRAM0
    .stack        : load > MSMCSRAM0
    .bss          : load > MSMCSRAM0
    .const        : load > MSMCSRAM0
    .cinit        : load > MSMCSRAM0
    .pinit        : load > MSMCSRAM0
    .data         : load > MSMCSRAM0
    .fardata      : load > MSMCSRAM0
    .far          : load > MSMCSRAM0
    .neardata     : load > MSMCSRAM0
    .rodata       : load > MSMCSRAM0
    .switch       : load > MSMCSRAM0
    .sysmem       : load > MSMCSRAM0
    .cio          : load > MSMCSRAM0
    .args         : load > MSMCSRAM0 align = 0x4, fill = 0 { _argsize = 0x200; }
    .init_array   : load > MSMCSRAM0
    .kernel       : load > MSMCSRAM0
    .csl_vect     : load > MSMCSRAM0
    xdc.meta      : load > MSMCSRAM0, type = COPY
    .boot_load    : load > MSMCSRAM0  /* Boot section for flash boot */
#else
    .text         : load > MSMCSRAM1
    .stack        : load > MSMCSRAM1
    .bss          : load > MSMCSRAM1
    .const        : load > MSMCSRAM1
    .cinit        : load > MSMCSRAM1
    .pinit        : load > MSMCSRAM1
    .data         : load > MSMCSRAM1
    .fardata      : load > MSMCSRAM1
    .far          : load > MSMCSRAM1
    .neardata     : load > MSMCSRAM1
    .rodata       : load > MSMCSRAM1
    .switch       : load > MSMCSRAM1
    .sysmem       : load > MSMCSRAM1
    .cio          : load > MSMCSRAM1
    .args         : load > MSMCSRAM1 align = 0x4, fill = 0 { _argsize = 0x200; }
    .init_array   : load > MSMCSRAM1
    .kernel       : load > MSMCSRAM1
    .csl_vect     : load > MSMCSRAM1
    xdc.meta      : load > MSMCSRAM1, type = COPY
    .boot_load    : load > MSMCSRAM1  /* Boot section for flash boot */
#endif

    /* Shared Packet section: always mapped at same place! */
    .sharedPacket ALIGN(4096) : {} > MSMCSRAM_SH

}
