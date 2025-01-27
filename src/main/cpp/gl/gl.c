//
// Created by Swung0x48 on 2024/10/8.
//

#include "../includes.h"
#include "gl.h"
#include "glcorearb.h"
#include "log.h"
#include "loader.h"
#include "../gles/loader.h"
#include "mg.h"

/*
* Miscellaneous
*/

NATIVE_FUNCTION_HEAD(void, glClearIndex, GLfloat c ) NATIVE_FUNCTION_END_NO_RETURN(void,glClearIndex,c)

NATIVE_FUNCTION_HEAD(void, glClearColor, GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glClearColor,red,green,blue,alpha)

NATIVE_FUNCTION_HEAD(void, glClear, GLbitfield mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glClear,mask)

NATIVE_FUNCTION_HEAD(void, glIndexMask, GLuint mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexMask,mask)

NATIVE_FUNCTION_HEAD(void, glColorMask, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColorMask,red,green,blue,alpha)

NATIVE_FUNCTION_HEAD(void, glAlphaFunc, GLenum func, GLclampf ref ) NATIVE_FUNCTION_END_NO_RETURN(void,glAlphaFunc,func,ref)

NATIVE_FUNCTION_HEAD(void, glBlendFunc, GLenum sfactor, GLenum dfactor ) NATIVE_FUNCTION_END_NO_RETURN(void,glBlendFunc,sfactor,dfactor)

NATIVE_FUNCTION_HEAD(void, glLogicOp, GLenum opcode ) NATIVE_FUNCTION_END_NO_RETURN(void,glLogicOp,opcode)

NATIVE_FUNCTION_HEAD(void, glCullFace, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glCullFace,mode)

NATIVE_FUNCTION_HEAD(void, glFrontFace, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glFrontFace,mode)

NATIVE_FUNCTION_HEAD(void, glPointSize, GLfloat size ) NATIVE_FUNCTION_END_NO_RETURN(void,glPointSize,size)

NATIVE_FUNCTION_HEAD(void, glLineWidth, GLfloat width ) NATIVE_FUNCTION_END_NO_RETURN(void,glLineWidth,width)

NATIVE_FUNCTION_HEAD(void, glLineStipple, GLint factor, GLushort pattern ) NATIVE_FUNCTION_END_NO_RETURN(void,glLineStipple,factor,pattern)

NATIVE_FUNCTION_HEAD(void, glPolygonMode, GLenum face, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glPolygonMode,face,mode)

NATIVE_FUNCTION_HEAD(void, glPolygonOffset, GLfloat factor, GLfloat units ) NATIVE_FUNCTION_END_NO_RETURN(void,glPolygonOffset,factor,units)

NATIVE_FUNCTION_HEAD(void, glPolygonStipple, const GLubyte *mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glPolygonStipple,mask)

NATIVE_FUNCTION_HEAD(void, glGetPolygonStipple, GLubyte *mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetPolygonStipple,mask)

NATIVE_FUNCTION_HEAD(void, glEdgeFlag, GLboolean flag ) NATIVE_FUNCTION_END_NO_RETURN(void,glEdgeFlag,flag)

NATIVE_FUNCTION_HEAD(void, glEdgeFlagv, const GLboolean *flag ) NATIVE_FUNCTION_END_NO_RETURN(void,glEdgeFlagv,flag)

NATIVE_FUNCTION_HEAD(void, glScissor, GLint x, GLint y, GLsizei width, GLsizei height) NATIVE_FUNCTION_END_NO_RETURN(void,glScissor,x,y,width,height)

NATIVE_FUNCTION_HEAD(void, glClipPlane, GLenum plane, const GLdouble *equation ) NATIVE_FUNCTION_END_NO_RETURN(void,glClipPlane,plane,equation)

NATIVE_FUNCTION_HEAD(void, glGetClipPlane, GLenum plane, GLdouble *equation ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetClipPlane,plane,equation)

NATIVE_FUNCTION_HEAD(void, glDrawBuffer, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glDrawBuffer,mode)

NATIVE_FUNCTION_HEAD(void, glReadBuffer, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glReadBuffer,mode)

NATIVE_FUNCTION_HEAD(void, glEnable, GLenum cap ) NATIVE_FUNCTION_END_NO_RETURN(void,glEnable,cap)

NATIVE_FUNCTION_HEAD(void, glDisable, GLenum cap ) NATIVE_FUNCTION_END_NO_RETURN(void,glDisable,cap)

NATIVE_FUNCTION_HEAD(GLboolean, glIsEnabled, GLenum cap ) NATIVE_FUNCTION_END(GLboolean,glIsEnabled,cap)


NATIVE_FUNCTION_HEAD(void, glEnableClientState, GLenum cap ) NATIVE_FUNCTION_END_NO_RETURN(void,glEnableClientState,cap)

NATIVE_FUNCTION_HEAD(void, glDisableClientState, GLenum cap ) NATIVE_FUNCTION_END_NO_RETURN(void,glDisableClientState,cap)


NATIVE_FUNCTION_HEAD(void, glGetBooleanv, GLenum pname, GLboolean *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetBooleanv,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetDoublev, GLenum pname, GLdouble *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetDoublev,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetFloatv, GLenum pname, GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetFloatv,pname,params)

//NATIVE_FUNCTION_HEAD(void, glGetIntegerv, GLenum pname, GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetIntegerv,pname,params)

GLAPI GLAPIENTRY void glGetIntegerv(GLenum pname, GLint *params) {
	LOG();
	LOG_D("glGetIntegerv, pname: %d",pname);
	if (pname == GL_CONTEXT_PROFILE_MASK) {
		(*params) = GL_CONTEXT_COMPATIBILITY_PROFILE_BIT;
		return;
	}
	LOAD_GLES(glGetIntegerv, void, GLenum pname, GLint *params);
	gles_glGetIntegerv(pname, params);
	LOAD_GLES(glGetError, GLenum)            
    GLenum ERR = gles_glGetError();          
    if (ERR != GL_NO_ERROR)                  
        LOG_E("ERROR: %d", ERR)              
}

NATIVE_FUNCTION_HEAD(void, glPushAttrib, GLbitfield mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glPushAttrib,mask)

NATIVE_FUNCTION_HEAD(void, glPopAttrib) NATIVE_FUNCTION_END_NO_RETURN(void,glPopAttrib)


NATIVE_FUNCTION_HEAD(void, glPushClientAttrib, GLbitfield mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glPushClientAttrib,mask)

NATIVE_FUNCTION_HEAD(void, glPopClientAttrib) NATIVE_FUNCTION_END_NO_RETURN(void,glPopClientAttrib)


NATIVE_FUNCTION_HEAD(GLint, glRenderMode, GLenum mode) NATIVE_FUNCTION_END(GLint,glRenderMode,mode)

//NATIVE_FUNCTION_HEAD(GLenum,glGetError) NATIVE_FUNCTION_END(GLenum,glGetError)
GLAPI GLAPIENTRY GLenum glGetError() {
	LOG();
	LOAD_GLES(glGetError, GLenum); 
	return gles_glGetError();
}

NATIVE_FUNCTION_HEAD(const GLubyte *,glGetStringi,GLenum name,GLuint index) NATIVE_FUNCTION_END(GLenum,glGetStringi, name, index)

GLAPI GLAPIENTRY const GLubyte * glGetString( GLenum name ) {
	LOG();
	switch (name) {
	case GL_VENDOR:
		return (const GLubyte *)"Swung0x48, BZLZHH";
	case GL_RENDERER:
		return (const GLubyte *)"2.1 MobileGlues";
	case GL_VERSION:
		return (const GLubyte *)"2.1.0";
	}
	return (const GLubyte *)"NotSupported_GLenum";
}

NATIVE_FUNCTION_HEAD(void, glFinish) NATIVE_FUNCTION_END_NO_RETURN(void,glFinish)

NATIVE_FUNCTION_HEAD(void, glFlush) NATIVE_FUNCTION_END_NO_RETURN(void,glFlush)

NATIVE_FUNCTION_HEAD(void, glHint, GLenum target, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glHint,target,mode)


/*
* Depth Buffer
*/

GLAPI GLAPIENTRY void glClearDepth(GLclampd depth) {
	LOG();
	glClearDepthf(depth);
	glClear(GL_DEPTH_BUFFER_BIT);
}

NATIVE_FUNCTION_HEAD(void, glClearDepthf, GLfloat depth ) NATIVE_FUNCTION_END_NO_RETURN(void,glClearDepthf, depth)

NATIVE_FUNCTION_HEAD(void, glDepthFunc, GLenum func ) NATIVE_FUNCTION_END_NO_RETURN(void,glDepthFunc,func)

NATIVE_FUNCTION_HEAD(void, glDepthMask, GLboolean flag ) NATIVE_FUNCTION_END_NO_RETURN(void,glDepthMask,flag)

NATIVE_FUNCTION_HEAD(void, glDepthRange, GLclampd near_val, GLclampd far_val ) NATIVE_FUNCTION_END_NO_RETURN(void,glDepthRange, near_val, far_val)


/*
* Accumulation Buffer
*/

NATIVE_FUNCTION_HEAD(void, glClearAccum, GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glClearAccum,red,green,blue,alpha)

