// anim.cpp version 5.0 -- Template code for drawing an articulated figure.  CS 174A.

#define NOMINMAX

#ifdef _WIN32
#pragma comment(lib, "GL/Win32/freeglut.lib")
#pragma comment(lib, "GL/Win32/glew32s.lib")
#pragma comment(lib, "GL/Win32/glew32mxs.lib")
#else
#pragma comment(lib, "GL/x64/freeglut.lib")
#pragma comment(lib, "GL/x64/glew32s.lib")
#pragma comment(lib, "GL/x64/glew32mxs.lib")
#endif

#define _CRT_SECURE_NO_DEPRECATE
#ifndef EMSCRIPTEN
#include <Windows.h>
#define GLEW_STATIC
#include "..\GL\glew.h"
#endif
#define GL_GLEXT_PROTOTYPES
#include "..\GL\freeglut.h"

#include <math.h>
#include <assert.h>
#include <cmath>
#include <string>
#include <vector>

using namespace std;

extern GLuint LoadVertexBuffer(GLsizei size_bytes, float* verts);

#include "../math/ArcBall.h"
#include "../math/FrameSaver.h"
#include "../math/Timer.h"
#include "../math/Shapes.h"
#include "../math/mat.h"
#include "../math/vec.h"
#include "../math/InitShaders.h"

#include "maths.h"
#include "butterfly.h"
#include "ocean.h"
#include "blade.h"
#include "heightmap.h"
#include "heightmap_procs.h"
#include "spline.h"

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextProfile(a)
#define glewExperimental int glewExperimentalAPPLE
#define glewInit()
#endif

FrameSaver FrSaver;
Timer TM;

int Width = 800, Height = 800;
float Zoom = 1;

int Animate = 1, Recording = 0;

const int STRLEN = 100;
typedef char STR[STRLEN];

#define X 0
#define Y 1
#define Z 2

struct Program
{
	vector<string> m_preprocs;
	GLuint m_program;

	GLint m_attrib_position, m_attrib_normal, m_attrib_flex;

	GLint m_uniform_model, m_uniform_projection, m_uniform_view,
		m_uniform_ambient, m_uniform_diffuse, m_uniform_specular, m_uniform_shininess,
		m_uniform_camera_position, m_uniform_time, m_uniform_flex_length, m_uniform_alpha;
};

typedef enum
{
	SHADER_DEFAULT = 0,
	SHADER_FLEX = 1,
	SHADER_NOFIXUP = 2,
	SHADER_FADE = 3,
	MAX_SHADERS,
};
Program g_programs[MAX_SHADERS];

extern void set_color(int program, vec3 ambient, vec3 diffuse, vec3 specular, float s);
extern void SetupProgram(int program);

ShapeData g_sphere_data, g_cylinder_data;

double TIME = 0.0;

GLuint g_butterfly_vbo;
GLuint g_ocean_vbo;
GLuint g_blade_vbo;

float g_yaw;
float g_pitch;

double g_simulation_time = 0;

#ifdef _DEBUG
double g_target_framerate = 1 / 30.0;
#else
double g_target_framerate = 1 / 60.0;
#endif
double g_dt;

Angel::vec3 g_player_position;
Angel::vec3 g_velocity;
bool g_splining = true;

point3 g_overlay[6] = {
	vec3(-1.0, -1.0, 0.0),
	vec3(1.0, -1.0, 0.0),
	vec3(1.0, 1.0, 0.0),
	vec3(-1.0, -1.0, 0.0),
	vec3(1.0, 1.0, 0.0),
	vec3(-1.0, 1.0, 0.0),
};
GLuint g_overlay_vbo;

#include "boids.h"
#include "boids_procs.h" // I don't want to struggle with the project files.
#include "seagrass.h"
#include "seagrass_render.h"
#include "seaweed.h"

void instructions()
{
	printf("Press:\n");
	printf("  s to save the image\n");
	printf("  r to restore the original view.\n");
	printf("  0 to set it to the zero state.\n");
	printf("  a to toggle the animation.\n");
	printf("  m to toggle frame dumping.\n");
	printf("  q to quit.\n");
}

void SetProjectionMatrix(int program)
{
	mat4 projection = Perspective(50.0f, (float)Width / (float)Height, 0.01f, 1000.0f);
	glUniformMatrix4fv(g_programs[program].m_uniform_projection, 1, GL_FALSE, transpose(projection));
}

