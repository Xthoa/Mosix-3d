SECTIONS
{
	. = 0xffffff8000001000;
	.text : {
		*(.text)
	}
	.rdata : {
		*(.data)
		*(.rdata)
	}
	. = 0xffffff8000040000;
	.bss : {
		*(.bss)
	}
}