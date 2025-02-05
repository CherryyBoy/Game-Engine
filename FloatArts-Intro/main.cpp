/*
* By Matin Afzal Asr
* 27/9/2024
*
* ERROR Categories.
*	PVE: Pre-Visual Error.
*	AVE: After-Visual Error.
* ERROR Types.
*	INIT: Initializing Error.
*	MATH: Arithmetic Error.
*	SHAD: Shader Error.
*	PROG: Program Error.
*	LOAD: Loading Error.
*	NONE: Unknown Error.
*/
//#include <nfd.h>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>

#define GLM_FORCE_RADIANS // [https://stackoverflow.com/questions/79054516/fps-camera-system-cant-look-up-or-down]
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

float FOV = glm::radians(45.0f);
float NEAR_PLANE = 0.1f;
float FAR_PLANE = 400.0f;

// Structs
struct ObjectData
{
	const void* vertices;
	size_t verticesSize;
	const void* indices;
	size_t indicesSize;
};

struct Cube {
	glm::vec3 position; 
	float scale = 1.0f; 
	glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); 
};

std::vector<Cube> cubes;
std::size_t selectedCube;

// Functions
void display(GLFWwindow* window, GLuint lightShader, GLuint program, GLuint VAO, GLuint LVAO, double currentTime, 
	GLuint uniformCamMatrix);

GLuint programInit(const char* vertexShaderCode, const char* fragmentShaderCode);
void terminateProgram(GLuint program);

std::tuple<GLuint, GLuint, GLuint> createObject(ObjectData object, int layers, int length);
void terminateObject(GLuint VAO, GLuint VBO, GLuint EBO);

void inputs(GLFWwindow* window);
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void checkShaderCompileErrors(GLuint shader);
void checkProgramLinkErrors(GLuint program);
const char* getGLErrorString(GLenum err);
void checkOpenGLError(void);
void errorLog(std::string category, std::string type, std::string massage, std::string comment="");

// Global Variables
glm::mat4 model = glm::mat4(1.0f);
glm::mat4 lModel = glm::mat4(1.0f);
glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 2.0f);
glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

glm::vec4 lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
glm::vec3 lightPos = glm::vec3(1.0f, 1.0f, 2.0f);
glm::mat4 lightModel = glm::mat4(1.0f);

glm::vec3 cubePos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::mat4 cubeModel = glm::mat4(1.0f);

float rotation = 0.0f;
float rotatingSpeed = 2.0f;
float camSpeed = 0.1;
float sensitivity = 100.0;
bool firstClick = true;
bool firstClickR = true;

double startTime = glfwGetTime();
double lastTime = startTime;

GLuint textureFloatArts;
GLfloat scale = 1.0f;
GLfloat cubeMoveSpeed = 0.05f;
GLfloat cubeRotateSpeed = 0.2f; // سرعت چرخش مکعب
GLboolean rotatingCube = false;
GLdouble lastMouseX, lastMouseY;
GLfloat cubeRotationX = 0.0f, cubeRotationY = 0.0f;
// List of available textures
std::vector<std::string> availableTextures = {
	"textures/brick.png",
	"textures/wood.jpg",
	"textures/sand.bmp"
};

// List of loaded textures
std::vector<std::string> loadedTextures;


float screenColor[4] = { 0.1f, 0.2f, 0.3f, 1.f };




const char* vertexShaderCode =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"layout (location = 1) in vec3 aColor;\n"
"layout (location = 2) in vec2 aTex;\n"
"layout (location = 3) in vec3 aNormals;\n"
"uniform mat4 camMatrix;\n"
"uniform mat4 model;\n"
"out vec3 color;\n"
"out vec2 texCoord;\n"
"out vec3 crnt_pos;\n"
"out vec3 normals;\n"
"void main()\n"
"{\n"
"   crnt_pos = vec3(model * vec4(aPos, 1.0f));\n"
"	gl_Position = camMatrix * vec4(crnt_pos, 1.0f);\n"
"	color = aColor;\n"
"	texCoord = aTex;\n"
"   normals = aNormals;\n"
"}\0;"
;
const char* fragmentShaderCode =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec3 color;\n"
"in vec2 texCoord;\n"
"in vec3 crnt_pos;\n"
"in vec3 normals;\n"
"uniform sampler2D tex0;\n"
"uniform vec4 lightColor;\n"
"uniform vec3 lightPos;\n"
"uniform vec3 cam_pos;\n"
"void main()\n"
"{\n"
"   float ambient = 0.20;\n"

