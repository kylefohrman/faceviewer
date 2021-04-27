// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <array>
#include <stack>   
#include <sstream>
// Include GLEW
#include <GL/glew.h>
// Include GLFW
#include <GLFW/glfw3.h>
// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
using namespace glm;
// Include AntTweakBar
#include <AntTweakBar.h>

#include <common/shader.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

#include <common/texture.hpp>


const int window_width = 600, window_height = 600;

typedef struct Vertex {
	float Position[4];
	float Color[4];
	float Normal[3];
	float UV[2] = { -1.0f, -1.0f };

	void SetPosition(float *coords) {
		Position[0] = coords[0];
		Position[1] = coords[1];
		Position[2] = coords[2];
		Position[3] = 1.0;
	}
	void SetColor(float *color) {
		Color[0] = color[0];
		Color[1] = color[1];
		Color[2] = color[2];
		Color[3] = color[3];
	}
	void SetNormal(float *coords) {
		Normal[0] = coords[0];
		Normal[1] = coords[1];
		Normal[2] = coords[2];
	}
	void SetUV(float *coords){
		UV[0] = coords[0];
		UV[1] = coords[1];
	}
};

// function prototypes
int initWindow(void);
void initOpenGL(void);
void createVAOs(Vertex[], GLushort[], int);
void loadObject(char*, glm::vec4, Vertex* &, GLushort* &, int);
void createObjects(void);
void pickObject(void);
void renderScene(void);
void cleanup(void);
static void keyCallback(GLFWwindow*, int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);

void key_up(void);
void key_down(void);
void key_left(void);
void key_right(void);

// GLOBAL VARIABLES
GLFWwindow* window;

glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;

GLuint gPickedIndex = -1;
std::string gMessage;

GLuint programID;
GLuint pickingProgramID;

const GLuint NumObjects = 5;	// ATTN: THIS NEEDS TO CHANGE AS YOU ADD NEW OBJECTS
GLuint VertexArrayId[NumObjects];
GLuint VertexBufferId[NumObjects];
GLuint IndexBufferId[NumObjects];

// TL
size_t VertexBufferSize[NumObjects];
size_t IndexBufferSize[NumObjects];
size_t NumIdcs[NumObjects];
size_t NumVerts[NumObjects];

GLuint MatrixID;
GLuint ModelMatrixID;
GLuint ViewMatrixID;
GLuint ProjMatrixID;
GLuint PickingMatrixID;
GLuint pickingColorID;
GLuint LightID;

// Declare global objects
// TL
const size_t CoordVertsCount = 6;
const size_t GridVertsCount = 44;
Vertex CoordVerts[CoordVertsCount];
Vertex GridVerts[GridVertsCount];

char selection = 'C';
float adjustment = 0.0f;
bool shift_press = false;
bool f_wireframe = true;
bool f_facets = false;
bool p_smooth = false;
bool u_texture = true;
bool g_grid = true;
bool animate = false;
const float PI = 3.14159265;
bool texBool = false;
int texture;

//transformation variables
float rot_camera_side = PI/4;
float rot_camera_up = PI/3;

int initWindow(void) {
	// Initialise GLFW
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);	// FOR MAC

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(window_width, window_height, "Fohrman, Kyle (UFID 0514-1178)", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Initialize the GUI
	TwInit(TW_OPENGL_CORE, NULL);
	TwWindowSize(window_width, window_height);
	TwBar * GUI = TwNewBar("Picking");
	TwSetParam(GUI, NULL, "refresh", TW_PARAM_CSTRING, 1, "0.1");
	TwAddVarRW(GUI, "Last picked object", TW_TYPE_STDSTRING, &gMessage, NULL);

	// Set up inputs
	glfwSetCursorPos(window, window_width / 2, window_height / 2);
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	return 0;
}

