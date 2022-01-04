#include "global.hpp"
#include "gl.hpp"

static struct
{
	struct
	{
		int x;
		int y;
	} cursor;

	struct
	{
		int open;
	} display;

	struct
	{
		int alt;
		int lmb;
		int mmb;
		int wheel;
		int number_0;
		int number_1;
		int number_2;
		int number_3;
	} controller;
} win32;

LRESULT CALLBACK
windowproc_dummy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK
windowproc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;

	switch (message)
	{
	case WM_CREATE:
		win32.display.open = 1;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if (wParam == '1')
		{
			win32.controller.number_1 = 1;
		}
		else if (wParam == '2')
		{
			win32.controller.number_2 = 1;
		}
		else if (wParam == '3')
		{
			win32.controller.number_3 = 1;
		}
		else if (wParam == '0')
		{
			win32.controller.number_0 = 1;
		}
		break;
	case WM_KEYUP:
		if (wParam == '1')
		{
			win32.controller.number_1 = 0;
		}
		else if (wParam == '2')
		{
			win32.controller.number_2 = 0;
		}
		else if (wParam == '3')
		{
			win32.controller.number_3 = 0;
		}
		else if (wParam == '0')
		{
			win32.controller.number_0 = 0;
		}
		break;
	case WM_LBUTTONDOWN:
		win32.controller.lmb = 1;
		break;
	case WM_LBUTTONUP:
		win32.controller.lmb = 0;
		break;
	case WM_MBUTTONDOWN:
		win32.controller.mmb = 1;
		break;
	case WM_MBUTTONUP:
		win32.controller.mmb = 0;
		break;
	case WM_MOUSEWHEEL:
		win32.controller.wheel = GET_WHEEL_DELTA_WPARAM(wParam);
		break;
	case WM_SYSKEYDOWN:
		if (wParam == VK_MENU)
		{
			win32.controller.alt = 1;
		}
		break;
	case WM_SYSKEYUP:
		if (wParam == VK_MENU)
		{
			win32.controller.alt = 0;
		}
		break;
	case WM_MOUSEMOVE:
		win32.cursor.x = GET_X_LPARAM(lParam);
		win32.cursor.y = GET_Y_LPARAM(lParam);
		break;
	case WM_PAINT:
		BeginPaint(window, &ps);
		EndPaint(window, &ps);
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		win32.display.open = 0;
		break;
	default:
		return DefWindowProc(window, message, wParam, lParam);
	}
	return 0;
}

