#include <string>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include <glload/gl_3_3.h>
#include <glload/gll.hpp>
#include <GL/glfw.h>

#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
GLuint positionBufferObject;
GLuint positionBufferObject2;
GLuint positionBufferObject3;
GLuint program;
GLuint vao;

GLint myColorLoc;


GLint offsetLoc;

GLfloat anglex;
GLfloat angley;


//Perspective Matrix - Camera to Clip
GLuint pMatrixUni;

vrpn_Analog_Remote* vrpnAnalog;
vrpn_Button_Remote* vrpnButton;
vrpn_Tracker_Remote* vrpnTracker;

//Initial offset - camera starting position
float x = 0.0f;
float y = -0.2f;
float z = 0.0f;

//Angle for rotation
float rotatex = 0.0f;
float rotatey = 0.0f;

//Calculate FOV
float CalcFrustumScale(float fFovDeg)
{
	const float degToRad = 3.14159f * 2.0f / 360.0f;
	float fFovRad = fFovDeg * degToRad;
	return 1.0f / tan(fFovRad / 2.0f);
}


//Frustrum Scale - 45 degrees FOV
float fFrustrumScale = CalcFrustumScale(45);

std::vector< glm::vec3 > vertices;
std::vector< glm::vec2 > texture;
std::vector< glm::vec3 > normals;
GLuint vertexbuffer;



GLuint BuildShader(GLenum eShaderType, const std::string &shaderText)
{
	GLuint shader = glCreateShader(eShaderType);
	const char *strFileData = shaderText.c_str();
	glShaderSource(shader, 1, &strFileData, NULL);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		//With ARB_debug_output, we already get the info log on compile failure.
		if(!glext_ARB_debug_output)
		{
			GLint infoLogLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

			GLchar *strInfoLog = new GLchar[infoLogLength + 1];
			glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);

			const char *strShaderType = NULL;
			switch(eShaderType)
			{
			case GL_VERTEX_SHADER: strShaderType = "vertex"; break;
			case GL_GEOMETRY_SHADER: strShaderType = "geometry"; break;
			case GL_FRAGMENT_SHADER: strShaderType = "fragment"; break;
			}

			fprintf(stderr, "Compile failure in %s shader:\n%s\n", strShaderType, strInfoLog);
			delete[] strInfoLog;
		}

		throw std::runtime_error("Compile failure in shader.");
	}

	return shader;
}

void VRPN_CALLBACK handle_analog( void* userData, const vrpn_ANALOGCB a )
{
	int nbChannels = a.num_channel;

	std::cout << "Analog : ";

	for( int i=0; i < a.num_channel; i++ )
	{
		std::cout << a.channel[i] << " ";
	}

	std::cout << std::endl;
}

void VRPN_CALLBACK handle_button( void* userData, const vrpn_BUTTONCB b )
{
	std::cout << "Button '" << b.button << "': " << b.state << std::endl;
}


void VRPN_CALLBACK handle_tracker(void* userData, const vrpn_TRACKERCB t )
{
	//Angle in quaternians
	std::cout << "Tracker '" << t.sensor << "' : " << t.pos[0] << "," <<  t.pos[1] << "," << t.pos[2] << std::endl;//<< "Angle : " << t.quat[0] << "," <<  t.quat[1] << "," << t.quat[2] << "," << t.quat[3] << std::endl;
	x=t.pos[0];
	y=t.pos[1];
	z=t.pos[2];
}

