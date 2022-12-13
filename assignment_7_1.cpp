/*
* CS330_FinalProject.cpp
*
Created on: Dec 11, 2022
*
Author: Denis Dzenyuy
*/

#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"     // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera.h" // Camera class

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
/*Header Inclusions*/
	const char* const WINDOW_TITLE = "7-1 Final Project: 3D_Study Desk (Denis Dzenyuy)"; //Window title Macro

// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;
	// Stores the GL data relative to a given mesh
	struct GLMesh {
		GLuint vao; // Handle for the vertex array object
		GLuint vbos[2]; // Handle for the vertex buffer object
		GLuint nIndices; // Number of indices of the mesh
	};
	GLFWwindow* gWindow = nullptr;
	// Main GLFW window
	GLMesh gMesh;
	// Triangle mesh data
	// Shader program
	GLuint gProgramId;
	GLuint gLampProgramId;

	// Texture
	GLuint gTextureId;
	glm::vec2 gUVScale(1.0f, 1.0f);
	GLint gTexWrapMode = GL_REPEAT;

	// camera
	Camera gCamera(glm::vec3(0.0f, 0.0f, 20.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f; // Subject position and scale
	glm::vec3 gSubjectPosition(0.0f, 0.0f, 0.0f);
	glm::vec3 gSubjectScale(0.1f); // Subject and light color
	glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
	glm::vec3 gLightColor(1.0f, 1.0f, 1.0f); // Light position and scale
	glm::vec3 gLightPosition(0.0f, 15.0f, 0.0f);
	float lScaleVar = 0.3;
	glm::vec3 gLightScale(lScaleVar);
}
/*
User-defined Function prototypes to:
initialize the program, set the window size,
redraw graphics on the window when resized,
and render graphics on the screen
*/
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char*
	fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
//Vertex Shader Source Code
const GLchar* vertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
layout(location = 1) in vec3 normal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;
out vec3 vertexNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;
//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
	gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates
		vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment /pixel position in world space only(exclude view and projection)
		vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);
