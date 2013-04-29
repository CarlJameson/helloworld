#include <string>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

#include <glload/gl_3_3.h>
#include <glload/gll.hpp>
#include <GL/glfw.h>

#include "vrpn_Tracker.h"
#include "vrpn_Button.h"
#include "vrpn_Analog.h"

#include <iostream>
GLuint positionBufferObject;
GLuint program;
GLuint vao;

GLint myColorLoc;
GLint offsetLoc;

vrpn_Analog_Remote* vrpnAnalog;
vrpn_Button_Remote* vrpnButton;
vrpn_Tracker_Remote* vrpnTracker;

float x, y, z;

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
	x = (float) t.pos[0];
	y = (float) t.pos[1];
	z = (float) t.pos[2];
}

void checkVRPN()
{
	vrpnAnalog->mainloop();
	vrpnButton->mainloop();
	vrpnTracker->mainloop();
}

void init()
{
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	const float vertexPositions[] = {
		0.75f, 0.75f, 0.0f, 1.0f,
		0.75f, -0.75f, 0.0f, 1.0f,
		-0.75f, -0.75f, 0.0f, 1.0f,
	};

	glGenBuffers(1, &positionBufferObject);
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPositions), vertexPositions, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const std::string vertexShader(
		"#version 330\n"
		"layout(location = 0) in vec4 position;\n"
		"uniform vec3 offset;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = position+vec4(offset, 1.0);\n"
		"}\n"
		);

	const std::string fragmentShader(
		"#version 330\n"
		"uniform vec3 myColor;\n"
		"out vec4 outputColor;\n"
		"void main()\n"
		"{\n"
		"   outputColor = vec4(myColor, 1.0f);\n"
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

	//get uniform myColor
	myColorLoc = glGetUniformLocation(program, "myColor");
	offsetLoc = glGetUniformLocation(program, "offset");
	
	//setup VRPN
	vrpnAnalog = new vrpn_Analog_Remote("DTrack0@localhost");
	//vrpnAnalog = new vrpn_Analog_Remote("Mouse0@localhost");

	vrpnButton = new vrpn_Button_Remote("DTrack0@localhost");
	//vrpnButton = new vrpn_Button_Remote("Mouse0@localhost");
	
	vrpnTracker = new vrpn_Tracker_Remote( "DTrack@localhost");
	//vrpnTracker = new vrpn_Tracker_Remote("Mouse0@localhost");

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
	glUniform3f(offsetLoc, x, y, 0.0f);
	
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);
	glUseProgram(0);

	//RIGHT
	glDrawBuffer(GL_BACK_RIGHT);                              //draw into back left buffer
	
	glUseProgram(program);

	//set uniform myColor
	glUniform3f(myColorLoc, 0.0f, 1.0f, 0.0f);
	glUniform3f(offsetLoc, 0.1f, 0.0f, 0.0f);

	glBindBuffer(GL_ARRAY_BUFFER, positionBufferObject);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLES, 0, 3);

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
	glfwOpenWindowHint( GLFW_STEREO, GL_TRUE );

	

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
	}

	glfwTerminate();
	return 0;
}