void checkVRPN()
{
	vrpnAnalog->mainloop();
	vrpnButton->mainloop();
	vrpnTracker->mainloop();
}
void loadObj(const char* path, std::vector < glm::vec3 > & out_vertices,
    std::vector < glm::vec2 > & out_texture,
    std::vector < glm::vec3 > & out_normals){
        std::vector< unsigned int > vertexIndices, textureIndices, normalIndices;
        std::vector< glm::vec3 > temp_vertices;
        std::vector< glm::vec2 > temp_texture;
        std::vector< glm::vec3 > temp_normals;
        FILE * file = fopen(path, "r");
        if (file == NULL){
                printf("Cannot open the file !\n");
        }
        char lineHeader[128];
        int res = fscanf(file, "%s", lineHeader);
        while(res != EOF){
                char lineHeader[128];
                int res = fscanf(file, "%s", lineHeader);
                if (res == EOF) break;
               
                if ( strcmp( lineHeader, "v" ) == 0 ){
                        glm::vec3 vertex;
                        fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
                        //vertex.x = vertex.x+50;
                        //vertex.y = vertex.y-1;
                        //vertex.z = vertex.z+50;
                        temp_vertices.push_back(vertex);
                }else if ( strcmp( lineHeader, "vt" ) == 0 ){
                        glm::vec2 texture;
                        fscanf(file, "%f %f\n", &texture.x, &texture.y );
                        temp_texture.push_back(texture);
                }else if ( strcmp( lineHeader, "vn" ) == 0 ){
                        glm::vec3 normal;
                        fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
                        temp_normals.push_back(normal);
                }else if ( strcmp( lineHeader, "f" ) == 0 ){
                        std::string vertex1, vertex2, vertex3;
                        unsigned int vertexIndex[3], textureIndex[3], normalIndex[3];
                        int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &textureIndex[0], &normalIndex[0], &vertexIndex[1], &textureIndex[1], &normalIndex[1], &vertexIndex[2], &textureIndex[2], &normalIndex[2] );
                        vertexIndices.push_back(vertexIndex[0]);
                        vertexIndices.push_back(vertexIndex[1]);
                        vertexIndices.push_back(vertexIndex[2]);
                        textureIndices.push_back(textureIndex[0]);
                        textureIndices.push_back(textureIndex[1]);
                        textureIndices.push_back(textureIndex[2]);
                        normalIndices.push_back(normalIndex[0]);
                        normalIndices.push_back(normalIndex[1]);
                        normalIndices.push_back(normalIndex[2]);
                }
        }
 
        for( unsigned int i=0; i<vertexIndices.size(); i++ ){
 
                // Get the indices of its attributes
                unsigned int vertexIndex = vertexIndices[i];
                unsigned int uvIndex = textureIndices[i];
                unsigned int normalIndex = normalIndices[i];
               
                // Get the attributes thanks to the index
                glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
                glm::vec2 texture = temp_texture[ uvIndex-1 ];
                glm::vec3 normal = temp_normals[ normalIndex-1 ];
               
                // Put the attributes in buffers
                out_vertices.push_back(vertex);
                out_texture.push_back(texture);
                out_normals.push_back(normal);
       
        }
}
void init(){
	glClearColor(0.0,0.0,0.0,0.0);
	loadObj("smallandroid.obj", vertices, texture, normals);
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        //glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthFunc(GL_EQUAL);
        //x = 0;y = 0;z = 0;
		      glGenBuffers(1, &vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
		
	glEnable(GL_CULL_FACE); //enable front face
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);



	//Perspective Matrix values
	
	float fzNear = 0.0f;
	float fzFar = 5.0f;

	//Room coordinates - cube
	const float vertexPositions[] = {


		//Front Wall
		
			//B Left
			-20, 0.0f, -20, 1.0f,
			//T Left
			-20, 20, -20, 1.0f,
			//B Right
			20, 0.0f, -20, 1.0f,

			//T Right
			20, 20, -20, 1.0f,
			//B Right
			20, 0.0f, -20, 1.0f,
			//T Left
			-20, 20, -20, 1.0f,


		//Left Wall


			//T Right
			-20, 20, 20, 1.0f,
			//T Left
			-20, 20, -20, 1.0f,		
			//B Left
			-20, 0.0f, -20, 1.0f,


			//B Left
			-20, 0.0f, -20, 1.0f,
			//B Right
			-20, 0.0f, 20, 1.0f,
			//T Right
			-20, 20, 20, 1.0f,

		//Back Wall
				
			//B Right
			20, 0.0f, 20, 1.0f,
			//T Left
			-20, 20, 20, 1.0f,
			//B Left
			-20, 0.0f, 20, 1.0f,

			//T Left
			-20, 20, 20, 1.0f,
			//B Right
			20, 0.0f, 20, 1.0f,
			//T Right
			20, 20, 20, 1.0f,


		//Right Wall

			//B Left
			20, 5, -20, 1.0f,
			//T Left
			20, 20, -20, 1.0f,
			//B Right
			20, 5, 20, 1.0f,

			//T Right
			20, 20, 20, 1.0f,
			//B Right
			20, 5, 20, 1.0f,
			//T Left
			20, 20, -20, 1.0f,

		//Floor


			//T Right
			20, 0.0f, 20, 1.0f,
			//T Left
			-20, 0.0f, 20, 1.0f,
			//B Left
			-20, 0.0f, -20, 1.0f,


			//B Left
			-20, 0.0f, -20, 1.0f,
			//B Right
			20, 0.0f, -20, 1.0f,
			//T Right
			20, 0.0f, 20, 1.0f,


		//Ceiling
		
			//B Left
			-20, 20, -20, 1.0f,
			//T Left
			-20, 20, 20, 1.0f,
			//B Right
			20, 20, -20, 1.0f,

			//T Right
			20, 20, 20, 1.0f,
			//B Right
			20, 20, -20, 1.0f,
			//T Left
			-20, 20, 20, 1.0f,

		


	};
	const float vertexPositions3[] = {
		//window
			20, 0.0f, -20, 1.0f,
			//T Left
			20, 5, -20, 1.0f,
			//B Right
			20, 0.0f, 20, 1.0f,

			//T Right
			20, 5, 20, 1.0f,
			//B Right
			20, 0.0f, 20, 1.0f,
			//T Left
			20, 5, -20, 1.0f,
	};

	const float vertexPositions2[] = {


		//Front Wall
		
			//B Left
			-40, 0.0f, -40, 1.0f,
			//T Left
			-40, 40, -40, 1.0f,
			//B Right
			40, 0.0f, -40, 1.0f,

			//T Right
			40, 40, -40, 1.0f,
			//B Right
			40, 0.0f, -40, 1.0f,
			//T Left
			-40, 40, -40, 1.0f,


		//Left Wall


			//T Right
			-40, 40, 40, 1.0f,
			//T Left
			-40, 40, -40, 1.0f,		
			//B Left
			-40, 0.0f, -40, 1.0f,


			//B Left
			-40, 0.0f, -40, 1.0f,
			//B Right
			-40, 0.0f, 40, 1.0f,
			//T Right
			-40, 40, 40, 1.0f,

		//Back Wall
				
			//B Right
			40, 0.0f, 40, 1.0f,
			//T Left
			-40, 40, 40, 1.0f,
			//B Left
			-40, 0.0f, 40, 1.0f,

			//T Left
			-40, 40, 40, 1.0f,
			//B Right
			40, 0.0f, 40, 1.0f,
			//T Right
			40, 40, 40, 1.0f,


		//Right Wall

			//B Left
			40, 0.0f, -40, 1.0f,
			//T Left
			40, 40, -40, 1.0f,
			//B Right
			40, 0.0f, 40, 1.0f,

			//T Right
			40, 40, 40, 1.0f,
			//B Right
			40, 0.0f, 40, 1.0f,
			//T Left
			40, 40, -40, 1.0f,

		//Floor


			//T Right
			40, 0.0f, 40, 1.0f,
			//T Left
			-40, 0.0f, 40, 1.0f,
			//B Left
			-40, 0.0f, -40, 1.0f,


			//B Left
			-40, 0.0f, -40, 1.0f,
			//B Right
			40, 0.0f, -40, 1.0f,
			//T Right
			40, 0.0f, 40, 1.0f,


		//Ceiling
		
			//B Left
			-40, 40, -40, 1.0f,
			//T Left
			-40, 40, 40, 1.0f,
			//B Right
			40, 40, -40, 1.0f,

			//T Right
			40, 40, 40, 1.0f,
			//B Right
			40, 40, -40, 1.0f,
			//T Left
			-40, 40, 40, 1.0f,



	};

	glGenBuffers(1, &positionBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &positionBufferObject2);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions2), vertexPositions2, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &positionBufferObject3);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject3);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions3), vertexPositions3, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const std::string vertexShader(
		"#version 330\n"
		"layout(location = 0) in vec4 position;\n"
		"uniform vec3 offset;\n"
		"uniform float anglex;\n"
		"uniform float angley;\n"
		"uniform mat4 pMatrix;\n"
		"mat4 rotate_y()\n"
		"{\n"
		"\n"
			"return mat4(\n"
			"	vec4(cos(angley), 0.0, sin(angley), 0.0),\n"
			"	vec4(0.0,		  1.0, 0.0,			0.0),\n"
			"	vec4(-sin(angley),0.0, cos(angley), 0.0),\n"
			"	vec4(0.0,		  0.0, 0.0,	        1.0)\n"
			");\n"
		"}\n"
			"\n"
			"\n"
		"mat4 rotate_x()\n"
		"{\n"
			"return mat4(\n"
			"	vec4(1.0, 0.0,			0.0,			0.0),\n"
			"	vec4(0.0, cos(anglex), -sin(anglex),	0.0),\n"
			"	vec4(0.0, sin(anglex), cos(anglex),	    0.0),\n"
			"	vec4(0.0,  0.0,			0.0,	        1.0)\n"
			");\n"
		"}\n"
		
		"void main()\n"
		"{\n"
		"   vec4 cameraposition = (position + vec4(offset, 0.0)) * (rotate_x()) * (rotate_y());\n"
		"	gl_Position = pMatrix * cameraposition;\n"
		"}\n"
		);

	const std::string fragmentShader(
		"#version 330\n"
		"uniform vec4 myColor;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = myColor;\n"
	    //"	outputColor.r = 1;\n"
		"}\n"
		);

	GLuint vertShader = BuildShader(GL_VERTEX_SHADER, vertexShader);
	GLuint fragShader = BuildShader(GL_FRAGMENT_SHADER, fragmentShader);

	program = glCreateProgram();
	glAttachShader(program, vertShader);
	glAttachShader(program, fragShader);	
	glLinkProgram(program);

	GLint status;
	glGetProgramiv (program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		if(!glext_ARB_debug_output)
		{
			GLint infoLogLength;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

			GLchar *strInfoLog = new GLchar[infoLogLength + 1];
			glGetProgramInfoLog(program, infoLogLength, NULL, strInfoLog);
			fprintf(stderr, "Linker failure: %s\n", strInfoLog);
			delete[] strInfoLog;
		}

		throw std::runtime_error("Shader could not be linked.");
	}


	

	//Setup Perspective Matrix
	pMatrixUni = glGetUniformLocation(program, "pMatrix");

	float matrix[16];
	memset(matrix, 0, sizeof(float) * 16);

	matrix[0] = fFrustrumScale;
	matrix[5] = fFrustrumScale;
	matrix[10] = (fzFar + fzNear) / (fzNear - fzFar);
	matrix[14] = (2 * fzFar * fzNear) / (fzNear - fzFar);
	matrix[11] = -1.0f;

	glUseProgram(program);
	glUniformMatrix4fv(pMatrixUni, 1, GL_FALSE, matrix);
	glUseProgram(0);




	//get uniform myColor
	myColorLoc = glGetUniformLocation(program, "myColor");
	offsetLoc = glGetUniformLocation(program, "offset");
	anglex = glGetUniformLocation(program, "anglex");
	angley = glGetUniformLocation(program, "angley");
	
	//setup VRPN
	vrpnAnalog = new vrpn_Analog_Remote("DTrack0@localhost");
	//vrpnAnalog = new vrpn_Analog_Remote("Mouse0@localhost");

	vrpnButton = new vrpn_Button_Remote("DTrack0@localhost");
	vrpnTracker = new vrpn_Tracker_Remote( "DTrack@localhost");

	vrpnAnalog->register_change_handler( 0, handle_analog );
	vrpnButton->register_change_handler( 0, handle_button );
	vrpnTracker->register_change_handler( 0, handle_tracker );
}







