
#include <string>
#include <fstream>
#include <vector>

#include <gl/GLew.h>

GLuint LoadShaderSouce(const char* vertexSource, const char*fragmentSource, const char* geometrySource);

GLuint LoadShaders(const char * vertex_file_path, const char * fragment_file_path, const char*geometry_file_path) {


	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);

	if (VertexShaderStream.is_open())
	{
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();

	}
	else
	{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open())
	{
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}
	else
	{
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path);
		getchar();
		return 0;
	}
	//Read geometry_file_path
	std::string GeometryShaderCode;
	if (geometry_file_path != NULL)
	{
		std::ifstream GeometryShaderStream(geometry_file_path, std::ios::in);
		if (GeometryShaderStream.is_open())
		{
			std::string Line = "";
			while (getline(GeometryShaderStream, Line))
				GeometryShaderCode += "\n" + Line;
			GeometryShaderStream.close();
		}
		else
		{
			printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", geometry_file_path);
			getchar();
			return 0;
		}

	}
	return LoadShaderSouce(VertexShaderCode.c_str(), FragmentShaderCode.c_str(), GeometryShaderCode.c_str());

}

GLuint LoadShaderSouce(const char* vertexSource, const char*fragmentSource, const char* geometrySource)
{

	// Create the shaders
	GLuint VertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint GeometryShaderId;

	if (!geometrySource) GeometryShaderId = glCreateShader(GL_GEOMETRY_SHADER);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling vertex shader ...");


	char const * VertexSourcePointer = vertexSource;
	glShaderSource(VertexShaderId, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderId);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderId, GL_COMPILE_STATUS, &Result);
	if (!GL_TRUE) return 0;


	// Compile Fragment Shader
	printf("Compiling frame shader ...\n");
	char const * FragmentSourcePointer = fragmentSource;
	glShaderSource(FragmentShaderId, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderId);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderId, GL_COMPILE_STATUS, &Result);
	if (!GL_TRUE) return 0;

	//COMPILE GEOMETRY SAHDER
	if (!geometrySource)
	{
		char const * GeometrySourcePointer = geometrySource;
		glShaderSource(GeometryShaderId, 1, &GeometrySourcePointer, NULL);
		glCompileShader(GeometryShaderId);

		// Check Fragment Shader
		glGetShaderiv(GeometryShaderId, GL_COMPILE_STATUS, &Result);
		if (!GL_TRUE) return 0;
	}

	// Link the program
	printf("Linking program...\n");
	GLuint ShaderProgramId = glCreateProgram();
	glAttachShader(ShaderProgramId, VertexShaderId);
	glAttachShader(ShaderProgramId, FragmentShaderId);
	if (!geometrySource) glAttachShader(ShaderProgramId, GeometryShaderId);


	glLinkProgram(ShaderProgramId);

	// Check the program
	glGetProgramiv(ShaderProgramId, GL_LINK_STATUS, &Result);
	glGetProgramiv(ShaderProgramId, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ShaderProgramId, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}
	if (!GL_TRUE) return 0;

	return ShaderProgramId;
}