void SetupProgram(int program)
{
	glUseProgram(g_programs[program].m_program);

	glUniform3f(g_programs[program].m_uniform_camera_position, g_player_position.x, g_player_position.y, g_player_position.z);

	mat4 view;

	if (g_splining)
		view = LookAt(g_player_position, vec3(15, 0, -6), vec3(0, 0, 1));
	else
		view = LookAt(g_player_position, g_player_position + AngleVector(g_pitch, g_yaw), vec3(0, 0, 1));

	glUniformMatrix4fv(g_programs[program].m_uniform_view, 1, GL_FALSE, transpose(view));

	glUniform1f(g_programs[program].m_uniform_time, (float)g_simulation_time);

	SetProjectionMatrix(program);
}

void RecompileShader(bool fail_means_die)
{
#ifndef EMSCRIPTEN
	string directory = "../BlueOcean/";
#else
	string directory = "";
#endif

	string vshader = directory + "vshader.glsl";
	string fshader = directory + "fshader.glsl";
	string shared = directory + "shared.glsl";

	for (int i = 0; i < MAX_SHADERS; i++)
	{
		GLuint program = InitShader(vshader.c_str(), fshader.c_str(), shared.c_str(), g_programs[i].m_preprocs, fail_means_die);

		if (!program)
			continue;

		g_programs[i].m_program = program;

		glUseProgram(program);

		g_programs[i].m_attrib_position = glGetAttribLocation(program, "vPosition");
		g_programs[i].m_attrib_normal = glGetAttribLocation(program, "vNormal");
		g_programs[i].m_attrib_flex = glGetAttribLocation(program, "vFlex");

		g_programs[i].m_uniform_model = glGetUniformLocation(program, "Model");
		g_programs[i].m_uniform_projection = glGetUniformLocation(program, "Projection");
		g_programs[i].m_uniform_view = glGetUniformLocation(program, "View");

		g_programs[i].m_uniform_ambient = glGetUniformLocation(program, "AmbientProduct");
		g_programs[i].m_uniform_diffuse = glGetUniformLocation(program, "DiffuseProduct");
		g_programs[i].m_uniform_specular = glGetUniformLocation(program, "SpecularProduct");
		g_programs[i].m_uniform_shininess = glGetUniformLocation(program, "Shininess");
		g_programs[i].m_uniform_camera_position = glGetUniformLocation(program, "camera_position");
		g_programs[i].m_uniform_time = glGetUniformLocation(program, "time");
		g_programs[i].m_uniform_flex_length = glGetUniformLocation(program, "flex_length");
		g_programs[i].m_uniform_alpha = glGetUniformLocation(program, "alpha");
	}

	glUseProgram(g_programs[SHADER_DEFAULT].m_program);

	glUniform3f(g_programs[SHADER_DEFAULT].m_uniform_ambient, 0.2f, 0.2f, 0.2f);
	glUniform3f(g_programs[SHADER_DEFAULT].m_uniform_diffuse, 0.6f, 0.6f, 0.6f);
	glUniform3f(g_programs[SHADER_DEFAULT].m_uniform_specular, 0.2f, 0.2f, 0.2f);
	glUniform1f(g_programs[SHADER_DEFAULT].m_uniform_shininess, 100.0f);

	SetProjectionMatrix(SHADER_DEFAULT);
}

// this function gets called for any keypresses
void myKey(unsigned char key, int x, int y)
{
	switch (key) {
	case 'q':
	case 27:
		exit(0);
	case 'r':
		RecompileShader(false);
		break;
	case '0':
		//reset your object
		break;
	case 'm':
		if (Recording == 1)
		{
			printf("Frame recording disabled.\n");
			Recording = 0;
		}
		else
		{
			printf("Frame recording enabled.\n");
			Recording = 1;
		}
		FrSaver.Toggle(Width);
		break;
	case 'h':
	case '?':
		instructions();
		break;
	case 'w':
		g_splining = false;
		g_velocity.x = 1;
		break;
	case 's':
		g_splining = false;
		g_velocity.x = -1;
		break;
	case 'a':
		g_splining = false;
		g_velocity.y = -1;
		break;
	case 'd':
		g_splining = false;
		g_velocity.y = 1;
		break;
	case 'f':
		FishSchool::s_school->MakeAllTheFish();
		break;
	}
	glutPostRedisplay();
}

