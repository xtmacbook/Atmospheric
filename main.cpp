

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "render.h"

#include <stdio.h>
#include <iostream>


int VIEW_SIZE_WIDTH = 1024;
int VIEW_SIZE_HEIGHT = 576;

using namespace std;

void keyboard(unsigned char key, int x, int y)
{
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	
}

void mouseMove(int x, int y)
{
 
}

int main(int argc, char** argv)
{
	return 0;
}