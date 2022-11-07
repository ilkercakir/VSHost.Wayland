/*
 * YUV420RGBgl.c
 * 
 * Copyright 2017  <pi@raspberrypi>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#include "YUV420RGBgl.h"

// Idle Queue

void init_oglidle(oglidle *i)
{
	i->stoprequested = 0;
	i->commandbusy = 1;

	i->A = malloc(sizeof(pthread_mutex_t));
	i->ready = malloc(sizeof(pthread_cond_t));
	i->done = malloc(sizeof(pthread_cond_t));
	i->busy = malloc(sizeof(pthread_cond_t));

	i->data.pixbufmutex = malloc(sizeof(pthread_mutex_t));

	int ret;
	if((ret=pthread_mutex_init(i->A, NULL))!=0 )
		printf("A init failed, %d\n", ret);
		
	if((ret=pthread_cond_init(i->ready, NULL))!=0 )
		printf("ready init failed, %d\n", ret);

	if((ret=pthread_cond_init(i->done, NULL))!=0 )
		printf("done init failed, %d\n", ret);

	if((ret=pthread_cond_init(i->busy, NULL))!=0 )
		printf("busy init failed, %d\n", ret);

	if((ret=pthread_mutex_init(i->data.pixbufmutex, NULL))!=0 )
		printf("pixbufmutex init failed, %d\n", ret);
}

void close_oglidle(oglidle *i)
{
	pthread_mutex_destroy(i->A);
	free(i->A);
	pthread_cond_destroy(i->ready);
	free(i->ready);
	pthread_cond_destroy(i->done);
	free(i->done);
	pthread_cond_destroy(i->busy);
	free(i->busy);

	pthread_mutex_destroy(i->data.pixbufmutex);
	free(i->data.pixbufmutex);
}

void* oglidle_thread(void *args)
{
	int ctype = PTHREAD_CANCEL_ASYNCHRONOUS;
	int ctype_old;
	pthread_setcanceltype(ctype, &ctype_old);

	oglidle *i = (oglidle *)args;

	pthread_mutex_lock(i->A);
	i->commandready = 0;
	i->commandbusy = 0;
	pthread_cond_signal(i->busy);

	while (!i->stoprequested)
	{
		while (!(i->commandready))
		{
			pthread_cond_wait(i->ready, i->A);
		}
		if (i->stoprequested) break;

		(i->func)(&(i->data));
		i->commanddone = 1;
		pthread_cond_signal(i->done); // Should wake up *one* thread
		i->commandready = 0;
	}
	pthread_mutex_unlock(i->A);

//printf("exiting idle thread\n");
	i->retval = 0;
	pthread_exit(&(i->retval));
}

void oglidle_add(oglidle *i, void (*f)(oglparameters *data), oglparameters *data)
{
	pthread_mutex_lock(i->A);
	while (i->commandbusy)
	{
		pthread_cond_wait(i->busy, i->A);
	}
	i->func = f;
	i->data = *data;

	i->commandbusy = 1;
	i->commanddone = 0;

	i->commandready = 1;
	pthread_cond_signal(i->ready);
	
	while (!i->commanddone)
	{
		pthread_cond_wait(i->done, i->A);
	}
	i->commandbusy = 0;
	pthread_cond_signal(i->busy);
	pthread_mutex_unlock(i->A);
}

void oglidle_start(oglidle *i)
{
	init_oglidle(i);

	int err = pthread_create(&(i->tid), NULL, &oglidle_thread, (void *)i);
	if (err)
	{}
}

void oglidle_stop(oglidle *i)
{
	int ret;

	pthread_mutex_lock(i->A);
	i->stoprequested = 1;
	i->commandready = 1;
	pthread_cond_signal(i->ready);
	pthread_mutex_unlock(i->A);

	if ((ret=pthread_join(i->tid, NULL)))
		printf("pthread_join error, i->tid, %d\n", ret);

	close_oglidle(i);
}

// EGL

void init_vertices_indices(oglstate *ogl)
{
	int i;

	GLfloat xvVertices[8] = { -1.0f,  1.0f,  // Position 0
							  -1.0f, -1.0f,  // Position 1
							   1.0f, -1.0f,  // Position 2
							   1.0f,  1.0f,  // Position 3
							};

	GLfloat xtVertices[8] = { 0.0f,  1.0f,   // TexCoord 0 
							  0.0f,  0.0f,   // TexCoord 1
							  1.0f,  0.0f,   // TexCoord 2
							  1.0f,  1.0f    // TexCoord 3
							};

	GLushort xIndices[6] = { 0, 1, 2, 0, 2, 3 };

	for(i=0;i<sizeof(xvVertices)/sizeof(GLfloat);i++)
	{
		ogl->vVertices[i] = xvVertices[i];
	}

	for(i=0;i<sizeof(xtVertices)/sizeof(GLfloat);i++)
	{
		ogl->tVertices[i] = xtVertices[i];
	}

// rescale width
	if (ogl->linewidth>ogl->width)
	{
		ogl->tVertices[6] = ogl->tVertices[4] = (float)ogl->width / (float)ogl->linewidth;
		//printf("rescale width %2.5f\n", (float)ogl->width / (float)ogl->linewidth);
	}
//rescale height
	if (ogl->height>ogl->codecheight)
	{
		ogl->tVertices[7] = ogl->tVertices[1] = (float)ogl->codecheight / (float)ogl->height;
		//printf("rescale height %2.5f\n", (float)ogl->codecheight / (float)ogl->height);
	}

	for(i=0;i<sizeof(xIndices)/sizeof(GLushort);i++)
	{
		ogl->indices[i] = xIndices[i];
	}
}

// Create a shader object, load the shader source, and compile the shader.
GLuint LoadShader(GLenum type, const char *shaderSrc)
{
	GLuint shader;
	GLint compiled;
	GLint infoLen = 0;
	char *infoLog;

	// Create the shader object
	shader = glCreateShader(type);
	if (!shader)
		return 0;

	// Load the shader source
	glShaderSource(shader, 1, &shaderSrc, NULL);
	// Compile the shader
	glCompileShader(shader);
	// Check the compile status
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1)
		{
			infoLog = malloc(sizeof(char)*infoLen);
			glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
			fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

GLuint LoadProgram(const char *vertShaderSrc, const char *fragShaderSrc)
{
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint programObject;
	GLint linked;
	GLint infoLen = 0;
	char *infoLog;

	// Load the vertex/fragment shaders
	vertexShader = LoadShader(GL_VERTEX_SHADER, vertShaderSrc);
	if (!vertexShader)
		return 0;

	fragmentShader = LoadShader(GL_FRAGMENT_SHADER, fragShaderSrc);
	if (!fragmentShader)
	{
		glDeleteShader(vertexShader);
		return 0;
	}

	// Create the program object
	programObject = glCreateProgram();

	if (!programObject)
		return 0;

	glAttachShader(programObject, vertexShader);
	glAttachShader(programObject, fragmentShader);

	// Link the program
	glLinkProgram(programObject);

	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

	if (!linked) 
	{
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 1)
		{
			infoLog = malloc(sizeof(char)*infoLen);
			glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
			fprintf(stderr, "Error linking program:\n%s\n", infoLog);
			free(infoLog);
		}
		glDeleteProgram(programObject);
		return 0;
	}

	// Free up no longer needed shader resources
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return programObject;
}

void init_ogl_rgba(oglparameters *op)
{
	oglstate *ogl = op->ogl;

	GLbyte vShaderStr[] = 
	"attribute vec4 a_position;   \n"
	"attribute vec2 a_texCoord;   \n"
	"varying vec2 v_texCoord;     \n"
	"void main()                  \n"
	"{                            \n"
	"   gl_Position = a_position; \n"
	"   v_texCoord = a_texCoord;  \n"
	"}                            \n\0";

	GLbyte fShaderStr[] = 
	"precision mediump float;                            \n"
	"varying vec2 v_texCoord;                            \n"
	"uniform sampler2D s_texture;                        \n"
	"void main()                                         \n"
	"{                                                   \n"
	"  gl_FragColor = texture2D(s_texture, v_texCoord);  \n"
	"}                                                   \n\0";

	// Load the shaders and get a linked program object
	ogl->program = LoadProgram((char*)vShaderStr, (char*)fShaderStr);
	if (!ogl->program)
	{
		printf("Load Program %d\n", ogl->program);
		return;
	}

	ogl->samplerLoc = glGetUniformLocation(ogl->program, "s_texture");

	// Get the attribute locations
	ogl->positionLoc = glGetAttribLocation(ogl->program, "a_position"); // Query only
	ogl->texCoordLoc = glGetAttribLocation(ogl->program, "a_texCoord");

	init_vertices_indices(ogl);

	// Use the program object
	glUseProgram(ogl->program); 

	glGenTextures(1, &(ogl->tex));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ogl->tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ogl->width, ogl->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Load the vertex position
	//glVertexAttribPointer(ogl->positionLoc, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), ogl->vVertices);
	glVertexAttribPointer(ogl->positionLoc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), ogl->vVertices);

	// Load the texture coordinate
	glVertexAttribPointer(ogl->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), ogl->tVertices);

	glEnableVertexAttribArray(ogl->positionLoc);
	glEnableVertexAttribArray(ogl->texCoordLoc);
   
	glUniform1i(ogl->samplerLoc, 0);

	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // red
	//glClear(GL_COLOR_BUFFER_BIT);
}

void init_ogl_yuv420(oglparameters *op)
{
	oglstate *ogl = op->ogl;

	GLbyte vShaderStr[] = 
		"attribute vec4 a_position;   \n"
		"attribute vec2 a_texCoord;   \n"
		"varying vec2 v_texCoord;     \n"
		"void main()                  \n"
		"{                            \n"
		"   gl_Position = a_position; \n"
		"   v_texCoord = a_texCoord;  \n"
		"}                            \n\0";

	GLbyte fShaderStr[] = 
		"precision mediump float;                              \n"
		"varying vec2 v_texCoord;                              \n"
		"uniform mat3 yuv2rgb;                                 \n"
		"uniform sampler2D y_texture;                          \n"
		"uniform sampler2D u_texture;                          \n"
		"uniform sampler2D v_texture;                          \n"
		"void main()                                           \n"
		"{                                                     \n"
		"  float y, u, v;                                      \n"

		"  y=texture2D(y_texture, v_texCoord).s;               \n"
		"  u=texture2D(u_texture, v_texCoord).s;               \n"
		"  v=texture2D(v_texture, v_texCoord).s;               \n"

		"  vec3 yuv=vec3(1.1643*(y-0.0625), u-0.5, v-0.5);     \n"
		"  vec3 rgb=yuv*yuv2rgb;                               \n"

		"  gl_FragColor=vec4(rgb, 1.0);                        \n"
		"}                                                     \n\0";

	// Load the shaders and get a linked program object
	ogl->program = LoadProgram((char*)vShaderStr, (char*)fShaderStr);
	if (!ogl->program)
	{
		printf("Load Program %d\n", ogl->program);
		return;
	}

	// Use the program object
	glUseProgram(ogl->program); 

	glGenTextures(3, &(ogl->texyuv[0]));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ogl->texyuv[0]);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, op->linewidth, op->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, op->linewidth, op->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ogl->texyuv[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, op->linewidth/2, op->height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ogl->texyuv[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, op->linewidth/2, op->height/2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	ogl->yuvsamplerLoc[0] = glGetUniformLocation(ogl->program, "y_texture");
	glUniform1i(ogl->yuvsamplerLoc[0], 0);

	ogl->yuvsamplerLoc[1] = glGetUniformLocation(ogl->program, "u_texture");
	glUniform1i(ogl->yuvsamplerLoc[1], 1);

	ogl->yuvsamplerLoc[2] = glGetUniformLocation(ogl->program, "v_texture");
	glUniform1i(ogl->yuvsamplerLoc[2], 2);

	ogl->cmatrixLoc = glGetUniformLocation(ogl->program, "yuv2rgb");
	GLfloat yuv2rgbmatrix420[9] = { 1.0, 0.0, 1.5958, 1.0, -0.3917, -0.8129, 1.0, 2.017, 0.0 }; // YUV420
	glUniformMatrix3fv(ogl->cmatrixLoc, 1, GL_FALSE, yuv2rgbmatrix420);

	init_vertices_indices(ogl);

	// Get the attribute locations
	ogl->positionLoc = glGetAttribLocation(ogl->program, "a_position"); // Query only
	ogl->texCoordLoc = glGetAttribLocation(ogl->program, "a_texCoord");

	// Load the vertex position
	//glVertexAttribPointer(ogl->positionLoc, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), ogl->vVertices);
	glVertexAttribPointer(ogl->positionLoc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), ogl->vVertices);
	glEnableVertexAttribArray(ogl->positionLoc);

	// Load the texture coordinate
	glVertexAttribPointer(ogl->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), ogl->tVertices);
	glEnableVertexAttribArray(ogl->texCoordLoc);
   
	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // red
	//glClear(GL_COLOR_BUFFER_BIT);
}

void init_ogl_yuv422(oglparameters *op)
{
	oglstate *ogl = op->ogl;

	GLbyte vShaderStr[] = 
		"attribute vec4 a_position;   \n"
		"attribute vec2 a_texCoord;   \n"
		"varying vec2 v_texCoord;     \n"
		"void main()                  \n"
		"{                            \n"
		"   gl_Position = a_position; \n"
		"   v_texCoord = a_texCoord;  \n"
		"}                            \n\0";

	GLbyte fShaderStr[] =
		"precision mediump float;                              \n"
		"varying vec2 v_texCoord;                              \n"
		"uniform mat3 yuv2rgb;                                 \n"
		"uniform sampler2D y_texture;                          \n"
		"uniform sampler2D u_texture;                          \n"
		"uniform sampler2D v_texture;                          \n"
		"void main()                                           \n"
		"{                                                     \n"
		"  float y, u, v;                                      \n"

		"  y=texture2D(y_texture, v_texCoord).s;               \n"
		"  u=texture2D(u_texture, v_texCoord).s;               \n"
		"  v=texture2D(v_texture, v_texCoord).s;               \n"

		"  vec3 yuv=vec3(y, u-0.5, v-0.5);                     \n"
		"  vec3 rgb=yuv*yuv2rgb;                               \n"

		"  gl_FragColor=vec4(rgb, 1.0);                        \n"
		"}                                                     \n\0";

	// Load the shaders and get a linked program object
	ogl->program = LoadProgram((char*)vShaderStr, (char*)fShaderStr);
	if (!ogl->program)
	{
		printf("Load Program %d\n", ogl->program);
		return;
	}

	// Use the program object
	glUseProgram(ogl->program); 

	glGenTextures(3, &(ogl->texyuv[0]));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ogl->texyuv[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, op->linewidth, op->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, ogl->texyuv[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, op->linewidth/2, op->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, ogl->texyuv[2]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, op->linewidth/2, op->height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, NULL);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	ogl->yuvsamplerLoc[0] = glGetUniformLocation(ogl->program, "y_texture");
	glUniform1i(ogl->yuvsamplerLoc[0], 0);

	ogl->yuvsamplerLoc[1] = glGetUniformLocation(ogl->program, "u_texture");
	glUniform1i(ogl->yuvsamplerLoc[1], 1);

	ogl->yuvsamplerLoc[2] = glGetUniformLocation(ogl->program, "v_texture");
	glUniform1i(ogl->yuvsamplerLoc[2], 2);

	ogl->cmatrixLoc = glGetUniformLocation(ogl->program, "yuv2rgb");
	GLfloat yuv2rgbmatrix420[9] = { 1.0, 0.0, 1.5958, 1.0, -0.3917, -0.8129, 1.0, 2.017, 0.0 }; // YUV420
	glUniformMatrix3fv(ogl->cmatrixLoc, 1, GL_FALSE, yuv2rgbmatrix420);

	init_vertices_indices(ogl);

	// Get the attribute locations
	ogl->positionLoc = glGetAttribLocation(ogl->program, "a_position"); // Query only
	ogl->texCoordLoc = glGetAttribLocation(ogl->program, "a_texCoord");

	// Load the vertex position
	//glVertexAttribPointer(ogl->positionLoc, 3, GL_FLOAT, GL_FALSE, 3*sizeof(GLfloat), ogl->vVertices);
	glVertexAttribPointer(ogl->positionLoc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), ogl->vVertices);
	glEnableVertexAttribArray(ogl->positionLoc);

	// Load the texture coordinate
	glVertexAttribPointer(ogl->texCoordLoc, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), ogl->tVertices);
	glEnableVertexAttribArray(ogl->texCoordLoc);
   
	//glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // red
	//glClear(GL_COLOR_BUFFER_BIT);
}

void init_ogl(oglparameters *op)
{
	switch(op->fmt)
	{
		case YUV420:
			init_ogl_yuv420(op);
			break;
		case YUV422:
			init_ogl_yuv422(op);
			break;
		case RGBA:
		default:
			init_ogl_rgba(op);
			break;
	}
}

void tex2D_rgba(oglstate *ogl, char *rgba)
{
	glActiveTexture(GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth, ogl->height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

void tex2D_yuv420(oglstate *ogl, char *yuv)
{
	glActiveTexture(GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth, ogl->height, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv);

	glActiveTexture(GL_TEXTURE1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth/2, ogl->height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv+(ogl->linewidth*ogl->height));

	glActiveTexture(GL_TEXTURE2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth/2, ogl->height/2, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv+(ogl->linewidth*ogl->height*5/4));
}

void tex2D_yuv422(oglstate *ogl, char *yuv)
{
	glActiveTexture(GL_TEXTURE0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth, ogl->height, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv);

	glActiveTexture(GL_TEXTURE1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth/2, ogl->height, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv+(ogl->linewidth*ogl->height));

	glActiveTexture(GL_TEXTURE2);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ogl->linewidth/2, ogl->height, GL_LUMINANCE, GL_UNSIGNED_BYTE, yuv+(ogl->linewidth*ogl->height*3/2));
}

void tex2D_ogl(oglparameters *op)
{
	oglstate *ogl = op->ogl;
	char *buf = op->buf;

	switch(op->fmt)
	{
		case YUV420:
			tex2D_yuv420(ogl, buf);
			break;
		case YUV422:
			tex2D_yuv422(ogl, buf);
			break;
		case RGBA:
		default:
			tex2D_rgba(ogl, buf);
			break;
	}
}

void close_ogl_rgba(oglstate *ogl)
{
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);

	glDeleteTextures(1, &(ogl->tex));
	glDeleteBuffers(1, &(ogl->vb_buffer));
	glDeleteBuffers(1, &(ogl->ea_buffer));
	glDeleteProgram(ogl->program);
//printf("close_ogl_rgba\n");
}

void close_ogl_yuv420(oglstate *ogl)
{
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);

	glDeleteTextures(3, &(ogl->texyuv[0]));
	glDeleteBuffers(1, &(ogl->vb_buffer));
	glDeleteBuffers(1, &(ogl->ea_buffer));
	glDeleteProgram(ogl->program);
//printf("close_ogl_yuv420\n");
}

void close_ogl_yuv422(oglstate *ogl)
{
	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glUseProgram(0);

	glDeleteTextures(3, &(ogl->texyuv[0]));
	glDeleteBuffers(1, &(ogl->vb_buffer));
	glDeleteBuffers(1, &(ogl->ea_buffer));
	glDeleteProgram(ogl->program);
//printf("close_ogl_yuv422\n");
}

void close_ogl(oglparameters *op)
{
	oglstate *ogl = op->ogl;

	switch(ogl->fmt)
	{
		case RGBA:
			close_ogl_rgba(ogl);
			break;
		case YUV422:
			close_ogl_yuv422(ogl);
			break;
		case YUV420:
		default:
			close_ogl_yuv420(ogl);
			break;
	}
}

void makecurrent_ogl(oglparameters *op)
{
	oglstate *ogl = op->ogl;
	EGLBoolean result;
//	int32_t success;

// initialize
	static const EGLint attribute_list[] =
	{
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 0,
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};

	static const EGLint context_attributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	// get an EGL display connection
	ogl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	// initialize the EGL display connection
	result = eglInitialize(ogl->display, NULL, NULL);
	assert(result != EGL_FALSE);

	// get an appropriate EGL frame buffer configuration
	result = eglChooseConfig(ogl->display, attribute_list, &(ogl->config), 1, &(ogl->num_config));
	assert(result != EGL_FALSE);

	// get an appropriate EGL frame buffer configuration
	result = eglBindAPI(EGL_OPENGL_ES_API);
	assert(result != EGL_FALSE);

	// create an EGL rendering context
	ogl->context = eglCreateContext(ogl->display, ogl->config, EGL_NO_CONTEXT, context_attributes);
	assert(ogl->context != EGL_NO_CONTEXT);

//    success = graphics_get_display_size(0 /* LCD */, &(ogl->screen_width), &(ogl->screen_height));
//    assert(success >= 0);
//printf("%d %d\n", ogl->screen_width, ogl->screen_height);
	ogl->screen_width = gdk_screen_width();
	ogl->screen_height = gdk_screen_height();

	EGLint pbuffer_attributes[] = 
	{
		EGL_WIDTH, ogl->screen_width,
		EGL_HEIGHT, ogl->screen_height,
		EGL_NONE
	};

	ogl->surface = eglCreatePbufferSurface(ogl->display, ogl->config, pbuffer_attributes);
	assert(ogl->surface!=EGL_NO_SURFACE);

	// connect the context to the surface
	result = eglMakeCurrent(ogl->display, ogl->surface, ogl->surface, ogl->context);
	assert(result!=EGL_FALSE);