//Called to update the display.
//You should call glutSwapBuffers after all of your rendering to display what you rendered.
//If you need continuous updates of the screen, call glutPostRedisplay() at the end of the function.
void display()
{
	glDrawBuffer(GL_BACK);                                   //draw into both back buffers
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);      //clear color and depth buffers

	
	//LEFT
	glDrawBuffer(GL_BACK_LEFT);                              //draw into back left buffer
	
	glUseProgram(program);

	//set uniform myColor
	glUniform3f(myColorLoc, 1.0f, 0.0f, 0.0f);
	glUniform3f(offsetLoc, x, y, z);
	glUniform1f(angley, rotatey);
	glUniform1f(anglex, rotatex);

				glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject2);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);


	//Draw each outside wall with a different colour
	glUniform4f(myColorLoc, 1.0f, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUniform4f(myColorLoc, 0.9f, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 6, 12);
	glUniform4f(myColorLoc, 0.8f, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 12, 18);
	glUniform4f(myColorLoc, 0.7f, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 18, 24);
	glUniform4f(myColorLoc, 0.6f, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 24, 30);
	glUniform4f(myColorLoc, 0.5f, 0.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 30, 36);

	glDisableVertexAttribArray(0);
		glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);


	//Draw each wall with a different colour
	glUniform4f(myColorLoc, 0.0f, 0.4f, 0.4f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glUniform4f(myColorLoc, 0.0f, 0.0f, 1.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 6, 12);
	glUniform4f(myColorLoc, 0.0f, 1.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 12, 18);
	glUniform4f(myColorLoc, 1.0f, 0.0f, 1.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 18, 24);
	glUniform4f(myColorLoc, 1.0f, 1.0f, 0.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 24, 30);
	glUniform4f(myColorLoc, 0.0f, 1.0f, 1.0f, 1.0f);
	glDrawArrays(GL_TRIANGLES, 30, 36);
	glDisableVertexAttribArray(0);

	//draw the windowowowowowowoww
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject3);

	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glUniform4f(myColorLoc, 0.0f, 1.0f, 0.0f, 0.1f);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glDisableVertexAttribArray(0);



	 glEnableVertexAttribArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
                glVertexAttribPointer(
                        0,                  // attribute
                        3,                  // size
                        GL_FLOAT,           // type
                        GL_FALSE,           // normalized?
                        0,                  // stride
                        (void*)0            // array buffer offset
                );
 
                //glDepthMask(0);
                                glUniform4f(myColorLoc, 0.0f, 1.0f, 0.0f,0.6f);
                // Draw the triangle !
                glDrawArrays(GL_TRIANGLES, 0, vertices.size() );
 
                glDisableVertexAttribArray(0);
	glUseProgram(0);

	//RIGHT
	glDrawBuffer(GL_BACK_RIGHT);                              //draw into back left buffer
	
	glUseProgram(program);

	//set uniform myColor
	glUniform3f(myColorLoc, 0.0f, 1.0f, 0.0f);

	//Pass offset
	glUniform3f(offsetLoc, x+0.5f,y,z);
	
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	//glDrawArrays(GL_TRIANGLES, 0, 36);

	glDisableVertexAttribArray(0);
	glUseProgram(0);


	glfwSwapBuffers();
}

