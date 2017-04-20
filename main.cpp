

#include "camera.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "TextureManager.h"
#include "skybox.h"
#include "Sphere.h"

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
GLuint vao_index = 0;

GLuint sceneFbo, sunFbo, blurFbo, sunScatterFbo, lensFlareHaloFbo, quadFbo;
GLuint textureColorBufferMultiSampled;
GLuint vertexAttribute, texAttribute;

glm::mat4 scale(glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 5.0f, 5.0f)));
float rotation[3] = { 0.0, 0.0, 0.0 };
float sphereScale = 20.0f, cloudScale = sphereScale + 0.1, atmosphereScale = sphereScale + 0.3f;

// Light Variables
glm::vec4 lightColor(1.25, 1.25, 1.25, 1.0);
glm::vec4 lightPosition(100.0, 0.0, 100.0, 1.0);
glm::vec3 sunPos(20.0, 0.0, 20.0);

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
// Object variables
shared_ptr<Sphere> s, cs, as, ss;
Skybox* skybox;
TextureManager& textureManager = TextureManager::GetInstance();

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

//quad variables
// x,y vertex positions
float quad_vertices[] = {
	-1.0, -1.0, 0.0,
	1.0, -1.0, 0.0,
	1.0,  1.0, 0.0,
	1.0,  1.0, 0.0,
	-1.0,  1.0, 0.0,
	-1.0, -1.0, 0.0
};

// per-vertex texture coordinates
float quad_texcoords[] = {
	0.0, 0.0,
	1.0, 0.0,
	1.0, 1.0,
	1.0, 1.0,
	0.0, 1.0,
	0.0, 0.0
};

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

void initAtmosphericUniforms(vector<GLuint> shaderPrograms)
{
	GLfloat innerRadius = s->getRadius() * s->getSize().x;
	GLfloat outerRadius = as->getRadius() * as->getSize().x;
	GLfloat scale = 1.0f / (outerRadius - innerRadius);
	GLfloat scaleDepth = 0.25;
	GLfloat scaleOverScaleDepth = scale / scaleDepth;
	GLfloat Kr = 0.0025f;
	GLfloat Km = 0.0010f;
	GLfloat ESun = 16.0f;
	GLfloat g = -0.99f;

	for (GLuint i = 0; i < shaderPrograms.size(); i++)
	{
		glUseProgram(shaderPrograms[i]);
		glUniform3f(glGetUniformLocation(shaderPrograms[i], "v3InvWavelength"), 1.0f / powf(0.650f, 4.0f), 1.0f / powf(0.570f, 4.0f), 1.0f / powf(0.475f, 4.0f));
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fInnerRadius"), innerRadius);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fInnerRadius2"), innerRadius * innerRadius);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fOuterRadius"), outerRadius);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fOuterRadius2"), outerRadius * outerRadius);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fKrESun"), Kr * ESun);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fKmESun"), Km * ESun);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fKr4PI"), Kr * 4.0f * (float)PI);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fKm4PI"), Km * 4.0f * (float)PI);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fScale"), scale);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fScaleDepth"), scaleDepth);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "fScaleOverScaleDepth"), scaleOverScaleDepth);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "g"), g);
		glUniform1f(glGetUniformLocation(shaderPrograms[i], "g2"), g * g);
		glUniform1i(glGetUniformLocation(shaderPrograms[i], "Samples"), 4);
	}

	glUseProgram(0);
}


void initBuffers(GLuint& index, GLuint program) {
	vertexAttribute = glGetAttribLocation(program, "vPosition");
	texAttribute = glGetAttribLocation(program, "vTexCoord");

	glBindVertexArray(vao[index]);

	glGenBuffers(2, vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(vertexAttribute);
	glVertexAttribPointer(vertexAttribute, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_texcoords), quad_texcoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(texAttribute);
	glVertexAttribPointer(texAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0);
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

	map<string, string> textureHandles;

	curr_program = normalmap_program;


	textureHandles.clear();
	textureHandles.insert(make_pair("earthDay", "day"));
	textureHandles.insert(make_pair("earthNight", "night"));
	textureHandles.insert(make_pair("earthSpecularMap", "specMap"));
	textureHandles.insert(make_pair("earthNormalMap", "bumpMap"));
	textureHandles.insert(make_pair("earthClouds", "clouds"));
	//earth
	s = shared_ptr<Sphere>(new Sphere(vao[vao_index], 1.0f, 50, 50, 0.015f, 0.f));
	s->generateMesh();
	s->setTextureHandles(textureHandles);
	s->setScale(sphereScale, sphereScale, sphereScale);
	s->setDiffuseColor(1.0f, 0.941f, 0.898f);
	s->initBuffers(curr_program);

	vao_index++;
	curr_program = cloudmap_program;

	textureHandles.clear();
	textureHandles.insert(make_pair("earthClouds", "clouds"));
	textureHandles.insert(make_pair("earthCloudsNormalMap", "cloudBumpMap"));
	textureHandles.insert(make_pair("earthNightBlur", "nightLights"));

	//cloud
	cs = shared_ptr<Sphere>(new Sphere(vao[vao_index], 1.0f, 50, 50, 0.02f, 0.f));
	cs->generateMesh();
	cs->setTextureHandles(textureHandles);
	cs->setScale(cloudScale, cloudScale, cloudScale);
	cs->setDiffuseColor(1.0f, 0.941f, 0.898f);
	cs->initBuffers(curr_program);

	vao_index++;
	curr_program = atmosphere_program;

	atmosphereScale = sphereScale + 0.3;

	//atmosphere
	as = shared_ptr<Sphere>(new Sphere(vao[vao_index], 1.0f, 50, 50, 0.0, 0.0));
	as->setScale(atmosphereScale, atmosphereScale, atmosphereScale);
	as->generateMesh();
	as->initBuffers(curr_program);

	vao_index++; //sun
	curr_program = sun_program;
	ss = shared_ptr<Sphere>(new Sphere(vao[vao_index], 1.0f, 50, 50, 0.0, 0.0));
	ss->setPosition(sunPos.x, sunPos.y, sunPos.z);
	ss->setScale(100, 100, 100);
	ss->generateMesh();
	ss->initBuffers(curr_program);

	vao_index++;
	curr_program = skybox_program;

	textureHandles.clear();
	textureHandles.insert(make_pair("skybox", "skybox"));

	skybox = new Skybox(vao[vao_index], faces);
	skybox->generateMesh();
	skybox->setScale(5.0, 5.0, 5.0);
	skybox->setDiffuseColor(0.0, 0.0, 0.5);
	skybox->setTextureHandles(textureHandles);
	skybox->enableCubemap();
	skybox->initBuffers(curr_program);

	vao_index++;
	curr_program = fxaa_program;

	initBuffers(vao_index, fxaa_program);

	vector<GLuint> shaderPrograms;
	shaderPrograms.push_back(atmosphere_program);
	shaderPrograms.push_back(normalmap_program);
	initAtmosphericUniforms(shaderPrograms);

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