/*!
\file Surface.cpp
\brief 
*/

#include "include.h"
#include "Surface.h"
using namespace Surface;
#ifdef HAS_SDL_OPENGL
#include <SDL/SDL_syswm.h>
#endif
#ifdef HAS_GLX
#include <GL/glx.h>
#endif

#ifdef HAS_GLX
Display* CSurface::s_dpy = 0;
#endif

#ifdef HAS_SDL
CSurface::CSurface(int width, int height, bool doublebuffer, CSurface* shared,
		   CSurface* window, SDL_Surface* parent, bool fullscreen,
		   bool pixmap, bool pbuffer) 
{
  CLog::Log(LOGDEBUG, "Constructing surface");
  m_bOK = false;
  m_iWidth = width;
  m_iHeight = height;
  m_bDoublebuffer = doublebuffer;
  m_iRedSize = 8;
  m_iGreenSize = 8;
  m_iBlueSize = 8;
  m_iAlphaSize = 8;
  m_bFullscreen = fullscreen;  
  m_pShared = shared;

#ifdef HAS_GLX
  m_glWindow = 0;
  m_glContext = 0;
  m_glPBuffer = 0;
  m_glPixmap = 0;

  GLXFBConfig *fbConfigs=0;
  bool mapWindow = false;
  int num = 0;

  int singleVisAttributes[] = 
      {
	  GLX_RENDER_TYPE, GLX_RGBA_BIT,
	  GLX_RED_SIZE, m_iRedSize,
	  GLX_GREEN_SIZE, m_iGreenSize,
	  GLX_BLUE_SIZE, m_iBlueSize,
	  GLX_ALPHA_SIZE, m_iAlphaSize,
	  GLX_DEPTH_SIZE, 8,
	  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	  None
      };

  int doubleVisAttributes[] = 
      {
	  GLX_RENDER_TYPE, GLX_RGBA_BIT,
	  GLX_RED_SIZE, m_iRedSize,
	  GLX_GREEN_SIZE, m_iGreenSize,
	  GLX_BLUE_SIZE, m_iBlueSize,
	  GLX_ALPHA_SIZE, m_iAlphaSize,
	  GLX_DEPTH_SIZE, 8,
	  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	  GLX_DOUBLEBUFFER, True,
	  None
      };
  
  if (window) 
  {
    m_glWindow = window->GetWindow();
    CLog::Log(LOGINFO, "GLX Info: Using destination window");
  } else {
    m_glWindow = 0;
    CLog::Log(LOGINFO, "GLX Info: NOT Using destination window");
  }

  if (!s_dpy) 
  {
    // try to open root display
    s_dpy = XOpenDisplay(0);
    if (!s_dpy) 
    {
      CLog::Log(LOGERROR, "GLX Error: No Display found");
      return;
    }
  }

  if (pbuffer) 
  {
    MakePBuffer();
    return;
  }

  if (pixmap)
  {
    MakePixmap();
    return;
  }
  
  if (doublebuffer) 
  {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
  } else {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);
  }
  if (fbConfigs==NULL) 
  {
    CLog::Log(LOGERROR, "GLX Error: No compatible framebuffers found");
    return;
  }

  // if no window is specified, create a window
  if (!m_glWindow) 
  {
    XVisualInfo *vInfo;
    XSetWindowAttributes  swa;
    int swaMask;
    Window p;

    vInfo = glXGetVisualFromFBConfig(s_dpy, fbConfigs[0]);

    m_SDLSurface = parent;

    // check if a parent was passed
    if (parent) 
    {
      SDL_SysWMinfo info;

      SDL_VERSION(&info.version);
      SDL_GetWMInfo(&info);
      p = info.info.x11.window;
      CLog::Log(LOGINFO, "GLX Info: Using parent window");
    } else  {
      p = RootWindow(s_dpy, vInfo->screen);
      swaMask = CWBorderPixel;
    }
    swa.border_pixel = 0;
    swa.event_mask = StructureNotifyMask;
    swa.colormap = XCreateColormap(s_dpy, p, vInfo->visual, AllocNone);
    swaMask |= (CWColormap | CWEventMask);
    m_glWindow = XCreateWindow(s_dpy, p, 0, 0, m_iWidth, m_iHeight,
			       0, vInfo->depth, InputOutput, vInfo->visual,
			       swaMask, &swa );
    mapWindow = true;
    if (!m_glWindow) 
    {
      XFree(fbConfigs);
      CLog::Log(LOGERROR, "GLX Error: Could not create window");
      return;
    }
  }

  // still no window? then we got a problem  
  if (shared) 
  {
    CLog::Log(LOGINFO, "GLX Info: Creating shared context");
    m_glContext = glXCreateNewContext(s_dpy, fbConfigs[0], GLX_RGBA_TYPE, shared->GetContext(), True);
  } else {
    CLog::Log(LOGINFO, "GLX Info: Creating unshared context");
    m_glContext = glXCreateNewContext(s_dpy, fbConfigs[0], GLX_RGBA_TYPE, NULL, True);
  }
  XFree(fbConfigs);
  if (mapWindow) 
  {
    XEvent event;
    XMapWindow(s_dpy, m_glWindow);
    XIfEvent(s_dpy, &event, WaitForNotify, (XPointer)m_glWindow);
  }
  if (m_glContext) 
  {
    glXMakeContextCurrent(s_dpy, m_glWindow, m_glWindow, m_glContext);
    glewInit();
    m_bOK = true;
  } else {
    CLog::Log(LOGERROR, "GLX Error: Could not create context");
  }