NATIVE_FUNCTION_HEAD(void, glAccum, GLenum op, GLfloat value ) NATIVE_FUNCTION_END_NO_RETURN(void,glAccum,op,value)


/*
* Transformation
*/

NATIVE_FUNCTION_HEAD(void, glMatrixMode, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glMatrixMode,mode)

NATIVE_FUNCTION_HEAD(void, glOrtho, GLdouble left, GLdouble right,GLdouble bottom, GLdouble top,GLdouble near_val, GLdouble far_val ) NATIVE_FUNCTION_END_NO_RETURN(void,glOrtho,left,right,bottom,top,near_val,far_val)

NATIVE_FUNCTION_HEAD(void, glFrustum, GLdouble left, GLdouble right,GLdouble bottom, GLdouble top,GLdouble near_val, GLdouble far_val ) NATIVE_FUNCTION_END_NO_RETURN(void,glFrustum,left,right,bottom,top,near_val,far_val)

NATIVE_FUNCTION_HEAD(void, glViewport, GLint x, GLint y,GLsizei width, GLsizei height ) NATIVE_FUNCTION_END_NO_RETURN(void,glViewport,x,y,width,height)

NATIVE_FUNCTION_HEAD(void, glPushMatrix) NATIVE_FUNCTION_END_NO_RETURN(void,glPushMatrix)

NATIVE_FUNCTION_HEAD(void, glPopMatrix) NATIVE_FUNCTION_END_NO_RETURN(void,glPopMatrix)

NATIVE_FUNCTION_HEAD(void, glLoadIdentity) NATIVE_FUNCTION_END_NO_RETURN(void,glLoadIdentity)

NATIVE_FUNCTION_HEAD(void, glLoadMatrixd, const GLdouble *m ) NATIVE_FUNCTION_END_NO_RETURN(void,glLoadMatrixd,m)
NATIVE_FUNCTION_HEAD(void, glLoadMatrixf, const GLfloat *m ) NATIVE_FUNCTION_END_NO_RETURN(void,glLoadMatrixf,m)

NATIVE_FUNCTION_HEAD(void, glMultMatrixd, const GLdouble *m ) NATIVE_FUNCTION_END_NO_RETURN(void,glMultMatrixd,m)
NATIVE_FUNCTION_HEAD(void, glMultMatrixf, const GLfloat *m ) NATIVE_FUNCTION_END_NO_RETURN(void,glMultMatrixf,m)

NATIVE_FUNCTION_HEAD(void, glRotated, GLdouble angle,GLdouble x, GLdouble y, GLdouble z ) NATIVE_FUNCTION_END_NO_RETURN(void,glRotated,angle,x,y,z)
NATIVE_FUNCTION_HEAD(void, glRotatef, GLfloat angle,GLfloat x, GLfloat y, GLfloat z ) NATIVE_FUNCTION_END_NO_RETURN(void,glRotatef,angle,x,y,z)

NATIVE_FUNCTION_HEAD(void, glScaled, GLdouble x, GLdouble y, GLdouble z ) NATIVE_FUNCTION_END_NO_RETURN(void,glScaled,x,y,z)
NATIVE_FUNCTION_HEAD(void, glScalef, GLfloat x, GLfloat y, GLfloat z ) NATIVE_FUNCTION_END_NO_RETURN(void,glScalef,x,y,z)

NATIVE_FUNCTION_HEAD(void, glTranslated, GLdouble x, GLdouble y, GLdouble z ) NATIVE_FUNCTION_END_NO_RETURN(void,glTranslated,x,y,z)
NATIVE_FUNCTION_HEAD(void, glTranslatef, GLfloat x, GLfloat y, GLfloat z ) NATIVE_FUNCTION_END_NO_RETURN(void,glTranslatef,x,y,z)


/*
* Display Lists
*/

NATIVE_FUNCTION_HEAD(GLboolean, glIsList, GLuint list ); NATIVE_FUNCTION_END(GLboolean,glIsList,list)

NATIVE_FUNCTION_HEAD(void, glDeleteLists, GLuint list, GLsizei range ) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteLists,list,range)

NATIVE_FUNCTION_HEAD(GLuint, glGenLists, GLsizei range ) NATIVE_FUNCTION_END(GLuint,glGenLists,range)

NATIVE_FUNCTION_HEAD(void, glNewList, GLuint list, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glNewList,list,mode)

NATIVE_FUNCTION_HEAD(void, glEndList) NATIVE_FUNCTION_END_NO_RETURN(void,glEndList)

NATIVE_FUNCTION_HEAD(void, glCallList, GLuint list ) NATIVE_FUNCTION_END_NO_RETURN(void,glCallList,list)

NATIVE_FUNCTION_HEAD(void, glCallLists, GLsizei n, GLenum type,const GLvoid* lists ) NATIVE_FUNCTION_END_NO_RETURN(void,glCallLists,n,type,lists)

NATIVE_FUNCTION_HEAD(void, glListBase, GLuint base ) NATIVE_FUNCTION_END_NO_RETURN(void,glListBase,base)


/*
* Drawing Functions
*/

NATIVE_FUNCTION_HEAD(void, glBegin, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glBegin,mode)

NATIVE_FUNCTION_HEAD(void, glEnd) NATIVE_FUNCTION_END_NO_RETURN(void,glEnd)


NATIVE_FUNCTION_HEAD(void, glVertex2d, GLdouble x, GLdouble y ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2d,x,y)
NATIVE_FUNCTION_HEAD(void, glVertex2f, GLfloat x, GLfloat y ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2f,x,y)
NATIVE_FUNCTION_HEAD(void, glVertex2i, GLint x, GLint y ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2i,x,y)
NATIVE_FUNCTION_HEAD(void, glVertex2s, GLshort x, GLshort y ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2s,x,y)

NATIVE_FUNCTION_HEAD(void, glVertex3d, GLdouble x, GLdouble y, GLdouble z ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3d,x,y,z)
NATIVE_FUNCTION_HEAD(void, glVertex3f, GLfloat x, GLfloat y, GLfloat z ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3f,x,y,z)
NATIVE_FUNCTION_HEAD(void, glVertex3i, GLint x, GLint y, GLint z ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3i,x,y,z)
NATIVE_FUNCTION_HEAD(void, glVertex3s, GLshort x, GLshort y, GLshort z ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3s,x,y,z)

NATIVE_FUNCTION_HEAD(void, glVertex4d, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4d,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glVertex4f, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4f,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glVertex4i, GLint x, GLint y, GLint z, GLint w ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4i,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glVertex4s, GLshort x, GLshort y, GLshort z, GLshort w ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4s,x,y,z,w)

NATIVE_FUNCTION_HEAD(void, glVertex2dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2dv,v)
NATIVE_FUNCTION_HEAD(void, glVertex2fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2fv,v)
NATIVE_FUNCTION_HEAD(void, glVertex2iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2iv,v)
NATIVE_FUNCTION_HEAD(void, glVertex2sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex2sv,v)

NATIVE_FUNCTION_HEAD(void, glVertex3dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3dv,v)
NATIVE_FUNCTION_HEAD(void, glVertex3fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3fv,v)
NATIVE_FUNCTION_HEAD(void, glVertex3iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3iv,v)
NATIVE_FUNCTION_HEAD(void, glVertex3sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex3sv,v)

NATIVE_FUNCTION_HEAD(void, glVertex4dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4dv,v)
NATIVE_FUNCTION_HEAD(void, glVertex4fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4fv,v)
NATIVE_FUNCTION_HEAD(void, glVertex4iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4iv,v)
NATIVE_FUNCTION_HEAD(void, glVertex4sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertex4sv,v)


NATIVE_FUNCTION_HEAD(void, glNormal3b, GLbyte nx, GLbyte ny, GLbyte nz ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3b,nx,ny,nz)
NATIVE_FUNCTION_HEAD(void, glNormal3d, GLdouble nx, GLdouble ny, GLdouble nz ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3d,nx,ny,nz)
NATIVE_FUNCTION_HEAD(void, glNormal3f, GLfloat nx, GLfloat ny, GLfloat nz ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3f,nx,ny,nz)
NATIVE_FUNCTION_HEAD(void, glNormal3i, GLint nx, GLint ny, GLint nz ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3i,nx,ny,nz)
NATIVE_FUNCTION_HEAD(void, glNormal3s, GLshort nx, GLshort ny, GLshort nz ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3s,nx,ny,nz)

NATIVE_FUNCTION_HEAD(void, glNormal3bv, const GLbyte *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3bv,v)
NATIVE_FUNCTION_HEAD(void, glNormal3dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3dv,v)
NATIVE_FUNCTION_HEAD(void, glNormal3fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3fv,v)
NATIVE_FUNCTION_HEAD(void, glNormal3iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3iv,v)
NATIVE_FUNCTION_HEAD(void, glNormal3sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormal3sv,v)


NATIVE_FUNCTION_HEAD(void, glIndexd, GLdouble c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexd,c)
NATIVE_FUNCTION_HEAD(void, glIndexf, GLfloat c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexf,c)
NATIVE_FUNCTION_HEAD(void, glIndexi, GLint c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexi,c)
NATIVE_FUNCTION_HEAD(void, glIndexs, GLshort c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexs,c)
NATIVE_FUNCTION_HEAD(void, glIndexub, GLubyte c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexub,c)

