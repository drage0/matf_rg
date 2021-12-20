#pragma once

#define CURSOR_MODE_STAGNANT 0
#define CURSOR_MODE_ORBIT 1
#define CURSOR_MODE_PAN 2
struct r_tick
{
	struct
	{
		int x;
		int y;
		int dx;
		int dy;
		int mode;
		int wheel;
	} cursor;
};

extern int r_glbegin(void);
extern void r_gltick(struct r_tick tick);
extern void r_glexit(void);