void initOpenGL(void) {
	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);
	// Or, for an ortho camera :
	//gProjectionMatrix = glm::ortho(-4.0f, 4.0f, -3.0f, 3.0f, 0.0f, 100.0f); // In world coordinates

	// Camera matrix
	gViewMatrix = glm::lookAt(glm::vec3(10.0, 10.0, 10.0f),	// eye
		glm::vec3(0.0, 0.0, 0.0),	// center
		glm::vec3(0.0, 1.0, 0.0));	// up

	// Create and compile our GLSL program from the shaders
	programID = LoadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	pickingProgramID = LoadShaders("Picking.vertexshader", "Picking.fragmentshader");

	// Get a handle for our "MVP" uniform
	MatrixID = glGetUniformLocation(programID, "MVP");
	ModelMatrixID = glGetUniformLocation(programID, "M");
	ViewMatrixID = glGetUniformLocation(programID, "V");
	ProjMatrixID = glGetUniformLocation(programID, "P");

	PickingMatrixID = glGetUniformLocation(pickingProgramID, "MVP");
	// Get a handle for our "pickingColorID" uniform
	pickingColorID = glGetUniformLocation(pickingProgramID, "PickingColor");
	// Get a handle for our "LightPosition" uniform
	LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// TL
	// Define objects
	createObjects();

	// ATTN: create VAOs for each of the newly created objects here:
	VertexBufferSize[0] = sizeof(CoordVerts);
	VertexBufferSize[1] = sizeof(GridVerts);
	NumVerts[0] = CoordVertsCount;
	NumVerts[1] = GridVertsCount;

	createVAOs(CoordVerts, NULL, 0);
	createVAOs(GridVerts, NULL, 1);
}

void createVAOs(Vertex Vertices[], unsigned short Indices[], int ObjectId) {
	GLenum ErrorCheckValue = glGetError();
	const size_t VertexSize = sizeof(Vertices[0]);
	const size_t RgbOffset = sizeof(Vertices[0].Position);
	const size_t Normaloffset = sizeof(Vertices[0].Color) + RgbOffset;
	const size_t UVoffset = sizeof(Vertices[0].Normal) + Normaloffset;

	// Create Vertex Array Object
	glGenVertexArrays(1, &VertexArrayId[ObjectId]);
	glBindVertexArray(VertexArrayId[ObjectId]);

	// Create Buffer for vertex data
	glGenBuffers(1, &VertexBufferId[ObjectId]);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBufferId[ObjectId]);
	glBufferData(GL_ARRAY_BUFFER, VertexBufferSize[ObjectId], Vertices, GL_STATIC_DRAW);

	// Create Buffer for indices
	if (Indices != NULL) {
		glGenBuffers(1, &IndexBufferId[ObjectId]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId[ObjectId]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, IndexBufferSize[ObjectId], Indices, GL_STATIC_DRAW);
	}

	// Assign vertex attributes
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, VertexSize, 0);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)RgbOffset);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)Normaloffset);	// TL
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, VertexSize, (GLvoid*)UVoffset);	// TL

	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);	// normal
	glEnableVertexAttribArray(3);	// uv

	// Disable our Vertex Buffer Object 
	glBindVertexArray(0);

	ErrorCheckValue = glGetError();
	if (ErrorCheckValue != GL_NO_ERROR)
	{
		fprintf(
			stderr,
			"ERROR: Could not create a VBO: %s \n",
			gluErrorString(ErrorCheckValue)
		);
	}
}