NATIVE_FUNCTION_HEAD(void, glIndexdv, const GLdouble *c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexdv,c)
NATIVE_FUNCTION_HEAD(void, glIndexfv, const GLfloat *c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexfv,c)
NATIVE_FUNCTION_HEAD(void, glIndexiv, const GLint *c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexiv,c)
NATIVE_FUNCTION_HEAD(void, glIndexsv, const GLshort *c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexsv,c)
NATIVE_FUNCTION_HEAD(void, glIndexubv, const GLubyte *c ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexubv,c)

NATIVE_FUNCTION_HEAD(void, glColor3b, GLbyte red, GLbyte green, GLbyte blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3b,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3d, GLdouble red, GLdouble green, GLdouble blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3d,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3f, GLfloat red, GLfloat green, GLfloat blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3f,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3i, GLint red, GLint green, GLint blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3i,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3s, GLshort red, GLshort green, GLshort blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3s,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3ub, GLubyte red, GLubyte green, GLubyte blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3ub,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3ui, GLuint red, GLuint green, GLuint blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3ui,red,green,blue)
NATIVE_FUNCTION_HEAD(void, glColor3us, GLushort red, GLushort green, GLushort blue ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3us,red,green,blue)

NATIVE_FUNCTION_HEAD(void, glColor4b, GLbyte red, GLbyte green,GLbyte blue, GLbyte alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4b,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4d, GLdouble red, GLdouble green,GLdouble blue, GLdouble alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4d,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4f, GLfloat red, GLfloat green,GLfloat blue, GLfloat alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4f,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4i, GLint red, GLint green,GLint blue, GLint alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4i,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4s, GLshort red, GLshort green,GLshort blue, GLshort alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4s,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4ub, GLubyte red, GLubyte green,GLubyte blue, GLubyte alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4ub,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4ui, GLuint red, GLuint green,GLuint blue, GLuint alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4ui,red,green,blue,alpha)
NATIVE_FUNCTION_HEAD(void, glColor4us, GLushort red, GLushort green,GLushort blue, GLushort alpha ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4us,red,green,blue,alpha)


NATIVE_FUNCTION_HEAD(void, glColor3bv, const GLbyte *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3bv,v)
NATIVE_FUNCTION_HEAD(void, glColor3dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3dv,v)
NATIVE_FUNCTION_HEAD(void, glColor3fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3fv,v)
NATIVE_FUNCTION_HEAD(void, glColor3iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3iv,v)
NATIVE_FUNCTION_HEAD(void, glColor3sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3sv,v)
NATIVE_FUNCTION_HEAD(void, glColor3ubv, const GLubyte *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3ubv,v)
NATIVE_FUNCTION_HEAD(void, glColor3uiv, const GLuint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3uiv,v)
NATIVE_FUNCTION_HEAD(void, glColor3usv, const GLushort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor3usv,v)

NATIVE_FUNCTION_HEAD(void, glColor4bv, const GLbyte *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4bv,v)
NATIVE_FUNCTION_HEAD(void, glColor4dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4dv,v)
NATIVE_FUNCTION_HEAD(void, glColor4fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4fv,v)
NATIVE_FUNCTION_HEAD(void, glColor4iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4iv,v)
NATIVE_FUNCTION_HEAD(void, glColor4sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4sv,v)
NATIVE_FUNCTION_HEAD(void, glColor4ubv, const GLubyte *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4ubv,v)
NATIVE_FUNCTION_HEAD(void, glColor4uiv, const GLuint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4uiv,v)
NATIVE_FUNCTION_HEAD(void, glColor4usv, const GLushort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glColor4usv,v)


NATIVE_FUNCTION_HEAD(void, glTexCoord1d, GLdouble s ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1d,s)
NATIVE_FUNCTION_HEAD(void, glTexCoord1f, GLfloat s ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1f,s)
NATIVE_FUNCTION_HEAD(void, glTexCoord1i, GLint s ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1i,s)
NATIVE_FUNCTION_HEAD(void, glTexCoord1s, GLshort s ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1s,s)

NATIVE_FUNCTION_HEAD(void, glTexCoord2d, GLdouble s, GLdouble t ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2d,s,t)
NATIVE_FUNCTION_HEAD(void, glTexCoord2f, GLfloat s, GLfloat t ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2f,s,t)
NATIVE_FUNCTION_HEAD(void, glTexCoord2i, GLint s, GLint t ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2i,s,t)
NATIVE_FUNCTION_HEAD(void, glTexCoord2s, GLshort s, GLshort t ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2s,s,t)

NATIVE_FUNCTION_HEAD(void, glTexCoord3d, GLdouble s, GLdouble t, GLdouble r ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3d,s,t,r)
NATIVE_FUNCTION_HEAD(void, glTexCoord3f, GLfloat s, GLfloat t, GLfloat r ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3f,s,t,r)
NATIVE_FUNCTION_HEAD(void, glTexCoord3i, GLint s, GLint t, GLint r ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3i,s,t,r)
NATIVE_FUNCTION_HEAD(void, glTexCoord3s, GLshort s, GLshort t, GLshort r ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3s,s,t,r)

NATIVE_FUNCTION_HEAD(void, glTexCoord4d, GLdouble s, GLdouble t, GLdouble r, GLdouble q ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4d,s,t,r,q)
NATIVE_FUNCTION_HEAD(void, glTexCoord4f, GLfloat s, GLfloat t, GLfloat r, GLfloat q ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4f,s,t,r,q)
NATIVE_FUNCTION_HEAD(void, glTexCoord4i, GLint s, GLint t, GLint r, GLint q ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4i,s,t,r,q)
NATIVE_FUNCTION_HEAD(void, glTexCoord4s, GLshort s, GLshort t, GLshort r, GLshort q ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4s,s,t,r,q)

NATIVE_FUNCTION_HEAD(void, glTexCoord1dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1dv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord1fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1fv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord1iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1iv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord1sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord1sv,v)

NATIVE_FUNCTION_HEAD(void, glTexCoord2dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2dv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord2fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2fv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord2iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2iv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord2sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord2sv,v)

NATIVE_FUNCTION_HEAD(void, glTexCoord3dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3dv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord3fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3fv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord3iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3iv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord3sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord3sv,v)

NATIVE_FUNCTION_HEAD(void, glTexCoord4dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4dv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord4fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4fv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord4iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4iv,v)
NATIVE_FUNCTION_HEAD(void, glTexCoord4sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoord4sv,v)


NATIVE_FUNCTION_HEAD(void, glRasterPos2d, GLdouble x, GLdouble y ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2d,x,y)
NATIVE_FUNCTION_HEAD(void, glRasterPos2f, GLfloat x, GLfloat y ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2f,x,y)
NATIVE_FUNCTION_HEAD(void, glRasterPos2i, GLint x, GLint y ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2i,x,y)
NATIVE_FUNCTION_HEAD(void, glRasterPos2s, GLshort x, GLshort y ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2s,x,y)

NATIVE_FUNCTION_HEAD(void, glRasterPos3d, GLdouble x, GLdouble y, GLdouble z ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3d,x,y,z)
NATIVE_FUNCTION_HEAD(void, glRasterPos3f, GLfloat x, GLfloat y, GLfloat z ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3f,x,y,z)
NATIVE_FUNCTION_HEAD(void, glRasterPos3i, GLint x, GLint y, GLint z ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3i,x,y,z)
NATIVE_FUNCTION_HEAD(void, glRasterPos3s, GLshort x, GLshort y, GLshort z ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3s,x,y,z)

NATIVE_FUNCTION_HEAD(void, glRasterPos4d, GLdouble x, GLdouble y, GLdouble z, GLdouble w ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4d,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glRasterPos4f, GLfloat x, GLfloat y, GLfloat z, GLfloat w ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4f,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glRasterPos4i, GLint x, GLint y, GLint z, GLint w ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4i,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glRasterPos4s, GLshort x, GLshort y, GLshort z, GLshort w ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4s,x,y,z,w)

NATIVE_FUNCTION_HEAD(void, glRasterPos2dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2dv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos2fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2fv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos2iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2iv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos2sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos2sv,v)

NATIVE_FUNCTION_HEAD(void, glRasterPos3dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3dv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos3fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3fv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos3iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3iv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos3sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos3sv,v)

NATIVE_FUNCTION_HEAD(void, glRasterPos4dv, const GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4dv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos4fv, const GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4fv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos4iv, const GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4iv,v)
NATIVE_FUNCTION_HEAD(void, glRasterPos4sv, const GLshort *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glRasterPos4sv,v)


NATIVE_FUNCTION_HEAD(void, glRectd, GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRectd,x1,y1,x2,y2)
NATIVE_FUNCTION_HEAD(void, glRectf, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRectf,x1,y1,x2,y2)
NATIVE_FUNCTION_HEAD(void, glRecti, GLint x1, GLint y1, GLint x2, GLint y2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRecti,x1,y1,x2,y2)
NATIVE_FUNCTION_HEAD(void, glRects, GLshort x1, GLshort y1, GLshort x2, GLshort y2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRects,x1,y1,x2,y2)