"   vec3 normal = normalize(normals);\n"
"   vec3 lightDirection = normalize(lightPos - crnt_pos);\n"
"   float diffuse = max(dot(normal, lightDirection), 0.0f);\n"

"   float specularLight = 0.50f;\n"
"   vec3 viewDirection = normalize(cam_pos - crnt_pos);\n"
"   vec3 reflectionDirection = reflect(-lightDirection, normal);\n"
"   float specAmount = pow(max(dot(viewDirection, reflectionDirection), 0.0f), 8);\n"
"   float specular = specAmount * specularLight;\n"

"	FragColor = texture(tex0, texCoord) * lightColor  * (diffuse + ambient + specular);\n"
"}\n\0;"
;

const char* vertexShaderLightCode =
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform mat4 model;\n"
"uniform mat4 camMatrix;\n"
"void main()\n"
"{\n"
"	gl_Position = camMatrix * model * vec4(aPos, 1.0);\n"
"}\n\0;"
;
const char* fragmentShaderLightCode =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 lightColor;\n"
"void main()\n"
"{\n"
"	FragColor = lightColor;\n"
"}\n\0;"
;
GLuint VAO, VBO, EBO;
GLfloat objectFloatArtsvertices[] = {
	// Positions          // Colors               // Texture Coordinates           // Normals
	// Back Face
	-0.5f * scale, -0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,    1.0f, 0.0f,  0.0f, 0.0f, -1.0f,  // Back-left-bottom
	 0.5f * scale, -0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,     0.0f, 0.0f,  0.0f, 0.0f, -1.0f,  // Back-right-bottom
	 0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,     0.0f, 1.0f,  0.0f, 0.0f, -1.0f,  // Back-right-top
	-0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,    1.0f, 1.0f,  0.0f, 0.0f, -1.0f,  // Back-left-top

	// Front Face
	-0.5f * scale, -0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,    0.0f, 0.0f,  0.0f, 0.0f,  1.0f,  // Front-left-bottom
	 0.5f * scale, -0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,     1.0f, 0.0f,  0.0f, 0.0f,  1.0f,  // Front-right-bottom
	 0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,     1.0f, 1.0f,  0.0f, 0.0f,  1.0f,  // Front-right-top
	-0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,    0.0f, 1.0f,  0.0f, 0.0f,  1.0f,  // Front-left-top

	// Left Face (no texture)
	-0.5f * scale, -0.5f * scale, -0.5f * scale,  0.0f, 1.0f, 0.0f,    0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  // Back-left-bottom
	-0.5f * scale, -0.5f * scale,  0.5f * scale,  0.0f, 1.0f, 0.0f,    0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  // Front-left-bottom
	-0.5f * scale,  0.5f * scale,  0.5f * scale,  0.0f, 1.0f, 0.0f,    0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  // Front-left-top
	-0.5f * scale,  0.5f * scale, -0.5f * scale,  0.0f, 1.0f, 0.0f,    0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  // Back-left-top

	// Right Face (no texture)
	 0.5f * scale, -0.5f * scale, -0.5f * scale,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Back-right-bottom
	 0.5f * scale, -0.5f * scale,  0.5f * scale,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Front-right-bottom
	 0.5f * scale,  0.5f * scale,  0.5f * scale,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Front-right-top
	 0.5f * scale,  0.5f * scale, -0.5f * scale,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Back-right-top

	 // Top Face (no texture)
	  -0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 1.0f, 0.0f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Back-left-top
	   0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 1.0f, 0.0f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Back-right-top
	   0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 0.0f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Front-right-top
	  -0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 0.0f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Front-left-top

	  // Bottom Face (no texture)
	   -0.5f * scale, -0.5f * scale, -0.5f * scale,  0.5f, 0.5f, 0.5f,    0.0f, 0.0f,   0.0f, -1.0f, 0.0f,  // Back-left-bottom
		0.5f * scale, -0.5f * scale, -0.5f * scale,  0.5f, 0.5f, 0.5f,    0.0f, 0.0f,   0.0f, -1.0f, 0.0f,  // Back-right-bottom
		0.5f * scale, -0.5f * scale,  0.5f * scale,  0.5f, 0.5f, 0.5f,    0.0f, 0.0f,   0.0f, -1.0f, 0.0f,  // Front-right-bottom
	   -0.5f * scale, -0.5f * scale,  0.5f * scale,  0.5f, 0.5f, 0.5
};