void myUpKey(unsigned char key, int x, int y)
{
	switch (key) {
	case 'w':
	case 's':
		g_velocity.x = 0;
		break;
	case 'd':
	case 'a':
		g_velocity.y = 0;
		break;
	}
	glutPostRedisplay();
}

GLuint LoadVertexBuffer(GLsizei size_bytes, float* verts)
{
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, size_bytes, 0, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, 0, size_bytes, verts);

	GLint size = 0;
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	if (size_bytes != size)
	{
		exit(1);
		return 0;
	}

	return vbo;
}

void myinit(void)
{
	g_programs[SHADER_FLEX].m_preprocs.push_back(string("WITH_FLEX"));
	g_programs[SHADER_NOFIXUP].m_preprocs.push_back(string("NO_FIXUP"));
	g_programs[SHADER_FADE].m_preprocs.push_back(string("WITH_FADE"));

	RecompileShader(true);

	generateSphere(&g_sphere_data);
	generateCylinder(&g_cylinder_data);

	glClearColor(0.0f, 126.f / 255, 159.f / 255, 1.0f); // dark blue background

	g_butterfly_vbo = LoadVertexBuffer(sizeof(asset_butterfly), asset_butterfly);
	g_ocean_vbo = LoadVertexBuffer(sizeof(asset_ocean), asset_ocean);
	g_blade_vbo = LoadVertexBuffer(sizeof(asset_blade), asset_blade);
	g_overlay_vbo = LoadVertexBuffer(sizeof(g_overlay), (float*)g_overlay);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	FishSchool::MakeAllTheFish();

	for (int k = 0; k < sizeof(g_seaweed) / sizeof(g_seaweed[0]); k++)
	{
		g_seaweed[k].m_positions[0][0] = g_seaweed[k].m_positions[1][0] = vec3(g_seaweed_positions[k].x, g_seaweed_positions[k].y, GetSeaBedHeight(g_seaweed_positions[k].x, g_seaweed_positions[k].y));
		for (int n = 1; n < SEAWEED_LINKS; n++)
			g_seaweed[k].m_positions[0][n] = g_seaweed[k].m_positions[1][n] = g_seaweed[k].m_positions[0][n - 1] + vec3(0, 0, g_seaweed_link_length);
	}

	g_spline.m_points[0] = vec3(-37, 0, -12);
	g_spline.m_points[1] = vec3(5, 0, -5);
	g_spline.m_points[2] = vec3(15, -5, -4);
	g_spline.m_points[3] = vec3(20, 0, -4);
	g_spline.m_points[4] = vec3(15, 5, -4);
	g_spline.m_points[5] = vec3(7, 0, -3.9f);
	g_spline.m_points[6] = vec3(-20, 0, 5.5f);
	g_spline.m_points[7] = vec3(-50, 0, 15);
	g_spline.InitializeSpline();

	g_player_position = g_spline.m_points[0];
	g_yaw = 0;
	g_pitch = 8;
}

void set_color(int program, vec3 ambient, vec3 diffuse, vec3 specular, float s)
{
	glUniform3f(g_programs[program].m_uniform_ambient, ambient.x, ambient.y, ambient.z);
	glUniform3f(g_programs[program].m_uniform_diffuse, diffuse.x, diffuse.y, diffuse.z);
	glUniform3f(g_programs[program].m_uniform_specular, specular.x, specular.y, specular.z);
	glUniform1f(g_programs[program].m_uniform_shininess, s);
}

