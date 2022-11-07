#ifndef YUV420RGBgl
#define YUV420RGBgl

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
#include <pthread.h>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <wayland-client.h>

typedef enum
{
	RGBA,
	YUV420,
	YUV422
}YUVformats;

typedef struct
{
	GLuint program;
	GLuint vb_buffer;
	GLuint ea_buffer;
	GLint positionLoc;
	GLint texCoordLoc;
	GLint samplerLoc;
	GLint yuvsamplerLoc[3];
	GLint sizeLoc;
	GLint cmatrixLoc;
	GLuint tex;
	GLuint texyuv[3];
	GLint texture_coord_attribute;
	GLfloat vVertices[8];
	GLfloat tVertices[8];
	GLushort indices[6];

	EGLint num_config;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
	EGLConfig config;
    uint32_t screen_width;
    uint32_t screen_height;

	YUVformats fmt;
	int width, height; // texture size
	int linewidth, codecheight; // cropped video
}oglstate;

typedef struct
{
	YUVformats fmt;
	int width, height, linewidth, codecheight;
	char *buf; // texture data
	GdkPixbuf *pix;
	char *imgdata; // raw pixel data for pixbuf
	oglstate *ogl;
	GtkWidget *widget;
	pthread_mutex_t *pixbufmutex;
}oglparameters;

typedef struct
{
	pthread_mutex_t *A;
	pthread_cond_t *ready;
	pthread_cond_t *done;
	pthread_cond_t *busy;
	int commandready, commanddone, commandbusy;
	void (*func)(oglparameters *data);
	oglparameters data;
	oglstate ogl;
	pthread_t tid;
	int retval;
	int stoprequested;
	GtkWidget *widget;
}oglidle;

void realize_da_event(GtkWidget *widget, gpointer data);
gboolean draw_da_event(GtkWidget *widget, cairo_t *cr, gpointer data);
gboolean size_allocate_da_event(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data);
void destroy_da_event(GtkWidget *widget, gpointer data);

void oglidle_thread_init_ogl(oglidle *i, YUVformats fmt, int width, int height, int linewidth, int codecheight);
void oglidle_thread_tex2d_ogl(oglidle *i, char *buf);
void oglidle_thread_draw_widget(oglidle *i);
void oglidle_thread_readpixels_ogl(oglidle *i);
void oglidle_thread_close_ogl(oglidle *i);

void reinit_ogl(oglidle *i, YUVformats fmt, int width, int height, int linewidth, int codecheight);
void draw_texture(oglidle *i, char *buf);
#endif
