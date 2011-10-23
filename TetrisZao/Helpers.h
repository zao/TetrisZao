#pragma once 

#define ACCESSOR(type, name, member) \
			type& name() { return m.member; } \
			self& name(type const& t) { m.member = t; return *this; }

namespace win {
	struct window_class {
		typedef window_class self;
		window_class() { ZeroMemory(&m, sizeof(m)); m.cbSize = sizeof(m); }

		ACCESSOR(UINT, style, style)
		ACCESSOR(WNDPROC, window_proc, lpfnWndProc)
		ACCESSOR(int, class_extra, cbClsExtra)
		ACCESSOR(int, window_extra, cbWndExtra)
		ACCESSOR(HINSTANCE, instance, hInstance)
		ACCESSOR(HICON, icon, hIcon)
		ACCESSOR(HCURSOR, cursor, hCursor)
		ACCESSOR(HBRUSH, background, hbrBackground)
		ACCESSOR(LPCTSTR, menu_name, lpszMenuName)
		ACCESSOR(LPCTSTR, class_name, lpszClassName)
		ACCESSOR(HICON, small_icon, hIconSm)

		operator WNDCLASSEX * () { return &m; }
	private:
		WNDCLASSEX m;
	};

	void resize_client_area(HWND wnd, int w, int h);
}

namespace dx {
	struct presentation_parameters {
		typedef presentation_parameters self;
		presentation_parameters() { ZeroMemory(&m, sizeof(m)); }

		ACCESSOR(UINT, backbuffer_width, BackBufferWidth)
		ACCESSOR(UINT, backbuffer_height, BackBufferHeight)
		ACCESSOR(D3DFORMAT, backbuffer_format, BackBufferFormat)
		ACCESSOR(UINT, backbuffer_count, BackBufferCount)
		ACCESSOR(D3DMULTISAMPLE_TYPE, multisample_type, MultiSampleType)
		ACCESSOR(DWORD, multisample_quality, MultiSampleQuality)
		ACCESSOR(D3DSWAPEFFECT, swap_effect, SwapEffect)
		ACCESSOR(HWND, device_window, hDeviceWindow)
		ACCESSOR(BOOL, windowed, Windowed)
		ACCESSOR(BOOL, auto_depth_stencil, EnableAutoDepthStencil)
		ACCESSOR(D3DFORMAT, auto_depth_stencil_format, AutoDepthStencilFormat)
		ACCESSOR(DWORD, flags, Flags)
		ACCESSOR(UINT, refresh_rate, FullScreen_RefreshRateInHz)
		ACCESSOR(UINT, presentation_interval, PresentationInterval)

		operator D3DPRESENT_PARAMETERS * () { return &m; }
	private:
		D3DPRESENT_PARAMETERS m;
	};

	void find_perfhud(IDirect3D9Ptr d3d, UINT& adapter, D3DDEVTYPE& device_type);

	template <typename T, typename Buf>
	struct lock : noncopyable {
		lock(Buf& buf) : buf(buf) {
			buf->Lock(0, 0, (void**)&p, 0);
		}
		~lock() {
			buf->Unlock();
		}
		T* data() {
			return p;
		}
	private:
		Buf& buf;
		T* p;
	};

	template <typename T>
	struct vertex_lock {
		typedef lock<T, IDirect3DVertexBuffer9Ptr> type;
	};
}

#define ARRAY_COUNT(arr) (sizeof(arr)/sizeof(arr[0]))

#undef ACCESSOR