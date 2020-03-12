//
//  main.cpp
//  OpenGL Shadows
//
//  Created by CGIS on 05/12/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#define GLEW_STATIC

#include <iostream>
#include "glm/glm.hpp"//core glm functionality
#include "glm/gtc/matrix_transform.hpp"//glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include "Shader.hpp"
#include "SkyBox.hpp"
#include "Camera.hpp"
#define TINYOBJLOADER_IMPLEMENTATION

#include "Model3D.hpp"
#include "Mesh.hpp"

int glWindowWidth = 1920; //1920
int glWindowHeight = 1200; //1200
int retina_width, retina_height;
GLFWwindow* glWindow = NULL;

//const GLuint SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
const GLuint SHADOW_WIDTH = 10144, SHADOW_HEIGHT = 10144;
//const GLuint SHADOW_WIDTH = 12288, SHADOW_HEIGHT = 49152;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat3 lightDirMatrix;
GLuint lightDirMatrixLoc;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

gps::Camera myCamera(glm::vec3(0.0f, 0.7f, 3.0f), glm::vec3(0.0f, 0.7f, -10.0f));
GLfloat cameraSpeed = 0.7f;

bool pressedKeys[1024];
GLfloat angle;
GLfloat lightAngle;

gps::Model3D myModel;
gps::Model3D ground;
gps::Model3D lightCube;
gps::Model3D scene;
gps::Model3D wheel;
gps::Model3D broken_taxi;
gps::Model3D taxi_branches;
gps::Model3D taxi_leaves;
gps::Model3D dove;
gps::Model3D grass;
gps::Shader myCustomShader;
gps::Shader lightShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;


glm::vec3 lightPos1;
GLuint lightPos1Loc;
glm::vec3 lightPos2;
GLuint lightPos2Loc;
glm::vec3 lightPos3;
GLuint lightPos3Loc;
glm::vec3 lightPos4;
GLuint lightPos4Loc;

//-------------------------
typedef struct {
	glm::vec3 min;
	glm::vec3 max;
} BOUND_BOX;
//-------------------------

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

//=====================================
std::vector<BOUND_BOX> boundingBoxes;
BOUND_BOX generateBoundingBox(const gps::Mesh &mesh) {
	glm::vec3 min = glm::vec3(100.0, 100.0, 100.0);
	glm::vec3 max = glm::vec3(-100.0, -100.0, -100.0);

	for (gps::Vertex position : mesh.vertices) {

		if (position.Position.x < min.x)
			min.x = position.Position.x;
		else min.x = min.x;

		if (position.Position.y < min.y)
			min.y = position.Position.y;
		else min.y = min.y;

		if (position.Position.z < min.z)
			min.z = position.Position.z;
		else min.z = min.z;

		if (position.Position.x > max.x)
			max.x = position.Position.x;
		else max.x = max.x;

		if (position.Position.y > max.y)
			max.y = position.Position.y;
		else max.y = max.y;

		if (position.Position.z > max.z)
			max.z = position.Position.z;
		else max.z = max.z;

	}

	BOUND_BOX box;

	box.max = max;
	box.min = min;

	return box;
}

bool canMoveCamera() {
	for (BOUND_BOX boundingBox : boundingBoxes) { //cameraPosition += cameraDirection * cameraSpeed
		if ((myCamera.getCameraPosition().x + myCamera.getCameraDirection().x * cameraSpeed > boundingBox.min.x && myCamera.getCameraPosition().x + myCamera.getCameraDirection().x * cameraSpeed < boundingBox.max.x) &&
			(myCamera.getCameraPosition().y + myCamera.getCameraDirection().y * cameraSpeed > boundingBox.min.y && myCamera.getCameraPosition().y + myCamera.getCameraDirection().y * cameraSpeed < boundingBox.max.y) &&
			(myCamera.getCameraPosition().z + myCamera.getCameraDirection().z * cameraSpeed > boundingBox.min.z && myCamera.getCameraPosition().z + myCamera.getCameraDirection().z * cameraSpeed < boundingBox.max.z)) {
			return false;
		}
	}

	return true;
}
//======================================


void windowResizeCallback(GLFWwindow* window, int width, int height)
{
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO
	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	myCustomShader.useShaderProgram();

	//set projection matrix
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	//send matrix data to shader
	GLint projLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	lightShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

	//set Viewport transform
	glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}


bool firstMouse = true;

