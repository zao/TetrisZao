#include "Pch.h"
#include "State.h"
#include "MenuState.h"
#include "GameState.h"
#include "Xtris.h"

extern IDirect3D9Ptr d3d;
extern IDirect3DDevice9Ptr dev;

std::string nick = "Nomnom";
std::string hostname = "nano.cs.umu.se";
std::string port = "19503";

std::string labels[] = {
	"Nick:", "Host:", "Port:",
};

std::string* editables[] = {
	&nick, &hostname, &port,
};

int item = 0;
int num_items = 3;

ID3DXFont* font = 0;

void connect_enter() {
	if (!font) {
		D3DXCreateFont(dev, 46, 0, 0, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
			FF_DONTCARE, L"Impact", &font);
	}
};

void connect_leave() {
	font->Release();
	font = 0;
};

void connect_key(WPARAM key, bool pressed) {
	std::string& target = *editables[item];
	if (pressed) {
		if (key == VK_BACK) {
			if (target.size())
				target.resize(target.size() - 1);
		}
	}
	else {
		if (key >= 'A' && key <= 'Z') {
			target += tolower(key);
		}
		if (key >= '0' && key <= '9') {
			target += key;
		}
		switch (key) {
			case VK_OEM_PERIOD: target += '.'; break;
			case VK_RETURN: {
				try {
					client.reset(new xtris::client(nick, hostname, port));
					change_state(&game_state);
				}
				catch (sys::error_code&) {
					change_state(&menu_state);
				}
			} break;
			case VK_ESCAPE: change_state(&menu_state); break;
			case VK_UP: --(item += num_items) %= num_items; break;
			case VK_DOWN: ++item %= num_items; break;
		}
	}
};

extern D3DXMATRIX ortho_proj;
extern D3DXMATRIX view_shift;

void connect_update(u32) {
	while (io.poll_one());

	if (dev) {
		dev->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			D3DXCOLOR(0.05f, 0.2f, 0.1f, 1.0f), 1.0f, 0);

		dev->SetTransform(D3DTS_PROJECTION, &ortho_proj);
		dev->SetTransform(D3DTS_VIEW, &view_shift);

		dev->BeginScene();
		for (int i = 0; i < num_items; ++i) {
			int y = 200 + i * 60;
			std::string text = *editables[i];
			if (i == item) {
				RECT r_arrow = { 0, y, 200, 500 };
				font->DrawTextA(0, "->", 2, &r_arrow, DT_RIGHT | DT_SINGLELINE, D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f));
				text += "_";
			}
			RECT r_label = { 200, y, 300, 500 };
			RECT r_text = { 300, y, 800, 500 };
			font->DrawTextA(0, labels[i].c_str(), -1, &r_label, DT_LEFT | DT_SINGLELINE, D3DXCOLOR(0.7f, 0.7f, 0.7f, 1.0f));
			font->DrawTextA(0, text.c_str(), -1, &r_text, DT_LEFT | DT_SINGLELINE, D3DXCOLOR(0.7f, 0.7f, 0.7f, 1.0f));
		}
		//for (size_t i = 0; i < item_count; ++i) {
			//draw_static_art(dev, std::make_pair<int const>(0, (i == current_item ? focused_item_art : normal_item_art)[i]));
		//}
		dev->EndScene();
		dev->Present(0, 0, 0, 0);
	}
};

state connect_state = {
	bind(&connect_enter),
	bind(&connect_leave),
	bind(&connect_key, _1, true),
	bind(&connect_key, _1, false),
	bind(&connect_update, _1),
};