//Fragment Shader Source Code
const GLchar* fragmentShaderSource = GLSL(440,
	in vec3 vertexNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;
out vec4 fragmentColor; // For outgoing pyramid color to the GPU
// Uniform / Global variables for object color, light color, light position, and camera / view position
uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;
void main() {
	/*Phong lighting model calculations to generate ambient, diffuse, and specularcomponents*/
	//Calculate Ambient lighting*/
	float ambientStrength = 1.0f; // Set ambient or global lighting strength
	vec3 ambient = ambientStrength * lightColor; // Generate ambient light color
	//Calculate Diffuse lighting*/
	vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance(light direction) between light sourceand fragments / pixels on cube
		float impact = max(dot(norm, lightDirection), 0.0); // Calculate diffuse impact by generating dot product of normal and light
		vec3 diffuse = impact * lightColor; // Generate diffuse light color
		//Calculate Specular lighting*/
	float specularIntensity = 0.8f; // Set specular light strength
	float highlightSize = 8.0f; // Set specular highlight size
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
		vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
		//Calculate specular component
	float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0),highlightSize);
	vec3 specular = specularIntensity * specularComponent * lightColor;
	// Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
	// Calculate phong result
	vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;
	fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
}
);
/* Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(440, layout(location = 0) in vec3
	position; // VAP position 0 for vertex position data
	//Uniform / Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
	gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
}
);
/* Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(440,
	out vec4 fragmentColor; // For outgoing lamp color (smaller shape) to the GPU
void main()
{
	fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha1.0
}
);
// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j) {
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;
		for (int i = width * channels; i > 0; --i) {
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}
int main(int argc, char* argv[]) {
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;
	// Create the mesh
	UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
	// Create the shader program
	if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource,gProgramId))
		return EXIT_FAILURE;
	if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource,
		gLampProgramId))
		return EXIT_FAILURE;
	// Load texture
	const char* texFilename = "TexturePack10.png";
	if (!UCreateTexture(texFilename, gTextureId)) {
		cout << "Failed to load texture " << texFilename << endl;
		return EXIT_FAILURE;

	}
	glUseProgram(gProgramId);
	// tell opengl for each sampler to which texture unit it belongs to
		glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0);
	// We set the texture as texture unit 0
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Sets the background color of the window to black
		// render loop
		while (!glfwWindowShouldClose(gWindow)) {
			// per-frame timing
			float currentFrame = glfwGetTime();
			gDeltaTime = currentFrame - gLastFrame;
			gLastFrame = currentFrame;
			UProcessInput(gWindow);
			// input
			URender();
			// Render this frame
			glfwPollEvents();
		}
	UDestroyMesh(gMesh);
	// Release mesh data
	UDestroyTexture(gTextureId);
	// Release texture
	// Release shader programs
	UDestroyShaderProgram(gProgramId);
	UDestroyShaderProgram(gLampProgramId);
	exit(EXIT_SUCCESS);
	// Terminates the program successfully
}
// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window) {
	// GLFW: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// GLFW: window creation
	*window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL,
		NULL);
	if (*window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);
	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// GLEW: initialize
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();
	if (GLEW_OK != GlewInitResult) {
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}
	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;
	return true;
}

// process all input: query GLFW whether relevant keys are pressed//released this frame and react accordingly
void UProcessInput(GLFWwindow* window) {
	static const float cameraSpeed = 2.5f;
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && gTexWrapMode != GL_REPEAT) {
		glBindTexture(GL_TEXTURE_2D, gTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
		gTexWrapMode = GL_REPEAT;
		cout << "Current Texture Wrapping Mode: REPEAT" << endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode !=
		GL_MIRRORED_REPEAT)
	{
		glBindTexture(GL_TEXTURE_2D, gTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
		gTexWrapMode = GL_MIRRORED_REPEAT;
		cout << "Current Texture Wrapping Mode: REPEAT" << endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode !=
		GL_MIRRORED_REPEAT) {
		glBindTexture(GL_TEXTURE_2D, gTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		glBindTexture(GL_TEXTURE_2D, 0);
		gTexWrapMode = GL_MIRRORED_REPEAT;
		cout << "Current Texture Wrapping Mode: MIRRORED REPEAT" << endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && gTexWrapMode !=
		GL_CLAMP_TO_EDGE) {
		glBindTexture(GL_TEXTURE_2D, gTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
		gTexWrapMode = GL_CLAMP_TO_EDGE;
		cout << "Current Texture Wrapping Mode: CLAMP TO EDGE" << endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && gTexWrapMode !=
		GL_CLAMP_TO_BORDER) {
		float color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);
		glBindTexture(GL_TEXTURE_2D, gTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glBindTexture(GL_TEXTURE_2D, 0);
		gTexWrapMode = GL_CLAMP_TO_BORDER;
		cout << "Current Texture Wrapping Mode: CLAMP TO BORDER" << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) {
		gUVScale += 0.1f;
		cout << "Current gUVScale (" << gUVScale[0] << ", " << gUVScale[1] << ")"
			<< endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
		gUVScale -= 0.1f;
		cout << "Current gUVScale (" << gUVScale[0] << ", " << gUVScale[1] << ")"
			<< endl;
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS) {
		gSubjectScale += 0.05f;
		cout << "Current gSubjectScale (" << gSubjectScale[0] << ")" << endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS) {
		gSubjectScale -= 0.05f;
		cout << "Current gSubjectScale (" << gSubjectScale[0] << ")" << endl;
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		lScaleVar += 0.05f;
		cout << "Current lScaleVar (" << lScaleVar << ")" << endl;
	}
	else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		lScaleVar -= 0.05f;
		cout << "Current lScaleVar (" << lScaleVar << ")" << endl;
	}
}
// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}
// glfw: whenever the mouse moves, this callback is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos) {
	if (gFirstMouse) {
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}
	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top
		gLastX = xpos;
	gLastY = ypos;
	gCamera.ProcessMouseMovement(xoffset, yoffset);
}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	gCamera.ProcessMouseScroll(yoffset);
}
// glfw: handle mouse button events
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT: {
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
							   break;
	case GLFW_MOUSE_BUTTON_MIDDLE: {
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
								 break;
	case GLFW_MOUSE_BUTTON_RIGHT: {
		if (action == GLFW_PRESS)

			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;
	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}
// Function called to render a frame
void URender() {
	glEnable(GL_DEPTH_TEST);
	// Enable z-depth
	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindVertexArray(gMesh.vao);
	// Activate the cube VAO (used by cube and lamp)
	glUseProgram(gProgramId);
	// Set the shader to be used
	glm::mat4 model = glm::translate(gSubjectPosition) * glm::scale(gSubjectScale);
	// Model matrix: transformations are applied right-to-left order
	glm::mat4 view = gCamera.GetViewMatrix();
	// camera/view transformation
	glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom),
		(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	// Creates a perspective projection
		// Retrieves and passes transform matrices to the Shader program
		GLint modelLoc = glGetUniformLocation(gProgramId, "model");
	GLint viewLoc = glGetUniformLocation(gProgramId, "view");
	GLint projLoc = glGetUniformLocation(gProgramId, "projection");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	// Reference matrix uniforms from the Subject Shader program for the pyramid color, light color, light position, and camera position
		GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
	GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
	GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
	GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");
	// Pass color, light, and camera data to the Pyramid Shader program's corresponding uniforms
		glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
	glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
	glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y,
		gLightPosition.z);
	const glm::vec3 cameraPosition = gCamera.Position;
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y,cameraPosition.z);
	GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));
	glActiveTexture(GL_TEXTURE0);
	// bind textures on corresponding texture units
	glBindTexture(GL_TEXTURE_2D, gTextureId);
	glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL);
	glUseProgram(gLampProgramId);
	// LAMP: draw lamp
	// Reference matrix uniforms from the Lamp Shader program
	modelLoc = glGetUniformLocation(gLampProgramId, "model");
	viewLoc = glGetUniformLocation(gLampProgramId, "view");
	projLoc = glGetUniformLocation(gLampProgramId, "projection");
	// Pass matrix data to the Lamp Shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
	glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);
	glBindVertexArray(0);
	// Deactivate the Vertex Array Object
	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);
	// Flips the the back buffer with the front buffer every frame.
}
// Implements the UCreateMesh function
void UCreateMesh(GLMesh& mesh)
{
	// Position and Color data
	GLfloat verts[] = {
		//plane
		-200.0f,  100.0f,  0000.0f,   0.0f,  1.0f, //Top Left
		//0
		200.0f,100.0f,0000.0f,0.5f,1.0f, //Top Right
		//1
		200.0f,-100.0f,0000.0f,0.0f,0.5f, //Bottom Right
		//2
		-200.0f,-100.0f,0000.0f,0.5f,0.5f, //BottomLeft
		//3
		//Pencil
		-165.0f,-70.0f,5.0f,0.625f, 0.625f, //Base Origin
		//4
		-165.0f,-70.0f,10.0f,0.5f,0.5f, //Base 1
		//5
		-161.5f,-70.0f,8.5f,0.5f, 0.531f, //Base 2
		//6
		-160.0f,-70.0f,5.0f,0.5f, 0.562f, //Base 3
		//7
		-161.5f,-70.0f,1.5f,0.5f, 0.594f, //Base 4
		//8
		-165.0f,-70.0f,0.0f,0.5f, 0.625f,//Base 5
		//9
		-168.5f,-70.0f,1.5f,0.5f, 0.656f,//Base 6
		//10
		-170.0f,-70.0f,5.0f,0.5f, 0.688f,//Base 7
		//11
		-168.5f,-70.0f,8.5f,0.5f, 0.719f, //Base 8
		//12
		-165.0f,-25.0f,5.0f,0.625f, 0.625f, //Top Origin
		//13
		-165.0f,-25.0f,10.0f,0.75f,0.5f, //Top 1
		//14
		-161.5f,-25.0f,8.5f,0.75f, 0.531f, //Top 2
		//15
		-160.0f,-25.0f,5.0f,0.75f, 0.562f, //Top 3
		//16
		-161.5f,-25.0f,1.5f,0.75f, 0.594f, //Top 4
		//17
		-165.0f,-25.0f,0.0f,0.75f, 0.625f, //Top 5
		//18
		-168.5f,-25.0f,1.5f,0.75f, 0.656f, //Top 6
		//19
		-170.0f,-25.0f,5.0f,0.75f, 0.688f, //Top 7
		//20
		-168.5f,-25.0f,8.5f,0.75f, 0.719f, //Top 8
		//21
		-165.0f,-10.0f,5.0f,1.0f, 0.625f, //Cone Point
		//22
		//Paper Sheet
		-140.0f,30.0f,0.1f,0.5f,0.5f, //Top Left
		//23
		-60.0f,30.0f,0.1f,0.75f,0.5f, //Top Right
		//24
		-60.0f,-80.0f,0.1f,0.5f,0.25f, //Bottom Right
		//25
		-140.0f,-80.0f,0.1f,0.75f,0.25f, //Bottom Left
		//26
		//Keyboard
		-20.0f,0.0f,0.0f,0.0f,0.5f, //Base Top Left
		//27
		140.0f,0.0f,0.0f,0.5f,0.5f, //Base Top Right
		//28
		140.0f,-60.0f,0.0f,0.5f,0.0f, //Base Bottom Right
		//29
		-20.0f,-60.0f,0.0f,0.0f,0.0f,//Base Bottom Left
		//30
		-20.0f,0.0f,20.0f,0.5f,0.0f,//Top Top Left
		//31
		140.0f,0.0f,20.0f,0.0f,0.0f, //Top Top Right
		//32
		140.0f,-60.0f,20.0f,0.0f,0.5f, //Top Bottom Right
		//33
		-20.0f,-60.0f,20.0f,0.5f,0.5f, //Top Bottom Left
		//34
		//Monitor Base
		20.0f,80.0f,0.0f,0.0f,0.5f, //Base Top Left
		//35
		100.0f,80.0f,0.0f,0.5f,0.5f, //Base Top Right
		//36
		100.0f,50.0f,0.0f,0.5f,0.0f, //Base Bottom Right
		//37
		20.0f,50.0f,0.0f,0.0f,0.0f, //BaseBottom Left
		//38
		20.0f,80.0f,30.0f,0.5f,0.0f, //Top Top Left
		//39
		100.0f,80.0f,30.0f,0.0f,0.0f, //Top Top Right
		//40
		100.0f,50.0f,30.0f,0.0f,0.5f, //Top Bottom Right
		//41
		20.0f,50.0f,30.0f,0.5f,0.5f, //Top Bottom Left
		//42
		//Monitor Stand
		50.0f,70.0f,30.0f,0.0f,0.5f, //Base Top Left
		//43
		70.0f,70.0f,30.0f,0.5f,0.5f, //Base Top Right
		//44
		70.0f,60.0f,30.0f,0.5f,0.0f, //Base Bottom Right
		//45
		50.0f,60.0f,30.0f,0.0f,0.0f, //Base Bottom Left
		//46
		50.0f,70.0f,100.0f,0.5f,0.0f,//Top Top Left
		//47
		70.0f,70.0f,100.0f,0.0f,0.0f, //Top Top Right
		//48
		70.0f,60.0f,100.0f,0.0f,0.5f, //Top Bottom Right
		//49
		50.0f,60.0f,100.0f,0.5f,0.5f, //Top Bottom Left
		//50
		//Monitor Screen
		30.0f,60.0f,100.0f,0.0f,0.5f, //Base Top Left
		//51
		90.0f,60.0f,100.0f,0.5f,0.5f, //Base Top Right
		//52
		90.0f,60.0f,70.0f,0.5f,0.0f, //Base Bottom Right
		//53
		30.0f,60.0f,70.0f,0.0f,0.0f, //Base Bottom Left
		//54
		-40.0f,50.0f,130.0f,0.5f,0.0f, //Mid Top Left
		//55
		160.0f,50.0f,130.0f,0.0f,0.0f, //Mid Top Right
		//56
		160.0f,50.0f,40.0f,0.0f,0.5f, //MidBottom Right
		//57
		-40.0f,50.0f,40.0f,0.5f,0.5f, //MidBottom Left 
		//58
		-40.0f,40.0f,130.0f,0.0f,0.5f, //Top Top Left
		//59
		160.0f,40.0f,130.0f,0.5f,0.5f, //Top Top Right
		//60
		160.0f,40.0f,40.0f,0.5f,0.0f, //Top Bottom Right
		//61
		-40.0f,40.0f,40.0f,0.0f,0.0f, //Top Bottom Left
		//62
		//Cup
		-140.0f,65.0f,0.1f,0.5f,0.0f, //Base Origin
		//63
		-140.0f,80.0f,0.1f,0.5f,0.0f, //Base 1
		//64
		-128.0f,77.0f,0.1f,0.571f,0.0f, //Base 2
		//65
		-125.0f,65.0f,0.1f,0.643f,0.0f, //Base 3
		//66
		-128.0f,53.0f,0.1f,0.714f,0.0f, //Base 4
		//67
		-140.0f,50.0f,0.1f,0.786f,0.0f, //Base 5
		//68
		-152.0f,53.0f,0.1f,0.875f,0.0f, //Base 6
		//69
		-155.0f,65.0f,0.1f,0.929f,0.0f, //Base 7
		//70
		-152.0f,77.0f,0.1f,1.0f,0.0f, //Base 8
		//71
		-140.0f,65.0f,40.0f,0.5f,0.25f, //Top Origin
		//72
		-140.0f,80.0f,40.0f,0.5f,0.25f, //Top 1
		//73
		-128.0f,77.0f,40.0f,0.571f,0.25f, //Top 2
		//74
		-125.0f,65.0f,40.0f,0.643f,0.25f, //Top 3
		//75
		-128.0f,53.0f,40.0f,0.714f,0.25f, //Top 4
		//76
		-140.0f,50.0f,40.0f,0.786f,0.25f, //Top 5
		//77
		-152.0f,53.0f,40.0f,0.875f,0.25f, //Top 6
		//78
		-155.0f,65.0f,40.0f,0.929f,0.25f, //Top 7
		//79
		-152.0f,77.0f,40.0f,1.0f,0.25f, //Top 8
		//80
		//Mouse
		170.0f,-25.0f,0.0f,0.5f,0.0f, //Top Left
		//81
		180.0f,-25.0f,0.0f,0.0f,0.0f, //Top Right
		//82
		185.0f,-35.0f,0.0f,0.5f,0.0f, //Right
		//83
		180.0f,-50.0f,0.0f,0.0f,0.0f, //LowerRight
		//84
		177.5f,-55.0f,0.0f,0.0f,0.5f, //Bottom Right
		//85
		172.5f,-55.0f,0.0f,0.5f,0.5f, //Bottom Left
		//86
		170.0f,-50.0f,0.0f,0.0f,0.5f, //Lower Left
		//87
		165.0f,-35.0f,0.0f,0.5f,0.5f, //Left
		//88
		170.0f,-30.0f,10.0f,0.0f,0.5f, //Top Left
		//89
		180.0f,-30.0f,10.0f,0.5f,0.5f, //Top Right
		//90
		177.5f,-45.0f,10.0f,0.5f,0.0f, //Bottom Right
		//91
		172.5f,-45.0f,10.0f,0.0f,0.0f, //Bottom


		//Keyboard (key Texture)
		-20.0f,0.0f,20.0f,0.5f,1.0f,//Top Top Left
		//31
		//93
		140.0f,0.0f,20.0f,1.0f,1.0f, //Top Top Right
		//32
		//94
		140.0f,-60.0f,20.0f,1.0f,0.75f,//Top Bottom Right
		//33
		//95
		-20.0f,-60.0f,20.0f,0.5f,0.75f, //Top Bottom Left
		//34
		//96
		//Blank point
		//0000.0f,0000.0f,0000.0f,000.0f, 000.0f,
		//template
		//0
	};
	// Index data to share position data
	GLushort indices[] = {
		//Plane
		0,1,3,
		2,1,3,
		//Pencil
		4,5,6,
		4,6,7,
		4,7,8,
		4,8,9,
		4,9,10,
		4,10,11,
		4,11,12,
		4,12,5,
		5,6,14,
		15,6,14,
		6,7,15,
		16,7,15,
		7,8,16,
		17,8,16,
		8,9,17,
		18,9,17,
		9,10,18,
		19,10,18,
		10,11,19,
		20,11,19,
		11,12,20,
		21,12,20,
		12,5,21,
		14,5,21,
		14,15,22,
		15,16,22,
		16,17,22,
		17,18,22,
		18,19,22,
		19,20,22,
		20,21,22,
		21,14,22,
		//Paper Sheet
		23,24,26,
		25,24,26,
		//Keyboard
		27,28,30,
		29,28,30,
		27,28,31,
		31,32,28,
		28,29,32,
		32,33,29,
		29,30,33,
		33,34,30,
		27,30,31,
		31,34,30,
		93,94,96,
		95,96,94,
		//Monitor Base
		35,36,38,
		37,36,38,
		35,36,39,
		40,36,39,
		36,37,40,
		41,37,40,
		37,38,41,
		42,38,41,
		35,38,39,
		42,38,39,
		39,40,41,
		42,40,41,
		//Monitor Stand
		43,44,45,
		45,46,43,
		43,44,47,
		47,48,43,
		44,45,48,
		48,49,44,
		45,46,49,
		49,50,45,
		43,46,47,
		47,50,43,
		47,48,49,
		49,50,47,
		//Monitor Screen
		51,52,53,
		53,54,51,
		51,52,55,
		55,56,51,
		52,53,56,
		56,57,52,
		53,54,57,
		57,58,53,
		51,54,55,
		55,58,51,
		/**/
		55,56,59,
		59,60,55,
		56,57,60,
		60,61,56,
		57,58,61,
		61,62,57,
		55,58,59,
		59,62,55,
		59,60,61,
		61,62,59,
		//Cup
		63,64,65,
		63,65,66,
		63,66,67,
		63,67,68,
		63,68,69,
		63,69,70,
		63,70,71,
		63,71,64,
		64,65,73,
		74,65,73,
		65,66,74,
		75,66,74,
		66,67,75,
		76,67,75,
		67,68,76,
		77,68,76,
		68,69,77,
		78,69,77,
		69,70,78,
		79,70,78,
		70,71,79,
		80,71,79,
		71,64,80,
		73,64,80,
		//Mouse
		81,82,89,
		89,90,82,
		82,83,90,
		83,84,90,
		90,91,84,
		84,85,91,
		85,86,91,
		91,92,86,
		86,87,92,
		87,88,92,
		92,89,88,
		88,81,89,
		89,90,91,
		91,92,89,
	};
	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerColor = 2;
	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
		glBindVertexArray(mesh.vao);
	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU
		mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
		GL_STATIC_DRAW);
	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
		GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each
		// Create Vertex Attribute Pointers
		glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(2, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)
		(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(2);
}
void UDestroyMesh(GLMesh& mesh) {
	glDeleteVertexArrays(1, &mesh.vao);
	glDeleteBuffers(1, mesh.vbos);
}
//Generate and load the texture
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image) {
		flipImageVertically(image, width, height, channels);
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB,
				GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
				GL_UNSIGNED_BYTE, image);
		else {
			cout << "Not implemented to handle image with " << channels << "channels" << endl;
				return false;
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture
		return true;
	}
	return false;
	// Error loading the image
}
void UDestroyTexture(GLuint textureId) {
	glGenTextures(1, &textureId);
}
// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char*
	fragShaderSource, GLuint& programId) {
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];
	programId = glCreateProgram();
	// Create a Shader program object.
	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);
	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog <<
			std::endl;
		return false;
	}
	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog <<
			std::endl;
		return false;
	}
	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);
	glLinkProgram(programId);
	// links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog <<
			std::endl;
		return false;
	}
	glUseProgram(programId);
	// Uses the shader program
	return true;
}
void UDestroyShaderProgram(GLuint programId) {
	glDeleteProgram(programId);
}