//Called whenever the window is resized. The new window size is given, in pixels.
//This is an opportunity to call glViewport or glScissor to keep up with the change in size.
void reshape (int w, int h)
{
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}

void APIENTRY DebugFunc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
						const GLchar* message, GLvoid* userParam)
{
	std::string srcName;
	switch(source)
	{
	case GL_DEBUG_SOURCE_API_ARB: srcName = "API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB: srcName = "Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB: srcName = "Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB: srcName = "Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB: srcName = "Application"; break;
	case GL_DEBUG_SOURCE_OTHER_ARB: srcName = "Other"; break;
	}

	std::string errorType;
	switch(type)
	{
	case GL_DEBUG_TYPE_ERROR_ARB: errorType = "Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB: errorType = "Deprecated Functionality"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB: errorType = "Undefined Behavior"; break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB: errorType = "Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB: errorType = "Performance"; break;
	case GL_DEBUG_TYPE_OTHER_ARB: errorType = "Other"; break;
	}

	std::string typeSeverity;
	switch(severity)
	{
	case GL_DEBUG_SEVERITY_HIGH_ARB: typeSeverity = "High"; break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB: typeSeverity = "Medium"; break;
	case GL_DEBUG_SEVERITY_LOW_ARB: typeSeverity = "Low"; break;
	}

	printf("%s from %s,\t%s priority\nMessage: %s\n",
		errorType.c_str(), srcName.c_str(), typeSeverity.c_str(), message);
}

