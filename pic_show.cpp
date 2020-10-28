#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
//#include <sys/mman.h>
//#include <sys/stat.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <errno.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>

#include <sys/time.h>
#include <time.h>
//#include "ion.h"
//#include "drm_fourcc.h"
//#include <pthread.h>
//#include <semaphore.h>
//#include "videodev2_new.h"


#define EGL_EGLEXT_PROTOTYPES

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define glScreenW 640;
#define glScreenH 480;
unsigned char* image;

static GLuint yuvtex;
static GLint gYuvTexSamplerHandle;
struct fbdev_window native_window;

GLuint gProgram;

static const char *vertex_shader_source =
	"attribute vec4 aPosition;    \n"
	"attribute vec2 TexCoords;    \n"
	"varying vec2 yuvTexCoords;\n"
	"void main()                  \n"
	"{                            \n"
	"    yuvTexCoords = TexCoords ;\n"
	"    gl_Position = aPosition; \n"
	"}                            \n";

static const char *fragment_shader_source = 
    "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform sampler2D yuvTexSampler;\n"
    "varying vec2 yuvTexCoords;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(yuvTexSampler, yuvTexCoords);\n"
    "}\n";

static GLfloat vVertices[] = {  
			       -1.0f, -1.0f, 0.0f,
				1.0f, -1.0f, 0.0f,
-1.0f,  1.0f, 0.0f,
1.0f,  1.0f, 0.0f,
};


static GLfloat vTextcoords[] = {	
				0.0f, 1.0f,
				1.0f, 1.0f, 
				0.0f, 0.0f,
				1.0f,0.0f,
};


static EGLint const config_attribute_list[] = {
	EGL_RED_SIZE, 8,
	EGL_GREEN_SIZE, 8,
	EGL_BLUE_SIZE, 8,
	EGL_ALPHA_SIZE, 8,
	EGL_BUFFER_SIZE, 32,

	EGL_STENCIL_SIZE, 0,
	EGL_DEPTH_SIZE, 0,
	EGL_SAMPLE_BUFFERS, 1 ,
	EGL_SAMPLES, 4,

	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,

	EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PIXMAP_BIT,

	EGL_NONE
};

static EGLint window_attribute_list[] = {
	EGL_NONE
};

static const EGLint context_attribute_list[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

EGLDisplay egl_display;
EGLSurface egl_surface;
int main()
{
	EGLint egl_major, egl_minor;
	EGLConfig config;
	EGLint num_config;
	EGLContext context;
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;
	GLint ret;
	GLint width, height;
    GLenum erro_status;
    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	if (egl_display == EGL_NO_DISPLAY) {
		fprintf(stderr, "Error: No display found!\n");
		return -1;
	}

	if (!eglInitialize(egl_display, &egl_major, &egl_minor)) {
		fprintf(stderr, "Error: eglInitialise failed!\n");
		return -1;
	}


	eglChooseConfig(egl_display, config_attribute_list, &config, 1,
			&num_config);

	context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT,
				   context_attribute_list);
	if (context == EGL_NO_CONTEXT) {
		fprintf(stderr, "Error: eglCreateContext failed: 0x%08X\n",
			eglGetError());
		return -1;
	}
	native_window.width = glScreenW;
	native_window.height = glScreenH;

	egl_surface = eglCreateWindowSurface(egl_display, config,
					     &native_window,
					     window_attribute_list);
	if (egl_surface == EGL_NO_SURFACE) {
		fprintf(stderr, "Error: eglCreateWindowSurface failed: "
			"0x%08X\n", eglGetError());
		return -1;
	}

	if (!eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &width) ||
	    !eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &height)) {
		fprintf(stderr, "Error: eglQuerySurface failed: 0x%08X\n",
			eglGetError());
		return -1;
	}

	if (!eglMakeCurrent(egl_display, egl_surface, egl_surface, context)) {
		fprintf(stderr, "Error: eglMakeCurrent() failed: 0x%08X\n",
			eglGetError());
		return -1;
	}

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	if (!vertex_shader) {
		fprintf(stderr, "Error: glCreateShader(GL_VERTEX_SHADER) "
			"failed: 0x%08X\n", glGetError());
		return -1;
	}

	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		fprintf(stderr, "Error: vertex shader compilation failed!\n");
		glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = (char *)malloc(ret);
			glGetShaderInfoLog(vertex_shader, ret, NULL, log);
			fprintf(stderr, "%s", log);
			free ( log );  
		}
		glDeleteShader ( vertex_shader );
		return -1;
	}

	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	if (!fragment_shader) {
		fprintf(stderr, "Error: glCreateShader(GL_FRAGMENT_SHADER) "
			"failed: 0x%08X\n", glGetError());
		return -1;
	}

	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);

	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
	if (!ret) {
		char *log;

		fprintf(stderr, "Error: fragment shader compilation failed!\n");
		glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = (char *)malloc(ret);
			glGetShaderInfoLog(fragment_shader, ret, NULL, log);
			fprintf(stderr, "%s", log);
			free ( log ); 
		}
		glDeleteShader ( fragment_shader );
		return -1;
	}

	program = glCreateProgram();
	if (!program) {
		fprintf(stderr, "Error: failed to create program!\n");
		return -1;
	}

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	glBindAttribLocation(program, 0, "aPosition");
	glBindAttribLocation(program, 1, "TexCoords");

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &ret);
	if (!ret) {
		char *log;

		fprintf(stderr, "Error: program linking failed!\n");
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &ret);

		if (ret > 1) {
			log = (char *)malloc(ret);
			glGetProgramInfoLog(program, ret, NULL, log);
			fprintf(stderr, "%s", log);
			free ( log ); 
		}
		glDeleteShader ( program );
		return -1;
	}
	
    glClearColor(0.5, 0.2, 0.2, 1.0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vTextcoords);
	glEnableVertexAttribArray(1);
    if((erro_status = glGetError()))
       printf("Error:77 failed: 0x%08X\n",erro_status); 

   	glGenTextures(1,&yuvtex);  //创建贴图对象
	glBindTexture(GL_TEXTURE_2D,yuvtex);   //绑定贴图对象
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);

	int channel;
	image = stbi_load("1.jpg",&width,&height,&channel,3);
	if(image)
	{
		glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,image);		//装载纹理贴图数据
		printf("channel is %d \n",channel);
	}
	else
	{
		printf("faile to load texture\n");
	}
	stbi_image_free(image);

	glUseProgram(program);
    
    if((erro_status = glGetError()))
       printf("Error: 6666 failed: 0x%08X\n",erro_status); 
   glActiveTexture(GL_TEXTURE0);

    gYuvTexSamplerHandle = glGetUniformLocation(program, "yuvTexSampler");
    glUniform1i(gYuvTexSamplerHandle, 0);
    if((erro_status = glGetError()))
       printf("Error: 88 failed: 0x%08X\n",erro_status); 


   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glFlush();	
	eglSwapBuffers(egl_display, egl_surface);
	sleep(100);


}