GLfloat objectCubeVerticesFull[] = {
	// Positions          // Colors               // Texture Coordinates      // Normals
	// Back Face
	-0.5f * scale, -0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,    1.0f, 0.0f,  0.0f, 0.0f, -1.0f,  // Back-left-bottom
	 0.5f * scale, -0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,     0.0f, 0.0f,  0.0f, 0.0f, -1.0f,  // Back-right-bottom
	 0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,     0.0f, 1.0f,  0.0f, 0.0f, -1.0f,  // Back-right-top
	-0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 0.0f, 0.0f,    1.0f, 1.0f,  0.0f, 0.0f, -1.0f,  // Back-left-top

	// Front Face
	-0.5f * scale, -0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,    0.0f, 0.0f,  0.0f, 0.0f,  1.0f,  // Front-left-bottom
	 0.5f * scale, -0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,    1.0f, 0.0f,  0.0f, 0.0f,  1.0f,  // Front-right-bottom
	 0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,    1.0f, 1.0f,  0.0f, 0.0f,  1.0f,  // Front-right-top
	-0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 1.0f,    0.0f, 1.0f,  0.0f, 0.0f,  1.0f,  // Front-left-top

	// Left Face
	-0.5f * scale, -0.5f * scale, -0.5f * scale,  0.0f, 1.0f, 0.0f,    0.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  // Back-left-bottom
	-0.5f * scale, -0.5f * scale,  0.5f * scale,  0.0f, 1.0f, 0.0f,    1.0f, 0.0f,  -1.0f, 0.0f, 0.0f,  // Front-left-bottom
	-0.5f * scale,  0.5f * scale,  0.5f * scale,  0.0f, 1.0f, 0.0f,    1.0f, 1.0f,  -1.0f, 0.0f, 0.0f,  // Front-left-top
	-0.5f * scale,  0.5f * scale, -0.5f * scale,  0.0f, 1.0f, 0.0f,    0.0f, 1.0f,  -1.0f, 0.0f, 0.0f,  // Back-left-top

	// Right Face
	 0.5f * scale, -0.5f * scale, -0.5f * scale,  0.0f, 0.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Back-right-bottom
	 0.5f * scale, -0.5f * scale,  0.5f * scale,  0.0f, 0.0f, 1.0f,    1.0f, 0.0f,   1.0f, 0.0f, 0.0f,  // Front-right-bottom
	 0.5f * scale,  0.5f * scale,  0.5f * scale,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f,   1.0f, 0.0f, 0.0f,  // Front-right-top
	 0.5f * scale,  0.5f * scale, -0.5f * scale,  0.0f, 0.0f, 1.0f,    0.0f, 1.0f,   1.0f, 0.0f, 0.0f,  // Back-right-top

	 // Top Face
	 -0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 1.0f, 0.0f,    0.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Back-left-top
	  0.5f * scale,  0.5f * scale, -0.5f * scale,  1.0f, 1.0f, 0.0f,    1.0f, 0.0f,   0.0f, 1.0f, 0.0f,  // Back-right-top
	  0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 0.0f,    1.0f, 1.0f,   0.0f, 1.0f, 0.0f,  // Front-right-top
	 -0.5f * scale,  0.5f * scale,  0.5f * scale,  1.0f, 1.0f, 0.0f,    0.0f, 1.0f,   0.0f, 1.0f, 0.0f,  // Front-left-top

	 // Bottom Face
	 -0.5f * scale, -0.5f * scale, -0.5f * scale,  0.5f, 0.5f, 0.5f,    0.0f, 1.0f,   0.0f, -1.0f, 0.0f,  // Back-left-bottom
	  0.5f * scale, -0.5f * scale, -0.5f * scale,  0.5f, 0.5f, 0.5f,    1.0f, 1.0f,   0.0f, -1.0f, 0.0f,  // Back-right-bottom
	  0.5f * scale, -0.5f * scale,  0.5f * scale,  0.5f, 0.5f, 0.5f,    1.0f, 0.0f,   0.0f, -1.0f, 0.0f,  // Front-right-bottom
	 -0.5f * scale, -0.5f * scale,  0.5f * scale,  0.5f, 0.5f
};

GLuint objectCubeIndices[] = {
	// Back face
	0, 1, 2,
	0, 2, 3,
	// Front face
	4, 5, 6,
	4, 6, 7,
	// Left face
	8, 9, 10,
	8, 10, 11,
	// Right face
	12, 13, 14,
	12, 14, 15,
	// Top face
	16, 17, 18,
	16, 18, 19,
	// Bottom face
	20, 21, 22,
	20, 22, 23
};