int main(int argc, char** argv)
{
	if(!glfwInit())
		return -1;

	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef DEBUG
	glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	//Stereo setup
	//glfwOpenWindowHint( GLFW_STEREO, GL_TRUE );

	

	if(!glfwOpenWindow(500, 500, 8, 8, 8, 8, 24, 8, GLFW_WINDOW))
	{
		glfwTerminate();
		return -1;
	}

	if(glload::LoadFunctions() == glload::LS_LOAD_FAILED)
	{
		glfwTerminate();
		return -1;
	}

	glfwSetWindowTitle("GLFW Demo");

	if(glext_ARB_debug_output)
	{
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		glDebugMessageCallbackARB(DebugFunc, (void*)15);
	}

	init();

	glfwSetWindowSizeCallback(reshape);

	//Main loop
	while(true)
	{
		checkVRPN();
		display();

		if(glfwGetKey(GLFW_KEY_ESC) || !glfwGetWindowParam(GLFW_OPENED))
			break;


		                if (glfwGetKey('A') == GLFW_PRESS){ x+=0.1f;printf("A is pressed");}
                if (glfwGetKey('D') == GLFW_PRESS){ x-=0.1f;printf("D is pressed");}
                if (glfwGetKey('Q') == GLFW_PRESS){ z+=0.1f;printf("Q is pressed");}
                if (glfwGetKey('E') == GLFW_PRESS){ z-=0.1f;printf("E is pressed");}
                if (glfwGetKey('W') == GLFW_PRESS){ y+=0.1f;printf("W is pressed");}
                if (glfwGetKey('S') == GLFW_PRESS){ y-=0.1f;printf("S is pressed");}
                if (glfwGetKey('T') == GLFW_PRESS){ rotatey+=0.05;}
        if (glfwGetKey('R') == GLFW_PRESS){ rotatey-=0.05;}
                if (glfwGetKey('F') == GLFW_PRESS){ rotatex+=0.01;}
                if (glfwGetKey('G') == GLFW_PRESS){ rotatex-=0.01;}




	}

	glfwTerminate();
	return 0;


}