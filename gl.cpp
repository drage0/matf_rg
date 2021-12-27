#include "global.hpp"
#include "gl.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/*
 * Trackball parameters are used in cam_trackball_update to update the focus and translation vectors.
 * Ported from C.
 */
struct trackball
{
	/* Parameters defining the trackball state. */
	float radius;
	float pitch;
	float yaw;
	float panx;
	float pany;
	float aspect; /* Aspect ratio for viewing frustum. */
	glm::vec3 focus; /* Center of sphere. */

	/* Calcualted matrices after a call to cam_trackball. Right and up are orthonormal. */
	glm::mat4 viewproj;
	glm::mat4 viewproj_sky;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 front;
	glm::vec3 position;
	glm::vec3 p_x;
	glm::vec3 p_y;
	glm::vec3 p_z;
};

struct object
{
	uint32_t vcount;
	uint32_t vfirst;
	std::string material;
	std::string name;
	glm::vec3 explicit_position = { 0.0f, 0.0f, 0.0f };
};

struct billboard
{
	glm::vec3 position;
	glm::vec3 facing;
	uint32_t texture;
};

struct material
{
	glm::vec3 ambient = { 0.0f, 0.0f, 0.0f }; /* Ka */
	glm::vec3 diffuse = { 1.0f, 1.0f, 1.0f }; /* Kd */
	glm::vec4 specular = { 0.0f, 0.0f, 0.0f, 20.0 }; /* Ks, Ns */
	float transparency = 0.0f;
	uint32_t diffuse_texture = 0;   /* map_Kd */
	uint32_t normal_texture = 0;   /* bump */
};

static struct
{
	uint32_t vbo, vbo_sky, vbo_bb, vbo_line, vbo_ppfx;
	uint32_t vao, vao_sky, vao_bb, vao_line, vao_ppfx;
	std::vector<struct object> object;
	std::vector<struct object> object_transparent;
	std::vector<struct billboard> billboard;
	std::map<std::string, struct material> material;

	GLuint texture_white;
	GLuint texture_normal;
	GLuint texture_scene1;
	GLuint texture_scene2;
	GLuint texture_cubemap2;
	GLuint texture_cubemap;
	GLuint texture_billboard_sunguy;
	GLuint texture_fb_display;

	GLuint fb_display;

	GLuint rbo;

	struct trackball trackball;

	struct
	{
		uint32_t id;
		struct
		{
			uint32_t mvp;
			uint32_t eye;
			uint32_t ambient;
			uint32_t specular;
			uint32_t transparency;
			uint32_t diffuse;
			uint32_t distant_light_dir;
			uint32_t imgtexture;
			uint32_t normalmap;
			uint32_t model;
		} uniform;
	} program;

	struct
	{
		uint32_t id;
		struct
		{
			uint32_t mvp;
			uint32_t gamma;
		} uniform;
	} program_sky;

	struct
	{
		uint32_t id;
		struct
		{
			uint32_t mvp;
			uint32_t screenwh;
		} uniform;
	} program_bb;

	struct
	{
		uint32_t id;
		struct
		{
			uint32_t mvp;
			uint32_t colour;
		} uniform;
	} program_line;

	struct
	{
		uint32_t id;
		struct
		{
			uint32_t imgtexture;
			uint32_t display_resolution;
		} uniform;
	} program_display;

	enum scene scene;
} gl;

/*
 * Defines XY rotation (see axis_rot).
 *
 * R   - right
 * U   - up
 * B   - back (-front)
 * eye - position in parent coordinate system
 *           .                 .
 *          / Rx  Ux  Bx  eyex, \
 *	view = |  Ry  Uy  By  eyey,  |
 *	       |  Rz  Uz  Bz  eyez,  |
 *	       |  0   0   0   1      |
 *          \                   /
 */
