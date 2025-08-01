#include "FSR1.h"
#include "FSRShaderSource.h"
#include "../../config/settings.h"

#define DEBUG 0

struct GLStateGuard {
    GLint prevProgram;
    GLint prevVAO;
    GLint prevArrayBuffer;
    GLint prevActiveTexture;
    GLint prevTexture;
    //GLint prevViewport[4];
    GLint prevReadFBO;
    GLint prevDrawFBO;
    GLint prevRenderbuffer;

    GLStateGuard() {
        GLES.glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);
        GLES.glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &prevVAO);
        GLES.glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &prevArrayBuffer);
        GLES.glGetIntegerv(GL_ACTIVE_TEXTURE, &prevActiveTexture);
        GLES.glActiveTexture(prevActiveTexture);
        GLES.glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);
        //GLES.glGetIntegerv(GL_VIEWPORT, prevViewport);
        GLES.glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevReadFBO);
        GLES.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &prevDrawFBO);
        GLES.glGetIntegerv(GL_RENDERBUFFER_BINDING, &prevRenderbuffer);
    }

    ~GLStateGuard() {
        GLES.glUseProgram(prevProgram);
        GLES.glBindVertexArray(prevVAO);
        GLES.glBindBuffer(GL_ARRAY_BUFFER, prevArrayBuffer);
        GLES.glActiveTexture(prevActiveTexture);
        GLES.glBindTexture(GL_TEXTURE_2D, prevTexture);
        //GLES.glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
        GLES.glBindRenderbuffer(GL_RENDERBUFFER, prevRenderbuffer);
        GLES.glBindFramebuffer(GL_READ_FRAMEBUFFER, prevReadFBO);
        GLES.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, prevDrawFBO);
    }
};

namespace FSR1_Context {
	GLuint g_renderFBO = 0;
	GLuint g_renderTexture = 0;
	GLuint g_depthStencilRBO = 0;
	GLuint g_quadVAO = 0;
	GLuint g_quadVBO = 0;
    GLuint g_fsrProgram = 0;
    
    GLuint g_targetFBO = 0;
    GLuint g_targetTexture = 0;

	GLuint g_currentDrawFBO = 0;
	GLint g_viewport[4] = { 0 };
	GLsizei g_targetWidth = 2400;
	GLsizei g_targetHeight = 1080;
	GLsizei g_renderWidth = 1200;
	GLsizei g_renderHeight = 540;
	bool g_dirty = false;

	bool g_resolutionChanged = false;
	GLsizei g_pendingWidth = 0;
	GLsizei g_pendingHeight = 0;
}

void CalculateTargetResolution(FSR1_Quality_Preset preset,
    int renderWidth, int renderHeight,
    int* targetWidth, int* targetHeight)
{
    float scale;
    switch (preset) {
    case FSR1_Quality_Preset::UltraQuality: scale = 1.3f; break;
    case FSR1_Quality_Preset::Quality:       scale = 1.5f; break;
    case FSR1_Quality_Preset::Balanced:      scale = 1.7f; break;
    case FSR1_Quality_Preset::Performance:   scale = 2.0f; break;
    default:                        scale = 1.5f; break;
    }

    *targetWidth = static_cast<int>(renderWidth * scale);
    *targetHeight = static_cast<int>(renderHeight * scale);

    *targetWidth = (*targetWidth + 1) & ~1;
    *targetHeight = (*targetHeight + 1) & ~1;
    LOG_D("Render resolution: %dx%d", renderWidth, renderHeight);
	LOG_D("Target resolution: %dx%d", *targetWidth, *targetHeight);
}

void CalculateRenderResolution(FSR1_Quality_Preset preset,
    int targetWidth, int targetHeight,
    int* renderWidth, int* renderHeight)
{
    float scale;
    switch (preset) {
    case FSR1_Quality_Preset::UltraQuality:
        scale = 1.3f;
        break;
    case FSR1_Quality_Preset::Quality:
        scale = 1.5f;
        break;
    case FSR1_Quality_Preset::Balanced:
        scale = 1.7f;
        break;
    case FSR1_Quality_Preset::Performance:
        scale = 2.0f;
        break;
    default:
        scale = 1.5f;
    }

    *renderWidth = (int)(targetWidth / scale);
    *renderHeight = (int)(targetHeight / scale);

    *renderWidth = (*renderWidth + 1) & ~1;
    *renderHeight = (*renderHeight + 1) & ~1;
}

GLuint CompileFSRShader() {
    GLuint program = glCreateProgram();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	LOG_D("Vertex shader source:\n%s", FSR_VSSource);
    glShaderSource(vs, 1, &FSR_VSSource, nullptr);
    glCompileShader(vs);

    GLint status;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(vs, 512, nullptr, log);
        LOG_F("Vertex shader error: %s\n", log);
        return 0;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	LOG_D("Fragment shader source:\n%s", FSR_FSSource);
    glShaderSource(fs, 1, &FSR_FSSource, nullptr);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(fs, 512, nullptr, log);
        LOG_F("Fragment shader error: %s\n", log);
        return 0;
    }

    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        LOG_F("Program link error: %s\n", log);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