GLfloat objectPyramidVertices[] =
{ //     COORDINATES     /        COLORS          /    TexCoord   /        NORMALS       //
	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,      0.0f, -1.0f, 0.0f, // Bottom side
	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 5.0f,      0.0f, -1.0f, 0.0f, // Bottom side
	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 5.0f,      0.0f, -1.0f, 0.0f, // Bottom side
	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, -1.0f, 0.0f, // Bottom side

	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,     -0.8f, 0.5f,  0.0f, // Left Side
	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,     -0.8f, 0.5f,  0.0f, // Left Side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,     -0.8f, 0.5f,  0.0f, // Left Side

	-0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, 0.5f, -0.8f, // Non-facing side
	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 0.0f,      0.0f, 0.5f, -0.8f, // Non-facing side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.0f, 0.5f, -0.8f, // Non-facing side

	 0.5f, 0.0f, -0.5f,     0.83f, 0.70f, 0.44f,	 0.0f, 0.0f,      0.8f, 0.5f,  0.0f, // Right side
	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.8f, 0.5f,  0.0f, // Right side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.8f, 0.5f,  0.0f, // Right side

	 0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f,	 5.0f, 0.0f,      0.0f, 0.5f,  0.8f, // Facing side
	-0.5f, 0.0f,  0.5f,     0.83f, 0.70f, 0.44f, 	 0.0f, 0.0f,      0.0f, 0.5f,  0.8f, // Facing side
	 0.0f, 0.8f,  0.0f,     0.92f, 0.86f, 0.76f,	 2.5f, 5.0f,      0.0f, 0.5f,  0.8f  // Facing side
};

// Indices for vertices order
GLuint objectPyramidIndices[] =
{
	0, 1, 2, // Bottom side
	0, 2, 3, // Bottom side
	4, 6, 5, // Left side
	7, 9, 8, // Non-facing side
	10, 12, 11, // Right side
	13, 15, 14 // Facing side
};

GLfloat objectLightVertices[] = {
	//     COORDINATES        
	-0.1f, -0.1f,  0.1f,      
	-0.1f, -0.1f, -0.1f,      
	 0.1f, -0.1f, -0.1f,      
	 0.1f, -0.1f,  0.1f,      
	-0.1f,  0.1f,  0.1f,      
	-0.1f,  0.1f, -0.1f,     
	 0.1f,  0.1f, -0.1f,    
	 0.1f,  0.1f,  0.1f
};

GLuint objectLightIndices[] =
{
	0, 1, 2,
	0, 2, 3,
	0, 4, 7,
	0, 7, 3,
	3, 7, 6,
	3, 6, 2,
	2, 6, 5,
	2, 5, 1,
	1, 5, 4,
	1, 4, 0,
	4, 5, 6,
	4, 6, 7
};

