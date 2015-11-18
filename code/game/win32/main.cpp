/*
 *  main.cpp
 *
 *  Bork3D Game Engine
 *  Copyright (c) 2009 Bork 3D LLC. All rights reserved.
 *
 *	This file is based on code from the Nehe OpenGL example
 *	nehe.gamedev.net
 */


#include "Rude.h"

#include "RBGame.h"
#include "RudeDebug.h"
#include "RudeText.h"
#include "RudeFont.h"
#include "RudeTweaker.h"
#include "RudeUnitTest.h"

#include "nvapi.h"
#include "opengl_3dv.h"


GLD3DBuffers gl_d3d_buffers = { 0 };

RBGame *gVBGame = 0;

HDC			hDC = NULL;		// Private GDI Device Context
HGLRC		hRC = NULL;		// Permanent Rendering Context
HWND		hWnd = NULL;	// Holds Our Window Handle
HINSTANCE	hInstance;		// Holds The Instance Of The Application

bool keys[256];				// Array Used For The Keyboard Routine
bool active = true;			// Window Active Flag Set To TRUE By Default
bool fullscreen = false;	// Fullscreen Flag Set To Fullscreen Mode By Default

#if RUDE_IPAD == 1
int windowWidth = 768;
int windowHeight = 1024;
#else
int windowWidth = 320;
int windowHeight = 420;
#endif

const char * kWindowTitle = "Bork3D Game Engine";

PFNGLACTIVETEXTUREPROC           glActiveTexture;
PFNGLCLIENTACTIVETEXTUREPROC     glClientActiveTexture;
PFNGLCOMPRESSEDTEXIMAGE2DPROC    glCompressedTexImage2D;
PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObjectARB;
PFNGLDELETEOBJECTARBPROC         glDeleteObjectARB;
PFNGLUSEPROGRAMOBJECTARBPROC     glUseProgramObjectARB;
PFNGLCREATESHADEROBJECTARBPROC   glCreateShaderObjectARB;
PFNGLSHADERSOURCEARBPROC         glShaderSourceARB;
PFNGLCOMPILESHADERARBPROC        glCompileShaderARB;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
PFNGLATTACHOBJECTARBPROC         glAttachObjectARB;
PFNGLGETINFOLOGARBPROC           glGetInfoLogARB;
PFNGLLINKPROGRAMARBPROC          glLinkProgramARB;
PFNGLGETUNIFORMLOCATIONARBPROC   glGetUniformLocationARB;
PFNGLUNIFORM4FARBPROC            glUniform4fARB;
PFNGLUNIFORM4FVARBPROC           glUniform4fvARB;
PFNGLUNIFORM1IARBPROC            glUniform1iARB;
PFNGLUNIFORM1FARBPROC            glUniform1fARB;
PFNGLUNIFORMMATRIX4FVARBPROC     glUniformMatrix4fvARB;
PFNGLVERTEXATTRIB4FVARBPROC      glVertexAttrib4fvARB;
PFNGLBINDATTRIBLOCATIONARBPROC   glBindAttribLocationARB;
PFNGLGETACTIVEATTRIBARBPROC      glGetActiveAttribARB;
PFNGLGETATTRIBLOCATIONARBPROC    glGetAttribLocationARB;

PFNGLGENFRAMEBUFFERSEXTPROC      glGenFramebuffersEXT;
PFNGLBINDFRAMEBUFFEREXTPROC      glBindFramebufferEXT;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
PFNGLDELETEFRAMEBUFFERSEXTPROC   glDeleteFramebuffers;
PFNGLDELETERENDERBUFFERSEXTPROC  glDeleteRenderbuffersEXT;

#define DECLARE_OGL_API(cast, func) \
	func = (cast) wglGetProcAddress(#func); \
	if(func == 0) { RudeAssert(__FILE__, __LINE__, "wglGetProcAddress failed"); }

LRESULT	CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// Declaration For WndProc

GLvoid ReSizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if(height < 1)
		height = 1;
	if(width < 1)
		width = 1;

	windowHeight = height;
	windowWidth = width;

	RGL.SetDeviceHeight((float) windowHeight);
	RGL.SetDeviceWidth((float) windowWidth);
}

int InitGL(GLvoid)										// All Setup For OpenGL Goes Here
{
	glShadeModel(GL_SMOOTH);							// Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);				// Black Background
	glClearDepth(1.0f);									// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);							// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);								// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// Really Nice Perspective Calculations

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	return TRUE;										// Initialization Went OK
}