// Ensure your .obj files are in the correct format and properly loaded by looking at the following function
void loadObject(char* file, glm::vec4 color, Vertex* &out_Vertices, GLushort* &out_Indices, int ObjectId) {
	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	bool res;
	if (texBool == false) {
		res = loadOBJ(file, vertices, normals, uvs);
	}
	else {
		res = loadOBJ_tris(file, vertices, normals, uvs);
	}

	std::vector<GLushort> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;

	indexVBO(vertices, normals, uvs, indices, indexed_vertices, indexed_normals, indexed_uvs);

	const size_t vertCount = indexed_vertices.size();
	const size_t idxCount = indices.size();

	// populate output arrays
	out_Vertices = new Vertex[vertCount];
	float coords[2] = { -1.0f, -1.0f };
	for (int i = 0; i < vertCount; i++) {
		out_Vertices[i].SetPosition(&indexed_vertices[i].x);
		out_Vertices[i].SetNormal(&indexed_normals[i].x);

		if (texBool == false) {
			out_Vertices[i].SetUV(coords);
		}
		else {
			out_Vertices[i].SetUV(&indexed_uvs[i].x);
		}
		
		out_Vertices[i].SetColor(&color[0]);
	}
	out_Indices = new GLushort[idxCount];
	for (int i = 0; i < idxCount; i++) {
		out_Indices[i] = indices[i];
	}

	// set global variables!!
	NumIdcs[ObjectId] = idxCount;
	VertexBufferSize[ObjectId] = sizeof(out_Vertices[0]) * vertCount;
	IndexBufferSize[ObjectId] = sizeof(GLushort) * idxCount;
}