NATIVE_FUNCTION_HEAD(void, glRectdv, const GLdouble *v1, const GLdouble *v2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRectdv,v1,v2)
NATIVE_FUNCTION_HEAD(void, glRectfv, const GLfloat *v1, const GLfloat *v2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRectfv,v1,v2)
NATIVE_FUNCTION_HEAD(void, glRectiv, const GLint *v1, const GLint *v2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRectiv,v1,v2)
NATIVE_FUNCTION_HEAD(void, glRectsv, const GLshort *v1, const GLshort *v2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glRectsv,v1,v2)


/*
* Vertex Arrays (1.1)
*/

NATIVE_FUNCTION_HEAD(void, glVertexPointer, GLint size, GLenum type,GLsizei stride, const GLvoid* ptr ) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexPointer,size,type,stride,ptr)

NATIVE_FUNCTION_HEAD(void, glNormalPointer, GLenum type, GLsizei stride,const GLvoid* ptr ) NATIVE_FUNCTION_END_NO_RETURN(void,glNormalPointer,type,stride,ptr)

NATIVE_FUNCTION_HEAD(void, glColorPointer, GLint size, GLenum type,GLsizei stride, const GLvoid* ptr ) NATIVE_FUNCTION_END_NO_RETURN(void,glColorPointer,size,type,stride,ptr)

NATIVE_FUNCTION_HEAD(void, glIndexPointer, GLenum type, GLsizei stride,const GLvoid* ptr ) NATIVE_FUNCTION_END_NO_RETURN(void,glIndexPointer,type,stride,ptr)

NATIVE_FUNCTION_HEAD(void, glTexCoordPointer, GLint size, GLenum type,GLsizei stride, const GLvoid* ptr ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexCoordPointer,size,type,stride,ptr)

NATIVE_FUNCTION_HEAD(void, glEdgeFlagPointer, GLsizei stride, const GLvoid* ptr ) NATIVE_FUNCTION_END_NO_RETURN(void,glEdgeFlagPointer,stride,ptr)

NATIVE_FUNCTION_HEAD(void, glGetPointerv, GLenum pname, GLvoid* *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetPointerv,pname,params)

NATIVE_FUNCTION_HEAD(void, glArrayElement, GLint i ) NATIVE_FUNCTION_END_NO_RETURN(void,glArrayElement,i)

NATIVE_FUNCTION_HEAD(void, glDrawArrays, GLenum mode, GLint first, GLsizei count ) NATIVE_FUNCTION_END_NO_RETURN(void,glDrawArrays,mode,first,count)

NATIVE_FUNCTION_HEAD(void, glDrawElements, GLenum mode, GLsizei count,GLenum type, const GLvoid* indices ) NATIVE_FUNCTION_END_NO_RETURN(void,glDrawElements,mode,count,type,indices)

NATIVE_FUNCTION_HEAD(void, glInterleavedArrays, GLenum format, GLsizei stride,const GLvoid* pointer ) NATIVE_FUNCTION_END_NO_RETURN(void,glInterleavedArrays,format,stride,pointer)

/*
* Lighting
*/

NATIVE_FUNCTION_HEAD(void, glShadeModel, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glShadeModel,mode)

NATIVE_FUNCTION_HEAD(void, glLightf, GLenum light, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightf,light,pname,param)
NATIVE_FUNCTION_HEAD(void, glLighti, GLenum light, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glLighti,light,pname,param)
NATIVE_FUNCTION_HEAD(void, glLightfv, GLenum light, GLenum pname,const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightfv,light,pname,params)
NATIVE_FUNCTION_HEAD(void, glLightiv, GLenum light, GLenum pname,const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightiv,light,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetLightfv, GLenum light, GLenum pname,GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetLightfv,light,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetLightiv, GLenum light, GLenum pname,GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetLightiv,light,pname,params)

NATIVE_FUNCTION_HEAD(void, glLightModelf, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightModelf,pname,param)
NATIVE_FUNCTION_HEAD(void, glLightModeli, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightModeli,pname,param)
NATIVE_FUNCTION_HEAD(void, glLightModelfv, GLenum pname, const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightModelfv,pname,params)
NATIVE_FUNCTION_HEAD(void, glLightModeliv, GLenum pname, const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glLightModeliv,pname,params)

NATIVE_FUNCTION_HEAD(void, glMaterialf, GLenum face, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glMaterialf,face,pname,param)
NATIVE_FUNCTION_HEAD(void, glMateriali, GLenum face, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glMateriali,face,pname,param)
NATIVE_FUNCTION_HEAD(void, glMaterialfv, GLenum face, GLenum pname, const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glMaterialfv,face,pname,params)
NATIVE_FUNCTION_HEAD(void, glMaterialiv, GLenum face, GLenum pname, const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glMaterialiv,face,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetMaterialfv, GLenum face, GLenum pname, GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetMaterialfv,face,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetMaterialiv, GLenum face, GLenum pname, GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetMaterialiv,face,pname,params)

NATIVE_FUNCTION_HEAD(void, glColorMaterial, GLenum face, GLenum mode ) NATIVE_FUNCTION_END_NO_RETURN(void,glColorMaterial,face,mode)


/*
* Raster functions
*/

NATIVE_FUNCTION_HEAD(void, glPixelZoom, GLfloat xfactor, GLfloat yfactor ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelZoom,xfactor,yfactor)

NATIVE_FUNCTION_HEAD(void, glPixelStoref, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelStoref,pname,param)
NATIVE_FUNCTION_HEAD(void, glPixelStorei, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelStorei,pname,param)

NATIVE_FUNCTION_HEAD(void, glPixelTransferf, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelTransferf,pname,param)
NATIVE_FUNCTION_HEAD(void, glPixelTransferi, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelTransferi,pname,param)

NATIVE_FUNCTION_HEAD(void, glPixelMapfv, GLenum map, GLsizei mapsize,const GLfloat *values ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelMapfv,map,mapsize,values)
NATIVE_FUNCTION_HEAD(void, glPixelMapuiv, GLenum map, GLsizei mapsize,const GLuint *values ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelMapuiv,map,mapsize,values)
NATIVE_FUNCTION_HEAD(void, glPixelMapusv, GLenum map, GLsizei mapsize,const GLushort *values ) NATIVE_FUNCTION_END_NO_RETURN(void,glPixelMapusv,map,mapsize,values)

NATIVE_FUNCTION_HEAD(void, glGetPixelMapfv, GLenum map, GLfloat *values ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetPixelMapfv,map,values)
NATIVE_FUNCTION_HEAD(void, glGetPixelMapuiv, GLenum map, GLuint *values ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetPixelMapuiv,map,values)
NATIVE_FUNCTION_HEAD(void, glGetPixelMapusv, GLenum map, GLushort *values ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetPixelMapusv,map,values)

NATIVE_FUNCTION_HEAD(void, glBitmap, GLsizei width, GLsizei height,GLfloat xorig, GLfloat yorig,GLfloat xmove, GLfloat ymove,const GLubyte *bitmap ) NATIVE_FUNCTION_END_NO_RETURN(void,glBitmap,width,height,xorig,yorig,xmove,ymove,bitmap)

NATIVE_FUNCTION_HEAD(void, glReadPixels, GLint x, GLint y,GLsizei width, GLsizei height,GLenum format, GLenum type,GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glReadPixels,x,y,width,height,format,type,pixels)

NATIVE_FUNCTION_HEAD(void, glDrawPixels, GLsizei width, GLsizei height,GLenum format, GLenum type,const GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glDrawPixels,width,height,format,type,pixels)

NATIVE_FUNCTION_HEAD(void, glCopyPixels, GLint x, GLint y,GLsizei width, GLsizei height,GLenum type ) NATIVE_FUNCTION_END_NO_RETURN(void,glCopyPixels,x,y,width,height,type)

/*
* Stenciling
*/

NATIVE_FUNCTION_HEAD(void, glStencilFunc, GLenum func, GLint ref, GLuint mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glStencilFunc,func,ref,mask)

NATIVE_FUNCTION_HEAD(void, glStencilMask, GLuint mask ) NATIVE_FUNCTION_END_NO_RETURN(void,glStencilMask,mask)

NATIVE_FUNCTION_HEAD(void, glStencilOp, GLenum fail, GLenum zfail, GLenum zpass ) NATIVE_FUNCTION_END_NO_RETURN(void,glStencilOp,fail,zfail,zpass)

NATIVE_FUNCTION_HEAD(void, glClearStencil, GLint s ) NATIVE_FUNCTION_END_NO_RETURN(void,glClearStencil,s)



/*
* Texture mapping
*/

NATIVE_FUNCTION_HEAD(void, glTexGend, GLenum coord, GLenum pname, GLdouble param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexGend,coord,pname,param)
NATIVE_FUNCTION_HEAD(void, glTexGenf, GLenum coord, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexGenf,coord,pname,param)
NATIVE_FUNCTION_HEAD(void, glTexGeni, GLenum coord, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexGeni,coord,pname,param)

