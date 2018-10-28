/**
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

#include "log.hpp"
#include "filesystem.hpp"
#include "input.hpp"
#include "runtime.hpp"
#include "hook_manager.hpp"
#include "version.h"
#include <Windows.h>
#include "JWEMSingleton.h"

HMODULE g_module_handle = nullptr;

#if defined(RESHADE_TEST_APPLICATION)

#include "d3d9/d3d9.hpp"
#include "d3d11/d3d11.hpp"
#include "opengl/opengl_stubs.hpp"

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	using namespace reshade;

	g_module_handle = hInstance;
	runtime::s_reshade_dll_path = filesystem::get_module_path(nullptr);
	runtime::s_target_executable_path = filesystem::get_module_path(nullptr);

	log::open(filesystem::path(runtime::s_reshade_dll_path).replace_extension(".log"));

	hooks::register_module("user32.dll");

	// Register window class
	{
		WNDCLASS wc = { sizeof(wc) };
		wc.hInstance = hInstance;
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpszClassName = TEXT("Test");
		wc.lpfnWndProc = &WndProc;

		RegisterClass(&wc);
	}

	// Create and show window instance
	const HWND window_handle = CreateWindow(TEXT("test"), TEXT("ReShade ") TEXT(VERSION_STRING_FILE) TEXT(" by crosire"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0, 0, 1024, 800, nullptr, nullptr, hInstance, nullptr);

	if (window_handle == nullptr)
	{
		return 0;
	}

	ShowWindow(window_handle, nCmdShow);

	MSG msg = {};

#if RESHADE_TEST_APPLICATION == 1
	hooks::register_module("d3d9.dll");
	const auto d3d9_module = LoadLibrary(TEXT("d3d9.dll"));

	// Initialize Direct3D 9
	com_ptr<IDirect3D9> d3d(Direct3DCreate9(D3D_SDK_VERSION), true);
	com_ptr<IDirect3DDevice9> device;
	{
		RECT client_rect;
		GetClientRect(window_handle, &client_rect);

		D3DPRESENT_PARAMETERS pp = {};
		pp.BackBufferCount = 1;
		pp.BackBufferWidth = client_rect.right - client_rect.left;
		pp.BackBufferHeight = client_rect.bottom - client_rect.top;
		pp.Windowed = true;
		pp.hDeviceWindow = window_handle;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;

		if (d3d == nullptr || FAILED(d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window_handle, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device)))
		{
			return 0;
		}
	}

	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
		}

		device->Clear(0, nullptr, D3DCLEAR_TARGET, 0xFF7F7F7F, 0, 0);

		device->Present(nullptr, nullptr, nullptr, nullptr);
	}

	FreeLibrary(d3d9_module);
#endif
#if RESHADE_TEST_APPLICATION == 2
	hooks::register_module("dxgi.dll");
	hooks::register_module("d3d11.dll");
	const auto d3d11_module = LoadLibrary(TEXT("d3d11.dll"));

	// Initialize Direct3D 11
	com_ptr<ID3D11Device> device;
	com_ptr<ID3D11DeviceContext> immediate_context;
	com_ptr<IDXGISwapChain> swapchain;
	{
		RECT client_rect;
		GetClientRect(window_handle, &client_rect);

		DXGI_SWAP_CHAIN_DESC scdesc = {};
		scdesc.BufferCount = 1;
		scdesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scdesc.BufferDesc.Width = client_rect.right - client_rect.left;
		scdesc.BufferDesc.Height = client_rect.bottom - client_rect.top;
		scdesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scdesc.SampleDesc = { 1, 0 };
		scdesc.Windowed = true;
		scdesc.OutputWindow = window_handle;

		if (FAILED(D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_DEBUG, nullptr, 0, D3D11_SDK_VERSION, &scdesc, &swapchain, &device, nullptr, &immediate_context)))
		{
			return 0;
		}
	}

	com_ptr<ID3D11Texture2D> backbuffer;
	swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
	com_ptr<ID3D11RenderTargetView> target;
	device->CreateRenderTargetView(backbuffer.get(), nullptr, &target);

	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
		}

		const float color[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
		immediate_context->ClearRenderTargetView(target.get(), color);

		swapchain->Present(1, 0);
	}

	FreeLibrary(d3d11_module);
#endif
#if RESHADE_TEST_APPLICATION == 3
	hooks::register_module("opengl32.dll");
	const auto opengl_module = LoadLibrary(TEXT("opengl32.dll"));

	// Initialize OpenGL
	const HDC hdc = GetDC(window_handle);

	PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd) };
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;

	const int pf = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pf, &pfd);

	const HGLRC hglrc = wglCreateContext(hdc);

	if (hglrc == nullptr)
	{
		return 0;
	}

	wglMakeCurrent(hdc, hglrc);

	while (msg.message != WM_QUIT)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			DispatchMessage(&msg);
		}

		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		SwapBuffers(hdc);
	}

	wglMakeCurrent(nullptr, nullptr);
	wglDeleteContext(hglrc);

	FreeLibrary(opengl_module);
#endif

	return static_cast<int>(msg.wParam);
}

#else

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
	using namespace reshade;

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			g_module_handle = hModule;
			runtime::s_reshade_dll_path = filesystem::get_module_path(hModule);
			runtime::s_target_executable_path = filesystem::get_module_path(nullptr);

			log::open(filesystem::path(runtime::s_reshade_dll_path).replace_extension(".log"));

			Singleton * JWEmSingleton = Singleton::getInstance();
			JWEmSingleton->init(runtime::s_reshade_dll_path.remove_filename().string());

#ifdef WIN64
			LOG(INFO) << "Initializing crosire's ReShade version '" VERSION_STRING_FILE "' (64-bit) built on '" VERSION_DATE " " VERSION_TIME "' loaded from " << runtime::s_reshade_dll_path << " to " << runtime::s_target_executable_path << " ...";
#else
			LOG(INFO) << "Initializing crosire's ReShade version '" VERSION_STRING_FILE "' (32-bit) built on '" VERSION_DATE " " VERSION_TIME "' loaded from " << runtime::s_reshade_dll_path << " to " << runtime::s_target_executable_path << " ...";
#endif

			const auto system_path = filesystem::get_special_folder_path(filesystem::special_folder::system);
			hooks::register_module(system_path / "d3d9.dll");
			hooks::register_module(system_path / "d3d10.dll");
			hooks::register_module(system_path / "d3d10_1.dll");
			hooks::register_module(system_path / "d3d11.dll");
			hooks::register_module(system_path / "dxgi.dll");
			//hooks::register_module(system_path / "opengl32.dll");
			hooks::register_module(system_path / "user32.dll");
			hooks::register_module(system_path / "ws2_32.dll");

			LOG(INFO) << "Initialized.";
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			LOG(INFO) << "Exiting ...";

			input::uninstall();
			hooks::uninstall();

			LOG(INFO) << "Exited.";
			break;
		}
	}

	return TRUE;
}

#endif