static void
cam_trackball(struct trackball* c)
{
	glm::mat4 view, proj;
	glm::vec3 axis_rot;
	glm::vec3 point;

	proj = glm::perspective(glm::radians(45.0f), c->aspect, 0.01f, 8000.0f);

	view = glm::identity<glm::mat4>();

	// Y axis is up.
	view[1][1] = -1.0f;

	//
	// Rotate on XY axis.
	//

	axis_rot = { 1.0f, 0.0f, 0.0f };
	view = glm::rotate(view, c->pitch, axis_rot);

	axis_rot = { 0.0f, 1.0f, 0.0f };
	view = glm::rotate(view, c->yaw, axis_rot);

	//
	// Calculate axis on unit circle.
	// (p_x, p_y, p_z) for drawing rotated axis in viewport.
	//

	point[0] = 1.0f;
	point[1] = 0.0f;
	point[2] = 0.0f;
	c->p_x = glm::normalize(glm::vec3(view * glm::vec4(point, 1.0f)));

	point[0] = 0.0f;
	point[1] = 1.0f;
	point[2] = 0.0f;
	c->p_y = glm::normalize(glm::vec3(view * glm::vec4(point, 1.0f)));

	point[0] = 0.0f;
	point[1] = 0.0f;
	point[2] = 1.0f;
	c->p_z = glm::normalize(glm::vec3(view * glm::vec4(point, 1.0f)));

	// Translate on Z axis by radius.
	view[3][2] = -0.02f;
	c->viewproj_sky = proj * view;
	view[3][2] = -c->radius;
	view = glm::translate(view, c->focus);

	//
	// Record data for future use.
	//

	c->right[0] = view[0][0];
	c->right[1] = view[1][0];
	c->right[2] = view[2][0];

	c->up[0] = view[0][1];
	c->up[1] = view[1][1];
	c->up[2] = view[2][1];

	c->front[0] = -view[0][2];
	c->front[1] = -view[1][2];
	c->front[2] = -view[2][2];

	c->viewproj = proj * view;

	{
		glm::mat4 inv = glm::inverse(view);

		c->position[0] = inv[3][0];
		c->position[1] = -inv[3][1];
		c->position[2] = inv[3][2];
	}
}

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
program_new(const char* vertex_file_path, const char* fragment_file_path, int which)
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

	// TODO obrisi ovo
	if (which == 0)
	{
		gl.program.uniform.mvp = glGetUniformLocation(program, "mvp");
		gl.program.uniform.eye = glGetUniformLocation(program, "eye");
		gl.program.uniform.ambient = glGetUniformLocation(program, "ambient_in");
		gl.program.uniform.specular = glGetUniformLocation(program, "specular_in");
		gl.program.uniform.transparency = glGetUniformLocation(program, "transparency_in");
		gl.program.uniform.diffuse = glGetUniformLocation(program, "diffuse_in");
		gl.program.uniform.distant_light_dir = glGetUniformLocation(program, "distant_light_dir_in");
		gl.program.uniform.imgtexture = glGetUniformLocation(program, "imgtexture");
		gl.program.uniform.normalmap = glGetUniformLocation(program, "normalmap");
		gl.program.uniform.model = glGetUniformLocation(program, "model");
	}
	else if (which == 1)
	{
		gl.program_sky.uniform.mvp = glGetUniformLocation(program, "mvp");
		gl.program_sky.uniform.gamma = glGetUniformLocation(program, "gamma");
	}
	else if (which == 2)
	{
		gl.program_bb.uniform.mvp = glGetUniformLocation(program, "mvp");
		gl.program_bb.uniform.screenwh = glGetUniformLocation(program, "screenwh");
	}
	else if (which == 3)
	{
		gl.program_line.uniform.mvp = glGetUniformLocation(program, "mvp");
		gl.program_line.uniform.colour = glGetUniformLocation(program, "colour");
	}
	else if (which == 4)
	{
		gl.program_display.uniform.imgtexture = glGetUniformLocation(program, "imgtexture");
		gl.program_display.uniform.display_resolution = glGetUniformLocation(program, "display_resolution");
	}

	glDetachShader(program, vertex);
	glDetachShader(program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

int
r_newscene(enum scene scene)
{
	const std::string workdir = R"(C:\Users\pengu\Documents\maya\projects\matf-rg\export\)";
	std::string line, active_material = "none", active_name = "none";
	std::ifstream fp, fm;
	std::vector<glm::vec3> buffer_vertex;
	std::vector<glm::vec2> buffer_uv;
	std::vector<glm::vec3> buffer_normal;
	std::vector<float> buffer_final;
	std::vector<float> buffer_line_final;
	uint32_t vfirst = 0;

	gl.trackball.aspect = 1680.0f / 1050.0f;
	switch (scene)
	{
	default:
		return 1;
		break;
	case scene::SCENE_VOID:
		gl.trackball.pitch = -3.14159f / 8.0f;
		gl.trackball.yaw = -3.14159f / 4.0f;
		gl.trackball.focus = { 0.0f, 0.0f, 0.0f };
		gl.trackball.radius = 8.0f;
		gl.billboard[0].position = { 0.0f, 0.0f, 0.0f };
		break;
	case scene::SCENE_ROOM:
		fp.open(workdir + "parts.obj");
		fm.open(workdir + "parts.mtl");
		gl.trackball.pitch = -3.14159f / 8.0f;
		gl.trackball.yaw = -3.14159f / 4.0f;
		gl.trackball.focus = { 20.0f, 6.0f, 4.0f };
		gl.trackball.radius = 64.0f;
		gl.billboard[0].position = { 6.0f, -14.0f, 11.0f };
		break;
	case scene::SCENE_PRIMITIVES:
		fp.open(workdir + "parts2.obj");
		fm.open(workdir + "parts2.mtl");
		gl.trackball.pitch = -3.14159f / 8.0f;
		gl.trackball.yaw = -3.14159f / 4.0f;
		gl.trackball.focus = { 0.0f, 0.0f, 0.0f };
		gl.trackball.radius = 16.0f;
		gl.billboard[0].position = { 4.0f, -10.0f, 2.0f };
		break;
	}
	gl.billboard[0].facing = -gl.billboard[0].position;
	gl.billboard[0].facing.y *= -1;
	if (glm::length(gl.billboard[0].facing) > 0.0f)
	{
		gl.billboard[0].facing = glm::normalize(gl.billboard[0].facing);
	}
	else
	{
		gl.billboard[0].facing = glm::vec3(0.0f, 0.0f, 1.0f);
	}
	cam_trackball(&gl.trackball);

	if (gl.scene == scene && scene != scene::SCENE_VOID)
	{
		return 0;
	}

	for (auto& m : gl.material)
	{
		if (m.second.diffuse_texture)
		{
			glDeleteTextures(1, &m.second.diffuse_texture);
		}
	}
	gl.material.clear();
	gl.object.clear();
	gl.object_transparent.clear();

	/* Material library. */
	if (fm.is_open())
	{
		while (std::getline(fm, line))
		{
			if (line.empty())
			{
				continue;
			}

			if (line.substr(0, 6) == "newmtl")
			{
				active_material = line.substr(7);
				std::cout << "mat: " << active_material << std::endl;
			}
			else if (line.substr(0, 2) == "Ka")
			{
				std::istringstream iss(line.substr(3));

				iss >> gl.material[active_material].ambient.r >> gl.material[active_material].ambient.g >> gl.material[active_material].ambient.b;
			}
			else if (line.substr(0, 2) == "Kd")
			{
				std::istringstream iss(line.substr(3));

				iss >> gl.material[active_material].diffuse.r >> gl.material[active_material].diffuse.g >> gl.material[active_material].diffuse.b;
			}
			else if (line.substr(0, 2) == "Ks")
			{
				std::istringstream iss(line.substr(3));

				iss >> gl.material[active_material].specular.r >> gl.material[active_material].specular.g >> gl.material[active_material].specular.b;
			}
			else if (line.substr(0, 2) == "Ns")
			{
				std::istringstream iss(line.substr(3));

				iss >> gl.material[active_material].specular.a;
				gl.material[active_material].specular.a *= 1;
			}
			else if (line.substr(0, 2) == "Tf")
			{
				float unused, unused2;
				std::istringstream iss(line.substr(3));

				iss >> gl.material[active_material].transparency >> unused >> unused2;
				gl.material[active_material].transparency = 1.0f - gl.material[active_material].transparency;
			}
			else if (line.substr(0, 6) == "map_Kd")
			{
				std::istringstream iss(line.substr(7));
				std::string path;
				unsigned char* data = nullptr;
				GLuint texture;
				int w = 0, h = 0, c = 0;

				iss >> path;

				/* Example -mm_0.7181_0.214286_NameOfFile.jpg */
				if (path[0] == '-')
				{
					size_t i1 = path.find('_');
					size_t i2 = path.find('_', i1 + 1);
					size_t i3 = path.find('_', i2 + 1);

					path = path.substr(i3 + 1);
				}

				data = stbi_load((workdir + path).c_str(), &w, &h, &c, 0);
				glGenTextures(1, &texture);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glGenerateMipmap(GL_TEXTURE_2D);
				stbi_image_free(data);
				gl.material[active_material].diffuse_texture = texture;
				gl.material[active_material].diffuse = { 1.0f, 1.0f, 1.0f };
			}
			else if (line.substr(0, 4) == "bump")
			{
				/* bump  -bm 1 WoodFlooring14_NRM_6K.jpg*/
				std::string path = line.substr(12);
				unsigned char* data = nullptr;
				GLuint texture;
				int w, h, c;

				data = stbi_load((workdir + path).c_str(), &w, &h, &c, 0);
				glGenTextures(1, &texture);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glGenerateMipmap(GL_TEXTURE_2D);
				stbi_image_free(data);
				gl.material[active_material].normal_texture = texture;
			}
			else
			{
				std::cout << "noop " << line << std::endl;
			}
		}
	}

	/* Mesh data. */
	if (fp.is_open())
	{
		while (std::getline(fp, line))
		{
			if (line.empty() || line[0] == '#')
			{
				continue;
			}

			if (line[0] == 'g')
			{
				std::string name = line.substr(2);

				if (name != "default")
				{
					/*
					if (!gl.object.empty() && gl.object.at(gl.object.size() - 1).material == "")
					{
						std::cout << "total vertices: " << gl.object.at(gl.object.size() - 1).vcount << std::endl;
					}
					*/
					gl.object.push_back({});
					gl.object.at(gl.object.size() - 1).vfirst = vfirst;
					gl.object.at(gl.object.size() - 1).vcount = 0;
					gl.object.at(gl.object.size() - 1).name = name;
					std::cout << name << std::endl;
				}
			}
			else if (line[0] == 'v' && line[1] == ' ')
			{
				char c;
				float x, y, z;

				std::istringstream iss(line);
				if (!(iss >> c >> x >> y >> z))
				{
					return 1;
				}
				y *= -1;

				buffer_vertex.push_back(glm::vec3(x, y, z));
			}
			else if (line[0] == 'v' && line[1] == 't')
			{
				char c1, c2;
				float u, v;

				std::istringstream iss(line);
				if (!(iss >> c1 >> c2 >> u >> v))
				{
					return 1;
				}
				buffer_uv.push_back(glm::vec2(u, v));
			}
			else if (line[0] == 'v' && line[1] == 'n')
			{
				char c1, c2;
				float x, y, z;

				std::istringstream iss(line);
				if (!(iss >> c1 >> c2 >> x >> y >> z))
				{
					return 1;
				}
				buffer_normal.push_back(glm::vec3(x, y, z));
			}
			else if (line[0] == 'f')
			{
				char c;
				int i, v[3], t[3], n[3];

				std::istringstream iss(line);
				if (!(iss >> c >> v[0] >> c >> t[0] >> c >> n[0] >> v[1] >> c >> t[1] >> c >> n[1] >> v[2] >> c >> t[2] >> c >> n[2]))
				{
					return 1;
				}

				/* Tangent and bitagent. */
				glm::vec3 pos1 = buffer_vertex[v[0] - 1];
				glm::vec3 pos2 = buffer_vertex[v[1] - 1];
				glm::vec3 pos3 = buffer_vertex[v[2] - 1];
				glm::vec2 uv1 = buffer_uv[t[0] - 1];
				glm::vec2 uv2 = buffer_uv[t[1] - 1];
				glm::vec2 uv3 = buffer_uv[t[2] - 1];

				glm::vec3 edge1 = pos2 - pos1;
				glm::vec3 edge2 = pos3 - pos1;
				glm::vec2 deltaUV1 = uv2 - uv1;
				glm::vec2 deltaUV2 = uv3 - uv1;
				float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

				glm::vec3 tangent1;
				glm::vec3 bitangent1;

				tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
				tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
				tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

				bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
				bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
				bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

				for (i = 0; i < 3; i++)
				{
					buffer_final.push_back(buffer_vertex[v[i] - 1].x);
					buffer_final.push_back(buffer_vertex[v[i] - 1].y);
					buffer_final.push_back(buffer_vertex[v[i] - 1].z);
					buffer_final.push_back(buffer_uv[t[i] - 1].x);
					buffer_final.push_back(buffer_uv[t[i] - 1].y);
					buffer_final.push_back(buffer_normal[n[i] - 1].x);
					buffer_final.push_back(buffer_normal[n[i] - 1].y);
					buffer_final.push_back(buffer_normal[n[i] - 1].z);
					buffer_final.push_back(tangent1.x);
					buffer_final.push_back(tangent1.y);
					buffer_final.push_back(tangent1.z);
					buffer_final.push_back(bitangent1.x);
					buffer_final.push_back(bitangent1.y);
					buffer_final.push_back(bitangent1.z);
				}

				vfirst += 3;
				gl.object.at(gl.object.size() - 1).vcount += 3;
			}
			else if (line[0] == 'u')
			{
				gl.object.at(gl.object.size() - 1).material = line.substr(7);
			}
			else
			{
				std::cout << "noop " << line << std::endl;
			}
		}
	}

	{
		auto it = gl.object.begin();

		while (it != gl.object.end())
		{
			if (gl.material[it->material].transparency > 0.0f)
			{
				gl.object_transparent.push_back(*it);
				it = gl.object.erase(it);
			}
			else
			{
				it++;
			}
		}
	}

	if (gl.vbo)
	{
		glDeleteBuffers(1, &gl.vbo);
		glDeleteVertexArrays(1, &gl.vao);
		glDeleteBuffers(1, &gl.vbo_line);
		glDeleteVertexArrays(1, &gl.vao_line);
	}
	glGenBuffers(1, &gl.vbo);
	glGenVertexArrays(1, &gl.vao);
	glBindVertexArray(gl.vao);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buffer_final.size(), buffer_final.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 + 2 + 3 + 3 + 3) * sizeof(float), (void*)(sizeof(float) * (0)));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (3 + 2 + 3 + 3 + 3) * sizeof(float), (void*)(sizeof(float) * (3)));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (3 + 2 + 3 + 3 + 3) * sizeof(float), (void*)(sizeof(float) * (3 + 2)));
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, (3 + 2 + 3 + 3 + 3) * sizeof(float), (void*)(sizeof(float) * (3 + 2 + 3)));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, (3 + 2 + 3 + 3 + 3) * sizeof(float), (void*)(sizeof(float) * (3 + 2 + 3 + 3)));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);

	buffer_line_final.push_back(gl.billboard[0].position.x);
	buffer_line_final.push_back(gl.billboard[0].position.y);
	buffer_line_final.push_back(gl.billboard[0].position.z);
	buffer_line_final.push_back(gl.billboard[0].position.x+ gl.billboard[0].facing.x * 2);
	buffer_line_final.push_back(gl.billboard[0].position.y - gl.billboard[0].facing.y * 2);
	buffer_line_final.push_back(gl.billboard[0].position.z + gl.billboard[0].facing.z * 2);
	glGenBuffers(1, &gl.vbo_line);
	glGenVertexArrays(1, &gl.vao_line);
	glBindVertexArray(gl.vao_line);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo_line);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buffer_line_final.size(), buffer_line_final.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3) * sizeof(float), (void*)(sizeof(float) * 0));
	glEnableVertexAttribArray(0);

	gl.scene = scene;

	return 0;
}