NATIVE_FUNCTION_HEAD(void, glTexGendv, GLenum coord, GLenum pname, const GLdouble *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexGendv,coord,pname,params)
NATIVE_FUNCTION_HEAD(void, glTexGenfv, GLenum coord, GLenum pname, const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexGenfv,coord,pname,params)
NATIVE_FUNCTION_HEAD(void, glTexGeniv, GLenum coord, GLenum pname, const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexGeniv,coord,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetTexGendv, GLenum coord, GLenum pname, GLdouble *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexGendv,coord,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetTexGenfv, GLenum coord, GLenum pname, GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexGenfv,coord,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetTexGeniv, GLenum coord, GLenum pname, GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexGeniv,coord,pname,params)


NATIVE_FUNCTION_HEAD(void, glTexEnvf, GLenum target, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexEnvf,target,pname,param)
NATIVE_FUNCTION_HEAD(void, glTexEnvi, GLenum target, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexEnvi,target,pname,param)

NATIVE_FUNCTION_HEAD(void, glTexEnvfv, GLenum target, GLenum pname, const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexEnvfv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glTexEnviv, GLenum target, GLenum pname, const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexEnviv,target,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetTexEnvfv, GLenum target, GLenum pname, GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexEnvfv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetTexEnviv, GLenum target, GLenum pname, GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexEnviv,target,pname,params)


//NATIVE_FUNCTION_HEAD(void, glTexParameterf, GLenum target, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexParameterf,target,pname,param)

NATIVE_FUNCTION_HEAD(void, glTexParameteri, GLenum target, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexParameteri,target,pname,param)

NATIVE_FUNCTION_HEAD(void, glTexParameterfv, GLenum target, GLenum pname,const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexParameterfv,target,pname,params)

NATIVE_FUNCTION_HEAD(void, glTexParameteriv, GLenum target, GLenum pname,const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexParameteriv,target,pname,params)

NATIVE_FUNCTION_HEAD(void, glGetTexParameterfv, GLenum target,GLenum pname, GLfloat *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexParameterfv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetTexParameteriv, GLenum target,GLenum pname, GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexParameteriv,target,pname,params)

//NATIVE_FUNCTION_HEAD(void, glGetTexLevelParameterfv, GLenum target, GLint level,GLenum pname, GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexLevelParameterfv,target,level,pname,params)

//NATIVE_FUNCTION_HEAD(void, glGetTexLevelParameteriv, GLenum target, GLint level,GLenum pname, GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexLevelParameteriv,target,level,pname,params)


//NATIVE_FUNCTION_HEAD(void, glTexImage1D, GLenum target, GLint level,GLint internalFormat,GLsizei width, GLint border,GLenum format, GLenum type,const GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexImage1D,target,level,internalFormat,width,border,format,type,pixels)

//NATIVE_FUNCTION_HEAD(void, glTexImage2D, GLenum target, GLint level,GLint internalFormat,GLsizei width, GLsizei height,GLint border, GLenum format, GLenum type,const GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexImage2D,target,level,internalFormat,width,height,border,format,type,pixels)

NATIVE_FUNCTION_HEAD(void, glGetTexImage, GLenum target, GLint level,GLenum format, GLenum type,GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexImage,target,level,format,type,pixels)


/* 1.1 functions */

NATIVE_FUNCTION_HEAD(void, glGenTextures, GLsizei n, GLuint *textures ) NATIVE_FUNCTION_END_NO_RETURN(void,glGenTextures,n,textures)

NATIVE_FUNCTION_HEAD(void, glDeleteTextures, GLsizei n, const GLuint *textures) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteTextures,n,textures)

NATIVE_FUNCTION_HEAD(void, glBindTexture, GLenum target, GLuint texture ) NATIVE_FUNCTION_END_NO_RETURN(void,glBindTexture,target,texture)

NATIVE_FUNCTION_HEAD(void, glPrioritizeTextures, GLsizei n,const GLuint *textures,const GLclampf *priorities ) NATIVE_FUNCTION_END_NO_RETURN(void,glPrioritizeTextures,n,textures,priorities)

NATIVE_FUNCTION_HEAD(GLboolean, glAreTexturesResident, GLsizei n,const GLuint *textures,GLboolean *residences ) NATIVE_FUNCTION_END(GLboolean,glAreTexturesResident,n,textures,residences)

NATIVE_FUNCTION_HEAD(GLboolean, glIsTexture, GLuint texture ) NATIVE_FUNCTION_END(GLboolean,glIsTexture,texture)


NATIVE_FUNCTION_HEAD(void, glTexSubImage1D, GLenum target, GLint level,GLint xoffset,GLsizei width, GLenum format,GLenum type, const GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexSubImage1D,target,level,xoffset,width,format,type,pixels)


NATIVE_FUNCTION_HEAD(void, glTexSubImage2D, GLenum target, GLint level,GLint xoffset, GLint yoffset,GLsizei width, GLsizei height,GLenum format, GLenum type,const GLvoid* pixels ) NATIVE_FUNCTION_END_NO_RETURN(void,glTexSubImage2D,target,level,xoffset,yoffset,width,height,format,type,pixels)


//NATIVE_FUNCTION_HEAD(void, glCopyTexImage1D, GLenum target, GLint level,GLenum internalformat,GLint x, GLint y,GLsizei width, GLint border ) NATIVE_FUNCTION_END_NO_RETURN(void,glCopyTexImage1D,target,level,internalformat,x,y,width,border)


//NATIVE_FUNCTION_HEAD(void, glCopyTexImage2D, GLenum target, GLint level,GLenum internalformat,GLint x, GLint y,GLsizei width,GLsizei height,GLint border ) NATIVE_FUNCTION_END_NO_RETURN(void,glCopyTexImage2D,target,level,internalformat,x,y,width,height,border)


NATIVE_FUNCTION_HEAD(void, glCopyTexSubImage1D, GLenum target, GLint level,GLint xoffset, GLint x, GLint y,GLsizei width ) NATIVE_FUNCTION_END_NO_RETURN(void,glCopyTexSubImage1D,target,level,xoffset,x,y,width)


NATIVE_FUNCTION_HEAD(void, glCopyTexSubImage2D, GLenum target, GLint level,GLint xoffset, GLint yoffset,GLint x, GLint y,GLsizei width, GLsizei height ) NATIVE_FUNCTION_END_NO_RETURN(void,glCopyTexSubImage2D,target,level,xoffset,yoffset,x,y,width,height)


/*
* Evaluators
*/

NATIVE_FUNCTION_HEAD(void, glMap1d, GLenum target, GLdouble u1, GLdouble u2,GLint stride,GLint order, const GLdouble *points ) NATIVE_FUNCTION_END_NO_RETURN(void,glMap1d,target,u1,u2,stride,order,points)
NATIVE_FUNCTION_HEAD(void, glMap1f, GLenum target, GLfloat u1, GLfloat u2,GLint stride,GLint order, const GLfloat *points ) NATIVE_FUNCTION_END_NO_RETURN(void,glMap1f,target,u1,u2,stride,order,points)

NATIVE_FUNCTION_HEAD(void, glMap2d, GLenum target,GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,const GLdouble *points ) NATIVE_FUNCTION_END_NO_RETURN(void,glMap2d,target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points)
NATIVE_FUNCTION_HEAD(void, glMap2f, GLenum target,GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,const GLfloat *points ) NATIVE_FUNCTION_END_NO_RETURN(void,glMap2f,target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points)

NATIVE_FUNCTION_HEAD(void, glGetMapdv, GLenum target, GLenum query, GLdouble *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetMapdv,target,query,v)
NATIVE_FUNCTION_HEAD(void, glGetMapfv, GLenum target, GLenum query, GLfloat *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetMapfv,target,query,v)
NATIVE_FUNCTION_HEAD(void, glGetMapiv, GLenum target, GLenum query, GLint *v ) NATIVE_FUNCTION_END_NO_RETURN(void,glGetMapiv,target,query,v)

NATIVE_FUNCTION_HEAD(void, glEvalCoord1d, GLdouble u ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord1d,u)
NATIVE_FUNCTION_HEAD(void, glEvalCoord1f, GLfloat u ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord1f,u)

NATIVE_FUNCTION_HEAD(void, glEvalCoord1dv, const GLdouble *u ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord1dv,u)
NATIVE_FUNCTION_HEAD(void, glEvalCoord1fv, const GLfloat *u ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord1fv,u)

NATIVE_FUNCTION_HEAD(void, glEvalCoord2d, GLdouble u, GLdouble v ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord2d,u,v)
NATIVE_FUNCTION_HEAD(void, glEvalCoord2f, GLfloat u, GLfloat v ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord2f,u,v)

NATIVE_FUNCTION_HEAD(void, glEvalCoord2dv, const GLdouble *u ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord2dv,u)
NATIVE_FUNCTION_HEAD(void, glEvalCoord2fv, const GLfloat *u ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalCoord2fv,u)

NATIVE_FUNCTION_HEAD(void, glMapGrid1d, GLint un, GLdouble u1, GLdouble u2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glMapGrid1d,un,u1,u2)
NATIVE_FUNCTION_HEAD(void, glMapGrid1f, GLint un, GLfloat u1, GLfloat u2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glMapGrid1f,un,u1,u2)

