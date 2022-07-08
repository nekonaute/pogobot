INCLUDE generated/output_format.ld
ENTRY(_start)

INCLUDE generated/regions.ld

SECTIONS
{
	.text :
	{
		_ftext = .;
        /* Make sure crt0 files come first, and */
        /* don't get disposed of by greedy optimisation */
        *crt0*(.text)
        KEEP(*crt0*(.text))
		_etext = .;
	} > rom

	.data :
	{
		. = ALIGN(8);
		_fdata = .;
        KEEP(*(.text.isr))
		*(.text .stub .text.* .gnu.linkonce.t.*)
		*(.data .data.* .gnu.linkonce.d.*)
		*(.data1)
		*(.sdata .sdata.* .gnu.linkonce.s.*)
        /* .rodata now in .data */
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.srodata .srodata.* )
		*(.rodata1)
		*(.got .got.*)
		*(.toc .toc.*)

        /* bios commands */
		. = ALIGN(8);
		PROVIDE_HIDDEN (__bios_cmd_start = .);
		KEEP(*(.bios_cmd))
		PROVIDE_HIDDEN (__bios_cmd_end = .);

        PROVIDE_HIDDEN (__bios_init_start = .);
        KEEP(*(.bios_init))
        PROVIDE_HIDDEN (__bios_init_end = .);


		/* Make sure the file is aligned on disk as well
		   as in memory; CRC calculation requires that. */
		FILL(0);
		. = ALIGN(8);
		_edata = .;
	} > sram AT > rom

	.bss :
	{
		. = ALIGN(8);
		_fbss = .;
		*(.dynsbss)
		*(.sbss .sbss.* .gnu.linkonce.sb.*)
		*(.scommon)
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(8);
		_ebss = .;
		_end = .;
	} > sram

	/DISCARD/ :
	{
		*(.eh_frame)
		*(.comment)
	}
}

PROVIDE(_fstack = ORIGIN(sram) + LENGTH(sram));

PROVIDE(_fdata_rom = LOADADDR(.data));
PROVIDE(_edata_rom = LOADADDR(.data) + SIZEOF(.data));