int
r_glbegin(void)
{
	static const uint8_t white[4 * 2 * 2] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
	static float skybox_vertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	static const float billboard_vertices[] =
	{
		-1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 0.0f, 1.0f,

		1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	static const float display_vertices[] =
	{
		-1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 0.0f, 0.0f, 1.0f,

		1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};
	struct billboard billboard;
	unsigned char* data = nullptr;
	int w = 0, h = 0, c = 0;

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_FRAMEBUFFER_SRGB);

	//
	// Framebuffer display
	//
	glGenFramebuffers(1, &gl.fb_display);
	glBindFramebuffer(GL_FRAMEBUFFER, gl.fb_display);

	glGenTextures(1, &gl.texture_fb_display);
	glBindTexture(GL_TEXTURE_2D, gl.texture_fb_display);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1680, 1050, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl.texture_fb_display, 0);

	glGenRenderbuffers(1, &gl.rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, gl.rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1680, 1050);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl.rbo);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1680, 1050, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	gl.program.id = program_new("rom/program/default_vert.glsl", "rom/program/default_frag.glsl", 0);
	gl.program_sky.id = program_new("rom/program/skybox_vert.glsl", "rom/program/skybox_frag.glsl", 1);
	gl.program_bb.id = program_new("rom/program/billboard_vert.glsl", "rom/program/billboard_frag.glsl", 2);
	gl.program_line.id = program_new("rom/program/line_vert.glsl", "rom/program/line_frag.glsl", 3);
	gl.program_display.id = program_new("rom/program/ppfx_vert.glsl", "rom/program/ppfx_frag.glsl", 4);

	glGenTextures(1, &gl.texture_white);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl.texture_white);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, white);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	data = stbi_load("rom/normal.jpg", &w, &h, &c, 0);
	glGenTextures(1, &gl.texture_normal);
	glBindTexture(GL_TEXTURE_2D, gl.texture_normal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	data = stbi_load("rom/scene1.jpg", &w, &h, &c, 0);
	glGenTextures(1, &gl.texture_scene1);
	glBindTexture(GL_TEXTURE_2D, gl.texture_scene1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);
	data = stbi_load("rom/scene2.jpg", &w, &h, &c, 0);
	glGenTextures(1, &gl.texture_scene2);
	glBindTexture(GL_TEXTURE_2D, gl.texture_scene2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);

	billboard.position = { 0.0f, 0.0f, 0.0f };
	data = stbi_load("rom/sun.png", &w, &h, &c, 0);
	if (!data) { std::cout << "rom/sun issue\n"; }
	glGenTextures(1, &billboard.texture);
	glBindTexture(GL_TEXTURE_2D, billboard.texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	stbi_image_free(data);
	gl.billboard.push_back(billboard);

	glGenTextures(1, &gl.texture_cubemap2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, gl.texture_cubemap2);
	for (uint32_t i = 0; i < 6; i++)
	{
		static const char* cubemap_picture[] = { "rom/cbm_left.jpg", "rom/cbm_right.jpg", "rom/cbm_top.jpg", "rom/cbm_bottom.jpg", "rom/cbm_back.jpg", "rom/cbm_front.jpg", };

		data = stbi_load(cubemap_picture[i], &w, &h, &c, 0);
		if (!data)
		{
			std::cout << "cubemap issue\n";
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenTextures(1, &gl.texture_cubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, gl.texture_cubemap);
	for (uint32_t i = 0; i < 6; i++)
	{
		static const char* cubemap_picture[] = { "rom/cbb_right.jpg", "rom/cbb_left.jpg", "rom/cbb_top.jpg", "rom/cbm_bottom.jpg", "rom/cbb_front.jpg", "rom/cbb_back.jpg", };

		data = stbi_load(cubemap_picture[i], &w, &h, &c, 0);
		if (!data)
		{
			std::cout << "cubemap issue\n";
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		stbi_image_free(data);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glGenBuffers(1, &gl.vbo_sky);
	glGenVertexArrays(1, &gl.vao_sky);
	glBindVertexArray(gl.vao_sky);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo_sky);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3) * sizeof(float), (void*)(sizeof(float) * 0));
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &gl.vbo_bb);
	glGenVertexArrays(1, &gl.vao_bb);
	glBindVertexArray(gl.vao_bb);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo_bb);
	glBufferData(GL_ARRAY_BUFFER, sizeof(billboard_vertices), billboard_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 + 2) * sizeof(float), (void*)(sizeof(float) * 0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (3 + 2) * sizeof(float), (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &gl.vbo_ppfx);
	glGenVertexArrays(1, &gl.vao_ppfx);
	glBindVertexArray(gl.vao_ppfx);
	glBindBuffer(GL_ARRAY_BUFFER, gl.vbo_ppfx);
	glBufferData(GL_ARRAY_BUFFER, sizeof(display_vertices), display_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 + 2) * sizeof(float), (void*)(sizeof(float) * 0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (3 + 2) * sizeof(float), (void*)(sizeof(float) * 3));
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glViewport(0, 0, 1680, 1050);

	return r_newscene(scene::SCENE_VOID);
}

void
r_gltick(struct r_tick tick)
{
	const float radius_factor = powf(gl.trackball.radius / 10.0f, 1.225f);
	uint32_t i;

	if (tick.cursor.wheel != 0)
	{
		int sign = (tick.cursor.wheel > 0 ? -1 : 1);
		float d = sign * abs(radius_factor * tick.cursor.wheel) / 100.0f;

		gl.trackball.radius += d;
		cam_trackball(&gl.trackball);
	}

	switch (tick.cursor.mode)
	{
	default:
	case CURSOR_MODE_STAGNANT:
		break;
	case CURSOR_MODE_ORBIT:
		gl.trackball.yaw -= tick.cursor.dx / 100.0f;
		gl.trackball.pitch += tick.cursor.dy / 100.0f;
		cam_trackball(&gl.trackball);
		break;
	case CURSOR_MODE_PAN:
		gl.trackball.focus[0] -= radius_factor / 5.0f * gl.trackball.right[0] * (tick.cursor.dx / 10.0f) - radius_factor / 5.0f * gl.trackball.up[0] * (tick.cursor.dy / 10.0f);
		gl.trackball.focus[1] += radius_factor / 5.0f * gl.trackball.up[1] * (tick.cursor.dy / 10.0f);
		gl.trackball.focus[2] -= radius_factor / 5.0f * gl.trackball.right[2] * (tick.cursor.dx / 10.0f) - radius_factor / 5.0f * gl.trackball.up[2] * (tick.cursor.dy / 10.0f);
		cam_trackball(&gl.trackball);
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, gl.fb_display);

	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glFrontFace(GL_CW);
	glUseProgram(gl.program_sky.id);
	glUniformMatrix4fv(gl.program_sky.uniform.mvp, 1, GL_FALSE, glm::value_ptr(gl.trackball.viewproj_sky));
	glBindVertexArray(gl.vao_sky);
	switch (gl.scene)
	{
	default:
	case scene::SCENE_ROOM:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, gl.texture_cubemap);
		glUniform1f(gl.program_sky.uniform.gamma, 1.0f);
		break;
	case scene::SCENE_PRIMITIVES:
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, gl.texture_cubemap2);
		glUniform1f(gl.program_sky.uniform.gamma, 0.9f);
		break;
	}
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDepthMask(GL_TRUE);
	glFrontFace(GL_CCW);

	glUseProgram(gl.program_bb.id);
	glBindVertexArray(gl.vao_bb);
	glUniform2fv(gl.program_bb.uniform.screenwh, 1, glm::value_ptr(glm::vec2(1680.0f, 1050.0f)));
	for (i = 0; i < gl.billboard.size(); i++)
	{
		glm::mat4 mvp;
		glm::mat4 model = glm::identity<glm::mat4>();

		model = glm::translate(model, gl.billboard[i].position);
		model = glm::rotate(model, gl.trackball.yaw + 3.1415f, glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::rotate(model, gl.trackball.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		mvp = gl.trackball.viewproj * model;

		glUniformMatrix4fv(gl.program_bb.uniform.mvp, 1, GL_FALSE, glm::value_ptr(mvp));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gl.billboard[i].texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	glUseProgram(gl.program_line.id);
	glBindVertexArray(gl.vao_line);
	glUniformMatrix4fv(gl.program_line.uniform.mvp, 1, GL_FALSE, glm::value_ptr(gl.trackball.viewproj));
	for (i = 0; i < gl.billboard.size(); i++)
	{
		glUniform3fv(gl.program_line.uniform.colour, 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
		glDrawArrays(GL_LINES, 0, 2);
	}

	glUseProgram(gl.program.id);
	glUniformMatrix4fv(gl.program.uniform.mvp, 1, GL_FALSE, glm::value_ptr(gl.trackball.viewproj));
	glUniform3fv(gl.program.uniform.eye, 1, glm::value_ptr(gl.trackball.position));
	glUniform3fv(gl.program.uniform.distant_light_dir, 1, glm::value_ptr(glm::vec3(gl.billboard[0].position.x, -gl.billboard[0].position.y, gl.billboard[0].position.z)));
	glUniformMatrix4fv(gl.program.uniform.model, 1, GL_FALSE, glm::value_ptr(glm::identity<glm::mat4>()));
	glUniform1i(gl.program.uniform.imgtexture, 0);
	glUniform1i(gl.program.uniform.normalmap, 1);

	glBindVertexArray(gl.vao);
	for (i = 0; i < gl.object.size(); i++)
	{
		const struct material& m = gl.material.find(gl.object.at(i).material)->second;

		glUniform3fv(gl.program.uniform.ambient, 1, glm::value_ptr(m.ambient));
		glUniform4fv(gl.program.uniform.specular, 1, glm::value_ptr(m.specular));
		glUniform3fv(gl.program.uniform.diffuse, 1, glm::value_ptr(m.diffuse));
		glUniform1f(gl.program.uniform.transparency, m.transparency);

		if (m.diffuse_texture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m.diffuse_texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m.normal_texture);
		}
		else
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gl.texture_white);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gl.texture_normal);
		}

		glDrawArrays(GL_TRIANGLES, gl.object[i].vfirst, gl.object[i].vcount);
	}

	for (i = 0; i < gl.object_transparent.size(); i++)
	{
		if (gl.object_transparent[i].name == "pCube1") { gl.object_transparent[i].explicit_position = { -5.0f, 0.0f, 0.0f }; }
		else if (gl.object_transparent[i].name == "pCylinder1") { gl.object_transparent[i].explicit_position = { 5.0f, 0.0f, 0.0f }; }
		else if (gl.object_transparent[i].name == "pCone1") { gl.object_transparent[i].explicit_position = { 0.0f, 0.0f, -5.0f }; }
		else if (gl.object_transparent[i].name == "pTorus1") { gl.object_transparent[i].explicit_position = { 0.0f, 0.0f, 5.0f }; }
	}
	{
		struct object_compare
		{
			inline bool operator() (const struct object& o1, const struct object& o2)
			{
				float d1 = glm::distance(o1.explicit_position, gl.trackball.position);
				float d2 = glm::distance(o2.explicit_position, gl.trackball.position);

				return (d2 < d1);
			}
		};

		std::sort(gl.object_transparent.begin(), gl.object_transparent.end(), object_compare());
	}

	glFrontFace(GL_CW);
	for (i = 0; i < gl.object_transparent.size(); i++)
	{
		const struct material& m = gl.material.find(gl.object_transparent.at(i).material)->second;
		glm::mat4 mvp;
		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::translate(model, gl.object_transparent[i].explicit_position);
		mvp = gl.trackball.viewproj * model;

		glUniformMatrix4fv(gl.program.uniform.model, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(gl.program.uniform.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform3fv(gl.program.uniform.diffuse, 1, glm::value_ptr(m.diffuse));
		glUniform3fv(gl.program.uniform.ambient, 1, glm::value_ptr(m.ambient));
		glUniform4fv(gl.program.uniform.specular, 1, glm::value_ptr(m.specular));
		glUniform1f(gl.program.uniform.transparency, m.transparency);

		if (m.diffuse_texture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m.diffuse_texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m.normal_texture);
		}
		else
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gl.texture_white);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gl.texture_normal);
		}

		glDrawArrays(GL_TRIANGLES, gl.object_transparent[i].vfirst, gl.object_transparent[i].vcount);
	}
	glFrontFace(GL_CCW);
	for (i = 0; i < gl.object_transparent.size(); i++)
	{
		const struct material& m = gl.material.find(gl.object_transparent.at(i).material)->second;
		glm::mat4 mvp;
		glm::mat4 model = glm::identity<glm::mat4>();
		model = glm::translate(model, gl.object_transparent[i].explicit_position);
		mvp = gl.trackball.viewproj * model;

		std::cout << gl.object_transparent[i].name << "\n";

		glUniformMatrix4fv(gl.program.uniform.model, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(gl.program.uniform.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
		glUniform3fv(gl.program.uniform.ambient, 1, glm::value_ptr(m.ambient));
		glUniform3fv(gl.program.uniform.diffuse, 1, glm::value_ptr(m.diffuse));
		glUniform4fv(gl.program.uniform.specular, 1, glm::value_ptr(m.specular));
		glUniform1f(gl.program.uniform.transparency, m.transparency);

		if (m.diffuse_texture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m.diffuse_texture);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, m.normal_texture);
		}
		else
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, gl.texture_white);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gl.texture_normal);
		}

		glDrawArrays(GL_TRIANGLES, gl.object_transparent[i].vfirst, gl.object_transparent[i].vcount);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(gl.program_display.id);
	glBindVertexArray(gl.vbo_ppfx);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(gl.program_display.uniform.imgtexture, 0);
	glUniform2fv(gl.program_display.uniform.display_resolution, 1, glm::value_ptr(glm::vec2(1680.0f, 1050.0f)));
	glBindTexture(GL_TEXTURE_2D, gl.texture_fb_display);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glEnable(GL_DEPTH_TEST);
}

void
r_glexit(void)
{
}
