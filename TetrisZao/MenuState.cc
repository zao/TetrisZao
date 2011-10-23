#include "Pch.h"
#include "State.h"
#include "GameState.h"
#include "ConnectState.h"
#include "Helpers.h"
#include "Art.h"

extern IDirect3D9Ptr d3d;
extern IDirect3DDevice9Ptr dev;

static_art_map main_menu_art_collection;

enum menu_item {
	single_player,
	multi_player,
	quit,
	item_count,
};

std::vector<static_art> focused_item_art;
std::vector<static_art> normal_item_art;

menu_item current_item;
extern HWND wnd;

void menu_enter() {
	current_item = single_player;
	if (focused_item_art.empty()) {
		static_art_description focused_art[] = {
			static_art_description(400, 380, 50, 256, 64, L"menu-single-focus.png"),
			static_art_description(400, 300, 50, 256, 64, L"menu-multi-focus.png"),
			static_art_description(400, 220, 50, 256, 64, L"menu-exit-focus.png"),
		};

		static_art_description normal_art[] = {
			static_art_description(400, 380, 50, 256, 64, L"menu-single.png"),
			static_art_description(400, 300, 50, 256, 64, L"menu-multi.png"),
			static_art_description(400, 220, 50, 256, 64, L"menu-exit.png"),
		};

		BOOST_FOREACH(static_art_description const& sad, focused_art) {
			push_back(focused_item_art)(compile_static_art(dev, sad));
		}
		BOOST_FOREACH(static_art_description const& sad, normal_art) {
			push_back(normal_item_art)(compile_static_art(dev, sad));
		}
	}
}

void menu_leave() {
}

void advance_item(int n) {
	int item = current_item;
	(item += n + item_count) %= item_count;
	current_item = (menu_item)item;
}

void menu_key(WPARAM key, bool pressed) {
	if (!pressed) {
		switch (key) {
			case VK_ESCAPE: DestroyWindow(wnd); break;
			case VK_RETURN:
				switch (current_item) {
					case single_player:
						current_state->on_exit();
						current_state = &game_state;
						current_state->on_enter();
						break;
					case multi_player:
						change_state(&connect_state);
						break;
					case quit:
						DestroyWindow(wnd);
						break;
				}
				break;
			case VK_UP: advance_item(-1); break;
			case VK_DOWN: advance_item(1); break;
		}
	}
}

extern D3DXMATRIX ortho_proj;
extern D3DXMATRIX view_shift;

void menu_update(u32 t) {
	if (dev) {
		dev->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			D3DXCOLOR(0.05f, 0.2f, 0.1f, 1.0f), 1.0f, 0);

		dev->SetTransform(D3DTS_PROJECTION, &ortho_proj);
		dev->SetTransform(D3DTS_VIEW, &view_shift);

		dev->BeginScene();
		for (size_t i = 0; i < item_count; ++i) {
			draw_static_art(dev, std::make_pair<int const>(0, (i == current_item ? focused_item_art : normal_item_art)[i]));
		}
		dev->EndScene();
		dev->Present(0, 0, 0, 0);
	}
}

state menu_state = {
	bind(&menu_enter),
	bind(&menu_leave),
	bind(&menu_key, _1, true),
	bind(&menu_key, _1, false),
	bind(&menu_update, _1),
};