int main() {
	const char* screenTitle = "Float Arts";
	int viewPortX1 = 0, viewPortY1 = 0, viewPortX2 = SCREEN_WIDTH, viewPortY2 = SCREEN_HEIGHT;

	if (!glfwInit()) {
		errorLog("PVE", "INIT", "can't initilize glfw.", "");
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, screenTitle, NULL, NULL);
	if (window == NULL) {
		errorLog("PVE", "INIT", "can't create window.", "");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();
	glViewport(viewPortX1, viewPortY1, viewPortX2, viewPortY2);

	// Shader program and Bindings
	GLuint program = programInit(vertexShaderCode, fragmentShaderCode);
	
	ObjectData floatArtsCube = { objectCubeVerticesFull, sizeof(objectCubeVerticesFull), objectCubeIndices, sizeof(objectCubeIndices) };
	//ObjectData pyramid = { objectPyramidVertices, sizeof(objectPyramidVertices), objectPyramidIndices, sizeof(objectPyramidIndices) };
	std::tie(VAO, VBO, EBO) = createObject(floatArtsCube, 3, 11);
	//std::tie(VAO, VBO, EBO) = createObject(pyramid, 3, 11);
	checkOpenGLError();

	GLuint lightShader = programInit(vertexShaderLightCode, fragmentShaderLightCode);
	GLuint LVAO, LVBO, LEBO;
	ObjectData lightCube = { objectLightVertices, sizeof(objectLightVertices), objectLightIndices, sizeof(objectCubeIndices) };
	std::tie(LVAO, LVBO, LEBO) = createObject(lightCube, 0, 3);
	
	checkOpenGLError();

	// Uniforms
	GLuint uniformTexture0 = glGetUniformLocation(program, "tex0");
	GLuint uniformCamMatrix = glGetUniformLocation(program, "camMatrix");

	GLuint uniformLightColor_L = glGetUniformLocation(lightShader, "lightColor");
	GLuint uniformCamMatrix_L = glGetUniformLocation(lightShader, "camMatrix");
	GLuint uniformModel_L = glGetUniformLocation(lightShader, "model");

	// Light
	lightModel = glm::translate(lightModel, lightPos);
	cubeModel = glm::translate(cubeModel, cubePos);

	glUseProgram(lightShader);
	glUniformMatrix4fv(uniformModel_L, 1, GL_FALSE, glm::value_ptr(lightModel));
	glUniform4f(uniformLightColor_L, lightColor.x, lightColor.y, lightColor.z, lightColor.w);

	glUseProgram(program);
	glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));
	glUniform4f(glGetUniformLocation(program, "lightColor"), lightColor.x, lightColor.y, lightColor.z, lightColor.w);
	glUniform3f(glGetUniformLocation(program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	// Textures
	int textureFA_Height, textureFA_Width, textureFA_Col;
	stbi_set_flip_vertically_on_load(1);
	unsigned char* lastLoadedTexture = stbi_load("brick.png", &textureFA_Width, &textureFA_Height, &textureFA_Col, 0);
	if (!lastLoadedTexture) {
		errorLog("AVE", "LOAD", "can't load (FloatArts.png) texture.", "");
		glfwTerminate();
		return -1;
	}
	glGenTextures(1, &textureFloatArts);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureFloatArts);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // GL_NEAREST, GL_LINEAR
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S R T : X Y Z
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureFA_Width, textureFA_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, lastLoadedTexture);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(lastLoadedTexture);
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(program);
	glUniform1i(uniformTexture0, 0);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	// Initialize ImGui for GLFW and OpenGL
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	glfwSwapInterval(1);
	glEnable(GL_DEPTH_TEST);
	Cube newCube;
	newCube.position = cubePos;
	newCube.scale = 1.0f;
	newCube.color = glm::vec4(0.5f, 0.1f, 0.3f, 1.f);
	cubes.push_back(newCube);
	while (!glfwWindowShouldClose(window)) {
		display(window, program, lightShader, VAO, LVAO, glfwGetTime(), uniformCamMatrix);
		checkOpenGLError();
	}

	terminateObject(VAO, VBO, EBO);
	terminateProgram(program);
	terminateProgram(lightShader);
	terminateObject(LVAO, LVBO, LEBO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glDeleteTextures(1, &textureFloatArts);
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

//******************************************************************************************************************************
void display(GLFWwindow* window, GLuint program, GLuint lightShader, GLuint VAO, GLuint LVAO, double currentTime, 
	GLuint uniformCamMatrix) {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();




	// Create an Assets menu window
 // Set menu size
	ImGui::Begin("Assets", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos(ImVec2(300, 0)); // Stick to the top-left corner
	ImGui::SetWindowSize(ImVec2(300, SCREEN_HEIGHT));
	// Display a list of available textures
	ImGui::Text("Available Textures:");
	for (size_t i = 0; i < availableTextures.size(); i++) {
		if (ImGui::Button((std::to_string(i + 1) + ": " + availableTextures[i]).c_str())) {
			// Add the selected texture to the loaded textures list
			loadedTextures.push_back(availableTextures[i]);
		}
	}

	// Display a list of loaded textures
	ImGui::Text("Loaded Textures:");
	for (size_t i = 0; i < loadedTextures.size(); i++) {
		ImGui::Text("%d: %s", i + 1, loadedTextures[i].c_str());
		if (ImGui::Button(("Remove ##" + std::to_string(i)).c_str())) {
			loadedTextures.erase(loadedTextures.begin() + i);
			break; // Exit loop after deletion
		}
	}

	ImGui::End();




	


	// Create a menu window
  // Set menu size
	ImGui::Begin("Control Menu", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
	ImGui::SetWindowPos(ImVec2(0, 0)); // Stick to the right side
	ImGui::SetWindowSize(ImVec2(300, SCREEN_HEIGHT));
	
	if (ImGui::CollapsingHeader("Object")) {
		if (ImGui::Button("Add Cube")) {
			Cube newCube;
			newCube.position = glm::vec3(0.0f, 0.0f, 0.0f); // Default position
			newCube.scale = 1.0f;                           // Default scale
			newCube.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Default color
			cubes.push_back(newCube); // Add new cube to the list
		}

		// Control each cube
		for (size_t i = 0; i < cubes.size(); i++) {
			ImGui::PushID(static_cast<int>(i)); // Push unique ID for this cube

			ImGui::Text("Cube %d", i + 1);
			ImGui::SliderFloat3("Position", &cubes[i].position.x, -10.0f, 10.0f);
			ImGui::SliderFloat("Scale", &cubes[i].scale, 0.1f, 10.0f);
			ImGui::ColorEdit4("Color", &cubes[i].color.x);

			// Delete button for the cube
			if (ImGui::Button("Delete")) {
				cubes.erase(cubes.begin() + i);
				 // Pop ID before breaking out of the loop
				break; // Exit the loop after deleting the cube
			}

			if (ImGui::Button("Select")) {
				//cubes.erase(cubes.begin() + i);
				selectedCube = i;
				ImGui::PopID();
				 // Pop ID before breaking out of the loop
				break; 
			}

			ImGui::PopID(); // Ensure ID is popped even if no deletion occurs
		}
	}




	// تنظیمات نور
	if (ImGui::CollapsingHeader("Light Settings")) {
		ImGui::Text("Light Position");
		ImGui::SliderFloat3("Position", &lightPos.x, -10.0f, 10.0f);

		ImGui::Text("Light Color");
		ImGui::ColorEdit4("Color", &lightColor.x);

		ImGui::Text("Light Intensity");
		float lightIntensity = glm::length(glm::vec3(lightColor));
		if (ImGui::SliderFloat("Intensity", &lightIntensity, 0.0f, 5.0f)) {
			lightColor = glm::vec4(glm::normalize(glm::vec3(lightColor)) * lightIntensity, lightColor.w);
		}
	}

	if (ImGui::CollapsingHeader("Background Settings")) {
		ImGui::Text("Background Color");
		ImGui::ColorEdit4("Color", screenColor);
	}


	ImGui::End();

	// Update model matrix for each cube

	glClearColor(screenColor[0], screenColor[1], screenColor[2], screenColor[3]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



	inputs(window);



	// Camera
	glm::mat4 view = glm::mat4(1.0f);
	glm::mat4 proj = glm::mat4(1.0f);
	view = glm::lookAt(camPos, camPos + orientation, up);
	proj = glm::perspective(FOV, (float)SCREEN_WIDTH / SCREEN_HEIGHT, NEAR_PLANE, FAR_PLANE);

	glUseProgram(program);
	glUniform3f(glGetUniformLocation(program, "cam_pos"), camPos.x, camPos.y, camPos.z);
	glUniformMatrix4fv(uniformCamMatrix, 1, GL_FALSE, glm::value_ptr(proj * view));

	//***********************************************************************************
	//model = glm::mat4(1.0f);
	//model = glm::translate(model, cubePos); // جابه‌جایی
	//model = glm::rotate(model, glm::radians(cubeRotationY), glm::vec3(0.0f, 1.0f, 0.0f)); // چرخش Y
	//model = glm::rotate(model, glm::radians(cubeRotationX), glm::vec3(1.0f, 0.0f, 0.0f)); // چرخش X
	//model = glm::scale(model, glm::vec3(scale, scale, scale));

	lModel = glm::rotate(lModel, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
	//rotation = rotatingSpeed; // static rotation

	//double crntTime = glfwGetTime();
	//if (crntTime - lastTime >= 1 / 60) { 
	//	if (rotation <= 10400)
	//		rotation += rotatingSpeed;
	//	lastTime = crntTime;
	//}


	

	for (size_t i = 0; i < cubes.size(); i++) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, cubes[i].position); // جابه‌جایی به موقعیت مکعب
		model = glm::scale(model, glm::vec3(cubes[i].scale)); // مقیاس‌دهی مکعب
		glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(model));
		glUniform4f(glGetUniformLocation(program, "objectColor"), cubes[i].color.x, cubes[i].color.y, cubes[i].color.z, cubes[i].color.w);

		glBindTexture(GL_TEXTURE_2D, textureFloatArts);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, sizeof(objectCubeIndices) / sizeof(int), GL_UNSIGNED_INT, 0);
	}



	//cubeModel = glm::translate(model, cubePos);
	//glUniformMatrix4fv(glGetUniformLocation(program, "model"), 1, GL_FALSE, glm::value_ptr(cubeModel));

	lightModel = glm::translate(lModel, lightPos);
	glUseProgram(lightShader);
	glUniformMatrix4fv(glGetUniformLocation(lightShader, "model"), 1, GL_FALSE, glm::value_ptr(lightModel));
	glUseProgram(program); // Back to main program.
	//***********************************************************************************

	/*glBindTexture(GL_TEXTURE_2D, textureFloatArts);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, sizeof(objectCubeIndices) / sizeof(int), GL_UNSIGNED_INT, 0);*/

	glUseProgram(lightShader);
	glUniformMatrix4fv(glGetUniformLocation(lightShader, "camMatrix"), 1, GL_FALSE, glm::value_ptr(proj * view));
	glBindVertexArray(LVAO);
	glDrawElements(GL_TRIANGLES, sizeof(objectCubeIndices) / sizeof(int), GL_UNSIGNED_INT, 0);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	glfwSwapBuffers(window);
	glfwPollEvents();
}

GLuint programInit(const char* vertexShaderCode, const char* fragmentShaderCode) {
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER),
		fragmentShader = glCreateShader(GL_FRAGMENT_SHADER),
		shaderProgram = glCreateProgram();

	glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
	glCompileShader(vertexShader);
	checkShaderCompileErrors(vertexShader);

	glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
	glCompileShader(fragmentShader);
	checkShaderCompileErrors(fragmentShader);

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	checkProgramLinkErrors(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

void terminateProgram(GLuint program) {
	glDeleteProgram(program);
}

std::tuple<GLuint, GLuint, GLuint> createObject(ObjectData object, int layers, int length) {
	GLuint VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, object.verticesSize, object.vertices, GL_STATIC_DRAW);
	checkOpenGLError();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, object.indicesSize, object.indices, GL_STATIC_DRAW);
	checkOpenGLError();

	if (layers >= 0) {
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, length * sizeof(float), (void*)0); // vertices
		glEnableVertexAttribArray(0);
		checkOpenGLError();
	}

	if (layers >= 1) {
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, length * sizeof(float), (void*)(3 * sizeof(float))); // colors
		glEnableVertexAttribArray(1);
		checkOpenGLError();
	}

	if (layers >= 2) {
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, length * sizeof(float), (void*)(6 * sizeof(float))); // Textures
		glEnableVertexAttribArray(2);
		checkOpenGLError();
	}

	if (layers >= 3) {
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, length * sizeof(float), (void*)(8 * sizeof(float))); // Normals
		glEnableVertexAttribArray(3);
		checkOpenGLError();
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return std::make_tuple(VAO, VBO, EBO);
}