void createObjects(void) {
	//-- COORDINATE AXES --//
	CoordVerts[0] = { { 0.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[1] = { { 5.0, 0.0, 0.0, 1.0 }, { 1.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[2] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[3] = { { 0.0, 5.0, 0.0, 1.0 }, { 0.0, 1.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[4] = { { 0.0, 0.0, 0.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	CoordVerts[5] = { { 0.0, 0.0, 5.0, 1.0 }, { 0.0, 0.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	
	//-- GRID --//
	
	for (int i = 0; i < 11; i++)
	{
		float xcoord = -5.0 + i;
		GridVerts[2 * i] = { { xcoord, 0.0, -5.0, 1.0 }, { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
		GridVerts[2 * i + 1] = { { xcoord, 0.0, 5.0, 1.0 }, { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	}
	for (float i = 11.0f; i < 22; i++)
	{
		float zcoord = -5.0f + (i - 11.0f);
		GridVerts[2 * (int)i] = { { -5.0, 0.0, zcoord, 1.0 }, { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
		GridVerts[2 * (int)i + 1] = { { 5.0, 0.0, zcoord, 1.0 }, { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 1.0 } };
	}
	
	//-- .OBJs --//

	// ATTN: Load your models here through .obj files -- example of how to do so is as shown
	
	
	Vertex* Verts;
	GLushort* Idcs;

	/*
	//truncated tetrahedron base, red
	if(selection == 'B') loadObject("base.obj", glm::vec4(1.0, 0.0, 0.0, 1.0), Verts, Idcs, 2);
	else loadObject("base.obj", glm::vec4(0.8, 0.0, 0.0, 1.0), Verts, Idcs, 2);
	createVAOs(Verts, Idcs, 2);
	*/

	//wireframe mesh
	loadObject("head.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 2);
	createVAOs(Verts, Idcs, 2);

	//GLuint Texture = loadBMP_custom("test.bmp");
	texBool = true;
	loadObject("headtris.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 3);
	createVAOs(Verts, Idcs, 3);

	loadObject("head2.obj", glm::vec4(1.0, 1.0, 1.0, 1.0), Verts, Idcs, 4);
	createVAOs(Verts, Idcs, 4);

	texBool = false;
	

}

void pickObject(void) {
	// Clear the screen in white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(pickingProgramID);
	{
		glm::mat4 ModelMatrix = glm::mat4(1.0); // TranslationMatrix * RotationMatrix;
		glm::mat4 MVP = gProjectionMatrix * gViewMatrix * ModelMatrix;

		// Send our transformation to the currently bound shader, in the "MVP" uniform
		glUniformMatrix4fv(PickingMatrixID, 1, GL_FALSE, &MVP[0][0]);

		// ATTN: DRAW YOUR PICKING SCENE HERE. REMEMBER TO SEND IN A DIFFERENT PICKING COLOR FOR EACH OBJECT BEFOREHAND
		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Wait until all the pending drawing commands are really done.
	// Ultra-mega-over slow ! 
	// There are usually a long time between glDrawElements() and
	// all the fragments completely rasterized.
	glFlush();
	glFinish();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Read the pixel at the center of the screen.
	// You can also use glfwGetMousePos().
	// Ultra-mega-over slow too, even for 1 pixel, 
	// because the framebuffer is on the GPU.
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	unsigned char data[4];
	glReadPixels(xpos, window_height - ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data); // OpenGL renders with (0,0) on bottom, mouse reports with (0,0) on top

	// Convert the color back to an integer ID
	gPickedIndex = int(data[0]);

	if (gPickedIndex == 255) { // Full white, must be the background !
		gMessage = "background";
	}
	else {
		std::ostringstream oss;
		oss << "point " << gPickedIndex;
		gMessage = oss.str();
	}

	// Uncomment these lines to see the picking shader in effect
	//glfwSwapBuffers(window);
	//continue; // skips the normal rendering
}

void renderScene(void) {
	//ATTN: DRAW YOUR SCENE HERE. MODIFY/ADAPT WHERE NECESSARY!

	// For dark blue background change blue to 0.2f
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Re-clear the screen for real rendering
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);
	{
		/* Camera rotations
		x = r * cos(latitudeAngle) * sin(longitudeAngle)
		y = r * cos(longitudeAngle
		z = r * sin(latitudeAngle) * sin(longitudeAngle)
		*/
		float x_cam, y_cam, z_cam, radius;
		radius = sqrt(300);
		x_cam = radius * cos(rot_camera_side) * sin(rot_camera_up);
		y_cam = radius * cos(rot_camera_up);
		z_cam = radius * sin(rot_camera_side) * sin(rot_camera_up);
		gViewMatrix = glm::lookAt(glm::vec3(x_cam, y_cam, z_cam),	// eye
			glm::vec3(0.0, 0.0, 0.0),	// center
			glm::vec3(0.0, 1.0, 0.0));	// up

		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glm::mat4x4 ModelMatrix = glm::mat4(1.0);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		if (g_grid == true) {
			glBindVertexArray(VertexArrayId[0]);	// Draw CoordAxes
			glDrawArrays(GL_LINES, 0, NumVerts[0]);

			glBindVertexArray(VertexArrayId[1]); //Draw Grid
			glDrawArrays(GL_LINES, 0, NumVerts[1]);
		}

		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, adjustment, 0.0f));
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
		glUniformMatrix4fv(ProjMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		if (u_texture == true) {
			texture = loadDDS("face.dds");
		}
		else {
			texture = loadDDS("white.dds");
		}
		
		glBindTexture(GL_TEXTURE_2D, texture);

		if (f_wireframe == true) {
			glBindVertexArray(VertexArrayId[2]);
			for (int i = 0; i <= 804; i += 4) {
				glDrawArrays(GL_LINE_STRIP, i, 4);
			}
		}
		else if (f_facets == true) {
			glBindVertexArray(VertexArrayId[3]);
			glDrawElements(GL_TRIANGLES, NumIdcs[3], GL_UNSIGNED_SHORT, (void*)0);
		}
		else if (p_smooth == true) {
			glBindVertexArray(VertexArrayId[4]);
			glDrawElements(GL_TRIANGLES, NumIdcs[4], GL_UNSIGNED_SHORT, (void*)0);
		}
		



		glBindVertexArray(0);
	}
	glUseProgram(0);
	// Draw GUI
	//TwDraw();

	// Swap buffers
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void cleanup(void) {
	// Cleanup VBO and shader
	for (int i = 0; i < NumObjects; i++) {
		glDeleteBuffers(1, &VertexBufferId[i]);
		glDeleteBuffers(1, &IndexBufferId[i]);
		glDeleteVertexArrays(1, &VertexArrayId[i]);
	}
	glDeleteProgram(programID);
	glDeleteProgram(pickingProgramID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();
}

// Alternative way of triggering functions on keyboard events
static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	// ATTN: MODIFY AS APPROPRIATE
	if (action == GLFW_PRESS) {
		switch (key)
		{
		case GLFW_KEY_R:
			rot_camera_side = PI / 4;
			rot_camera_up = PI / 3;
			adjustment = 0.0f;
			g_grid = true;
			f_wireframe = true;
			f_facets = false;
			p_smooth = false;
			break;
		case GLFW_KEY_F:
			if (shift_press == true) {
				if (f_facets == false) {
					f_wireframe = false;
					f_facets = true;
					p_smooth = false;
				}
				else {
					f_facets = false;
				}
			}
			else {
				if (f_wireframe == false) {
					f_wireframe = true;
					f_facets = false;
					p_smooth = false;
				}
				else {
					f_wireframe = false;
				}
			}
			break;
		case GLFW_KEY_P:
			if (p_smooth == false) {
				f_wireframe = false;
				f_facets = false;
				p_smooth = true;
			}
			else {
				p_smooth = false;
			}
			break;
		case GLFW_KEY_G:
			if (g_grid == false) {
				g_grid = true;
			}
			else {
				g_grid = false;
			}
			break;

		case GLFW_KEY_U:
			if (u_texture == true) {
				u_texture = false;
			}
			else {
				u_texture = true;
			}
			break;
		case GLFW_KEY_RIGHT_SHIFT:
			shift_press = true;
			break;
		case GLFW_KEY_LEFT_SHIFT:
			shift_press = true;
			break;

			//arrow keys
		case GLFW_KEY_UP:
			if (shift_press == true) {
				adjustment += 0.25f;
			}
			else {
				key_up();
			}
			break;
		case GLFW_KEY_DOWN:
			if (shift_press == true) {
				adjustment -= 0.25f;
			}
			else {
				key_down();
			}
			break;
		case GLFW_KEY_LEFT:
			key_left();
			break;
		case GLFW_KEY_RIGHT:
			key_right();
			break;
		}
		renderScene();
	}
	if (action == GLFW_RELEASE) {
		switch (key)
		{
		case GLFW_KEY_LEFT_SHIFT:
			shift_press = false;
			break;
		case GLFW_KEY_RIGHT_SHIFT:
			shift_press = false;
			break;
		default:
			break;
		}
	}
}


void key_up() {
	switch (selection)
	{
	case 'C':
		if (rot_camera_up > PI / 10) {
			rot_camera_up -= PI / 10;
		}
		break;
	default:
		break;
	}
}

void key_down() {
	switch (selection)
	{
	case 'C':
		if (rot_camera_up < 9 * PI / 10) {
			rot_camera_up += PI / 10;
		}
		break;
	default:
		break;
	}
}

void key_left() {
	switch (selection)
	{
	case 'C':
		rot_camera_side -= PI/10;
		break;
	default:
		break;
	}
}

void key_right() {
	switch (selection)
	{
	case 'C':
		rot_camera_side += PI/10;
		break;
	default:
		break;
	}
}

// Alternative way of triggering functions on mouse click events
static void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		pickObject();
	}
}

int main(void) {
	// TL
	// ATTN: Refer to https://learnopengl.com/Getting-started/Transformations, https://learnopengl.com/Getting-started/Coordinate-Systems,
	// and https://learnopengl.com/Getting-started/Camera to familiarize yourself with implementing the camera movement

	// ATTN (Project 3 only): Refer to https://learnopengl.com/Getting-started/Textures to familiarize yourself with mapping a texture
	// to a given mesh

	// Initialize window
	int errorCode = initWindow();
	if (errorCode != 0)
		return errorCode;

	// Initialize OpenGL pipeline
	initOpenGL();

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;
	do {
		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0){ // If last prinf() was more than 1sec ago
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// DRAWING POINTS
		renderScene();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
	glfwWindowShouldClose(window) == 0);

	cleanup();

	return 0;
}