#elif defined(HAS_SDL_OPENGL)
  int options = SDL_OPENGL | (fullscreen?SDL_FULLSCREEN:0);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   m_iRedSize);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, m_iGreenSize);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  m_iBlueSize);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, m_iAlphaSize);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, m_bDoublebuffer?1:0);
  m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 0, options);
  if (m_SDLSurface) {
    m_bOK = true;
  }
#else
  int options = SDL_HWSURFACE | SDL_DOUBLEBUF;
  if (fullscreen) 
  {
    options |= SDL_FULLSCREEN;
  }
  m_SDLSurface = SDL_SetVideoMode(m_iWidth, m_iHeight, 32, options);
  if (m_SDLSurface) 
  {
    m_bOK = true;
  }
#endif
}
#endif

#ifdef HAS_GLX
bool CSurface::MakePBuffer()
{
  int num=0;
  GLXFBConfig *fbConfigs=0;
  bool status = false;
  int singleVisAttributes[] = 
      {
	  GLX_RENDER_TYPE, GLX_RGBA_BIT,
	  GLX_RED_SIZE, m_iRedSize,
	  GLX_GREEN_SIZE, m_iGreenSize,
	  GLX_BLUE_SIZE, m_iBlueSize,
	  GLX_ALPHA_SIZE, m_iAlphaSize,
	  GLX_DEPTH_SIZE, 8,
	  GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
	  None
      };

  int doubleVisAttributes[] = 
      {
	  GLX_RENDER_TYPE, GLX_RGBA_BIT,
	  GLX_RED_SIZE, m_iRedSize,
	  GLX_GREEN_SIZE, m_iGreenSize,
	  GLX_BLUE_SIZE, m_iBlueSize,
	  GLX_ALPHA_SIZE, m_iAlphaSize,
	  GLX_DEPTH_SIZE, 8,
	  GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
	  GLX_DOUBLEBUFFER, True,
	  None
      };

  int pbufferAttributes[] = {GLX_PBUFFER_WIDTH, m_iWidth, GLX_PBUFFER_HEIGHT, m_iHeight, GLX_LARGEST_PBUFFER};

  if (m_bDoublebuffer) 
  {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), doubleVisAttributes, &num);
  } else {
    fbConfigs = glXChooseFBConfig(s_dpy, DefaultScreen(s_dpy), singleVisAttributes, &num);
  }
  if (fbConfigs==NULL) 
  {
    CLog::Log(LOGERROR, "MakePBuffer: GLX Error: No compatible framebuffers found");
    XFree(fbConfigs);
    return status;
  }
  m_glPBuffer = glXCreatePbuffer(s_dpy, fbConfigs[0], pbufferAttributes);

  if (m_glPBuffer)
  {
    if (m_pShared) 
    {
      CLog::Log(LOGINFO, "GLX Info: Creating shared context");
      m_glContext = glXCreateNewContext(s_dpy, fbConfigs[0], GLX_RGBA_TYPE, m_pShared->GetContext(), True);
    } else {
      CLog::Log(LOGINFO, "GLX Info: Creating unshared context");
      m_glContext = glXCreateNewContext(s_dpy, fbConfigs[0], GLX_RGBA_TYPE, NULL, True);
    }
    if (glXMakeContextCurrent(s_dpy, m_glPBuffer, m_glPBuffer, m_glContext))
    {
      m_bOK = status = true;      
    } else {
      CLog::Log(LOGINFO, "GLX Error: Could not make PBuffer current");
      status = false;
    }
  } else {
    CLog::Log(LOGINFO, "GLX Error: Could not create PBuffer");
    status = false;
  }
  XFree(fbConfigs);
  return status;
}

bool CSurface::MakePixmap()
{
// FIXME: this needs to be implemented
  return false;
}
#endif

CSurface::~CSurface() 
{
#ifdef HAS_GLX
  if (m_glContext) {
    glXDestroyContext(s_dpy, m_glContext);
  }
  if (m_glPBuffer) {
    glXDestroyPbuffer(s_dpy, m_glPBuffer);
  }
  if (m_glWindow) {
    glXDestroyWindow(s_dpy, m_glWindow);
  }
#else
  if (IsValid() && m_SDLSurface) {
    SDL_FreeSurface(m_SDLSurface);
  }
#endif
}

void CSurface::Flip() 
{
  if (m_bOK && m_bDoublebuffer) 
  {
#ifdef HAS_GLX
    glXSwapBuffers(s_dpy, m_glWindow);
#elif HAS_SDL_OPENGL
    SDL_GL_SwapBuffers();
#else
    SDL_Flip(m_SDLSurface);
#endif
  } else {
    OutputDebugString("Surface Error: Could not flip surface.");
  }
}

bool CSurface::MakeCurrent()
{
#ifdef HAS_GLX
  if (m_glWindow)
    return (bool)glXMakeContextCurrent(s_dpy, m_glWindow, m_glWindow, m_glContext);
  if (m_glPBuffer)
    return (bool)glXMakeContextCurrent(s_dpy, m_glPBuffer, m_glPBuffer, m_glContext);
#endif
}

void CSurface::ReleaseContext()
{
#ifdef HAS_GLX
  if (m_glWindow)
    glXMakeContextCurrent(s_dpy, None, None, NULL);
  if (m_glPBuffer)
    glXMakeContextCurrent(s_dpy, None, None, NULL);
#endif
}
