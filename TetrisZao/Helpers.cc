#include "Pch.h"
#include "Helpers.h"

namespace win {
	void resize_client_area(HWND wnd, int w, int h) {
		RECT client, window;
		GetClientRect(wnd, &client);
		GetWindowRect(wnd, &window);
		LONG ncWidth = (window.right - window.left) - (client.right - client.left);
		LONG ncHeight = (window.bottom - window.top) - (client.bottom - client.top);
		MoveWindow(wnd, window.left, window.top, w + ncWidth, h + ncHeight, FALSE);
	}
}

namespace dx {
	void find_perfhud(IDirect3D9Ptr d3d, UINT& adapter, D3DDEVTYPE& device_type) {
		for (UINT a = 0; a < d3d->GetAdapterCount(); ++a) {
			D3DADAPTER_IDENTIFIER9 identifier;
			d3d->GetAdapterIdentifier(a, 0, &identifier);
			if (strstr(identifier.Description, "PerfHUD") != 0) {
				adapter = a;
				device_type = D3DDEVTYPE_REF;
				break;
			}
		}
	}
}