float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch;

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.15f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	myCamera.rotate(pitch, yaw);
}

bool ok = false;
bool okGrass = false;
gps::MOVE_DIRECTION moveDirection;
void processMovement()
{
	if (pressedKeys[GLFW_KEY_R]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	if (pressedKeys[GLFW_KEY_Q]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}

	if (pressedKeys[GLFW_KEY_E]) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}

	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		moveDirection = gps::MOVE_FORWARD;
	}

	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		moveDirection = gps::MOVE_LEFT;
	}

	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		moveDirection = gps::MOVE_BACKWARD;

	}
	
	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		moveDirection = gps::MOVE_RIGHT;

	}

	if (pressedKeys[GLFW_KEY_UP]) {
		myCamera.move(gps::MOVE_UP, cameraSpeed);
		moveDirection = gps::MOVE_UP;

	}

	if (pressedKeys[GLFW_KEY_LEFT]) {
		myCamera.move(gps::ROTATE_LEFT, cameraSpeed);
		moveDirection = gps::ROTATE_LEFT;

	}

	if (pressedKeys[GLFW_KEY_DOWN]) {
		myCamera.move(gps::MOVE_DOWN, cameraSpeed);
		moveDirection = gps::MOVE_DOWN;

	}

	if (pressedKeys[GLFW_KEY_RIGHT]) {
		myCamera.move(gps::ROTATE_RIGHT, cameraSpeed);
		moveDirection = gps::ROTATE_RIGHT;

	}

	if (pressedKeys[GLFW_KEY_J]) {

		lightAngle += 3.0f;
		if (lightAngle > 360.0f)
			lightAngle -= 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_L]) {
		lightAngle -= 3.0f;
		if (lightAngle < 0.0f)
			lightAngle += 360.0f;
		glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
		myCustomShader.useShaderProgram();
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDirTr));
	}

	if (pressedKeys[GLFW_KEY_M]) {
		ok = true;
	}

	if (pressedKeys[GLFW_KEY_SPACE]) {
		ok = false;
	}

	if (pressedKeys[GLFW_KEY_N]) { //horror
		//lightColor = glm::vec3(0.2f, 0.2f, 0.2f);
		myCustomShader.useShaderProgram();
		lightColor = glm::vec3(0.2f, 0.2f, 0.2f);
		lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		//lightShader.useShaderProgram();
		//glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	}

	if (pressedKeys[GLFW_KEY_B]) {//normal
		myCustomShader.useShaderProgram();
		lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		//lightShader.useShaderProgram();
		//glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
	}

	if (pressedKeys[GLFW_KEY_G]) {
		okGrass = true;
	}

	if (pressedKeys[GLFW_KEY_H]) {
		okGrass = false;
	}

	//-------------------------------------------------------------------------------------
	if (!canMoveCamera()) {
		switch (moveDirection) {
		case gps::MOVE_FORWARD:
			myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
			break;
		case gps::MOVE_BACKWARD:
			myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
			break;
		case gps::MOVE_LEFT:
			myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
			break;
		case gps::MOVE_RIGHT:
			myCamera.move(gps::MOVE_LEFT, cameraSpeed);
			break;
		}
	}
	//-------------------------------------------------------------------------------------

}