NATIVE_FUNCTION_HEAD(void, glMapGrid2d, GLint un, GLdouble u1, GLdouble u2,GLint vn, GLdouble v1, GLdouble v2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glMapGrid2d,un,u1,u2,vn,v1,v2)
NATIVE_FUNCTION_HEAD(void, glMapGrid2f, GLint un, GLfloat u1, GLfloat u2,GLint vn, GLfloat v1, GLfloat v2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glMapGrid2f,un,u1,u2,vn,v1,v2)

NATIVE_FUNCTION_HEAD(void, glEvalPoint1, GLint i ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalPoint1,i)

NATIVE_FUNCTION_HEAD(void, glEvalPoint2, GLint i, GLint j ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalPoint2,i,j)

NATIVE_FUNCTION_HEAD(void, glEvalMesh1, GLenum mode, GLint i1, GLint i2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalMesh1,mode,i1,i2)

NATIVE_FUNCTION_HEAD(void, glEvalMesh2, GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 ) NATIVE_FUNCTION_END_NO_RETURN(void,glEvalMesh2,mode,i1,i2,j1,j2)


/*
* Fog
*/

NATIVE_FUNCTION_HEAD(void, glFogf, GLenum pname, GLfloat param ) NATIVE_FUNCTION_END_NO_RETURN(void,glFogf,pname,param)

NATIVE_FUNCTION_HEAD(void, glFogi, GLenum pname, GLint param ) NATIVE_FUNCTION_END_NO_RETURN(void,glFogi,pname,param)

NATIVE_FUNCTION_HEAD(void, glFogfv, GLenum pname, const GLfloat *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glFogfv,pname,params)

NATIVE_FUNCTION_HEAD(void, glFogiv, GLenum pname, const GLint *params ) NATIVE_FUNCTION_END_NO_RETURN(void,glFogiv,pname,params)


/*
* Selection and Feedback
*/

NATIVE_FUNCTION_HEAD(void, glFeedbackBuffer, GLsizei size, GLenum type, GLfloat *buffer ) NATIVE_FUNCTION_END_NO_RETURN(void,glFeedbackBuffer,size,type,buffer)

NATIVE_FUNCTION_HEAD(void, glPassThrough, GLfloat token ) NATIVE_FUNCTION_END_NO_RETURN(void,glPassThrough,token)

NATIVE_FUNCTION_HEAD(void, glSelectBuffer, GLsizei size, GLuint *buffer ) NATIVE_FUNCTION_END_NO_RETURN(void,glSelectBuffer,size,buffer)

NATIVE_FUNCTION_HEAD(void, glInitNames) NATIVE_FUNCTION_END_NO_RETURN(void,glInitNames)

NATIVE_FUNCTION_HEAD(void, glLoadName, GLuint name ) NATIVE_FUNCTION_END_NO_RETURN(void,glLoadName,name)

NATIVE_FUNCTION_HEAD(void, glPushName, GLuint name ) NATIVE_FUNCTION_END_NO_RETURN(void,glPushName,name)

NATIVE_FUNCTION_HEAD(void, glPopName) NATIVE_FUNCTION_END_NO_RETURN(void,glPopName)

// OpenGL 3.1

NATIVE_FUNCTION_HEAD(void, glColorMaski,GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a) NATIVE_FUNCTION_END_NO_RETURN(void,glColorMaski,index,r,g,b,a)
NATIVE_FUNCTION_HEAD(void, glGetBooleani_v,GLenum target, GLuint index, GLboolean *data) NATIVE_FUNCTION_END_NO_RETURN(void,glGetBooleani_v,target,index,data)
NATIVE_FUNCTION_HEAD(void, glGetIntegeri_v,GLenum target, GLuint index, GLint *data) NATIVE_FUNCTION_END_NO_RETURN(void,glGetIntegeri_v,target,index,data)
NATIVE_FUNCTION_HEAD(void, glEnablei,GLenum target, GLuint index) NATIVE_FUNCTION_END_NO_RETURN(void,glEnablei,target,index)
NATIVE_FUNCTION_HEAD(void, glDisablei,GLenum target, GLuint index) NATIVE_FUNCTION_END_NO_RETURN(void,glDisablei,target,index)
NATIVE_FUNCTION_HEAD(GLboolean, glIsEnabledi,GLenum target, GLuint index) NATIVE_FUNCTION_END(GLboolean,glIsEnabledi,target,index)
NATIVE_FUNCTION_HEAD(void, glBeginTransformFeedback,GLenum primitiveMode) NATIVE_FUNCTION_END_NO_RETURN(void,glBeginTransformFeedback,primitiveMode)
NATIVE_FUNCTION_HEAD(void, glEndTransformFeedback,void ) NATIVE_FUNCTION_END_NO_RETURN(void,glEndTransformFeedback)
NATIVE_FUNCTION_HEAD(void, glBindBufferRange,GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size) NATIVE_FUNCTION_END_NO_RETURN(void,glBindBufferRange,target,index,buffer,offset,size)
NATIVE_FUNCTION_HEAD(void, glBindBufferBase,GLenum target, GLuint index, GLuint buffer) NATIVE_FUNCTION_END_NO_RETURN(void,glBindBufferBase,target,index,buffer)
NATIVE_FUNCTION_HEAD(void, glTransformFeedbackVaryings,GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode) NATIVE_FUNCTION_END_NO_RETURN(void,glTransformFeedbackVaryings,program,count,varyings,bufferMode)
NATIVE_FUNCTION_HEAD(void, glGetTransformFeedbackVarying,GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTransformFeedbackVarying,program,index,bufSize,length,size,type,name)
NATIVE_FUNCTION_HEAD(void, glClampColor,GLenum target, GLenum clamp) NATIVE_FUNCTION_END_NO_RETURN(void,glClampColor,target,clamp)
NATIVE_FUNCTION_HEAD(void, glBeginConditionalRender,GLuint id, GLenum mode) NATIVE_FUNCTION_END_NO_RETURN(void,glBeginConditionalRender,id,mode)
NATIVE_FUNCTION_HEAD(void, glEndConditionalRender,void ) NATIVE_FUNCTION_END_NO_RETURN(void,glEndConditionalRender)
NATIVE_FUNCTION_HEAD(void, glVertexAttribIPointer,GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribIPointer,index,size,type,stride,pointer)
NATIVE_FUNCTION_HEAD(void, glGetVertexAttribIiv,GLuint index, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetVertexAttribIiv,index,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetVertexAttribIuiv,GLuint index, GLenum pname, GLuint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetVertexAttribIuiv,index,pname,params)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI1i,GLuint index, GLint x) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI1i,index,x)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI2i,GLuint index, GLint x, GLint y) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI2i,index,x,y)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI3i,GLuint index, GLint x, GLint y, GLint z) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI3i,index,x,y,z)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4i,GLuint index, GLint x, GLint y, GLint z, GLint w) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4i,index,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI1ui,GLuint index, GLuint x) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI1ui,index,x)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI2ui,GLuint index, GLuint x, GLuint y) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI2ui,index,x,y)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI3ui,GLuint index, GLuint x, GLuint y, GLuint z) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI3ui,index,x,y,z)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4ui,GLuint index, GLuint x, GLuint y, GLuint z, GLuint w) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4ui,index,x,y,z,w)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI1iv,GLuint index, const GLint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI1iv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI2iv,GLuint index, const GLint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI2iv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI3iv,GLuint index, const GLint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI3iv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4iv,GLuint index, const GLint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4iv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI1uiv,GLuint index, const GLuint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI1uiv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI2uiv,GLuint index, const GLuint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI2uiv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI3uiv,GLuint index, const GLuint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI3uiv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4uiv,GLuint index, const GLuint *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4uiv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4bv,GLuint index, const GLbyte *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4bv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4sv,GLuint index, const GLshort *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4sv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4ubv,GLuint index, const GLubyte *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4ubv,index,v)
NATIVE_FUNCTION_HEAD(void, glVertexAttribI4usv,GLuint index, const GLushort *v) NATIVE_FUNCTION_END_NO_RETURN(void,glVertexAttribI4usv,index,v)
NATIVE_FUNCTION_HEAD(void, glGetUniformuiv,GLuint program, GLint location, GLuint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetUniformuiv,program,location,params)
NATIVE_FUNCTION_HEAD(void, glBindFragDataLocation,GLuint program, GLuint color, const GLchar *name) NATIVE_FUNCTION_END_NO_RETURN(void,glBindFragDataLocation,program,color,name)
NATIVE_FUNCTION_HEAD(GLint, glGetFragDataLocation,GLuint program, const GLchar *name) NATIVE_FUNCTION_END(GLint,glGetFragDataLocation,program,name)
NATIVE_FUNCTION_HEAD(void, glUniform1ui,GLint location, GLuint v0) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform1ui,location,v0)
NATIVE_FUNCTION_HEAD(void, glUniform2ui,GLint location, GLuint v0, GLuint v1) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform2ui,location,v0,v1)
NATIVE_FUNCTION_HEAD(void, glUniform3ui,GLint location, GLuint v0, GLuint v1, GLuint v2) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform3ui,location,v0,v1,v2)
NATIVE_FUNCTION_HEAD(void, glUniform4ui,GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform4ui,location,v0,v1,v2,v3)
NATIVE_FUNCTION_HEAD(void, glUniform1uiv,GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform1uiv,location,count,value)
NATIVE_FUNCTION_HEAD(void, glUniform2uiv,GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform2uiv,location,count,value)
NATIVE_FUNCTION_HEAD(void, glUniform3uiv,GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform3uiv,location,count,value)
NATIVE_FUNCTION_HEAD(void, glUniform4uiv,GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glUniform4uiv,location,count,value)
NATIVE_FUNCTION_HEAD(void, glTexParameterIiv,GLenum target, GLenum pname, const GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glTexParameterIiv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glTexParameterIuiv,GLenum target, GLenum pname, const GLuint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glTexParameterIuiv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetTexParameterIiv,GLenum target, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexParameterIiv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glGetTexParameterIuiv,GLenum target, GLenum pname, GLuint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetTexParameterIuiv,target,pname,params)
NATIVE_FUNCTION_HEAD(void, glClearBufferiv,GLenum buffer, GLint drawbuffer, const GLint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glClearBufferiv,buffer,drawbuffer,value)
NATIVE_FUNCTION_HEAD(void, glClearBufferuiv,GLenum buffer, GLint drawbuffer, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glClearBufferuiv,buffer,drawbuffer,value)
NATIVE_FUNCTION_HEAD(void, glClearBufferfv,GLenum buffer, GLint drawbuffer, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glClearBufferfv,buffer,drawbuffer,value)
NATIVE_FUNCTION_HEAD(void, glClearBufferfi,GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil) NATIVE_FUNCTION_END_NO_RETURN(void,glClearBufferfi,buffer,drawbuffer,depth,stencil)
NATIVE_FUNCTION_HEAD(GLboolean, glIsRenderbuffer,GLuint renderbuffer) NATIVE_FUNCTION_END(GLboolean,glIsRenderbuffer,renderbuffer)
NATIVE_FUNCTION_HEAD(void, glBindRenderbuffer,GLenum target, GLuint renderbuffer) NATIVE_FUNCTION_END_NO_RETURN(void,glBindRenderbuffer,target,renderbuffer)
NATIVE_FUNCTION_HEAD(void, glDeleteRenderbuffers,GLsizei n, const GLuint *renderbuffers) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteRenderbuffers,n,renderbuffers)
NATIVE_FUNCTION_HEAD(void, glGenRenderbuffers,GLsizei n, GLuint *renderbuffers) NATIVE_FUNCTION_END_NO_RETURN(void,glGenRenderbuffers,n,renderbuffers)
//NATIVE_FUNCTION_HEAD(void, glRenderbufferStorage,GLenum target, GLenum internalformat, GLsizei width, GLsizei height) NATIVE_FUNCTION_END_NO_RETURN(void,glRenderbufferStorage,target,internalformat,width,height)
NATIVE_FUNCTION_HEAD(void, glGetRenderbufferParameteriv,GLenum target, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetRenderbufferParameteriv,target,pname,params)
NATIVE_FUNCTION_HEAD(GLboolean, glIsFramebuffer,GLuint framebuffer) NATIVE_FUNCTION_END(GLboolean,glIsFramebuffer,framebuffer)
NATIVE_FUNCTION_HEAD(void, glBindFramebuffer,GLenum target, GLuint framebuffer) NATIVE_FUNCTION_END_NO_RETURN(void,glBindFramebuffer,target,framebuffer)
NATIVE_FUNCTION_HEAD(void, glDeleteFramebuffers,GLsizei n, const GLuint *framebuffers) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteFramebuffers,n,framebuffers)
NATIVE_FUNCTION_HEAD(void, glGenFramebuffers,GLsizei n, GLuint *framebuffers) NATIVE_FUNCTION_END_NO_RETURN(void,glGenFramebuffers,n,framebuffers)
//NATIVE_FUNCTION_HEAD(GLenum, glCheckFramebufferStatus,GLenum target) NATIVE_FUNCTION_END(GLenum,glCheckFramebufferStatus,target)
GLAPI GLAPIENTRY GLenum glCheckFramebufferStatus(GLenum target) {
	LOG();
	LOAD_GLES(glCheckFramebufferStatus, GLenum, GLenum target);
	GLenum result = gles_glCheckFramebufferStatus(target);
	LOG_D("Result: %d", result);
	return result;
}

