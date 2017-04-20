

#include "camera.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "TextureManager.h"

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>


using namespace std;

int VIEW_SIZE_WIDTH = 1024;
int VIEW_SIZE_HEIGHT = 576;

float FULLSCREEN_WIDTH, FULLSCREEN_HEIGHT, FULLSCREEN_ASPECT_RATIO;
float VIEW_ASPECT_RATIO = (float)VIEW_SIZE_WIDTH / (float)VIEW_SIZE_HEIGHT;
float SCENE_WIDTH = 1600, SCENE_HEIGHT = 900;
float SUN_WIDTH = SCENE_WIDTH / 8.0, SUN_HEIGHT = SCENE_HEIGHT / 8.0;
float ANAMORPHIC_WIDTH = SCENE_WIDTH / 32.0, ANAMORPHIC_HEIGHT = SCENE_HEIGHT / 32.0;
int VIEWPORT_WIDTH = VIEW_SIZE_WIDTH, VIEWPORT_HEIGHT = VIEW_SIZE_HEIGHT;

GLuint vao[40];
GLuint vbo[2];
camera* cam;

glm::vec3 up_vector(0, 1, 0);
glm::vec3 camera_position(-20.0, 0.0, 50.0);
glm::vec3 camera_lookat(0.0, 0.0, 0.0);

float l = -0.005f;
float r = 0.005f;
float t = 0.005f;
float b = -0.005f;
float n = 0.01f;
float f = 10000.5f;

// Object variables
 
TextureManager& textureManager = TextureManager::GetInstance();
GLuint sceneFbo, sunFbo, blurFbo, sunScatterFbo, lensFlareHaloFbo, quadFbo;

GLuint cloudmap_program, 
		skybox_program, 
		normalmap_program, 
		atmosphere_program, 
		fxaa_program,
		curr_program,
		sun_program,
		sun_scatter_program, 
		blur_program, 
		quad_program, 
		lens_flare_halo_program;

extern GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path, const char*geometry_file_path);

void errorCB(int, const char*)
{

}

void keyboard(GLFWwindow*window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}
 

void mouseMove(int x, int y)
{
 
}

GLuint generateFrameBufferObject(const GLuint& renderTexture, const GLuint& depthTexture = 0) {
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTexture, 0);
	if (depthTexture)
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		cout << "Frame Buffer Object error status : " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << endl;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return fbo;
}