bool initOpenGLWindow()
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	//for Mac OS X
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwMakeContextCurrent(glWindow);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	//for RETINA display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	return true;
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);
	glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs()
{
	//generate FBO ID
	glGenFramebuffers(1, &shadowMapFBO);

	//create depth texture for FBO
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); //GL_NEAREST
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//attach texture to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix()
{
	const GLfloat near_plane = 7.0f, far_plane = 50.0f;
	glm::mat4 lightProjection = glm::ortho(-30.0f, 30.0f, -30.0f, 30.0f, near_plane, far_plane); //20.0

	glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
	glm::mat4 lightView = glm::lookAt(myCamera.getCameraTarget() + 1.0f * lightDirTr, myCamera.getCameraTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

	return lightProjection * lightView;
}

void initModels()
{
	myModel = gps::Model3D("objects/nanosuit/nanosuit.obj", "objects/nanosuit/");
	ground = gps::Model3D("objects/ground/ground.obj", "objects/ground/");
	lightCube = gps::Model3D("objects/cube/cube.obj", "objects/cube/");

	scene = gps::Model3D("objects/updatedv9/new_design.obj", "objects/updatedv9/"); //there are multiple versions (updatedv3, updatedv7, updatedv9, updatedv10)

	wheel = gps::Model3D("objects/Wheel/Wheel.obj", "objects/Wheel/");

	broken_taxi = gps::Model3D("objects/broken_taxi/broken_taxi.obj", "objects/broken_taxi/");
	taxi_branches = gps::Model3D("objects/taxi_branches/taxi_branches.obj", "objects/taxi_branches/");
	taxi_leaves = gps::Model3D("objects/taxi_leaves/taxi_leaves.obj", "objects/taxi_leaves/");

	dove = gps::Model3D("objects/porumbel/porumbel.obj", "objects/porumbel/");

	grass = gps::Model3D("objects/grass/grass.obj", "objects/grass/");
}

void initShaders()
{
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	depthMapShader.loadShader("shaders/simpleDepthMap.vert", "shaders/simpleDepthMap.frag");

	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
}

void initUniforms()
{
	myCustomShader.useShaderProgram();

	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");

	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");

	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");

	lightDirMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDirMatrix");

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 2.5f, 10.0f);
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//==============|point lights|===============
	lightPos1 = glm::vec3(0.0f, 0.0f, 0.0f);
	lightPos1Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos1"); //albastru
	glUniform3fv(lightPos1Loc, 1, glm::value_ptr(lightPos1));

	lightPos2 = glm::vec3(10.0f, 2.0f, -4.0f);
	lightPos2Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos2"); //galben
	glUniform3fv(lightPos2Loc, 1, glm::value_ptr(lightPos2));

	lightPos3 = glm::vec3(2.0f, 2.0f, -15.0f);
	lightPos3Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos3"); //galben
	glUniform3fv(lightPos3Loc, 1, glm::value_ptr(lightPos3));

	lightPos4 = glm::vec3(-4.0f, 0.0f, -9.0f);
	lightPos4Loc = glGetUniformLocation(myCustomShader.shaderProgram, "lightPos4"); //galben
	glUniform3fv(lightPos4Loc, 1, glm::value_ptr(lightPos4));
	//==============|/point lights|===============


	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

}

float wheelAngle;
float movement = -10.0f, deplasareS = -10.0f, deplasareD = -10.0f;
float i = 0.1f;
GLfloat pidgeonRotation = 0.0f, pidgeonRotationS = 0.0f, pidgeonRotationD = 0.0f;
void renderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	processMovement();

	//===============================|depthMapShader|=============================

	//render the scene to the depth buffer (first pass)

	depthMapShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	//create model matrix for nanosuit
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	myModel.Draw(depthMapShader);

	//create model matrix for ground
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "model"),
		1,
		GL_FALSE,
		glm::value_ptr(model));

	scene.Draw(depthMapShader);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//===============================|/depthMapShader|============================




	//===============================|myCustomShader|=============================
	//render the scene (second pass)

	myCustomShader.useShaderProgram();

	//send lightSpace matrix to shader
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));

	//send view matrix to shader
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(myCustomShader.shaderProgram, "view"),
		1,
		GL_FALSE,
		glm::value_ptr(view));

	//compute light direction transformation matrix
	lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
	//send lightDir matrix data to shader
	glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

	glViewport(0, 0, retina_width, retina_height);

	myCustomShader.useShaderProgram();

	//bind the depth map
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glUniform1i(glGetUniformLocation(myCustomShader.shaderProgram, "shadowMap"), 3);

	//create model matrix for nanosuit ---------------------------------------------NANOSUIT
	model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
	//send model matrix data to shader	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//compute normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	myModel.Draw(myCustomShader);

	//create model matrix for wheel -----------------------------------------------WHEEL

	wheelAngle += 0.1f;
	if (wheelAngle > 360.0f)
		wheelAngle -= 360.0f;

	model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f));
	model = glm::translate(model, glm::vec3(23.95f, -0.4f, -10.25f));
	model = glm::rotate(model, wheelAngle, glm::vec3(0, 1, 0));


	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	wheel.Draw(myCustomShader);

	//create model matrix for wheel -----------------------------------------------BROKEN_TAXI

	//model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(10.25f, -0.5f, -4.0f));
	model = glm::rotate(model, -glm::radians(90.0f), glm::vec3(0, 1, 0));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	broken_taxi.Draw(myCustomShader);

	//create model matrix for wheel -----------------------------------------------TAXI_BRANCHES

	//model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(10.2f, -0.5f, -4.0f));
	model = glm::rotate(model, -glm::radians(90.0f), glm::vec3(0, 1, 0));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	taxi_branches.Draw(myCustomShader);

	//create model matrix for wheel -----------------------------------------------TAXI_LEAVES

	//model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(10.2f, -0.5f, -4.0f));
	model = glm::rotate(model, -glm::radians(90.0f), glm::vec3(0, 1, 0));


	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	taxi_leaves.Draw(myCustomShader);

	//create model matrix for wheel -----------------------------------------------DOVE_CORP
	model = glm::mat4(1.0f);
	movement += i;

	if (movement > 10.0)
	{
		movement = 10.0f;
		i = -i; //-
		pidgeonRotation = glm::radians(180.0f);
	}
	if (movement < -10.0f)
	{
		movement = -10.0f;
		i = -i; //+
		pidgeonRotation = glm::radians(0.0f);
	}

	//model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f));
	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, -4.0f + movement));
	//model = glm::rotate(model, -glm::radians(90.0f), glm::vec3(0, 1, 0));
	model = glm::rotate(model, pidgeonRotation, glm::vec3(0, 1, 0));

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	dove.Draw(myCustomShader);

	//create model matrix for scene ------------------------------------------------SCENE

	model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	//send model matrix data to shader
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	//create normal matrix
	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	//send normal matrix data to shader
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	scene.Draw(myCustomShader);



	//create model matrix for scene ------------------------------------------------GRASS
	if (okGrass) {
		model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));
		//send model matrix data to shader
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		//create normal matrix
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		//send normal matrix data to shader
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

		grass.Draw(myCustomShader);
	}

	//===============================|/myCustomShader|============================




	//===============================|lightShader|================================

	//draw a white cube around the light

	lightShader.useShaderProgram();

	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

	model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::translate(model, 1.0f * lightDir);
	model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

	lightCube.Draw(lightShader);
	//===============================|/lightShader|===============================

	mySkyBox.Draw(skyboxShader, view, projection);
}

