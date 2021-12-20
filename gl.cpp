#include "global.hpp"
#include "gl.hpp"

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
};

struct material
{
	glm::vec3 ambient  = { 0.0f, 0.0f, 0.0f }; /* Ka */
	glm::vec3 diffuse  = { 0.0f, 0.0f, 0.0f }; /* Kd */
	glm::vec4 specular = { 0.0f, 0.0f, 0.0f, 20.0 }; /* Ks, Ns */
	int diffuse_map = 0;   /* map_Kd */
};

static struct
{
	uint32_t vbo;
	uint32_t vao;
	std::vector<struct object> object;
	std::map<std::string, struct material> material;

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
		} uniform;
	} program;
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

	proj = glm::perspective(glm::radians(45.0f), c->aspect, 0.01f, 8000.0f);
	c->viewproj = proj * view;

	{
		glm::mat4 inv = glm::inverse(view);

		c->position[0] = inv[3][0];
		c->position[1] = -inv[3][1];
		c->position[2] = inv[3][2];
	}


	//
	// Calculate projection matrix.
	//
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

	gl.program.uniform.mvp = glGetUniformLocation(program, "mvp");
	gl.program.uniform.eye = glGetUniformLocation(program, "eye");
	gl.program.uniform.ambient = glGetUniformLocation(program, "ambient_in");
	gl.program.uniform.specular = glGetUniformLocation(program, "specular_in");

	glDetachShader(program, vertex);
	glDetachShader(program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return program;
}

int
r_glbegin(void)
{
	glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	gl.program.id = program_new("default_vert.glsl", "default_frag.glsl");

	/* Load objects (parts). */
	{
		std::string workdir = R"(C:\Users\pengu\Documents\maya\projects\matf-rg\export\)";

		std::string line;
		std::ifstream f(workdir + "parts.obj");
		std::ifstream fm(workdir + "parts.mtl");
		std::vector<glm::vec3> buffer_vertex;
		std::vector<glm::vec2> buffer_uv;
		std::vector<glm::vec3> buffer_normal;
		std::vector<float> buffer_final;
		std::vector<std::string> buffer_name;
		uint32_t vfirst = 0;
		std::string active_material = "";

		/* Material library. */
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
			}
			else if (line.substr(0, 6) == "map_Kd")
			{
				std::istringstream iss(line.substr(7));
				std::string path;

				iss >> path;
			}
		}

		/* Mesh data. */
		while (std::getline(f, line))
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
					gl.object.push_back({});
					gl.object.at(gl.object.size() - 1).vfirst = vfirst;
					gl.object.at(gl.object.size() - 1).vcount = 0;
				}
			}
			else if (line[0] == 'v' && line[1] == ' ')
			{
				char c;
				float x, y, z;

				std::istringstream iss(line);
				if (!(iss >> c >> x >> y >> z))
				{
					break;
				}
				buffer_vertex.push_back(glm::vec3(x, y, z));
			}
			else if (line[0] == 'v' && line[1] == 't')
			{
				char c1, c2;
				float u, v;

				std::istringstream iss(line);
				if (!(iss >> c1 >> c2 >> u >> v))
				{
					break;
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
					break;
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
					break;
				}

				for (i = 0; i < 3; i++)
				{
					buffer_final.push_back(buffer_vertex[v[i] - 1].x);
					buffer_final.push_back(-buffer_vertex[v[i] - 1].y);
					buffer_final.push_back(buffer_vertex[v[i] - 1].z);
					buffer_final.push_back(buffer_uv[t[i] - 1].x);
					buffer_final.push_back(buffer_uv[t[i] - 1].y);
					buffer_final.push_back(buffer_normal[n[i] - 1].x);
					buffer_final.push_back(buffer_normal[n[i] - 1].y);
					buffer_final.push_back(buffer_normal[n[i] - 1].z);
				}

				vfirst += 3;
				gl.object.at(gl.object.size() - 1).vcount += 3;
			}
			else if (line[0] == 'u' && line[1] == 's' && line[2] == 'e' && line[3] == 'm' && line[4] == 't' && line[5] == 'l')
			{
				gl.object.at(gl.object.size() - 1).material = line.substr(7);
			}
		}

		glGenBuffers(1, &gl.vbo);
		glGenVertexArrays(1, &gl.vao);
		glBindVertexArray(gl.vao);
		glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * buffer_final.size(), buffer_final.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 + 2 + 3) * sizeof(float), (void*)(sizeof(float) * 0));
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (3 + 2 + 3) * sizeof(float), (void*)(sizeof(float) * 3));
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, (3 + 2 + 3) * sizeof(float), (void*)(sizeof(float) * 5));
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

	}

	/* Camera's initial state. */
	gl.trackball.aspect = 640.0f / 480.0f;
	gl.trackball.pitch = -3.14159f / 8.0f;
	gl.trackball.yaw = -3.14159f / 4.0f;
	gl.trackball.focus = { -64.0f, 0.0f, 0.0f };
	gl.trackball.radius = 64.0f;
	cam_trackball(&gl.trackball);

	return 0;
}

void
r_gltick(struct r_tick tick)
{
	const float radius_factor = powf(gl.trackball.radius / 10.0f, 1.225f);
	uint32_t i;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (tick.cursor.wheel != 0)
	{
		int sign = (tick.cursor.wheel > 0 ? -1 : 1);
		float d = sign * abs(radius_factor * tick.cursor.wheel) / 100.0f;

		gl.trackball.radius += d;
		cam_trackball(&gl.trackball);
	}
	std::cout << radius_factor << "\n";

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

	glUseProgram(gl.program.id);
	glUniformMatrix4fv(gl.program.uniform.mvp, 1, GL_FALSE, glm::value_ptr(gl.trackball.viewproj));
	glUniform3fv(gl.program.uniform.eye, 1, glm::value_ptr(gl.trackball.position));

	glBindVertexArray(gl.vao);
	for (i = 0; i < gl.object.size(); i++)
	{
		const struct material& m = gl.material.find(gl.object.at(i).material)->second;

		glUniform3fv(gl.program.uniform.ambient, 1, glm::value_ptr(m.ambient));
		glUniform4fv(gl.program.uniform.specular, 1, glm::value_ptr(m.specular));

		glDrawArrays(GL_TRIANGLES, gl.object[i].vfirst, gl.object[i].vcount);
	}
}

void
r_glexit(void)
{

}