int DrawGLScene(int camera)
{
	RGL.FlushEnables();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear Screen And Depth Buffer
	glLoadIdentity();									// Reset The Current Modelview Matrix

	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	static float lastTime = -1.0f;

	float ticks = (float) GetTickCount();
	float currentTime = ticks / 1000.0f;
	float deltaTime = 0.0f;

	if(lastTime > 0.0f)
	{
		deltaTime = currentTime - lastTime;
	}
	
	lastTime = currentTime;

	if(gVBGame)
	{
		gVBGame->Render(deltaTime, (float) windowWidth, (float) windowHeight, camera);
	}

	

	return TRUE;										// Everything Went OK
}

GLvoid KillGLWindow(GLvoid)								// Properly Kill The Window
{
	if (fullscreen)										// Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL,0);					// If So Switch Back To The Desktop
		ShowCursor(TRUE);								// Show Mouse Pointer
	}

	if (hRC)											// Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL,NULL))					// Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL,"Release Of DC And RC Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))						// Are We Able To Delete The RC?
		{
			MessageBox(NULL,"Release Rendering Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;										// Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd,hDC))					// Are We Able To Release The DC
	{
		MessageBox(NULL,"Release Device Context Failed.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hDC=NULL;										// Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd))					// Are We Able To Destroy The Window?
	{
		MessageBox(NULL,"Could Not Release hWnd.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hWnd=NULL;										// Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL",hInstance))			// Are We Able To Unregister Class
	{
		MessageBox(NULL,"Could Not Unregister Class.","SHUTDOWN ERROR",MB_OK | MB_ICONINFORMATION);
		hInstance=NULL;									// Set hInstance To NULL
	}
}


/*	This Code Creates Our OpenGL Window.  Parameters Are:					*
 *	title			- Title To Appear At The Top Of The Window				*
 *	width			- Width Of The GL Window Or Fullscreen Mode				*
 *	height			- Height Of The GL Window Or Fullscreen Mode			*
 *	bits			- Number Of Bits To Use For Color (8/16/24/32)			*
 *	fullscreenflag	- Use Fullscreen Mode (TRUE) Or Windowed Mode (FALSE)	*/
 
BOOL CreateGLWindow(const char *title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint		PixelFormat;			// Holds The Results After Searching For A Match
	WNDCLASS	wc;						// Windows Class Structure
	DWORD		dwExStyle;				// Window Extended Style
	DWORD		dwStyle;				// Window Style
	RECT		WindowRect;				// Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left=(long)0;			// Set Left Value To 0
	WindowRect.right=(long)width;		// Set Right Value To Requested Width
	WindowRect.top=(long)0;				// Set Top Value To 0
	WindowRect.bottom=(long)height;		// Set Bottom Value To Requested Height

	fullscreen=fullscreenflag;			// Set The Global Fullscreen Flag

	hInstance			= GetModuleHandle(NULL);				// Grab An Instance For Our Window
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;	// Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc		= (WNDPROC) WndProc;					// WndProc Handles Messages
	wc.cbClsExtra		= 0;									// No Extra Window Data
	wc.cbWndExtra		= 0;									// No Extra Window Data
	wc.hInstance		= hInstance;							// Set The Instance
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);			// Load The Default Icon
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);			// Load The Arrow Pointer
	wc.hbrBackground	= NULL;									// No Background Required For GL
	wc.lpszMenuName		= NULL;									// We Don't Want A Menu
	wc.lpszClassName	= "OpenGL";								// Set The Class Name

	if (!RegisterClass(&wc))									// Attempt To Register The Window Class
	{
		MessageBox(NULL,"Failed To Register The Window Class.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;											// Return FALSE
	}
	
	if (fullscreen)												// Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth	= width;				// Selected Screen Width
		dmScreenSettings.dmPelsHeight	= height;				// Selected Screen Height
		dmScreenSettings.dmBitsPerPel	= bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN)!=DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL,"The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?","Bork3D",MB_YESNO|MB_ICONEXCLAMATION)==IDYES)
			{
				fullscreen=FALSE;		// Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL,"Program Will Now Close.","ERROR",MB_OK|MB_ICONSTOP);
				return FALSE;									// Return FALSE
			}
		}
	}

	if (fullscreen)												// Are We Still In Fullscreen Mode?
	{
		dwExStyle=WS_EX_APPWINDOW;								// Window Extended Style
		dwStyle=WS_POPUP;										// Windows Style
		//ShowCursor(FALSE);										// Hide Mouse Pointer
	}
	else
	{
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;			// Window Extended Style
		dwStyle=WS_OVERLAPPEDWINDOW;							// Windows Style
	}

	if(!AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle))		// Adjust Window To True Requested Size
	{
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	// Create The Window
	if (!(hWnd=CreateWindowEx(	dwExStyle,							// Extended Style For The Window
								"OpenGL",							// Class Name
								title,								// Window Title
								dwStyle |							// Defined Window Style
								WS_CLIPSIBLINGS |					// Required Window Style
								WS_CLIPCHILDREN,					// Required Window Style
								0, 0,								// Window Position
								WindowRect.right-WindowRect.left,	// Calculate Window Width
								WindowRect.bottom-WindowRect.top,	// Calculate Window Height
								NULL,								// No Parent Window
								NULL,								// No Menu
								hInstance,							// Instance
								NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Window Creation Error.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	static	PIXELFORMATDESCRIPTOR pfd=				// pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
		1,											// Version Number
		PFD_DRAW_TO_WINDOW |						// Format Must Support Window
		PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
		PFD_DOUBLEBUFFER,							// Must Support Double Buffering
		PFD_TYPE_RGBA,								// Request An RGBA Format
		bits,										// Select Our Color Depth
		0, 0, 0, 0, 0, 0,							// Color Bits Ignored
		0,											// No Alpha Buffer
		0,											// Shift Bit Ignored
		0,											// No Accumulation Buffer
		0, 0, 0, 0,									// Accumulation Bits Ignored
		16,											// 16Bit Z-Buffer (Depth Buffer)  
		0,											// No Stencil Buffer
		0,											// No Auxiliary Buffer
		PFD_MAIN_PLANE,								// Main Drawing Layer
		0,											// Reserved
		0, 0, 0										// Layer Masks Ignored
	};
	
	if (!(hDC=GetDC(hWnd)))							// Did We Get A Device Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Device Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(PixelFormat=ChoosePixelFormat(hDC,&pfd)))	// Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Find A Suitable PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!SetPixelFormat(hDC,PixelFormat,&pfd))		// Are We Able To Set The Pixel Format?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Set The PixelFormat.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if (!(hRC=wglCreateContext(hDC)))				// Are We Able To Get A Rendering Context?
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Create A GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	if(!wglMakeCurrent(hDC,hRC))					// Try To Activate The Rendering Context
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Can't Activate The GL Rendering Context.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	ShowWindow(hWnd,SW_SHOW);						// Show The Window
	SetForegroundWindow(hWnd);						// Slightly Higher Priority
	SetFocus(hWnd);									// Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height);					// Set Up Our Perspective GL Screen

	DECLARE_OGL_API(PFNGLACTIVETEXTUREPROC,           glActiveTexture);
	DECLARE_OGL_API(PFNGLCLIENTACTIVETEXTUREPROC,     glClientActiveTexture);
	DECLARE_OGL_API(PFNGLCOMPRESSEDTEXIMAGE2DPROC,    glCompressedTexImage2D);
	DECLARE_OGL_API(PFNGLCREATEPROGRAMOBJECTARBPROC,  glCreateProgramObjectARB);
	DECLARE_OGL_API(PFNGLDELETEOBJECTARBPROC,         glDeleteObjectARB);
	DECLARE_OGL_API(PFNGLUSEPROGRAMOBJECTARBPROC,     glUseProgramObjectARB);
	DECLARE_OGL_API(PFNGLCREATESHADEROBJECTARBPROC,   glCreateShaderObjectARB);
	DECLARE_OGL_API(PFNGLSHADERSOURCEARBPROC,         glShaderSourceARB);
	DECLARE_OGL_API(PFNGLCOMPILESHADERARBPROC,        glCompileShaderARB);
	DECLARE_OGL_API(PFNGLGETOBJECTPARAMETERIVARBPROC, glGetObjectParameterivARB);
	DECLARE_OGL_API(PFNGLATTACHOBJECTARBPROC,         glAttachObjectARB);
	DECLARE_OGL_API(PFNGLGETINFOLOGARBPROC,           glGetInfoLogARB);
	DECLARE_OGL_API(PFNGLLINKPROGRAMARBPROC,          glLinkProgramARB);
	DECLARE_OGL_API(PFNGLGETUNIFORMLOCATIONARBPROC,   glGetUniformLocationARB);
	DECLARE_OGL_API(PFNGLUNIFORM4FARBPROC,            glUniform4fARB);
	DECLARE_OGL_API(PFNGLUNIFORM4FVARBPROC,           glUniform4fvARB);
	DECLARE_OGL_API(PFNGLUNIFORM1IARBPROC,            glUniform1iARB);
	DECLARE_OGL_API(PFNGLUNIFORM1FARBPROC,            glUniform1fARB);
	DECLARE_OGL_API(PFNGLUNIFORMMATRIX4FVARBPROC,     glUniformMatrix4fvARB);
	DECLARE_OGL_API(PFNGLVERTEXATTRIB4FVARBPROC,      glVertexAttrib4fvARB);
	DECLARE_OGL_API(PFNGLBINDATTRIBLOCATIONARBPROC,   glBindAttribLocationARB);
	DECLARE_OGL_API(PFNGLGETACTIVEATTRIBARBPROC,      glGetActiveAttribARB);
	DECLARE_OGL_API(PFNGLGETATTRIBLOCATIONARBPROC,    glGetAttribLocationARB);

	DECLARE_OGL_API(PFNGLGENFRAMEBUFFERSEXTPROC,      glGenFramebuffersEXT);
	DECLARE_OGL_API(PFNGLBINDFRAMEBUFFEREXTPROC,      glBindFramebufferEXT);
	DECLARE_OGL_API(PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, glFramebufferTexture2DEXT);
	DECLARE_OGL_API(PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, glCheckFramebufferStatusEXT);
	DECLARE_OGL_API(PFNGLDELETEFRAMEBUFFERSEXTPROC,   glDeleteFramebuffers);
	DECLARE_OGL_API(PFNGLDELETERENDERBUFFERSEXTPROC,  glDeleteRenderbuffersEXT);

	if (!InitGL())									// Initialize Our Newly Created GL Window
	{
		KillGLWindow();								// Reset The Display
		MessageBox(NULL,"Initialization Failed.","ERROR",MB_OK|MB_ICONEXCLAMATION);
		return FALSE;								// Return FALSE
	}

	return TRUE;									// Success
}

LRESULT CALLBACK WndProc(	HWND	hWnd,			// Handle For This Window
							UINT	uMsg,			// Message For This Window
							WPARAM	wParam,			// Additional Message Information
							LPARAM	lParam)			// Additional Message Information
{

	// This is sort-of a ridiculous way to emulate a touch screen..
	// Remove this non-sense if you're not emulating a touch-based device!
	static bool leftButtonDown = false;
	static RudeScreenVertex leftMousePos(-1, -1);

	switch (uMsg)									// Check For Windows Messages
	{
		case WM_ACTIVATE:							// Watch For Window Activate Message
		{
			if (!HIWORD(wParam))					// Check Minimization State
			{
				active=TRUE;						// Program Is Active
			}
			else
			{
				active=FALSE;						// Program Is No Longer Active
			}

			return 0;								// Return To The Message Loop
		}

		case WM_SYSCOMMAND:							// Intercept System Commands
		{
			switch (wParam)							// Check System Calls
			{
				case SC_SCREENSAVE:					// Screensaver Trying To Start?
				case SC_MONITORPOWER:				// Monitor Trying To Enter Powersave?
				return 0;							// Prevent From Happening
			}
			break;									// Exit
		}

		case WM_CLOSE:								// Did We Receive A Close Message?
		{
			PostQuitMessage(0);						// Send A Quit Message
			return 0;								// Jump Back
		}

		case WM_KEYDOWN:							// Is A Key Being Held Down?
		{
			keys[wParam] = TRUE;					// If So, Mark It As TRUE
			return 0;								// Jump Back
		}

		case WM_KEYUP:								// Has A Key Been Released?
		{
			keys[wParam] = FALSE;					// If So, Mark It As FALSE
			return 0;								// Jump Back
		}

		case WM_SIZE:								// Resize The OpenGL Window
		{
			/*
			ReSizeGLScene(LOWORD(lParam),HIWORD(lParam));  // LoWord=Width, HiWord=Height
			if(gVBGame)
				gVBGame->Resize();

			DrawGLScene(0);							// Draw The Scene
			SwapBuffers(hDC);						// Swap Buffers (Double Buffering)
			*/
			return 0;								// Jump Back
		}

		case WM_LBUTTONDOWN:
		{
			int xPos = LOWORD(lParam); 
			int yPos = HIWORD(lParam);
			leftButtonDown = true;

			leftMousePos = RudeScreenVertex(xPos, yPos);

			if(gVBGame)
				gVBGame->TouchDown(leftMousePos);

			TRACKMOUSEEVENT tme;
			tme.cbSize = sizeof(TRACKMOUSEEVENT);
			tme.dwFlags = TME_LEAVE;
			tme.hwndTrack = hWnd;

			TrackMouseEvent(&tme);
			
			return 0;
		}

		case WM_MOUSEMOVE:
		{
			int xPos = LOWORD(lParam); 
			int yPos = HIWORD(lParam);

			RudeScreenVertex newpos(xPos, yPos);

			if(leftButtonDown)
			{
				if(gVBGame)
					gVBGame->TouchMove(newpos, leftMousePos);
			}

			leftMousePos = newpos;

			return 0;
		}

		case WM_MOUSELEAVE:
		case WM_LBUTTONUP:
		{
			int xPos = LOWORD(lParam); 
			int yPos = HIWORD(lParam);
			RudeScreenVertex newpos(xPos, yPos);

			if(leftButtonDown)
			{
				if(gVBGame)
					gVBGame->TouchUp(newpos, leftMousePos);
			}
			
			leftButtonDown = false;
			leftMousePos = newpos;
			
			return 0;
		}


	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

int WINAPI WinMain(	HINSTANCE	hInstance,			// Instance
					HINSTANCE	hPrevInstance,		// Previous Instance
					LPSTR		lpCmdLine,			// Command Line Parameters
					int			nCmdShow)			// Window Show State
{
	MSG		msg;									// Windows Message Structure
	BOOL	done=FALSE;								// Bool Variable To Exit Loop

	// Ask The User Which Screen Mode They Prefer
	/*
	if (MessageBox(NULL,"Would You Like To Run In Fullscreen Mode?", "Start FullScreen?",MB_YESNO|MB_ICONQUESTION)==IDNO)
		fullscreen = false;
	else
		fullscreen = true;
	*/

	fullscreen = false;

	// Create Our OpenGL Window
	if (!CreateGLWindow(kWindowTitle, windowWidth, windowHeight, 32, fullscreen))
	{
		return 0;
	}

	GLD3DBuffers_create(&gl_d3d_buffers, hWnd, fullscreen, true, true);

	RGL.SetUpsideDown(true);

	RudeFontManager::InitFonts();

	if(gVBGame == 0)
		gVBGame = new RBGame();


	RudeText::Init();

#ifndef NO_RUDETWEAKER
	RudeTweaker::GetInstance()->Init();
#endif

	// Disable Floating Point Exceptions
	_clearfp();

	unsigned int control_word = 0;
	int err = _controlfp_s(&control_word, 0, 0);
	RUDE_REPORT("Old FP Settings: 0x%.4x\n", control_word);

	err = _controlfp_s(&control_word, _MCW_EM, _MCW_EM);
	RUDE_REPORT("New FP Settings: 0x%.4x\n", control_word);

	while(!done)									// Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg,NULL,0,0,PM_REMOVE))	// Is There A Message Waiting?
		{
			if (msg.message==WM_QUIT)				// Have We Received A Quit Message?
			{
				done=TRUE;							// If So done=TRUE
			}
			else									// If Not, Deal With Window Messages
			{
				TranslateMessage(&msg);				// Translate The Message
				DispatchMessage(&msg);				// Dispatch The Message
			}
		}
		else										// If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if (active)								// Program Active?
			{
				if (keys[VK_ESCAPE])				// Was ESC Pressed?
				{
					done=TRUE;						// ESC Signalled A Quit
				}
				else								// Not Time To Quit, Update Screen
				{
					GLD3DBuffers_activate_left(&gl_d3d_buffers);
					DrawGLScene(0);
					GLD3DBuffers_activate_right(&gl_d3d_buffers);
					DrawGLScene(1);
					GLD3DBuffers_deactivate(&gl_d3d_buffers);
					GLD3DBuffers_flush(&gl_d3d_buffers);

					glFinish();
				}
			}

			/*
			// Resizing on Windows doesn't work yet; it will blow away your textures.

			if (keys[VK_F1])						// Is F1 Being Pressed?
			{
				keys[VK_F1]=FALSE;					// If So Make Key FALSE
				KillGLWindow();						// Kill Our Current Window
				fullscreen=!fullscreen;				// Toggle Fullscreen / Windowed Mode
				// Recreate Our OpenGL Window
				if (!CreateGLWindow(kWindowTitle, windowWidth, windowHeight, 32, fullscreen))
				{
					return 0;						// Quit If Window Was Not Created
				}
			}
			*/
		}
	}

	// Shutdown
	KillGLWindow();									// Kill The Window
	return (msg.wParam);							// Exit The Program
}