NATIVE_FUNCTION_HEAD(void, glFramebufferTexture1D,GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) NATIVE_FUNCTION_END_NO_RETURN(void,glFramebufferTexture1D,target,attachment,textarget,texture,level)
//NATIVE_FUNCTION_HEAD(void, glFramebufferTexture2D,GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) NATIVE_FUNCTION_END_NO_RETURN(void,glFramebufferTexture2D,target,attachment,textarget,texture,level)
GLAPI GLAPIENTRY void glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
	LOG();
	LOG_D("glFramebufferTexture2D, target: %d, attachment: %d, textarget: %d, texture: %d, level: %d",target,attachment,textarget,texture,level);
	LOAD_GLES(glFramebufferTexture2D, void, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	gles_glFramebufferTexture2D(target,attachment,textarget,texture,level);
	LOAD_GLES(glGetError, GLenum)            
    GLenum ERR = gles_glGetError();          
    if (ERR != GL_NO_ERROR)                  
        LOG_E("ERROR: %d", ERR)              
}
NATIVE_FUNCTION_HEAD(void, glFramebufferTexture3D,GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset) NATIVE_FUNCTION_END_NO_RETURN(void,glFramebufferTexture3D,target,attachment,textarget,texture,level,zoffset)
NATIVE_FUNCTION_HEAD(void, glFramebufferRenderbuffer,GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) NATIVE_FUNCTION_END_NO_RETURN(void,glFramebufferRenderbuffer,target,attachment,renderbuffertarget,renderbuffer)
NATIVE_FUNCTION_HEAD(void, glGetFramebufferAttachmentParameteriv,GLenum target, GLenum attachment, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetFramebufferAttachmentParameteriv,target,attachment,pname,params)
NATIVE_FUNCTION_HEAD(void, glGenerateMipmap,GLenum target) NATIVE_FUNCTION_END_NO_RETURN(void,glGenerateMipmap,target)
NATIVE_FUNCTION_HEAD(void, glBlitFramebuffer,GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) NATIVE_FUNCTION_END_NO_RETURN(void,glBlitFramebuffer,srcX0,srcY0,srcX1,srcY1,dstX0,dstY0,dstX1,dstY1,mask,filter)
//NATIVE_FUNCTION_HEAD(void, glRenderbufferStorageMultisample,GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) NATIVE_FUNCTION_END_NO_RETURN(void,glRenderbufferStorageMultisample,target,samples,internalformat,width,height)
NATIVE_FUNCTION_HEAD(void, glFramebufferTextureLayer,GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) NATIVE_FUNCTION_END_NO_RETURN(void,glFramebufferTextureLayer,target,attachment,texture,level,layer)
NATIVE_FUNCTION_HEAD(void*, glMapBufferRange,GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access) NATIVE_FUNCTION_END(void*,glMapBufferRange,target,offset, length,access)
NATIVE_FUNCTION_HEAD(void, glFlushMappedBufferRange,GLenum target, GLintptr offset, GLsizeiptr length) NATIVE_FUNCTION_END_NO_RETURN(void,glFlushMappedBufferRange,target,offset, length)
NATIVE_FUNCTION_HEAD(void, glBindVertexArray,GLuint array) NATIVE_FUNCTION_END_NO_RETURN(void,glBindVertexArray,array)
NATIVE_FUNCTION_HEAD(void, glDeleteVertexArrays,GLsizei n, const GLuint *arrays) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteVertexArrays,n,arrays)
NATIVE_FUNCTION_HEAD(void, glGenVertexArrays,GLsizei n, GLuint *arrays) NATIVE_FUNCTION_END_NO_RETURN(void,glGenVertexArrays,n,arrays)
NATIVE_FUNCTION_HEAD(GLboolean, glIsVertexArray,GLuint array) NATIVE_FUNCTION_END(GLboolean,glIsVertexArray,array)

