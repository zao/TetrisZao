#include "Pch.h"
#include "Helpers.h"
#include "Tetris.h"
#include "Art.h"
#include "State.h"
#include "MenuState.h"
#include "GameState.h"

//#define MOVIE

IDirect3D9Ptr d3d;
IDirect3DDevice9Ptr dev;

IDirect3DSurface9Ptr shot_target = 0;

D3DXMATRIX ortho_proj;
D3DXMATRIX view_shift;

//int const block_size = 24;

LRESULT CALLBACK window_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	static bool shifted_key = false;
	switch (msg) {
		case WM_NCCREATE: return TRUE;
		case WM_DESTROY: PostQuitMessage(0); return 0;
		case WM_KEYUP: current_state->on_key_up(wparam); break;
		case WM_KEYDOWN: current_state->on_key_down(wparam); break;
	}
	return DefWindowProc(wnd, msg, wparam, lparam);
}

void idleHandler() {
	u32 t = timeGetTime();
	current_state->on_update(t);
}

HWND wnd = (HWND)INVALID_HANDLE_VALUE;

state * current_state = &menu_state;

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
	HRESULT hr = S_OK;
	CoInitializeEx(0, COINIT_APARTMENTTHREADED);

	win::window_class wc = win::window_class()
		.class_name(L"tetris-zao")
		.instance(instance)
		.style(CS_OWNDC)
		.window_proc(&window_proc)
		;
		
	RegisterClassEx(wc);

	wnd = CreateWindowEx(0, wc.class_name(), L"Tetris Zao", WS_OVERLAPPEDWINDOW & (~WS_THICKFRAME),
		CW_USEDEFAULT, 0, 800, 600, 0, 0, instance, 0);
	
	win::resize_client_area(wnd, 800, 600);

	d3d.Attach(Direct3DCreate9(D3D_SDK_VERSION));

	dx::presentation_parameters pp = dx::presentation_parameters()
		.auto_depth_stencil(TRUE)
		.auto_depth_stencil_format(D3DFMT_D24X8)
		.backbuffer_count(1)
		.backbuffer_format(D3DFMT_A8R8G8B8)
		.backbuffer_width(800).backbuffer_height(600)
		.device_window(wnd)
		.flags(D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL)
		.presentation_interval(D3DPRESENT_INTERVAL_ONE)
		.swap_effect(D3DSWAPEFFECT_DISCARD)
		.windowed(TRUE)
		;
	
	ShowWindow(wnd, SW_SHOW);

	UINT adapter = D3DADAPTER_DEFAULT;
	D3DDEVTYPE device_type = D3DDEVTYPE_HAL;

#if 1
	dx::find_perfhud(d3d, adapter, device_type);
#endif

	hr = d3d->CreateDevice(adapter, device_type, wnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, pp, &dev);

	dev->SetRenderState(D3DRS_ZENABLE, FALSE);
	dev->SetRenderState(D3DRS_LIGHTING, FALSE);
	dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	
	dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	D3DXMatrixOrthoOffCenterLH(&ortho_proj, 0.0f, 800.0f, 0.0f, 600.0f, 0.0f, 100.0f);
	D3DXMatrixTranslation(&view_shift, 0.0f, 0.0f, 0.0f);
	
	current_state->on_enter();

	bool running = true;
	do {
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				running = false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		idleHandler();
	}
	while (running);

	current_state->on_exit();

	dev.Release();
	d3d.Release();
	return 0;
}