/* https://github.com/matf-racunarska-grafika/project_base/blob/main/src/main.cpp */
#include "global.hpp"
#include <GLFW/glfw3.h>
#include "gl.hpp"

static struct r_tick tick;

// rep  win32
struct
{
	int mouse_left_down;
	int mouse_middle_down;
	
	int rep_scene1;
	int rep_scene2;
} platform;

void
framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	def_w = width;
	def_h = height;
	r_glexit();
	r_glbegin();
	//glViewport(0, 0, width, height);
}

/*
	if (win32.controller.lmb || win32.controller.mmb)
	{
		tick.cursor.dx = tick.cursor.x - win32.cursor.x;
		tick.cursor.dy = tick.cursor.y - win32.cursor.y;
		if (win32.controller.lmb && win32.controller.alt)
		{
			tick.cursor.mode = CURSOR_MODE_ORBIT;
		}
		else if (win32.controller.mmb && win32.controller.alt)
		{
			tick.cursor.mode = CURSOR_MODE_PAN;
		}
	}
*/
void
mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		std::cout << (mods & GLFW_MOD_ALT) << "dole <<<<<<<<" << std::endl;
		platform.mouse_left_down = 1;
		
		if (mods & GLFW_MOD_ALT)
		{
			tick.cursor.mode = CURSOR_MODE_ORBIT;
		}
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		std::cout << (mods & GLFW_MOD_ALT) << "gore <<<<<<<<" << std::endl;
		platform.mouse_left_down = 0;
		tick.cursor.mode = CURSOR_MODE_STAGNANT;
	}
	
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
	{
		std::cout << (mods & GLFW_MOD_ALT) << "dole <<<<<<<<" << std::endl;
		platform.mouse_middle_down = 1;
		
		if (mods & GLFW_MOD_ALT)
		{
			tick.cursor.mode = CURSOR_MODE_PAN;
		}
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
	{
		std::cout << (mods & GLFW_MOD_ALT) << "gore <<<<<<<<" << std::endl;
		platform.mouse_middle_down = 0;
		tick.cursor.mode = CURSOR_MODE_STAGNANT;
	}
}

void
mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
	if (platform.mouse_left_down || platform.mouse_middle_down)
	{
		tick.cursor.dx = tick.cursor.x - xpos;
		tick.cursor.dy = tick.cursor.y - ypos;
	}
	tick.cursor.x = xpos;
	tick.cursor.y = ypos;
}

void
scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	tick.cursor.wheel = yoffset*20.0;
}

void
key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        platform.rep_scene1 = 1;
    }
    else if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
    {
        platform.rep_scene2 = 1;
    }
}

void
processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int
main(int argc, char **argv)
{
	
	
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	GLFWwindow *window = glfwCreateWindow(def_w, def_h, "matf-rg 2021/2022 (%s)", NULL, NULL);
	
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	
	glewExperimental = GL_TRUE;
	glewInit();
    
    	r_glbegin();
    	r_newscene(scene::SCENE_ROOM);
    	tick = { };
    	while (!glfwWindowShouldClose(window))
    	{
    		processInput(window);
    		if (platform.rep_scene1)
    			r_newscene(scene::SCENE_ROOM);
    		else if (platform.rep_scene2)
    			r_newscene(scene::SCENE_PRIMITIVES);
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//std::cout << tick.cursor.dx << "  " << tick.cursor.dy << "  " << tick.cursor.x << "  " << tick.cursor.y << std::endl;
		r_gltick(tick);
		tick.cursor.dx = 0;
		tick.cursor.dy = 0;
		platform.rep_scene1 = 0;
		platform.rep_scene2=  0;
		tick.cursor.wheel = 0;
    		glfwSwapBuffers(window);
        	glfwPollEvents();
    	}
	return 0;
}