// Shader
NATIVE_FUNCTION_HEAD(void,glAttachShader,GLuint program, GLuint shader) NATIVE_FUNCTION_END_NO_RETURN(void,glAttachShader,program,shader);
NATIVE_FUNCTION_HEAD(void,glCompileShader,GLuint shader) NATIVE_FUNCTION_END_NO_RETURN(void,glCompileShader,shader);
NATIVE_FUNCTION_HEAD(GLuint,glCreateShader,GLenum type) NATIVE_FUNCTION_END(GLuint,glCreateShader,type);
NATIVE_FUNCTION_HEAD(void,glDeleteShader,GLuint shader) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteShader,shader);
NATIVE_FUNCTION_HEAD(void,glDetachShader,GLuint program, GLuint shader) NATIVE_FUNCTION_END_NO_RETURN(void,glDetachShader,program,shader);
NATIVE_FUNCTION_HEAD(void,glGetAttachedShaders,GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders) NATIVE_FUNCTION_END_NO_RETURN(void,glGetAttachedShaders,program,maxCount,count,shaders);
NATIVE_FUNCTION_HEAD(void,glGetShaderiv,GLuint shader, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetShaderiv,shader,pname,params);
NATIVE_FUNCTION_HEAD(void,glGetShaderInfoLog,GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog) NATIVE_FUNCTION_END_NO_RETURN(void,glGetShaderInfoLog,shader,bufSize,length,infoLog);
NATIVE_FUNCTION_HEAD(void,glGetShaderPrecisionFormat,GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision) NATIVE_FUNCTION_END_NO_RETURN(void,glGetShaderPrecisionFormat,shadertype,precisiontype,range,precision);
NATIVE_FUNCTION_HEAD(void,glGetShaderSource,GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source) NATIVE_FUNCTION_END_NO_RETURN(void,glGetShaderSource,shader,bufSize,length,source);
NATIVE_FUNCTION_HEAD(GLboolean,glIsShader,GLuint shader) NATIVE_FUNCTION_END(GLboolean,glIsShader,shader);
NATIVE_FUNCTION_HEAD(void,glReleaseShaderCompiler,void) NATIVE_FUNCTION_END_NO_RETURN(void,glReleaseShaderCompiler,);
NATIVE_FUNCTION_HEAD(void,glShaderBinary,GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length) NATIVE_FUNCTION_END_NO_RETURN(void,glShaderBinary,count,shaders,binaryformat,binary,length);
NATIVE_FUNCTION_HEAD(void,glActiveShaderProgram,GLuint pipeline, GLuint program) NATIVE_FUNCTION_END_NO_RETURN(void,glActiveShaderProgram,pipeline,program);
NATIVE_FUNCTION_HEAD(GLuint,glCreateShaderProgramv,GLenum type, GLsizei count, const GLchar *const*strings) NATIVE_FUNCTION_END(GLuint,glCreateShaderProgramv,type,count,strings);

// Program
NATIVE_FUNCTION_HEAD(GLuint,glCreateProgram,void) NATIVE_FUNCTION_END(GLuint,glCreateProgram,);
NATIVE_FUNCTION_HEAD(void,glDeleteProgram,GLuint program) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteProgram,program);
NATIVE_FUNCTION_HEAD(void,glGetProgramiv,GLuint program, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramiv,program,pname,params);
NATIVE_FUNCTION_HEAD(void,glGetProgramInfoLog,GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramInfoLog,program,bufSize,length,infoLog);
NATIVE_FUNCTION_HEAD(GLboolean,glIsProgram,GLuint program) NATIVE_FUNCTION_END(GLboolean,glIsProgram,program);
NATIVE_FUNCTION_HEAD(void,glLinkProgram,GLuint program) NATIVE_FUNCTION_END_NO_RETURN(void,glLinkProgram,program);
NATIVE_FUNCTION_HEAD(void,glUseProgram,GLuint program) NATIVE_FUNCTION_END_NO_RETURN(void,glUseProgram,program);
NATIVE_FUNCTION_HEAD(void,glValidateProgram,GLuint program) NATIVE_FUNCTION_END_NO_RETURN(void,glValidateProgram,program);
NATIVE_FUNCTION_HEAD(void,glGetProgramBinary,GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramBinary,program,bufSize,length,binaryFormat,binary);
NATIVE_FUNCTION_HEAD(void,glProgramBinary,GLuint program, GLenum binaryFormat, const void *binary, GLsizei length) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramBinary,program,binaryFormat,binary,length);
NATIVE_FUNCTION_HEAD(void,glProgramParameteri,GLuint program, GLenum pname, GLint value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramParameteri,program,pname,value);
NATIVE_FUNCTION_HEAD(void,glGetProgramInterfaceiv,GLuint program, GLenum programInterface, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramInterfaceiv,program,programInterface,pname,params);
NATIVE_FUNCTION_HEAD(GLuint,glGetProgramResourceIndex,GLuint program, GLenum programInterface, const GLchar *name) NATIVE_FUNCTION_END(GLuint,glGetProgramResourceIndex,program,programInterface,name);
NATIVE_FUNCTION_HEAD(void,glGetProgramResourceName,GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramResourceName,program,programInterface,index,bufSize,length,name);
NATIVE_FUNCTION_HEAD(void,glGetProgramResourceiv,GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramResourceiv,program,programInterface,index,propCount,props,bufSize,length,params);
NATIVE_FUNCTION_HEAD(GLint,glGetProgramResourceLocation,GLuint program, GLenum programInterface, const GLchar *name) NATIVE_FUNCTION_END(GLint,glGetProgramResourceLocation,program,programInterface,name);
NATIVE_FUNCTION_HEAD(void,glUseProgramStages,GLuint pipeline, GLbitfield stages, GLuint program) NATIVE_FUNCTION_END_NO_RETURN(void,glUseProgramStages,pipeline,stages,program);
NATIVE_FUNCTION_HEAD(void,glBindProgramPipeline,GLuint pipeline) NATIVE_FUNCTION_END_NO_RETURN(void,glBindProgramPipeline,pipeline);
NATIVE_FUNCTION_HEAD(void,glDeleteProgramPipelines,GLsizei n, const GLuint *pipelines) NATIVE_FUNCTION_END_NO_RETURN(void,glDeleteProgramPipelines,n,pipelines);
NATIVE_FUNCTION_HEAD(void,glGenProgramPipelines,GLsizei n, GLuint *pipelines) NATIVE_FUNCTION_END_NO_RETURN(void,glGenProgramPipelines,n,pipelines);
NATIVE_FUNCTION_HEAD(GLboolean,glIsProgramPipeline,GLuint pipeline) NATIVE_FUNCTION_END(GLboolean,glIsProgramPipeline,pipeline);
NATIVE_FUNCTION_HEAD(void,glGetProgramPipelineiv,GLuint pipeline, GLenum pname, GLint *params) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramPipelineiv,pipeline,pname,params);
NATIVE_FUNCTION_HEAD(void,glProgramUniform1i,GLuint program, GLint location, GLint v0) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform1i,program,location,v0);
NATIVE_FUNCTION_HEAD(void,glProgramUniform2i,GLuint program, GLint location, GLint v0, GLint v1) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform2i,program,location,v0,v1);
NATIVE_FUNCTION_HEAD(void,glProgramUniform3i,GLuint program, GLint location, GLint v0, GLint v1, GLint v2) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform3i,program,location,v0,v1,v2);
NATIVE_FUNCTION_HEAD(void,glProgramUniform4i,GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform4i,program,location,v0,v1,v2,v3);
NATIVE_FUNCTION_HEAD(void,glProgramUniform1ui,GLuint program, GLint location, GLuint v0) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform1ui,program,location,v0);
NATIVE_FUNCTION_HEAD(void,glProgramUniform2ui,GLuint program, GLint location, GLuint v0, GLuint v1) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform2ui,program,location,v0,v1);
NATIVE_FUNCTION_HEAD(void,glProgramUniform3ui,GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform3ui,program,location,v0,v1,v2);
NATIVE_FUNCTION_HEAD(void,glProgramUniform4ui,GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform4ui,program,location,v0,v1,v2,v3);
NATIVE_FUNCTION_HEAD(void,glProgramUniform1f,GLuint program, GLint location, GLfloat v0) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform1f,program,location,v0);
NATIVE_FUNCTION_HEAD(void,glProgramUniform2f,GLuint program, GLint location, GLfloat v0, GLfloat v1) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform2f,program,location,v0,v1);
NATIVE_FUNCTION_HEAD(void,glProgramUniform3f,GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform3f,program,location,v0,v1,v2);
NATIVE_FUNCTION_HEAD(void,glProgramUniform4f,GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform4f,program,location,v0,v1,v2,v3);
NATIVE_FUNCTION_HEAD(void,glProgramUniform1iv,GLuint program, GLint location, GLsizei count, const GLint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform1iv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform2iv,GLuint program, GLint location, GLsizei count, const GLint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform2iv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform3iv,GLuint program, GLint location, GLsizei count, const GLint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform3iv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform4iv,GLuint program, GLint location, GLsizei count, const GLint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform4iv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform1uiv,GLuint program, GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform1uiv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform2uiv,GLuint program, GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform2uiv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform3uiv,GLuint program, GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform3uiv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform4uiv,GLuint program, GLint location, GLsizei count, const GLuint *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform4uiv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform1fv,GLuint program, GLint location, GLsizei count, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform1fv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform2fv,GLuint program, GLint location, GLsizei count, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform2fv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform3fv,GLuint program, GLint location, GLsizei count, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform3fv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniform4fv,GLuint program, GLint location, GLsizei count, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniform4fv,program,location,count,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix2fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix2fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix3fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix3fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix4fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix4fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix2x3fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix2x3fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix3x2fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix3x2fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix2x4fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix2x4fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix4x2fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix4x2fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix3x4fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix3x4fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glProgramUniformMatrix4x3fv,GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value) NATIVE_FUNCTION_END_NO_RETURN(void,glProgramUniformMatrix4x3fv,program,location,count,transpose,value);
NATIVE_FUNCTION_HEAD(void,glValidateProgramPipeline,GLuint pipeline) NATIVE_FUNCTION_END_NO_RETURN(void,glValidateProgramPipeline,pipeline);
NATIVE_FUNCTION_HEAD(void,glGetProgramPipelineInfoLog,GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog) NATIVE_FUNCTION_END_NO_RETURN(void,glGetProgramPipelineInfoLog,pipeline,bufSize,length,infoLog);