//printf("OpenGL version is (%s)\n", glGetString(GL_VERSION));
}

void drawwidget_ogl(oglparameters *op)
{
	oglstate *ogl = op->ogl;
	GtkWidget *widget = op->widget;

//printf("%d %d\n", gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
	glViewport(0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, ogl->indices);
	glFinish();
	eglSwapBuffers(ogl->display, ogl->surface);
}

void readpixels_ogl(oglparameters *op)
{
	GtkWidget *widget = op->widget;
	pthread_mutex_lock(op->pixbufmutex);
	glReadPixels(0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget), GL_RGBA, GL_UNSIGNED_BYTE, op->imgdata);
	pthread_mutex_unlock(op->pixbufmutex);
}

void unmakecurrent_ogl(oglparameters *op)
{
	oglstate *ogl = op->ogl;

// Destroy
	eglMakeCurrent(ogl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(ogl->display, ogl->surface);
	eglDestroyContext(ogl->display, ogl->context);
	eglTerminate(ogl->display);
}

// Idle callback functions

void oglidle_thread_make_current(oglidle *i)
{
	oglstate *ogl = (oglstate*)&(i->ogl);
	oglparameters op;

	op = i->data;
	op.width = ogl->width;
	op.height = ogl->height;
	op.ogl = ogl;
	oglidle_add(i, &makecurrent_ogl, (void *)&op);
}

void oglidle_thread_init_ogl(oglidle *i, YUVformats fmt, int width, int height, int linewidth, int codecheight)
{
	oglstate *ogl = (oglstate*)&(i->ogl);
	oglparameters op;

	op = i->data;
	op.fmt = ogl->fmt = fmt;
	op.width = ogl->width = width;
	op.height = ogl->height = height;
	op.linewidth = ogl->linewidth = linewidth;
	op.codecheight = ogl->codecheight = codecheight;
	op.ogl = ogl;
	op.widget = i->widget;
	op.pixbufmutex = i->data.pixbufmutex;
	oglidle_add(i, &init_ogl, (void *)&op);
}

void oglidle_thread_draw_widget(oglidle *i)
{
	oglparameters op;

	op = i->data;
	oglidle_add(i, &drawwidget_ogl, (void *)&op);
}

void oglidle_thread_readpixels_ogl(oglidle *i)
{
	oglparameters op;

	op = i->data;
	oglidle_add(i, &readpixels_ogl, (void *)&op);
}

void oglidle_thread_close_ogl(oglidle *i)
{
	oglparameters op;

	op = i->data;
	oglidle_add(i, &close_ogl, (void *)&op);
}

void oglidle_thread_unmake_current(oglidle *i)
{
	oglparameters op;

	op = i->data;
	oglidle_add(i, &unmakecurrent_ogl, (void *)&op);
}

void oglidle_thread_tex2d_ogl(oglidle *i, char *buf)
{
	oglparameters op;

	op = i->data;
	op.buf = buf;
	oglidle_add(i, &tex2D_ogl, (void *)&op);
}

void destroynotify_pix(guchar *pixels, gpointer data)
{
	free(pixels);
//printf("destroynotify\n");
}

// Drawing area events
void realize_da_event(GtkWidget *widget, gpointer data)
{
	oglidle *oi = (oglidle*)data;
	oglstate *ogl = (oglstate*)&(oi->ogl);

	int width, height, imgsize;

	GtkAllocation* alloc = g_new(GtkAllocation, 1);
	gtk_widget_get_allocation(widget, alloc);
	width = alloc->width;
	height = alloc->height;
	g_free(alloc);

	// Start idle thread
	oglidle_start(oi);

	// Initalize GL
	oglidle_thread_make_current(oi);

	// Init
	oglidle_thread_init_ogl(oi, ogl->fmt, ogl->width, ogl->height, ogl->linewidth, ogl->codecheight);

	// Load initial texture
//printf("texture size is currently %dx%d\n", ogl->width, ogl->height);
	imgsize = ogl->width * ogl->height *4;
	oi->data.buf = malloc(imgsize); // w*h RGBA
	//int j;
	//for(j=0;j<imgsize;j++)
	//	oi->data.buf[j] = 0x90;
	FILE *f = fopen("./images/Splash.data", "rb");
	fread(oi->data.buf, 1, imgsize, f);
	fclose(f);

	oglidle_thread_tex2d_ogl(oi, oi->data.buf);
	free(oi->data.buf);

	// initialize pixbuf
	imgsize = width * height * 4;
	oi->data.imgdata = malloc(imgsize); // w*h RGBA
//	for(j=0;j<imgsize;j++)
//		oi->data.imgdata[j] = 0x90;

	oglidle_thread_draw_widget(oi);
	oglidle_thread_readpixels_ogl(oi);

	pthread_mutex_lock(oi->data.pixbufmutex);
	oi->data.pix = gdk_pixbuf_new_from_data((const guchar*)oi->data.imgdata, GDK_COLORSPACE_RGB, TRUE, 8, width, height, width*4, destroynotify_pix, NULL);
	pthread_mutex_unlock(oi->data.pixbufmutex);

	g_signal_connect(widget, "size-allocate", G_CALLBACK(size_allocate_da_event), data);
//printf("realize_da_event\n");
}

gboolean draw_da_event(GtkWidget *widget, cairo_t *cr, gpointer data)
{
	oglidle *oi = (oglidle*)data;

	pthread_mutex_lock(oi->data.pixbufmutex);
	gdk_cairo_set_source_pixbuf(cr, oi->data.pix, 0, 0);
	cairo_paint(cr);
	pthread_mutex_unlock(oi->data.pixbufmutex);

//printf("draw_da_event\n");
	return FALSE;
}

gboolean size_allocate_da_event(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data)
{
	oglidle *oi = (oglidle*)user_data;
	int width, height, imgsize;

	width = allocation->width;
	height = allocation->height;

	// reinitialize pixbuf
	pthread_mutex_lock(oi->data.pixbufmutex);
	g_object_unref(G_OBJECT(oi->data.pix));
	imgsize = width * height * 4;
	oi->data.imgdata = malloc(imgsize); // w*h RGBA
	oi->data.pix = gdk_pixbuf_new_from_data((const guchar*)oi->data.imgdata, GDK_COLORSPACE_RGB, TRUE, 8, width, height, width*4, destroynotify_pix, NULL);
	pthread_mutex_unlock(oi->data.pixbufmutex);

	oglidle_thread_draw_widget(oi);
	oglidle_thread_readpixels_ogl(oi);

//printf("size_allocate_da_event\n");
	return FALSE;
}

void destroy_da_event(GtkWidget *widget, gpointer data)
{
	oglidle *oi = (oglidle*)data;

	pthread_mutex_lock(oi->data.pixbufmutex);
	g_object_unref(oi->data.pix);
	pthread_mutex_unlock(oi->data.pixbufmutex);

	// Close
	oglidle_thread_close_ogl(oi);

	// Destroy GL
	oglidle_thread_unmake_current(oi);

	// Stop idle thread
	oglidle_stop(oi);
//printf("destroy_da_event\n");
}

// Public functions
void reinit_ogl(oglidle *i, YUVformats fmt, int width, int height, int linewidth, int codecheight)
{
	oglidle_thread_close_ogl(i);
	oglidle_thread_init_ogl(i, fmt, width, height, linewidth, codecheight);
//printf("reinit_ogl\n");
}

void draw_texture(oglidle *i, char *buf)
{
	oglidle_thread_tex2d_ogl(i, buf);
}