void terminateObject(GLuint VAO, GLuint VBO, GLuint EBO) {
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

void inputs(GLFWwindow* window) {


	static bool isScaling = false;

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		isScaling = true;
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_RELEASE) {
		isScaling = false;
	}

	glfwSetScrollCallback(window, [](GLFWwindow* window, double xoffset, double yoffset) {
		if (isScaling) {
			cubes[selectedCube].scale += yoffset * 0.1f;
			//scale = glm::clamp(scale, 0.1f, 10.0f);
		}
		});

	if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) 
			cubes[selectedCube].position.y += cubeMoveSpeed;
		//cubeModel = glm::translate(glm::mat4(1.0f), cubePos);
		
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			cubes[selectedCube].position.y -= cubeMoveSpeed;
		//cubeModel = glm::translate(glm::mat4(1.0f), cubePos);
		
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) 
			cubes[selectedCube].position.x -= cubeMoveSpeed;
		//cubeModel = glm::translate(glm::mat4(1.0f), cubePos);
		
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) 
			cubes[selectedCube].position.x += cubeMoveSpeed;
		//cubeModel = glm::translate(glm::mat4(1.0f), cubePos);
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			cubes[selectedCube].position.z += cubeMoveSpeed;
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			cubes[selectedCube].position.z -= cubeMoveSpeed;
	}
	
	
	//if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) { // زدن دکمه E
	//	Cube newCube;
	//	newCube.position = camPos; // موقعیت مکعب برابر موقعیت دوربین
	//	newCube.scale = 1.0f;
	//	newCube.color = glm::vec4(0.5f, 0.1f, 0.3f, 1.f);

	//	cubes.push_back(newCube); // اضافه کردن مکعب جدید به لیست
	//}

	//if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
	//	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // مخفی کردن نشانگر ماوس
	//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	//	if (firstClickR)
	//	{
	//		glfwSetCursorPos(window, (SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2));
	//		firstClickR = false;
	//	}

	//	double currentMouseX, currentMouseY;
	//	glfwGetCursorPos(window, &currentMouseX, &currentMouseY);

	//	if (!rotatingCube) {
	//		lastMouseX = currentMouseX;
	//		lastMouseY = currentMouseY;
	//		rotatingCube = true;
	//	}

	//	float deltaX = static_cast<float>(currentMouseX - lastMouseX);
	//	float deltaY = static_cast<float>(currentMouseY - lastMouseY);

	//	cubeRotationY += deltaX * cubeRotateSpeed; // چرخش در محور Y
	//	cubeRotationX += deltaY * cubeRotateSpeed; // چرخش در محور X

	//	lastMouseX = currentMouseX;
	//	lastMouseY = currentMouseY;

	//	//glfwSetCursorPos(window, (SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2));
	//	//if (currentMouseX <= 0 || currentMouseX >= SCREEN_WIDTH - 1 || currentMouseY <= 0 || currentMouseY >= SCREEN_HEIGHT - 1) {
	//	//	glfwSetCursorPos(window, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	//	//}
	//}
	//else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) {
	//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // نمایش مجدد نشانگر ماوس
	//	rotatingCube = false;
	//	firstClickR = true;
	//}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		// movement
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			camPos += camSpeed * orientation;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			camPos += camSpeed * -glm::normalize(glm::cross(orientation, up));
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			camPos += camSpeed * -orientation;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			camPos += camSpeed * glm::normalize(glm::cross(orientation, up));
		if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
			camPos += camSpeed * up;
		if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
			camPos += camSpeed * -up;

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
			camSpeed = 0.4f;

		else if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE)
			camSpeed = 0.1f;

		// camera

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

		if (firstClick)
		{
			glfwSetCursorPos(window, (SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2));
			firstClick = false;
		}

		double mouseX;
		double mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		float rotX = sensitivity * (float)(mouseY - (SCREEN_HEIGHT / 2)) / SCREEN_HEIGHT;
		float rotY = sensitivity * (float)(mouseX - (SCREEN_WIDTH / 2)) / SCREEN_WIDTH;

		glm::vec3 newOrientation = glm::rotate(orientation, glm::radians(-rotX), glm::normalize(glm::cross(orientation, up)));

		//std::cout << "abs(glm::angle(newOrientation, up) - glm::radians(90.0f): " << abs(glm::angle(newOrientation, up) - glm::radians(90.0f)) << std::endl;
		//std::cout << "orientation X/Y/Z" << orientation.x << "/" << orientation.y << "/" << orientation.z << std::endl;
		if (abs(glm::angle(newOrientation, up) - glm::radians(90.0f)) <= glm::radians(85.0f))
		{
			orientation = newOrientation;
		}

		orientation = glm::rotate(orientation, glm::radians(-rotY), up);

		glfwSetCursorPos(window, (SCREEN_WIDTH / 2), (SCREEN_HEIGHT / 2));
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		firstClick = true;
	}
}

