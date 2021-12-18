#include "global.hpp"
#include "gl.hpp"

int
r_glbegin(void)
{
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	return 0;
}

void
r_gltick(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void
r_glexit(void)
{

}
