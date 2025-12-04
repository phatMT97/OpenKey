#include "sciter-x.h"
#include "sciter-x-behavior.h"
#include "sciter-x-graphics.hpp"

#include "gles/sciter-gles.h"

namespace sciter
{
/*
SAMPLE:
   See: samples/behaviors/native-drawing.htm
*/


 enum {
   INIT_CONTEXT = FIRST_APPLICATION_EVENT_CODE,
   ANIMATE_CONTEXT = INIT_CONTEXT + 1,
   RENDER_CONTEXT = EGL_RENDER,
 };


struct opengl_drawing_demo: public event_handler
{
    HELEMENT self = nullptr;
    GLuint   programObject = 0;

    GLfloat vVertices[9] = { 0.0f,  0.5f, 0.0f,
                           -0.5f, -0.5f, 0.0f,
                           0.5f, -0.5f,  0.0f };

    GLfloat vDelta[9];

    // ctor
    opengl_drawing_demo() {
      for (int n = 0; n < 9; ++n) {
        vDelta[n] = float(rand()) / float(RAND_MAX) * 0.01f;
      }
    }
    virtual ~opengl_drawing_demo() {}

    virtual bool subscription( HELEMENT he, UINT& event_groups ) override
    {
      event_groups = HANDLE_ALL;   // it does drawing
      return true;
    }

    virtual void attached  (HELEMENT he ) override
    {
      self = he;
#if 0
      // call init_context in context of eglMakeCurrent()
      SciterEGLSendEvent(he, INIT_CONTEXT, 0);
      // request ANIMATE_CONTEXT to be sent on animation frame
      SciterRequestAnimationFrameEvent(he, ANIMATE_CONTEXT, 0);
#endif
    }
    virtual void detached  (HELEMENT he ) override {
      self = 0;
      asset_release(); 
    }

    void on_size(HELEMENT he) override {
      if (!programObject) {
        SciterEGLSendEvent(he, INIT_CONTEXT, 0);
        // request ANIMATE_CONTEXT to be sent on animation frame
        SciterRequestAnimationFrameEvent(he, ANIMATE_CONTEXT, 0);
      }


      // call render_context in context of eglMakeCurrent()
      SciterEGLSendEvent(he, RENDER_CONTEXT, 0);
    }

    void on_step(HELEMENT he) {
      // update vVertices locations
      for (int n = 0; n < 9; ++n) {
        vVertices[n] += vDelta[n];
        if (vVertices[n] > 1.0f) {
          vVertices[n] = 1.0f;
          vDelta[n] = -vDelta[n];
        }
        else if (vVertices[n] < -1.0f) {
          vVertices[n] = -1.0f;
          vDelta[n] = -vDelta[n];
        }
      }
      // request render_context() call
      SciterEGLSendEvent(he, RENDER_CONTEXT, 0);
    }

    bool on_event(HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason) override {
      switch (type) {
        case INIT_CONTEXT: init_context(he); return true;
        case RENDER_CONTEXT: render_context(he); return true;
        case ANIMATE_CONTEXT: on_step(he); return true; 
      }
      return false;
    }

    //
    // Initialize the shader and program object
    // 
    // NOTE: called after eglMakeCurrent(EGL context of this WebGL)
    // 
    bool init_context(HELEMENT he)
    {

      char vShaderStr[] =
        "attribute vec4 vPosition;   \n"
        "void main()                 \n"
        "{                           \n"
        "  gl_Position = vPosition;  \n"
        "}                           \n";

      char fShaderStr[] =
        "precision mediump float;                   \n"
        "void main()                                \n"
        "{                                          \n"
        "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0); \n"
        "}                                          \n";

      GLuint vertexShader;
      GLuint fragmentShader;
      
      GLint linked;

      // Load the vertex/fragment shaders
      vertexShader = load_shader(vShaderStr, GL_VERTEX_SHADER);
      fragmentShader = load_shader(fShaderStr, GL_FRAGMENT_SHADER);

      // Create the program object
      programObject = glCreateProgram();

      if (programObject == 0)
        return 0;

      glAttachShader(programObject, vertexShader);
      glAttachShader(programObject, fragmentShader);

      // Bind vPosition to attribute 0
      glBindAttribLocation(programObject, 0, "vPosition");

      // Link the program
      glLinkProgram(programObject);

      // Check the link status
      glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

      if (!linked)
      {
        GLint infoLen = 0;

        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
          char* infoLog = (char*)malloc(sizeof(char) * infoLen);

          glGetProgramInfoLog(programObject, infoLen, NULL, infoLog);
          printf("Error linking program:\n%s\n", infoLog);

          free(infoLog);
        }

        glDeleteProgram(programObject);
        return FALSE;
      }

      return true;
    }

    ///
    // Create a shader object, load the shader source, and
    // compile the shader.
    //
    GLuint load_shader(const char* shaderSrc, GLenum type)
    {
      GLuint shader;
      GLint compiled;

      // Create the shader object
      shader = glCreateShader(type);

      if (shader == 0)
        return 0;

      // Load the shader source
      glShaderSource(shader, 1, &shaderSrc, NULL);

      // Compile the shader
      glCompileShader(shader);

      // Check the compile status
      glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

      if (!compiled)
      {
        GLint infoLen = 0;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);

        if (infoLen > 1)
        {
          char* infoLog = (char*)malloc(sizeof(char) * infoLen);

          glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
          printf("Error compiling shader:\n%s\n", infoLog);
          free(infoLog);
        }

        glDeleteShader(shader);
        return 0;
      }
      return shader;
    }

    ///
    // Draw a triangle using the shader pair created in init_context()
    //
    // NOTE: called after eglMakeCurrent(EGL context of this WebGL),
    //       eglSwitchBuffers() is called immediately after the render_context
    //

    void render_context(HELEMENT he)
    {
      // Set the viewport
      RECT rc = dom::element(he).get_location_ppx();
      glViewport(0, 0, rc.right - rc.left,rc.bottom - rc.top);

      glClearColor(0.0f, 0.0f, 0.8f, 1.0f);
      
      int r = glGetError(); assert(r == GL_NO_ERROR);            

      glClearDepthf(1.0f); // Clear everything
      glEnable(GL_DEPTH_TEST); // Enable depth testing
      r = glGetError(); assert(r == GL_NO_ERROR);      
      glDepthFunc(GL_LEQUAL); // Near things obscure far things
      r = glGetError(); assert(r == GL_NO_ERROR);      

      // Clear the canvas before we start drawing on it.

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Load the vertex data
      // Load the vertex data
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
      glEnableVertexAttribArray(0);

      // Use the program object
      glUseProgram(programObject);
      glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // script API, N/A here

    /*SOM_PASSPORT_BEGIN_EX(openglDrawing, opengl_drawing_demo)
      //SOM_FUNCS()
      SOM_PROPS(
        SOM_PROP(theta),
        SOM_PROP(color),
      )
    SOM_PASSPORT_END*/


};

struct opengl_drawing_factory: public behavior_factory {

  opengl_drawing_factory(): behavior_factory("opengl-drawing") { }

  // the only behavior_factory method:
  virtual event_handler* create(HELEMENT he) {
    return new opengl_drawing_demo();
  }

};

// instantiating and attaching it to the global list
opengl_drawing_factory opengl_drawing_factory_instance;


}