void checkShaderCompileErrors(GLuint shader) {
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		errorLog("AVE", "SHAD", infoLog, "");
	}
}

void checkProgramLinkErrors(GLuint program) {
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char infoLog[512];
		glGetProgramInfoLog(program, 512, NULL, infoLog);
		errorLog("AVE", "PROG", infoLog, "");
	}
}

const char* getGLErrorString(GLenum err) {
	switch (err) {
	case GL_NO_ERROR:          return "No error";
	case GL_INVALID_ENUM:     return "Invalid enum";
	case GL_INVALID_VALUE:    return "Invalid value";
	case GL_INVALID_OPERATION: return "Invalid operation";
	case GL_OUT_OF_MEMORY:    return "Out of memory";
	default:                  return "Unknown error";
	}
}

void checkOpenGLError() {
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		errorLog("AVE", "NONE", getGLErrorString(err), "OpenGL ERROR");
	}
}

void errorLog(std::string category, std::string type, std::string massage, std::string comment) {
	if (comment.empty())
		std::cerr << "ERROR" << " [" << category << "], " << "TYPE" << " [" << type << "]: " << massage;
	else
		std::cerr << "ERROR" << " [" << category << "], " << "TYPE" << " [" << type << "] " << "(" << comment << ")" << massage;
}