//-------------------------------------------------------------------------------------
void initBoundingBoxes() {
	for (const gps::Mesh &mesh : scene.getMeshes()) {
		boundingBoxes.push_back(generateBoundingBox(mesh));
	}
}
//-------------------------------------------------------------------------------------

std::vector<const GLchar*> faces;

void createSkyBox() {
	faces.push_back("textures/skybox/right.tga");
	faces.push_back("textures/skybox/left.tga");
	faces.push_back("textures/skybox/top.tga");
	faces.push_back("textures/skybox/bottom.tga");
	faces.push_back("textures/skybox/back.tga");
	faces.push_back("textures/skybox/front.tga");
}

void loadSkyBox() {
	mySkyBox.Load(faces);
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
	view = myCamera.getViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE,
		glm::value_ptr(view));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE,
		glm::value_ptr(projection));
}

GLfloat angle2;
int okR = 0, okM = 0; //R - rotate; M - move
int decr, incr;
void sceneDemo() {

	if (angle2 < 275.0f && okR == 0) {
		myCamera.rotate(0.0f, angle2);
		angle2 += 5.0f;
	}
	else if (myCamera.getCameraPosition().z > -10.0f && okM == 0) {
		if (decr == 0) {
			angle2 -= 5.0f;
			decr = 1;
		}
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		okR = 1;
	}
	else if (angle2 > 85.0f && okR == 1) {
		myCamera.rotate(0.0f, angle2);
		angle2 -= 5.0f;
		okM = 1;
	}
	else if (myCamera.getCameraPosition().z < 3.0f && okM == 1) {
		if (incr == 0) {
			angle2 += 5.0f;
			incr = 1;
		}
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		okR = 2;
	}
	else if (angle2 < 275.0f && okR == 2) {
		myCamera.rotate(0.0f, angle2);
		angle2 += 5.0f;
	}
	else { ok = false; }


}

int main(int argc, const char * argv[]) {

	initOpenGLWindow();
	initOpenGLState();
	initFBOs();
	initModels();
	initShaders();
	//initUniforms();
	glCheckError();

	createSkyBox();
	loadSkyBox();

	initBoundingBoxes();
	//lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	initUniforms();

	while (!glfwWindowShouldClose(glWindow)) {
		renderScene();
		if (ok)
			sceneDemo();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	//close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}
