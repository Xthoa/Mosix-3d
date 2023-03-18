#pragma once
#include "types.h"

enum keys{
	k_esc=0x80,
	k_ctrl_l,k_ctrl_r,
	k_shift_l,k_shift_r,
	k_alt_l,k_alt_r,
	k_ctrl_ld,k_ctrl_rd,
	k_shift_ld,k_shift_rd,
	k_alt_ld,k_alt_rd,
	k_caps,
	k_f1,k_f2,k_f3,k_f4,k_f5,k_f6,
	k_f7,k_f8,k_f9,k_f10,k_f11,k_f12,
	k_nums,k_scr,
	k_home,k_end,
	k_pgup,k_pgdn,
	k_up,k_down,k_left,k_right,
	k_del,k_ins,
	k_prtsc,k_pause
};

#define KS_CTRL 0x100
#define KS_ALT 0x200
#define KS_CTRLALT 0x300
#define KS_SHIFT 0x400
#define KS_CAPS 0x800

#define PORT_KEYDAT 0x60
#define PORT_KEYSTA	0x64
#define PORT_KEYCMD	0x64
#define KEYSTA_NOREADY 0x2
#define KEYCMD_WMODE 0x60
#define KBC_MODE 0x47
#define KEYCMD_SENDTO_MOUSE		0xd4
#define UPDATE_LED 0xed
#define MOUSECMD_ENABLE			0xf4
#define KBC_ACK 0xfa
