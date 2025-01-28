//
// Created by Swung 0x48 on 2024/10/7.
//

#include <string.h>
#include "includes.h"
#include "gl/gl.h"
#include "egl/egl.h"
#include "egl/loader.h"
#include "gles/loader.h"
#include "gl/envvars.h"

#ifdef __cplusplus
extern "C" {
#endif

struct egl_func_t g_egl_func;

__eglMustCastToProperFunctionPointerType prehook(const char *procname);
__eglMustCastToProperFunctionPointerType posthook(const char *procname);

void proc_init() {
    LOG_V("Initializing %s @ %s", RENDERERNAME, __FUNCTION__);
    init_target_gles();
    init_target_egl();

    g_initialized = 1;
}

#define MAP_FUNC(name) if (strcmp(procname, #name) == 0)  \
    return (__eglMustCastToProperFunctionPointerType) name;

#define MAP_FUNC_MGLUES(name) if (strcmp(procname, #name) == 0)  \
    return (__eglMustCastToProperFunctionPointerType) mglues_##name;


__eglMustCastToProperFunctionPointerType prehook(const char *procname) {
//    if (!g_initialized)
//        proc_init();
    if (!strncmp(procname, "egl", 3)) {
        MAP_FUNC_MGLUES(eglCreateContext);
        MAP_FUNC_MGLUES(eglDestroyContext);
        MAP_FUNC_MGLUES(eglMakeCurrent);
//        if (strcmp(procname, "eglCreateContext") == 0)
//            return (__eglMustCastToProperFunctionPointerType) g_egl_func.eglCreateContext;
//        if (strcmp(procname, "eglDestroyContext") == 0)
//            return (__eglMustCastToProperFunctionPointerType) g_egl_func.eglDestroyContext;
//        if (strcmp(procname, "eglMakeCurrent") == 0)
//            return (__eglMustCastToProperFunctionPointerType) g_egl_func.eglMakeCurrent;
    }

    // OpenGL 1.1
    MAP_FUNC(glAccum);
    MAP_FUNC(glAreTexturesResident);
    MAP_FUNC(glAreTexturesResident);
    MAP_FUNC(glArrayElement);
    MAP_FUNC(glBegin);
    MAP_FUNC(glBitmap);
    MAP_FUNC(glCallList);
    MAP_FUNC(glCallLists);
    MAP_FUNC(glClearAccum);
    MAP_FUNC(glClearIndex);
    MAP_FUNC(glClipPlane);
    MAP_FUNC(glColor3b);
    MAP_FUNC(glColor3s);
    MAP_FUNC(glColor3i);
    MAP_FUNC(glColor3f);
    MAP_FUNC(glColor3d);
    MAP_FUNC(glColor3ub);
    MAP_FUNC(glColor3us);
    MAP_FUNC(glColor3ui);
    MAP_FUNC(glColor3bv);
    MAP_FUNC(glColor3sv);
    MAP_FUNC(glColor3iv);
    MAP_FUNC(glColor3fv);
    MAP_FUNC(glColor3dv);
    MAP_FUNC(glColor3ubv);
    MAP_FUNC(glColor3usv);
    MAP_FUNC(glColor3uiv);
    MAP_FUNC(glColor4b);
    MAP_FUNC(glColor4s);
    MAP_FUNC(glColor4i);
    MAP_FUNC(glColor4d);
    MAP_FUNC(glColor4us);
    MAP_FUNC(glColor4ui);
    MAP_FUNC(glColor4bv);
    MAP_FUNC(glColor4sv);
    MAP_FUNC(glColor4iv);
    MAP_FUNC(glColor4fv);
    MAP_FUNC(glColor4dv);
    MAP_FUNC(glColor4ubv);
    MAP_FUNC(glColor4usv);
    MAP_FUNC(glColor4uiv);
    MAP_FUNC(glColorMaterial);
    MAP_FUNC(glCopyPixels);
    MAP_FUNC(glDeleteLists);
    MAP_FUNC(glDrawPixels);
    MAP_FUNC(glEdgeFlag);
    MAP_FUNC(glEdgeFlagv);
    MAP_FUNC(glEdgeFlagPointer);
    MAP_FUNC(glEnd);
    MAP_FUNC(glEvalCoord1f);
    MAP_FUNC(glEvalCoord1fv);
    MAP_FUNC(glEvalCoord1d);
    MAP_FUNC(glEvalCoord1dv);
    MAP_FUNC(glEvalCoord2f);
    MAP_FUNC(glEvalCoord2fv);
    MAP_FUNC(glEvalCoord2d);
    MAP_FUNC(glEvalCoord2dv);
    MAP_FUNC(glEvalMesh1);
    MAP_FUNC(glEvalMesh2);
    MAP_FUNC(glEvalPoint1);
    MAP_FUNC(glEvalPoint2);
    MAP_FUNC(glFeedbackBuffer);
    MAP_FUNC(glFogi);
    MAP_FUNC(glFogiv);
    MAP_FUNC(glGenLists);
    MAP_FUNC(glGetClipPlane);
    MAP_FUNC(glGetLightiv);
    MAP_FUNC(glGetMapiv);
    MAP_FUNC(glGetMapfv);
    MAP_FUNC(glGetMapdv);
    MAP_FUNC(glGetMaterialiv);
    MAP_FUNC(glGetPixelMapfv);
    MAP_FUNC(glGetPixelMapusv);
    MAP_FUNC(glGetPixelMapuiv);
    MAP_FUNC(glGetPolygonStipple);
    MAP_FUNC(glGetTexGeniv);
    MAP_FUNC(glGetTexGenfv);
    MAP_FUNC(glGetTexGendv);
    MAP_FUNC(glIndexi);
    MAP_FUNC(glIndexub);
    MAP_FUNC(glIndexs);
    MAP_FUNC(glIndexf);
    MAP_FUNC(glIndexd);
    MAP_FUNC(glIndexiv);
    MAP_FUNC(glIndexubv);
    MAP_FUNC(glIndexsv);
    MAP_FUNC(glIndexfv);
    MAP_FUNC(glIndexdv);
    MAP_FUNC(glIndexMask);
    MAP_FUNC(glIndexPointer);
    MAP_FUNC(glInitNames);
    MAP_FUNC(glInterleavedArrays);
    MAP_FUNC(glIsList);
    MAP_FUNC(glLightModeli);
    MAP_FUNC(glLightModeliv);
    MAP_FUNC(glLighti);
    MAP_FUNC(glLightiv);
    MAP_FUNC(glLineStipple);
    MAP_FUNC(glListBase);
    MAP_FUNC(glLoadMatrixd);
    MAP_FUNC(glLoadName);
    MAP_FUNC(glMap1f);
    MAP_FUNC(glMap1d);
    MAP_FUNC(glMap2f);
    MAP_FUNC(glMap2d);
    MAP_FUNC(glMapGrid1f);
    MAP_FUNC(glMapGrid1d);
    MAP_FUNC(glMapGrid2f);
    MAP_FUNC(glMapGrid2d);
    MAP_FUNC(glMateriali);
    MAP_FUNC(glMaterialiv);
    MAP_FUNC(glMultMatrixd);
    MAP_FUNC(glFrustum);
    MAP_FUNC(glNewList);
    MAP_FUNC(glEndList);
    MAP_FUNC(glNormal3b);
    MAP_FUNC(glNormal3s);
    MAP_FUNC(glNormal3i);
    MAP_FUNC(glNormal3d);
    MAP_FUNC(glNormal3fv);
    MAP_FUNC(glNormal3bv);
    MAP_FUNC(glNormal3sv);
    MAP_FUNC(glNormal3iv);
    MAP_FUNC(glNormal3dv);
    MAP_FUNC(glOrtho);
    MAP_FUNC(glPassThrough);
    MAP_FUNC(glPixelMapfv);
    MAP_FUNC(glPixelMapusv);
    MAP_FUNC(glPixelMapuiv);
    MAP_FUNC(glPixelTransferi);
    MAP_FUNC(glPixelTransferf);
    MAP_FUNC(glPixelZoom);
    MAP_FUNC(glPolygonStipple);
    MAP_FUNC(glPushAttrib);
    MAP_FUNC(glPushClientAttrib);
    MAP_FUNC(glPopAttrib);
    MAP_FUNC(glPopClientAttrib);
    MAP_FUNC(glPopName);
    MAP_FUNC(glPrioritizeTextures);
    MAP_FUNC(glPushName);
    MAP_FUNC(glRasterPos2i);
    MAP_FUNC(glRasterPos2s);
    MAP_FUNC(glRasterPos2f);
    MAP_FUNC(glRasterPos2d);
    MAP_FUNC(glRasterPos2iv);
    MAP_FUNC(glRasterPos2sv);
    MAP_FUNC(glRasterPos2fv);
    MAP_FUNC(glRasterPos2dv);
    MAP_FUNC(glRasterPos3i);
    MAP_FUNC(glRasterPos3s);
    MAP_FUNC(glRasterPos3f);
    MAP_FUNC(glRasterPos3d);
    MAP_FUNC(glRasterPos3iv);
    MAP_FUNC(glRasterPos3sv);
    MAP_FUNC(glRasterPos3fv);
    MAP_FUNC(glRasterPos3dv);
    MAP_FUNC(glRasterPos4i);
    MAP_FUNC(glRasterPos4s);
    MAP_FUNC(glRasterPos4f);
    MAP_FUNC(glRasterPos4d);
    MAP_FUNC(glRasterPos4iv);
    MAP_FUNC(glRasterPos4sv);
    MAP_FUNC(glRasterPos4fv);
    MAP_FUNC(glRasterPos4dv);
    MAP_FUNC(glRecti);
    MAP_FUNC(glRects);
    MAP_FUNC(glRectf);
    MAP_FUNC(glRectd);
    MAP_FUNC(glRectiv);
    MAP_FUNC(glRectsv);
    MAP_FUNC(glRectfv);
    MAP_FUNC(glRectdv);
    MAP_FUNC(glRenderMode);
    MAP_FUNC(glRotated);
    MAP_FUNC(glScaled);
    MAP_FUNC(glSelectBuffer);
    MAP_FUNC(glTexCoord1f);
    MAP_FUNC(glTexCoord1s);
    MAP_FUNC(glTexCoord1i);
    MAP_FUNC(glTexCoord1d);
    MAP_FUNC(glTexCoord1fv);
    MAP_FUNC(glTexCoord1sv);
    MAP_FUNC(glTexCoord1iv);
    MAP_FUNC(glTexCoord1dv);
    MAP_FUNC(glTexCoord2f);
    MAP_FUNC(glTexCoord2s);
    MAP_FUNC(glTexCoord2i);
    MAP_FUNC(glTexCoord2d);
    MAP_FUNC(glTexCoord2fv);
    MAP_FUNC(glTexCoord2sv);
    MAP_FUNC(glTexCoord2iv);
    MAP_FUNC(glTexCoord2dv);
    MAP_FUNC(glTexCoord3f);
    MAP_FUNC(glTexCoord3s);
    MAP_FUNC(glTexCoord3i);
    MAP_FUNC(glTexCoord3d);
    MAP_FUNC(glTexCoord3fv);
    MAP_FUNC(glTexCoord3sv);
    MAP_FUNC(glTexCoord3iv);
    MAP_FUNC(glTexCoord3dv);
    MAP_FUNC(glTexCoord4f);
    MAP_FUNC(glTexCoord4s);
    MAP_FUNC(glTexCoord4i);
    MAP_FUNC(glTexCoord4d);
    MAP_FUNC(glTexCoord4fv);
    MAP_FUNC(glTexCoord4sv);
    MAP_FUNC(glTexCoord4iv);
    MAP_FUNC(glTexCoord4dv);
    MAP_FUNC(glTexGeni);
    MAP_FUNC(glTexGeniv);
    MAP_FUNC(glTexGenf);
    MAP_FUNC(glTexGenfv);
    MAP_FUNC(glTexGend);
    MAP_FUNC(glTexGendv);
    MAP_FUNC(glTranslated);
    MAP_FUNC(glVertex2f);
    MAP_FUNC(glVertex2s);
    MAP_FUNC(glVertex2i);
    MAP_FUNC(glVertex2d);
    MAP_FUNC(glVertex2fv);
    MAP_FUNC(glVertex2sv);
    MAP_FUNC(glVertex2iv);
    MAP_FUNC(glVertex2dv);
    MAP_FUNC(glVertex3f);
    MAP_FUNC(glVertex3s);
    MAP_FUNC(glVertex3i);
    MAP_FUNC(glVertex3d);
    MAP_FUNC(glVertex3fv);
    MAP_FUNC(glVertex3sv);
    MAP_FUNC(glVertex3iv);
    MAP_FUNC(glVertex3dv);
    MAP_FUNC(glVertex4f);
    MAP_FUNC(glVertex4s);
    MAP_FUNC(glVertex4i);
    MAP_FUNC(glVertex4d);
    MAP_FUNC(glVertex4fv);
    MAP_FUNC(glVertex4sv);
    MAP_FUNC(glVertex4iv);
    MAP_FUNC(glVertex4dv);
    MAP_FUNC(glClearDepth);
    MAP_FUNC(glDepthRange);
    MAP_FUNC(glDrawBuffer);
    MAP_FUNC(glGetDoublev);
    MAP_FUNC(glGetTexImage);
    MAP_FUNC(glPixelStoref);
    MAP_FUNC(glPolygonMode);
    MAP_FUNC(glTexImage1D);
    MAP_FUNC(glCopyTexImage1D);
    MAP_FUNC(glCopyTexSubImage1D);
    MAP_FUNC(glTexSubImage1D);
    MAP_FUNC(glGetError);
    MAP_FUNC(glGetString);
//    UNIMPL_PH(glMultiTexCoord1f);
//    UNIMPL_PH(glMultiTexCoord1s);
//    UNIMPL_PH(glMultiTexCoord1i);
//    UNIMPL_PH(glMultiTexCoord1d);
//    UNIMPL_PH(glMultiTexCoord1fv);
//    UNIMPL_PH(glMultiTexCoord1sv);
//    UNIMPL_PH(glMultiTexCoord1iv);
//    UNIMPL_PH(glMultiTexCoord1dv);
//    UNIMPL_PH(glMultiTexCoord2f);
//    UNIMPL_PH(glMultiTexCoord2s);
//    UNIMPL_PH(glMultiTexCoord2i);
//    UNIMPL_PH(glMultiTexCoord2d);
//    UNIMPL_PH(glMultiTexCoord2fv);
//    UNIMPL_PH(glMultiTexCoord2sv);
//    UNIMPL_PH(glMultiTexCoord2iv);
//    UNIMPL_PH(glMultiTexCoord2dv);
//    UNIMPL_PH(glMultiTexCoord3f);
//    UNIMPL_PH(glMultiTexCoord3s);
//    UNIMPL_PH(glMultiTexCoord3i);
//    UNIMPL_PH(glMultiTexCoord3d);
//    UNIMPL_PH(glMultiTexCoord3fv);
//    UNIMPL_PH(glMultiTexCoord3sv);
//    UNIMPL_PH(glMultiTexCoord3iv);
//    UNIMPL_PH(glMultiTexCoord3dv);
//    UNIMPL_PH(glMultiTexCoord4s);
//    UNIMPL_PH(glMultiTexCoord4i);
//    UNIMPL_PH(glMultiTexCoord4d);
//    UNIMPL_PH(glMultiTexCoord4fv);
//    UNIMPL_PH(glMultiTexCoord4sv);
//    UNIMPL_PH(glMultiTexCoord4iv);
//    UNIMPL_PH(glMultiTexCoord4dv);
//    UNIMPL_PH(glLoadTransposeMatrixf);
//    UNIMPL_PH(glLoadTransposeMatrixd);
//    UNIMPL_PH(glMultTransposeMatrixf);
//    UNIMPL_PH(glMultTransposeMatrixd);
//    UNIMPL_PH(glCompressedTexImage1D);
//    UNIMPL_PH(glCompressedTexSubImage1D);
//    UNIMPL_PH(glGetCompressedTexImage);
//    UNIMPL_PH(glFogCoordd);
//    UNIMPL_PH(glFogCoordfv);
//    UNIMPL_PH(glFogCoorddv);
//    UNIMPL_PH(glFogCoordPointer);
//    UNIMPL_PH(glSecondaryColor3b);
//    UNIMPL_PH(glSecondaryColor3s);
//    UNIMPL_PH(glSecondaryColor3i);
//    UNIMPL_PH(glSecondaryColor3f);
//    UNIMPL_PH(glSecondaryColor3d);
//    UNIMPL_PH(glSecondaryColor3ub);
//    UNIMPL_PH(glSecondaryColor3us);
//    UNIMPL_PH(glSecondaryColor3ui);
//    UNIMPL_PH(glSecondaryColor3bv);
//    UNIMPL_PH(glSecondaryColor3sv);
//    UNIMPL_PH(glSecondaryColor3iv);
//    UNIMPL_PH(glSecondaryColor3fv);
//    UNIMPL_PH(glSecondaryColor3dv);
//    UNIMPL_PH(glSecondaryColor3ubv);
//    UNIMPL_PH(glSecondaryColor3usv);
//    UNIMPL_PH(glSecondaryColor3uiv);
//    UNIMPL_PH(glSecondaryColorPointer);
//    UNIMPL_PH(glWindowPos2i);
//    UNIMPL_PH(glWindowPos2s);
//    UNIMPL_PH(glWindowPos2f);
//    UNIMPL_PH(glWindowPos2d);
//    UNIMPL_PH(glWindowPos2iv);
//    UNIMPL_PH(glWindowPos2sv);
//    UNIMPL_PH(glWindowPos2fv);
//    UNIMPL_PH(glWindowPos2dv);
//    UNIMPL_PH(glWindowPos3i);
//    UNIMPL_PH(glWindowPos3s);
//    UNIMPL_PH(glWindowPos3f);
//    UNIMPL_PH(glWindowPos3d);
//    UNIMPL_PH(glWindowPos3iv);
//    UNIMPL_PH(glWindowPos3sv);
//    UNIMPL_PH(glWindowPos3fv);
//    UNIMPL_PH(glWindowPos3dv);
//    UNIMPL_PH(glMultiDrawArrays);
//    UNIMPL_PH(glMultiDrawElements);
//    UNIMPL_PH(glPointParameteri);
//    UNIMPL_PH(glPointParameteriv);
//    UNIMPL_PH(glGetBufferSubData);
//    UNIMPL_PH(glMapBuffer);
//    UNIMPL_PH(glGetQueryObjectiv);
//    UNIMPL_PH(glVertexAttrib1s);
//    UNIMPL_PH(glVertexAttrib1d);
//    UNIMPL_PH(glVertexAttrib2s);
//    UNIMPL_PH(glVertexAttrib2d);
//    UNIMPL_PH(glVertexAttrib3s);
//    UNIMPL_PH(glVertexAttrib3d);
//    UNIMPL_PH(glVertexAttrib4s);
//    UNIMPL_PH(glVertexAttrib4d);
//    UNIMPL_PH(glVertexAttrib4Nub);
//    UNIMPL_PH(glVertexAttrib1sv);
//    UNIMPL_PH(glVertexAttrib1dv);
//    UNIMPL_PH(glVertexAttrib2sv);
//    UNIMPL_PH(glVertexAttrib2dv);
//    UNIMPL_PH(glVertexAttrib3sv);
//    UNIMPL_PH(glVertexAttrib3dv);
//    UNIMPL_PH(glVertexAttrib4sv);
//    UNIMPL_PH(glVertexAttrib4dv);
//    UNIMPL_PH(glVertexAttrib4iv);
//    UNIMPL_PH(glVertexAttrib4bv);
//    UNIMPL_PH(glVertexAttrib4ubv);
//    UNIMPL_PH(glVertexAttrib4usv);
//    UNIMPL_PH(glVertexAttrib4uiv);
//    UNIMPL_PH(glVertexAttrib4Nbv);
//    UNIMPL_PH(glVertexAttrib4Nsv);
//    UNIMPL_PH(glVertexAttrib4Niv);
//    UNIMPL_PH(glVertexAttrib4Nubv);
//    UNIMPL_PH(glVertexAttrib4Nusv);
//    UNIMPL_PH(glVertexAttrib4Nuiv);
//    UNIMPL_PH(glGetVertexAttribdv);
//    UNIMPL_PH(glVertexAttribI1i);
//    UNIMPL_PH(glVertexAttribI2i);
//    UNIMPL_PH(glVertexAttribI3i);
//    UNIMPL_PH(glVertexAttribI1ui);
//    UNIMPL_PH(glVertexAttribI2ui);
//    UNIMPL_PH(glVertexAttribI3ui);
//    UNIMPL_PH(glVertexAttribI1iv);
//    UNIMPL_PH(glVertexAttribI2iv);
//    UNIMPL_PH(glVertexAttribI3iv);
//    UNIMPL_PH(glVertexAttribI1uiv);
//    UNIMPL_PH(glVertexAttribI2uiv);
//    UNIMPL_PH(glVertexAttribI3uiv);
//    UNIMPL_PH(glVertexAttribI4bv);
//    UNIMPL_PH(glVertexAttribI4sv);
//    UNIMPL_PH(glVertexAttribI4ubv);
//    UNIMPL_PH(glVertexAttribI4usv);
//    UNIMPL_PH(glBindFragDataLocation);
//    UNIMPL_PH(glBeginConditionalRender);
//    UNIMPL_PH(glEndConditionalRender);
//    UNIMPL_PH(glClampColor);
//    UNIMPL_PH(glFramebufferTexture1D);
//    UNIMPL_PH(glFramebufferTexture3D);
//    UNIMPL_PH(glPrimitiveRestartIndex);
//    UNIMPL_PH(glGetActiveUniformName);
//    UNIMPL_PH(glMultiDrawElementsBaseVertex);
//    UNIMPL_PH(glProvokingVertex);
//    UNIMPL_PH(glTexImage2DMultisample);
//    UNIMPL_PH(glTexImage3DMultisample);
//    UNIMPL_PH(glVertexP2ui);
//    UNIMPL_PH(glVertexP3ui);
//    UNIMPL_PH(glVertexP4ui);
//    UNIMPL_PH(glVertexP2uiv);
//    UNIMPL_PH(glVertexP3uiv);
//    UNIMPL_PH(glVertexP4uiv);
//    UNIMPL_PH(glTexCoordP1ui);
//    UNIMPL_PH(glTexCoordP2ui);
//    UNIMPL_PH(glTexCoordP3ui);
//    UNIMPL_PH(glTexCoordP4ui);
//    UNIMPL_PH(glTexCoordP1uiv);
//    UNIMPL_PH(glTexCoordP2uiv);
//    UNIMPL_PH(glTexCoordP3uiv);
//    UNIMPL_PH(glTexCoordP4uiv);
//    UNIMPL_PH(glMultiTexCoordP1ui);
//    UNIMPL_PH(glMultiTexCoordP2ui);
//    UNIMPL_PH(glMultiTexCoordP3ui);
//    UNIMPL_PH(glMultiTexCoordP4ui);
//    UNIMPL_PH(glMultiTexCoordP1uiv);
//    UNIMPL_PH(glMultiTexCoordP2uiv);
//    UNIMPL_PH(glMultiTexCoordP3uiv);
//    UNIMPL_PH(glMultiTexCoordP4uiv);
//    UNIMPL_PH(glNormalP3ui);
//    UNIMPL_PH(glNormalP3uiv);
//    UNIMPL_PH(glColorP3ui);
//    UNIMPL_PH(glColorP4ui);
//    UNIMPL_PH(glColorP3uiv);
//    UNIMPL_PH(glColorP4uiv);
//    UNIMPL_PH(glSecondaryColorP3ui);
//    UNIMPL_PH(glSecondaryColorP3uiv);
//    UNIMPL_PH(glBindFragDataLocationIndexed);
//    UNIMPL_PH(glGetFragDataIndex);
//    UNIMPL_PH(glQueryCounter);
//    UNIMPL_PH(glGetQueryObjecti64v);
//    UNIMPL_PH(glGetQueryObjectui64v);
//    UNIMPL_PH(glVertexAttribP1ui);
//    UNIMPL_PH(glVertexAttribP2ui);
//    UNIMPL_PH(glVertexAttribP3ui);
//    UNIMPL_PH(glVertexAttribP4ui);
//    UNIMPL_PH(glVertexAttribP1uiv);
//    UNIMPL_PH(glVertexAttribP2uiv);
//    UNIMPL_PH(glVertexAttribP3uiv);
//    UNIMPL_PH(glVertexAttribP4uiv);
//    UNIMPL_PH(glTexStorageMem1DEXT);

    return NULL;
}

__eglMustCastToProperFunctionPointerType posthook(const char *procname) {
    return NULL;
}

EGLAPI __eglMustCastToProperFunctionPointerType EGLAPIENTRY eglGetProcAddress (const char *procname) {
    __android_log_print(ANDROID_LOG_VERBOSE, RENDERERNAME,
                        "%s @ %s(%s)", RENDERERNAME, __FUNCTION__, procname);

    __eglMustCastToProperFunctionPointerType proc = NULL;

    proc = prehook(procname);
    if (proc) return proc;
//    else
//        __android_log_print(ANDROID_LOG_VERBOSE, RENDERERNAME,
//                            "null func addr: %s(%s) @ prehook", __FUNCTION__, procname);

//    proc = g_egl_func.eglGetProcAddress(procname);
    if (proc) return proc;
//    else
//        __android_log_print(ANDROID_LOG_VERBOSE, RENDERERNAME,
//                            "null func addr: %s(%s) @ system", __FUNCTION__, procname);

    proc = posthook(procname);
//    if (!proc)
//        __android_log_print(ANDROID_LOG_VERBOSE, RENDERERNAME,
//                            "null func addr: %s(%s) @ posthook", __FUNCTION__, procname);
    return proc;
}

//static const unsigned char* vendor = "GL_VENDOR";
//static const unsigned char* renderer = "GL_RENDERER";
//static const unsigned char* defaultstring = "defaultstring";
//
//GL_APICALL const GLubyte *GL_APIENTRY glGetString (GLenum name) {
//    __android_log_print(ANDROID_LOG_VERBOSE, RENDERERNAME,
//                        "%s(0x%x, ...)", __FUNCTION__, name);
//    switch (name) {
//        case GL_VENDOR: return vendor;
//        case GL_RENDERER:
//            return renderer;
//        default:
//            return defaultstring;
//    }
//    return defaultstring;
//}

//GL_APICALL void glGetIntegerv( GLenum pname, GLint *params ) {
//    __android_log_print(ANDROID_LOG_VERBOSE, RENDERERNAME,
//                        "%s(0x%x, ...)", __FUNCTION__, pname);
//
//}

#ifdef __cplusplus
}
#endif