void DrawVBO(GLuint vbo, mat4 transform, GLsizei verts, GLsizei stride, int position_offset, int normal_offset)
{
	int program = SHADER_DEFAULT;

	glUniformMatrix4fv(g_programs[program].m_uniform_model, 1, GL_FALSE, transpose(transform));

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glEnableVertexAttribArray(g_programs[program].m_attrib_position);
	glVertexAttribPointer(g_programs[program].m_attrib_position, 3, GL_FLOAT, false, stride, BUFFER_OFFSET(position_offset));

	glEnableVertexAttribArray(g_programs[program].m_attrib_normal);
	glVertexAttribPointer(g_programs[program].m_attrib_normal, 3, GL_FLOAT, false, stride, BUFFER_OFFSET(normal_offset));

	glDrawArrays(GL_TRIANGLES, 0, verts);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DrawFlexVBO(GLuint vbo, mat4 transform, GLsizei verts, GLsizei stride, int position_offset, int normal_offset, int flex_offset)
{
	int program = SHADER_FLEX;

	glUniformMatrix4fv(g_programs[program].m_uniform_model, 1, GL_FALSE, transpose(transform));
	glUniform1f(g_programs[program].m_uniform_flex_length, 0.312f);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glEnableVertexAttribArray(g_programs[program].m_attrib_position);
	glVertexAttribPointer(g_programs[program].m_attrib_position, 3, GL_FLOAT, false, stride, BUFFER_OFFSET(position_offset));

	glEnableVertexAttribArray(g_programs[program].m_attrib_normal);
	glVertexAttribPointer(g_programs[program].m_attrib_normal, 3, GL_FLOAT, false, stride, BUFFER_OFFSET(normal_offset));

	glEnableVertexAttribArray(g_programs[program].m_attrib_flex);
	glVertexAttribPointer(g_programs[program].m_attrib_flex, 1, GL_FLOAT, false, stride, BUFFER_OFFSET(flex_offset));

	glDrawArrays(GL_TRIANGLES, 0, verts);

	glDisableVertexAttribArray(g_programs[program].m_attrib_flex);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DrawSphere(mat4 transform)
{
	int program = SHADER_NOFIXUP;
	ShapeData* data = &g_sphere_data;

	glUniformMatrix4fv(g_programs[program].m_uniform_model, 1, GL_FALSE, transpose(transform));

	glBindBuffer(GL_ARRAY_BUFFER, data->vbo);

	glEnableVertexAttribArray(g_programs[program].m_attrib_position);
	glVertexAttribPointer(g_programs[program].m_attrib_position, 3, GL_FLOAT, GL_FALSE, 12, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(g_programs[program].m_attrib_normal);
	glVertexAttribPointer(g_programs[program].m_attrib_normal, 3, GL_FLOAT, GL_FALSE, 12, BUFFER_OFFSET(0));

	glDrawArrays(GL_TRIANGLES, 0, data->numVertices);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void simulate()
{
	if (g_splining)
	{
		g_player_position = g_spline.ConstVelocitySplineAtTime((float)g_simulation_time * 2);
	}
	else
	{
		if (length(g_velocity) > 0.0001f)
		{
			vec3 forward = AngleVector(g_pitch, g_yaw);
			vec3 right = normalize(cross(forward, vec3(0, 0, 1)));
			mat3 player(forward, right, -normalize(cross(forward, right)));

			g_player_position += transpose(player)*(g_velocity * ((float)g_dt * 5));
		}
	}

	while (g_seaweed_simulation_time < g_simulation_time)
		SimulateSeaweed();

	FishSchool::s_school->Simulate();
}

void display(void)
{
	simulate();

	// Clear the screen with the background colour (set in myinit)
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	int program = 0;

	SetupProgram(0);

	set_color(program, vec3(0.1f, 0.1f, 0.1f), vec3(255, 255, 255) / 255, vec3(0.3f, 0.3f, 0.3f), 4);
	DrawVBO(g_ocean_vbo, mat4(), asset_ocean_num_verts, asset_ocean_vert_size_bytes, 0, asset_ocean_normal_offset_bytes);

	/*set_color(program, vec3(0.1f, 0.1f, 0.1f), vec3(255, 200, 0) / 255, vec3(1.5f, 1.5f, 1.5f), 30);
	mat4 fish;
	for (int x = HEIGHTS_MIN; x <= HEIGHTS_MAX; x++)
	{
		for (int y = HEIGHTS_MIN; y <= HEIGHTS_MAX; y++)
		{
			fish = Translate(vec3(x, y, GetSeaBedHeight(x, y)));
			DrawVBO(g_butterfly_vbo, fish, asset_butterfly_num_verts, asset_butterfly_vert_size_bytes, 0, asset_butterfly_normal_offset_bytes);
		}
	}*/
	/*set_color(program, vec3(0.1f, 0.1f, 0.1f), vec3(255, 200, 0) / 255, vec3(1.5f, 1.5f, 1.5f), 30);
	mat4 fish;
	fish *= Translate(vec3(45, 0, 0));
	fish *= RotateZ(g_simulation_time*15);
	DrawVBO(g_butterfly_vbo, fish, asset_butterfly_num_verts, asset_butterfly_vert_size_bytes, 0, asset_butterfly_normal_offset_bytes);*/

	FishSchool::s_school->Render();

	RenderSeaweed();

	RenderSeagrass();

	/*SetupProgram(1);

	set_color(program, vec3(36, 50, 31)/255, vec3(183, 255, 158) / 255, vec3(0.3f, 0.3f, 0.3f), 50);
	mat4 blade = Translate(vec3(45, 0, GetSeaBedHeight(45, 0)));
	DrawFlexVBO(g_blade_vbo, blade, asset_blade_num_verts, asset_blade_vert_size_bytes, 0, asset_blade_normal_offset_bytes, asset_blade_flex_offset_bytes);
	blade = Translate(vec3(45.1f, 0.1f, GetSeaBedHeight(45.1f, 0.1f)));
	DrawFlexVBO(g_blade_vbo, blade, asset_blade_num_verts, asset_blade_vert_size_bytes, 0, asset_blade_normal_offset_bytes, asset_blade_flex_offset_bytes);
	blade = Translate(vec3(46.0f, 1.0f, GetSeaBedHeight(46.0f, 1.0f)));
	DrawFlexVBO(g_blade_vbo, blade, asset_blade_num_verts, asset_blade_vert_size_bytes, 0, asset_blade_normal_offset_bytes, asset_blade_flex_offset_bytes);*/

/*	set_color(program, vec3(1.5f, 1.5f, 1.5f), vec3(1.5f, 1.5f, 1.5f), vec3(1.5f, 1.5f, 1.5f), 30);
	for (int k = 10; k < SPLINE_POINTS * 10; k++)
		DrawVBO(g_butterfly_vbo, Translate(g_spline.SplineAtTime((float)k / 10)) * Scale(vec3(0.2f, 0.2f, 0.2f)), asset_butterfly_num_verts, asset_butterfly_vert_size_bytes, 0, asset_butterfly_normal_offset_bytes);
	for (int k = 0; k < SPLINE_POINTS; k++)
		DrawVBO(g_butterfly_vbo, Translate(g_spline.SplineAtTime((float)k)) * Scale(vec3(0.5f, 0.5f, 0.5f)), asset_butterfly_num_verts, asset_butterfly_vert_size_bytes, 0, asset_butterfly_normal_offset_bytes);*/

//	for (int k = 0; k < 100; k++)
//		DrawVBO(g_butterfly_vbo, Translate(g_spline.ConstVelocitySplineAtTime((float)k / 10)) * Scale(vec3(0.5f, 0.5f, 0.5f)), asset_butterfly_num_verts, asset_butterfly_vert_size_bytes, 0, asset_butterfly_normal_offset_bytes);

	float fade = RemapValClamped((float)g_simulation_time, 0, 1, 1, 0) + RemapValClamped((float)g_simulation_time, 50, 55, 0, 1);
	if (fade)
	{
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		program = SHADER_FADE;

		glUseProgram(g_programs[program].m_program);

		glUniform3f(g_programs[program].m_uniform_camera_position, 0, 0, 0);
		glUniformMatrix4fv(g_programs[program].m_uniform_view, 1, GL_FALSE, mat4());
		glUniform1f(g_programs[program].m_uniform_time, (float)g_simulation_time);
		glUniformMatrix4fv(g_programs[program].m_uniform_projection, 1, GL_FALSE, mat4());

		if (g_simulation_time < 10)
			set_color(program, vec3(0, 0, 0), vec3(0, 0, 0), vec3(0, 0, 0), 1);
		else
		{
			float ramp = RemapValClamped((float)g_simulation_time, 55, 57, 0, 1);
			vec3 color = vec3(0, 0.235f, 1) * (1-ramp) + vec3(0, 0, 0) * ramp;
			set_color(program, color, color, color, 1);
		}

		glUniform1f(g_programs[program].m_uniform_alpha, fade);

		DrawVBO(g_overlay_vbo, mat4(), 6, sizeof(vec3), 0, 0);

		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}

	glutSwapBuffers();
	if (Recording == 1)
		FrSaver.DumpPPM(Width, Height);
}

void myReshape(int w, int h)		//handles the window being resized 
{
	glViewport(0, 0, Width = w, Height = h);
}

static int drag_start_x, drag_start_y;

void myMouseCB(int button, int state, int x, int y)	// start or end mouse interaction
{
	ArcBall_mouseButton = button;
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		drag_start_x = x;
		drag_start_y = y;
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		ArcBall_mouseButton = -1;
	}
	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		ArcBall_PrevZoomCoord = y;
	}

	// Tell the system to redraw the window
	glutPostRedisplay();
}

// interaction (mouse motion)
void myMotionCB(int x, int y)
{
	if (ArcBall_mouseButton == GLUT_LEFT_BUTTON)
	{
		float ydiff = (float)(y - drag_start_y);
		float xdiff = (float)(x - drag_start_x);
		g_pitch -= ydiff;
		g_yaw -= xdiff;

		if (g_pitch > 89.9f)
			g_pitch = 89.9f;
		if (g_pitch < -89.9f)
			g_pitch = -89.9f;

		drag_start_x = x;
		drag_start_y = y;

		glutPostRedisplay();
	}
	else if (ArcBall_mouseButton == GLUT_RIGHT_BUTTON)
	{
		if (y - ArcBall_PrevZoomCoord > 0)
			Zoom = Zoom * 1.03f;
		else
			Zoom = Zoom * 0.97f;
		ArcBall_PrevZoomCoord = y;
		glutPostRedisplay();
	}
}


void idleCB(void)
{
	if (Animate == 1)
	{
		// TM.Reset() ; // commenting out this will make the time run from 0
		// leaving 'Time' counts the time interval between successive calls to idleCB
		if (Recording == 0)
			TIME = TM.GetElapsedTime();
		else
			TIME += 0.033; // save at 30 frames per second.

		//printf("TIME %f\n", TIME);
		glutPostRedisplay();
	}
}

int main(int argc, char** argv)		// calls initialization, then hands over control to the event handler, which calls display() whenever the screen needs to be redrawn
{
	srand(0);

	glutInit(&argc, argv);

	Width = glutGet(GLUT_SCREEN_WIDTH) * 3 / 4;
	Height = glutGet(GLUT_SCREEN_HEIGHT) * 3 / 4;

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowPosition(glutGet(GLUT_SCREEN_WIDTH) / 2 - Width / 2, glutGet(GLUT_SCREEN_HEIGHT) / 2 - Height / 2);
	glutInitWindowSize(Width, Height);
	glutCreateWindow(argv[0]);
	printf("GL version %s\n", glGetString(GL_VERSION));
#ifndef EMSCRIPTEN
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	myinit();

	glutIdleFunc(idleCB);
	glutReshapeFunc(myReshape);
	glutKeyboardFunc(myKey);
	glutKeyboardUpFunc(myUpKey);
	glutMouseFunc(myMouseCB);
	glutMotionFunc(myMotionCB);
	instructions();

	glutDisplayFunc(display);

	double wakeup_goal_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000;

	int frames = 0;
	double next_frame_count = 1;
	double last_real_time = 0;
	double last_sim_time = 0;

	while (true)
	{
		double real_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000;

		frames++;
		if (real_time > next_frame_count)
		{
			printf("FPS: %d\n", frames);
			frames = 0;
			next_frame_count += 1;
			//printf("Simulation time: %f (+%f)\n", g_simulation_time, g_simulation_time - last_sim_time);
			last_sim_time = g_simulation_time;
			//printf("Real time: %f (+%f)\n", real_time, (double)glutGet(GLUT_ELAPSED_TIME) / 1000 - last_real_time);
			last_real_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000;
		}

		if (g_simulation_time > 60)
			exit(0);

		double frame_start_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000;

		g_dt = g_target_framerate * 0.7;
		g_simulation_time += g_dt;

		glutPostRedisplay();

		glutMainLoopEvent();

		double frame_end_time = (double)glutGet(GLUT_ELAPSED_TIME) / 1000;

		wakeup_goal_time += g_target_framerate;

		double time_to_sleep_seconds = wakeup_goal_time - frame_end_time;
		if (time_to_sleep_seconds > 0.001)
			Sleep((size_t)(time_to_sleep_seconds * 1000));
	}
}