void InitFullscreenQuad() {
    GLStateGuard state;
    const float quadVertices[] = {
        -1.0f,  1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f 
    };

    GLES.glGenVertexArrays(1, &FSR1_Context::g_quadVAO);
    GLES.glGenBuffers(1, &FSR1_Context::g_quadVBO);

    GLES.glBindVertexArray(FSR1_Context::g_quadVAO);
    GLES.glBindBuffer(GL_ARRAY_BUFFER, FSR1_Context::g_quadVBO);

    GLES.glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    GLES.glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    GLES.glEnableVertexAttribArray(0);

    GLES.glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    GLES.glEnableVertexAttribArray(1);

    GLES.glBindBuffer(GL_ARRAY_BUFFER, 0);
    GLES.glBindVertexArray(0);
}

bool fsrInitialized = false;
void InitFSRResources() {
	fsrInitialized = true;
    GLStateGuard state;

    FSR1_Context::g_fsrProgram = CompileFSRShader();

    GLint inputTexLoc = glGetUniformLocation(FSR1_Context::g_fsrProgram, "uInputTex");
    GLint const0Loc = glGetUniformLocation(FSR1_Context::g_fsrProgram, "uConst0");
    GLint viewportSizeLoc = glGetUniformLocation(FSR1_Context::g_fsrProgram, "uViewportSize");

    glUseProgram(FSR1_Context::g_fsrProgram);
    glUniform1i(inputTexLoc, 0);
    glUseProgram(0);

    InitFullscreenQuad();

    GLES.glGenTextures(1, &FSR1_Context::g_renderTexture);
    GLES.glBindTexture(GL_TEXTURE_2D, FSR1_Context::g_renderTexture);
    GLES.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    
    GLES.glGenRenderbuffers(1, &FSR1_Context::g_depthStencilRBO);
    GLES.glBindRenderbuffer(GL_RENDERBUFFER, FSR1_Context::g_depthStencilRBO);
    GLES.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
		FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight);
    
    GLES.glGenFramebuffers(1, &FSR1_Context::g_renderFBO);
    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_renderFBO);
    GLES.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, FSR1_Context::g_renderTexture, 0);
    GLES.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, FSR1_Context::g_depthStencilRBO);

    GLES.glGenTextures(1, &FSR1_Context::g_targetTexture);
    GLES.glBindTexture(GL_TEXTURE_2D, FSR1_Context::g_targetTexture);
    GLES.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FSR1_Context::g_targetWidth, FSR1_Context::g_targetHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    GLES.glGenFramebuffers(1, &FSR1_Context::g_targetFBO);
    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_targetFBO);
    GLES.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, FSR1_Context::g_targetTexture, 0);
    
    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_renderFBO);
}

void RecreateFSRFBO() {
    GLStateGuard state;
    GLES.glDeleteFramebuffers(1, &FSR1_Context::g_renderFBO);
    GLES.glDeleteTextures(1, &FSR1_Context::g_renderTexture);
    GLES.glDeleteRenderbuffers(1, &FSR1_Context::g_depthStencilRBO);
    
    GLES.glDeleteFramebuffers(1, &FSR1_Context::g_targetFBO);
    GLES.glDeleteTextures(1, &FSR1_Context::g_targetTexture);
    
    GLES.glGenTextures(1, &FSR1_Context::g_renderTexture);
    GLES.glBindTexture(GL_TEXTURE_2D, FSR1_Context::g_renderTexture);
    GLES.glTexImage2D(
        GL_TEXTURE_2D, 0,
        GL_RGBA32F,
        FSR1_Context::g_renderWidth,
        FSR1_Context::g_renderHeight,
        0,
        GL_RGBA,
        GL_FLOAT,
        nullptr
    );
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    GLES.glGenRenderbuffers(1, &FSR1_Context::g_depthStencilRBO);
    GLES.glBindRenderbuffer(GL_RENDERBUFFER, FSR1_Context::g_depthStencilRBO);
	GLES.glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
        FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight);
    GLES.glGenFramebuffers(1, &FSR1_Context::g_renderFBO);
    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_renderFBO);
    GLES.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, FSR1_Context::g_renderTexture, 0);
    GLES.glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER, FSR1_Context::g_depthStencilRBO);
        
    GLES.glGenTextures(1, &FSR1_Context::g_targetTexture);
    GLES.glBindTexture(GL_TEXTURE_2D, FSR1_Context::g_targetTexture);
    GLES.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FSR1_Context::g_targetWidth, FSR1_Context::g_targetHeight,
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLES.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    GLES.glGenFramebuffers(1, &FSR1_Context::g_targetFBO);
    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_targetFBO);
    GLES.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, FSR1_Context::g_targetTexture, 0);

	GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_renderFBO);
    GLES.glViewport(0, 0, FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight);

	LOG_D("FSR1 resources recreated: render %dx%d, target %dx%d", 
        FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight,
        FSR1_Context::g_targetWidth, FSR1_Context::g_targetHeight);
}