void init()
{
	cloudmap_program = LoadShaders("./shaders/normalmap_shader.vert", "shaders/cloudmap_shader.frag",NULL);
	if (!cloudmap_program)
	{
		cerr << "Error setting up Shaders!";
		exit(1);
	}

	normalmap_program = LoadShaders("shaders/normalmap_shader.vert", "shaders/normalmap_shader.frag", NULL);
	if (!normalmap_program)
	{
		cerr << "Error Setting Up scene shaders!" << endl;
		exit(1);
	}

	atmosphere_program = LoadShaders("shaders/atmosphere_shader.vert", "shaders/atmosphere_shader.frag", NULL);
	if (!atmosphere_program)
	{
		cerr << "Error Setting Up atmosphere shaders!" << endl;
		exit(1);
	}

	skybox_program = LoadShaders("shaders/skybox.vert", "shaders/skybox.frag", NULL);
	if (!skybox_program)
	{
		cerr << "Error setting up skybox shaders!";
		exit(1);
	}

	fxaa_program = LoadShaders("shaders/quad.vert", "shaders/fxaa.frag", NULL);
	if (!fxaa_program)
	{
		cerr << "Error setting up FXAA shaders!";
		exit(1);
	}

	sun_program = LoadShaders("shaders/sun.vert", "shaders/sun.frag", NULL);
	if (!sun_program)
	{
		cerr << "Error setting up sun shaders!";
		exit(1);
	}

	blur_program = LoadShaders("shaders/quad.vert", "shaders/blur.frag", NULL);
	if (!blur_program)
	{
		cerr << "Error setting up blur shaders!";
		exit(1);
	}

	quad_program = LoadShaders("shaders/quad.vert", "shaders/quad.frag", NULL);
	if (!quad_program)
	{
		cerr << "Error setting up quad shaders!";
		exit(1);
	}


	sun_scatter_program = LoadShaders("shaders/quad.vert", "shaders/sun_scatter.frag", NULL);
	if (!sun_scatter_program)
	{
		cerr << "Error setting up sun scatter shaders!";
		exit(1);
	}

	lens_flare_halo_program = LoadShaders("shaders/quad.vert", "shaders/lens_flare_halo.frag", NULL);
	if (!lens_flare_halo_program)
	{
		cerr << "Error setting up lens flare shaders!";
		exit(1);
	}

	glGenVertexArrays(40, vao);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	//glEnable(GL_POLYGON_SMOOTH);
	//glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glCullFace(GL_BACK);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GLfloat aspect_ratio = (GLfloat)VIEWPORT_WIDTH / (GLfloat)VIEWPORT_HEIGHT;

	cam = new camera(camera_position, camera_lookat, up_vector, l, r, t, b, n, f, aspect_ratio);


	std::vector<std::string> faces;
	string spaceboxDirectoryName("dark-space-skybox/");
	faces.push_back(spaceboxDirectoryName + "right.png");
	faces.push_back(spaceboxDirectoryName + "left.png");
	faces.push_back(spaceboxDirectoryName + "top.png");
	faces.push_back(spaceboxDirectoryName + "bottom.png");
	faces.push_back(spaceboxDirectoryName + "front.png");
	faces.push_back(spaceboxDirectoryName + "back.png");

	textureManager.LoadTextureCubeMap(faces, "skybox");
	textureManager.LoadTexture1D("textures/lenscolor.png", "lensColor");
	textureManager.LoadTexture2D("textures/earth_day_8k.jpg", "earthDay");
	textureManager.LoadTexture2D("textures/earth_night_8k.png", "earthNight");
	textureManager.LoadTexture2D("textures/earth_night_8k_blur.png", "earthNightBlur");
	textureManager.LoadTexture2D("textures/earth_specular.jpg", "earthSpecularMap");
	textureManager.LoadTexture2D("textures/earth_normalmap_8k.jpg", "earthNormalMap");
	textureManager.LoadTexture2D("textures/earth_clouds_8k.jpg", "earthClouds");
	textureManager.LoadTexture2D("textures/clouds_normalmap_8k.jpg", "earthCloudsNormalMap");
	textureManager.LoadTexture2D("textures/lensstar.png", "lensStar");
	textureManager.LoadTexture2D("textures/lensdirt_lowc.jpg", "lensDirt");
	textureManager.GenerateFBOTexture2D("sceneColorTexture", SCENE_WIDTH, SCENE_HEIGHT);
	textureManager.GenerateFBOTexture2D("sceneDepthTexture", SCENE_WIDTH, SCENE_HEIGHT, true);
	textureManager.GenerateFBOTexture2D("atmosphereColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("atmosphereColorTexture2", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("sunColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("sunDepthTexture", SUN_WIDTH, SUN_HEIGHT, true);
	textureManager.GenerateFBOTexture2D("blurHColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("blur16ColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("blur24ColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("blur32ColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("blur64ColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("sunScatterColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("lensFlareHaloColorTexture", SUN_WIDTH, SUN_HEIGHT);
	textureManager.GenerateFBOTexture2D("anamorphicLensColorTexture", ANAMORPHIC_WIDTH, ANAMORPHIC_HEIGHT);
	textureManager.GenerateFBOTexture2D("anamorphicLensDepthTexture", ANAMORPHIC_WIDTH, ANAMORPHIC_HEIGHT);
	textureManager.GenerateFBOTexture2D("anamorphicBlur16ColorTexture", ANAMORPHIC_WIDTH, ANAMORPHIC_HEIGHT);
	textureManager.GenerateFBOTexture2D("anamorphicBlur32ColorTexture", ANAMORPHIC_WIDTH, ANAMORPHIC_HEIGHT);
	textureManager.GenerateFBOTexture2D("anamorphicBlur64ColorTexture", ANAMORPHIC_WIDTH, ANAMORPHIC_HEIGHT);
	textureManager.GenerateFBOTexture2D("quadColorTexture", SCENE_WIDTH, SCENE_HEIGHT);

	sceneFbo = generateFrameBufferObject(textureManager["sceneColorTexture"], textureManager["sceneDepthTexture"]);
	sunFbo = generateFrameBufferObject(textureManager["sunColorTexture"], textureManager["sunDepthTexture"]);
	blurFbo = generateFrameBufferObject(textureManager["blurHColorTexture"]);
	sunScatterFbo = generateFrameBufferObject(textureManager["sunScatterColorTexture"]);
	lensFlareHaloFbo = generateFrameBufferObject(textureManager["lensFlareHaloColorTexture"]);
	quadFbo = generateFrameBufferObject(textureManager["quadColorTexture"]);


}

int main(int argc, char** argv)
{
	GLFWwindow * window;

	glfwSetErrorCallback(errorCB);

	if (!glfwInit())
		return 1;

	window = glfwCreateWindow(VIEWPORT_WIDTH, VIEWPORT_HEIGHT, "Atmospheric", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(window, keyboard);
	glfwMakeContextCurrent(window);

	GLenum err = glewInit();

	init();

	if (GLEW_OK != err)
	{
		fprintf(stderr, "Failed to initialize glew\n");
		exit(EXIT_FAILURE);
	}


	glfwSwapInterval(1);

	while (!glfwWindowShouldClose(window))
	{
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}