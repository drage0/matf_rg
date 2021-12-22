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

enum class scene: unsigned char
{
	SCENE_VOID,
	SCENE_ROOM,
	SCENE_PRIMITIVES,
};

extern int r_glbegin(void);
extern int r_newscene(enum scene scene);
extern void r_gltick(struct r_tick tick);
extern void r_glexit(void);
