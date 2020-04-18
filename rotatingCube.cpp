// rotatingCube.cpp: rotate a prismatic cube
// Christopher Go
#include <glad.h>
#include <glfw3.h>
#include <stdio.h>
#include "GLXtras.h"
#include <time.h>
#include <vector>

#include "VecMat.h"

//mouse control variables
//rotational continuity between mouse clicks
vec2 mouseDown;
vec2 rotOld, rotNew;
vec3 tranOld, tranNew (0, 0, -1);


//control rotational speed
float rotSpeed = 0.3f;

// GPU identifiers
GLuint vBuffer = 0;
GLuint program = 0;

//the cube
float l = -1, r = 1, b = -1, t = 1, n = -1, f = 1; //left , right , bottom, top, near, far
float points [][3] = {{l, b, n}, {l, b, f}, {l, t, n}, {l, t, f}, {r, b, n}, {r, b, f}, {r, t, n}, {r, t, f}}; //8 points
float colors [][3] = {{0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, {1, 1, 0}, {0, 0, 0}, {1, 1, 1}}; // 8 colors
int faces[][4] = {{1, 3, 2, 0}, {6, 7, 5, 4}, {4, 5, 1, 0}, {3, 7, 6, 2}, {2, 6, 4, 0}, {5, 7, 3, 1}}; //6 faces

//define the field of view, and scale and stretch factors for the cube
float fieldOfView = 30, cubeSize = 0.05f, cubeStretch = cubeSize;

//not sure how to put comments in the vertex shader without messing things up, so comments are here
// float c and float s = shortcuts for sin and cos
// x2 and y2 = rotational formulas given in the book
const char *vertexShader = "\
	#version 130								\n\
	in vec3 point;								\n\
	in vec3 color;								\n\
	uniform mat4 view;							\n\
	out vec4 vColor;								\n\
	void main() {								\n\
		gl_Position = view*vec4(point, 1);		\n\
	    vColor = vec4(color, 1);				\n\
	}";

const char *pixelShader = "\
	#version 130								\n\
	in vec4 vColor;								\n\
	out vec4 pColor;							\n\
	void main() {								\n\
        pColor = vColor;							\n\
	}";

void MouseButton(GLFWwindow *w, int butn, int action, int mods)
{
	//called when mouse button pressed or released
	if (action == GLFW_PRESS) {
		double x, y;
		glfwGetCursorPos(w, &x, &y);
		mouseDown = vec2((float)x, (float)y); //save reference for MouseDrag
	}
	if (action == GLFW_RELEASE) {
		rotOld = rotNew;
		//tranOld = tranNew;
	}
}

void MouseMove(GLFWwindow *w, double x, double y) {
	if (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		//find mouse drag difference
		vec2 mouse((float)x, (float)y), dif = mouse - mouseDown;
		rotNew = rotOld + rotSpeed * dif;
	}
}
void MouseWheel(GLFWwindow *w, double xoffset, double yoffset)
{
	double direction = yoffset;
	tranNew.z += direction > 0 ? -.1f : .1f; //dolly in/out
}
void InitVertexBuffer() {
	// make GPU buffer for points & colors, set it active buffer
	glGenBuffers(1, &vBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);
	// allocate buffer memory to hold vertex locations and colors
	int sPnts = sizeof(points), sCols = sizeof(colors);
	glBufferData(GL_ARRAY_BUFFER, sPnts + sCols, NULL, GL_STATIC_DRAW);
	// load data to the GPU
	glBufferSubData(GL_ARRAY_BUFFER, 0, sPnts, points);
	// start at beginning of buffer, for length of points array
	glBufferSubData(GL_ARRAY_BUFFER, sPnts, sCols, colors);
	// start at end of points array, for length of colors array
}

void Display(GLFWwindow *window) {
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	//smooth lines
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	
	int screenWidth, screenHeight;
	//obtain the current display size in pixels
	glfwGetWindowSize(window, &screenWidth, &screenHeight);
	//compute the aspect ratio
	int halfWidth = screenWidth/2;
	float aspectRatio = (float) halfWidth/ (float) screenHeight;
	//values for the near and far clipping planes for "VecMat.h"
	float N = 0.01f, F = 500; // near and far clipping plane distances
	mat4 persp = Perspective (fieldOfView, aspectRatio, N, F);

	//affine transformations to compute a modelView
	mat4 scale = Scale(cubeSize, cubeSize, cubeStretch); //unit cube too big
	mat4 rot = RotateY(rotNew.x)*RotateX(rotNew.y); //mouse-specified
	mat4 tran = Translate(tranNew); //mouse-specified
	mat4 modelview = tran*rot*scale; //total affine transformations
	mat4 view = persp*modelview; //one matrix for all transformations
	SetUniform(program, "view", view); //send final view to shader

	glClearColor(.5, .5, .5, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	// access GPU vertex buffer
	glUseProgram(program);

	// steps from book

	glBindBuffer(GL_ARRAY_BUFFER, vBuffer);

	// associate position input to shader with position array in vertex buffer
	VertexAttribPointer(program, "point", 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
	// associate color input to shader with color array in vertex buffer
	VertexAttribPointer(program, "color", 3, GL_FLOAT, GL_FALSE, 0, (void *) sizeof(points));
	// render three vertices as a triangle

	//shade cube on the left half of the display
	glViewport(0, 0, halfWidth, screenHeight); 
	glDrawElements(GL_QUADS, sizeof(faces) / sizeof(int), GL_UNSIGNED_INT, faces);
//	glDrawElements(GL_TRIANGLES, sizeof(triangles) / sizeof(int), GL_UNSIGNED_INT, triangles);

	//shade right half of cube
	glViewport(halfWidth, 0, halfWidth, screenHeight);
	glPointSize(5);
	glLineWidth(5);
	glDrawArrays(GL_POINTS, 0, 8);
	for(int i= 0; i < 6; i++)
		glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, &faces[i]);
	glFlush();
}

// application

void ErrorGFLW(int id, const char *reason) {
	printf("GFLW error %i: %s\n", id, reason);
	getchar();
}

void Keyboard(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

int main() {
	//register callback in main()
	glfwSetErrorCallback(ErrorGFLW);
	if (!glfwInit())
		return 1;
	GLFWwindow *window = glfwCreateWindow(600, 600, "Colorful Letter", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return 1;
	}
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	printf("GL version: %s\n", glGetString(GL_VERSION));
	PrintGLErrors();
	if (!(program = LinkProgramViaCode(&vertexShader, &pixelShader)))
		return 0;
	InitVertexBuffer();
	glfwSetKeyCallback(window, Keyboard);
	glfwSetMouseButtonCallback(window, MouseButton);
	glfwSetCursorPosCallback(window, MouseMove);
	glfwSetScrollCallback(window, MouseWheel);
	while (!glfwWindowShouldClose(window)) {
		Display(window);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}