std::vector<std::pair<GLsizei, GLsizei>> g_viewportStack;

void ApplyFSR() {
    GLStateGuard state;
    
    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_targetFBO);
    GLES.glViewport(0, 0, FSR1_Context::g_targetWidth, FSR1_Context::g_targetHeight);
    GLES.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    GLES.glClear(GL_COLOR_BUFFER_BIT);

    GLES.glUseProgram(FSR1_Context::g_fsrProgram);

    GLES.glActiveTexture(GL_TEXTURE0);
    GLES.glBindTexture(GL_TEXTURE_2D, FSR1_Context::g_renderTexture);

    glm::vec4 const0 = {
        float(FSR1_Context::g_renderWidth) / FSR1_Context::g_targetWidth,
        float(FSR1_Context::g_renderHeight) / FSR1_Context::g_targetHeight,
        1.0f / FSR1_Context::g_targetWidth,
        1.0f / FSR1_Context::g_targetHeight
    };

    GLES.glUniform1i(glGetUniformLocation(FSR1_Context::g_fsrProgram, "uInputTex"), 0);
    GLES.glUniform4fv(glGetUniformLocation(FSR1_Context::g_fsrProgram, "uConst0"), 1, reinterpret_cast<const GLfloat*>(&const0));

    glm::vec2 viewportSize = { (float)FSR1_Context::g_renderWidth, (float)FSR1_Context::g_renderHeight };
    GLES.glUniform2fv(glGetUniformLocation(FSR1_Context::g_fsrProgram, "uViewportSize"), 1, reinterpret_cast<const GLfloat*>(&viewportSize));

    GLES.glBindVertexArray(FSR1_Context::g_quadVAO);
    GLES.glDrawArrays(GL_TRIANGLES, 0, 6);
    GLES.glBindVertexArray(0);
    
    GLES.glBindFramebuffer(GL_READ_FRAMEBUFFER, FSR1_Context::g_targetFBO);
    GLES.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    GLES.glBlitFramebuffer(
        0, 0, FSR1_Context::g_targetWidth, FSR1_Context::g_targetHeight,
        0, 0, FSR1_Context::g_targetWidth, FSR1_Context::g_targetHeight,
        GL_COLOR_BUFFER_BIT, GL_LINEAR
    );

    GLES.glBindFramebuffer(GL_FRAMEBUFFER, FSR1_Context::g_renderFBO);
    GLES.glViewport(0, 0, FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight);
}

void CheckResolutionChange() {
	GLsizei width = 0, height = 0;
    LOAD_EGL(eglQuerySurface);
	static EGLDisplay display;
	static EGLSurface surface;
    if (!display || !surface) {
        display = eglGetCurrentDisplay();
        surface = eglGetCurrentSurface(EGL_DRAW);
	}
	egl_eglQuerySurface(display, surface, EGL_WIDTH, &width);
	egl_eglQuerySurface(display, surface, EGL_HEIGHT, &height);
	OnResize(width, height);
    
    if (FSR1_Context::g_resolutionChanged) {
        FSR1_Context::g_resolutionChanged = false;
		GLsizei width = FSR1_Context::g_pendingWidth;
		GLsizei height = FSR1_Context::g_pendingHeight;
		FSR1_Context::g_renderWidth = width;
		FSR1_Context::g_renderHeight = height;

        CalculateTargetResolution(global_settings.fsr1_setting,
            width, height,
            reinterpret_cast<int*>(&FSR1_Context::g_targetWidth),
            reinterpret_cast<int*>(&FSR1_Context::g_targetHeight));
        RecreateFSRFBO();
    }
    GLES.glViewport(0, 0, FSR1_Context::g_renderWidth, FSR1_Context::g_renderHeight);
}

void OnResize(int width, int height) {
    if (FSR1_Context::g_renderWidth == width && FSR1_Context::g_renderHeight == height)
        return;

    FSR1_Context::g_pendingWidth = width;
    FSR1_Context::g_pendingHeight = height;
    FSR1_Context::g_resolutionChanged = true;
}

void glViewport(GLint x, GLint y, GLsizei w, GLsizei h) {
    LOG()
	LOG_D("glViewport: x=%d, y=%d, w=%d, h=%d", x, y, w, h);
    
    if (w > FSR1_Context::g_pendingWidth || h > FSR1_Context::g_pendingHeight) {
        FSR1_Context::g_pendingWidth = w;
        FSR1_Context::g_pendingHeight = h;
        FSR1_Context::g_resolutionChanged = true;
    }
    
	GLES.glViewport(x, y, w, h);
}