int
WinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE previnstance, _In_ LPSTR cmdline, _In_ int showcmd)
{
	struct r_tick tick;
	PIXELFORMATDESCRIPTOR pfd;
	WNDCLASSEX wcex;
	HWND window;
	HWND windowdummy;
	HDC hdcdummy;
	HDC hdc;
	HGLRC rc;
	HGLRC rcdummy;
	MSG message;
	int format_id;
	UINT formatcount;
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
	LPCWSTR classdummy = TEXT("WYV_WIN32_GL_DUMMY");
	LPCWSTR classmain = TEXT("WYV_WIN32_GL");
	static char title[128];
	static const int contextAttribs[] =
	{
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 5,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	static const int pixel[] =
	{
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_COLOR_BITS_ARB, 32,
		WGL_ALPHA_BITS_ARB, 8,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
		WGL_SAMPLES_ARB, 16,
		0
	};

	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	/* Find wgl functions for context creation. */
	wcex = {};
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = windowproc_dummy;
	wcex.hInstance = instance;
	wcex.hCursor = NULL;
	wcex.lpszClassName = classdummy;
	RegisterClassEx(&wcex);

	windowdummy = CreateWindow(classdummy, TEXT(""), WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, def_w, def_h, NULL, NULL, instance, NULL);
	hdcdummy = GetDC(windowdummy);

	pfd = { };
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 24;
	format_id = ChoosePixelFormat(hdcdummy, &pfd);
	if (format_id == 0)
	{
		return 1;
	}
	if (SetPixelFormat(hdcdummy, format_id, &pfd) == 0)
	{
		return 1;
	}

	rcdummy = wglCreateContext(hdcdummy);
	if (wglMakeCurrent(hdcdummy, rcdummy) == 0)
	{
		return 1;
	}
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	/* Create the actual window. */
	wcex = {};
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = windowproc;
	wcex.hInstance = instance;
	wcex.hCursor = NULL;
	wcex.hbrBackground = NULL;
	wcex.lpszClassName = classmain;
	RegisterClassEx(&wcex);

	window = CreateWindowEx(0, classmain, TEXT("aaaaa"), WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 0, 0, def_w, def_h, NULL, NULL, instance, NULL);
	hdc = GetDC(window);

	wglChoosePixelFormatARB(hdc, pixel, NULL, 1, &format_id, &formatcount);
	DescribePixelFormat(hdc, format_id, sizeof(pfd), &pfd);
	SetPixelFormat(hdc, format_id, &pfd);

	rc = wglCreateContextAttribsARB(hdc, 0, contextAttribs);
	if (rc == NULL)
	{
		return 1;
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(rcdummy);
	ReleaseDC(windowdummy, hdcdummy);
	DestroyWindow(windowdummy);
	UnregisterClass(classdummy, instance);

	if (wglMakeCurrent(hdc, rc) == 0)
	{
		return 1;
	}
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
	glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
	glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
	glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
	glDetachShader = (PFNGLDETACHSHADERPROC)wglGetProcAddress("glDetachShader");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
	glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
	glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
	glUniform1iv = (PFNGLUNIFORM1IVPROC)wglGetProcAddress("glUniform1iv");
	glUniform2iv = (PFNGLUNIFORM2IVPROC)wglGetProcAddress("glUniform2iv");
	glUniform3iv = (PFNGLUNIFORM3IVPROC)wglGetProcAddress("glUniform3iv");
	glUniform4iv = (PFNGLUNIFORM4IVPROC)wglGetProcAddress("glUniform4iv");
	glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
	glUniform1fv = (PFNGLUNIFORM1FVPROC)wglGetProcAddress("glUniform1fv");
	glUniform2fv = (PFNGLUNIFORM2FVPROC)wglGetProcAddress("glUniform2fv");
	glUniform3fv = (PFNGLUNIFORM3FVPROC)wglGetProcAddress("glUniform3fv");
	glUniform4fv = (PFNGLUNIFORM4FVPROC)wglGetProcAddress("glUniform4fv");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)wglGetProcAddress("glUniformMatrix4fv");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)wglGetProcAddress("glGetAttribLocation");
	glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)wglGetProcAddress("glVertexAttrib1f");
	glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)wglGetProcAddress("glVertexAttrib1fv");
	glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)wglGetProcAddress("glVertexAttrib2fv");
	glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)wglGetProcAddress("glVertexAttrib3fv");
	glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)wglGetProcAddress("glVertexAttrib4fv");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glEnableVertexAttribArray");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)wglGetProcAddress("glDisableVertexAttribArray");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)wglGetProcAddress("glBindAttribLocation");
	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)wglGetProcAddress("glGetActiveUniform");
	glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
	glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)wglGetProcAddress("glVertexAttribPointer");
	glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)wglGetProcAddress("glDeleteBuffers");
	glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)wglGetProcAddress("glMultiDrawElements");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)wglGetProcAddress("glBufferSubData");
	glMapBuffer = (PFNGLMAPBUFFERPROC)wglGetProcAddress("glMapBuffer");
	glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)wglGetProcAddress("glUnmapBuffer");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)wglGetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)wglGetProcAddress("glBindVertexArray");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)wglGetProcAddress("glDeleteVertexArrays");
	glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
	glGenerateMipmap = (PFNGLGENERATEMIPMAPPROC)wglGetProcAddress("glGenerateMipmap");
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2D");
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)wglGetProcAddress("glBindFramebuffer");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)wglGetProcAddress("glGenRenderbuffers");
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)wglGetProcAddress("glBindRenderbuffer");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)wglGetProcAddress("glFramebufferRenderbuffer");
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)wglGetProcAddress("glRenderbufferStorage");
	strcpy_s(title, "matf rg 2021/2022 (");
	strcat_s(title, 128 - 1, (char*)glGetString(GL_VERSION));
	strcat_s(title, 128, ")");
	SetWindowTextA(window, title);
	ShowWindow(window, showcmd);
	UpdateWindow(window);

	message = { };
	tick = { };
	r_glbegin();
	while (win32.display.open)
	{
		while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		if (message.message == WM_QUIT)
		{
			win32.display.open = 0;
		}

		if (win32.controller.lmb || win32.controller.mmb)
		{
			tick.cursor.dx = tick.cursor.x - win32.cursor.x;
			tick.cursor.dy = tick.cursor.y - win32.cursor.y;
			if (win32.controller.lmb && win32.controller.alt)
			{
				tick.cursor.mode = CURSOR_MODE_ORBIT;
			}
			else if (win32.controller.mmb && win32.controller.alt)
			{
				tick.cursor.mode = CURSOR_MODE_PAN;
			}
		}
		else
		{
			tick.cursor.mode = CURSOR_MODE_STAGNANT;
		}

		if (win32.controller.number_1)
		{
			if (r_newscene(scene::SCENE_ROOM))
			{
				std::cout << "bad issue\n";
			}
		}
		else if (win32.controller.number_2)
		{
			if (r_newscene(scene::SCENE_PRIMITIVES))
			{
				std::cout << "bad issue\n";
			}
		}
		else if (win32.controller.number_3)
		{
			if (r_newscene(scene::SCENE_3))
			{
				std::cout << "bad issue\n";
			}
		}
		else if (win32.controller.number_0)
		{
			if (r_newscene(scene::SCENE_VOID))
			{
				std::cout << "bad issue\n";
			}
		}

		tick.cursor.wheel = win32.controller.wheel;
		tick.cursor.x = win32.cursor.x;
		tick.cursor.y = win32.cursor.y;
		r_gltick(tick);
		win32.controller.wheel = 0;
		SwapBuffers(hdc);
		Sleep(1);
	}

	r_glexit();
	ReleaseDC(window, hdc);
	DestroyWindow(window);
	UnregisterClass(classmain, instance);

	return (int)message.wParam;
}
