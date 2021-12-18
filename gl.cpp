#include "global.hpp"
#include "gl.hpp"

static struct
{
	uint32_t vbo;
	uint32_t ebo;
	uint32_t vao;

	struct
	{
		uint32_t def;
	} program;
} gl;

static uint32_t
program_module_compile(GLenum type, const char* file_path)
{
	std::ifstream file(file_path);
	std::stringstream sourcestream;
	std::string source;
	uint32_t shadermodule;
	const char* csource;

	sourcestream << file.rdbuf();
	source = sourcestream.str();
	csource = source.c_str();
	shadermodule = glCreateShader(type);
	glShaderSource(shadermodule, 1, &csource, NULL);
	glCompileShader(shadermodule);
	return shadermodule;
}

static uint32_t
program_new(const char* vertex_file_path, const char* fragment_file_path)
{
	uint32_t program;
	uint32_t fragment;
	uint32_t vertex;

	vertex = program_module_compile(GL_VERTEX_SHADER, vertex_file_path);
	fragment = program_module_compile(GL_FRAGMENT_SHADER, fragment_file_path);

	program = glCreateProgram();
	glAttachShader(program, vertex);
	glAttachShader(program, fragment);
	glLinkProgram(program);

	glDetachShader(program, vertex);
	glDetachShader(program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

int
r_glbegin(void)
{
	static float vertices[] =
	{
		 0.5f,  0.5f, 0.0f,
		 0.5f, -0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f
	};
	unsigned int indices[] =
	{
		0, 1, 3,
		1, 2, 3
	};

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	
	gl.program.def = program_new("default_vert.glsl", "default_frag.glsl");

	/* Allocate buffers. */
	glGenBuffers(1, &gl.vbo);
	glGenBuffers(1, &gl.ebo);
	glGenVertexArrays(1, &gl.vao);
	glBindVertexArray(gl.vao);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	return 0;
}

void
r_gltick(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(gl.program.def);
	glBindVertexArray(gl.vao);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void
r_glexit(void)
{

}
