#include "Pch.h"
#include "State.h"
#include "GameState.h"
#include "MenuState.h"
#include "ConnectState.h"
#include "Art.h"
#include "Helpers.h"
#include "Tetris.h"
#include "Xtris.h"
#include "NetworkProtocolHandlerImpl.h"

extern HWND wnd;

extern IDirect3D9Ptr d3d;
extern IDirect3DDevice9Ptr dev;
extern IDirect3DSurface9Ptr shot_target;
ID3DXFont* stats_font = 0;
ID3DXFont* chat_font = 0;

extern D3DXMATRIX ortho_proj;
extern D3DXMATRIX view_shift;
D3DXMATRIX playfield_to_world;
D3DXMATRIX preview_to_world;

static_art mini_back, mini_front;
static_art name_back, name_front;

static_art_map static_art_collection;
static_art block_art;
static_art block_decoration_art;

chat chat_messages(10);

tetris::game* game = 0;
shared_ptr<xtris::client> client;

int score = 0;
int lines = 0;

template <typename Array>
void insert_blocks(static_art_map& dst, Array const& blocks, D3DXMATRIX base, int offset_x, int offset_y) {
	for(int c = 0; c < (int)blocks.shape()[0]; ++c)
		for(int r = 0; r < (int)blocks.shape()[1]; ++r) {
			if (blocks[c][r] == tetris::piece::no_piece)
				continue;
			static_art block = block_art;
			static_art decor = block_decoration_art;

			D3DXMATRIX block_offset;
			D3DXMatrixTranslation(&block_offset, (float)((c + offset_x)*24), (float)((r + offset_y)*24), 0.0f);
			block.transform = decor.transform = block_offset * base;

			block.streams[1].offset = sizeof(D3DCOLOR)*blocks[c][r] * 4;
			insert(dst)(50, block)(45, decor);
		}
}

struct local_event_handler_impl : tetris::game::event_handler {
	void on_lines_removed(std::vector<tetris::board::index> lines) {
		::lines += lines.size();
	}

	void on_updated_blocks(std::vector<tetris::block> blocks) { }
	
	void on_lost_game() {
	}

	void on_clear() { }

	void on_score_increased(int amount) {
		::score += amount;
	}

	void on_line_added() { }

};

struct network_event_handler_impl : tetris::game::event_handler {
	void on_lines_removed(std::vector<tetris::board::index> lines) {
		::lines += lines.size();
		std::sort(lines.begin(), lines.end(), std::greater<tetris::board::index>());
		client->send_fall(lines);
		if (lines.size() > 1)
			client->send_lines(lines.size());
	}

	void on_updated_blocks(std::vector<tetris::block> blocks) {
		for (size_t i = 0; i < blocks.size(); i += 75) {
			std::vector<tetris::block> slice(blocks.begin() + i, blocks.begin() + min(i + 75, blocks.size()));
			client->send_draw(slice);
		}
	}

	void on_lost_game() {
		client->send_lost();
	}

	void on_clear() {
		client->send_clear();
	}

	void on_score_increased(int amount) {
		::score += amount;
	}

	void on_line_added() {
		client->send_grow();
	}
};

tetris::game::event_handler* event_handler;
shared_ptr<network_protocol_handler_impl> protocol_handler;

float multi_board_pos[2] = { 410.0f,  480.0f };
float multi_board_off[2] = { 110.0f, -190.0f };
float multi_name_off[2] =  {   0.0f,   92.0f };

void game_draw() {
	if (dev) {
		if (shot_target) {
			dev->SetRenderTarget(0, shot_target);
		}
		dev->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
			D3DXCOLOR(0.05f, 0.2f, 0.1f, 1.0f), 1.0f, 0);

		dev->SetTransform(D3DTS_PROJECTION, &ortho_proj);
		dev->SetTransform(D3DTS_VIEW, &view_shift);

		static_art_map art_queue(static_art_collection);

		// Enqueue block art
		if (game->is_alive()) {
			{
				tetris::board::field_view::type field = game->get_board().get_field();
				insert_blocks(art_queue, field, playfield_to_world, -5, -10);
			}

			// Enqueue current piece
			{
				tetris::piece const& piece = game->get_current_piece();
				tetris::piece::block_view::type blocks = piece.get_blocks();
				insert_blocks(art_queue, blocks, playfield_to_world, -5 + game->get_current_x(), -10 + game->get_current_y());
			}

			// Enqueue next piece
			{
				tetris::piece const& piece = game->get_next_piece();
				tetris::piece::block_view::type blocks = piece.get_blocks();
				insert_blocks(art_queue, blocks, preview_to_world, 0, 0);
			}
		}

		dev->BeginScene();
		// Draw art
		BOOST_FOREACH(static_art_map::value_type & sap, art_queue) {
			draw_static_art(dev, sap);
		}

		static_art_map multi_queue;
		// Draw secondary boards for multi-player
		if (client) {
			int i = 0;
			typedef std::map<u8, network_protocol_handler_impl::state::player> player_map;
			BOOST_FOREACH(player_map::value_type& pp, protocol_handler->s.players) {
				if (i == 8) break;
				static_art back = mini_back;
				static_art front = mini_front;
				static_art name_back = ::name_back;
				static_art name_front = ::name_front;

				float x = multi_board_pos[0] + (i%4)*multi_board_off[0];
				float y = multi_board_pos[1] + (i/4)*multi_board_off[1];

				D3DXMatrixTranslation(&back.transform,  x, y, 80.0f);
				D3DXMatrixTranslation(&front.transform, x, y, 10.0f);
				D3DXMatrixTranslation(&name_back.transform, x + multi_name_off[0], y + multi_name_off[1], 80.0f);
				D3DXMatrixTranslation(&name_front.transform, x + multi_name_off[0], y + multi_name_off[1], 10.0f);

				D3DXMATRIX block_adjust, nudge;
				D3DXMatrixTranslation(&nudge, -3.0f, 0.0f, 0.0f);
				D3DXMatrixScaling(&block_adjust, 0.25f, 0.25f, 1.0f);
				insert_blocks(multi_queue, pp.second.board.get_field(), block_adjust * nudge * back.transform, -4, -10);
				insert(multi_queue)
					(std::make_pair<int const>(80, back))
					(std::make_pair<int const>(10, front))
					(std::make_pair<int const>(80, name_back))
					(std::make_pair<int const>(10, name_front))
					;
				{
					long nx = long(x + multi_name_off[0]);
					long ny = 600 - long(y + multi_name_off[1]);
					RECT r = { nx - 50, ny - 8, nx + 50, ny + 8 };
					stats_font->DrawTextA(0, pp.second.nick.c_str(), -1, &r,
						DT_SINGLELINE | DT_CENTER | DT_VCENTER, ~0);
				}
				++i;
			}
		}

		BOOST_FOREACH(static_art_map::value_type & sap, multi_queue) {
			draw_static_art(dev, sap);
		}

		{
			RECT r = { 360, 413, 800, 433 + 20 };
			stats_font->DrawText(0, L"Next:", -1, &r,
				DT_SINGLELINE | DT_LEFT | DT_TOP, 0xFFF2B5E3U);
		}
		{
			RECT r = { 360, 510, 800, 530 };
			stats_font->DrawText(0, str(wformat(L"Score: %d") % score).c_str(), -1, &r,
				DT_SINGLELINE | DT_LEFT | DT_TOP, 0xFFF2B5E3U);
		}
		{
			RECT r = { 360, 540, 800, 560 };
			stats_font->DrawText(0, str(wformat(L"Lines: %d") % lines).c_str(), -1, &r,
				DT_SINGLELINE | DT_LEFT | DT_TOP, 0xFFF2B5E3U);
		}

		chat::iterator I = chat_messages.begin();
		for (int i = 0; I != chat_messages.end(); ++I, ++i) {
			RECT r = { 480, 410 + i*16, 775, 410 + (i + 1)*16 };
			chat_font->DrawTextA(0, I->c_str(), I->size(), &r,
				DT_SINGLELINE | DT_LEFT | DT_TOP, 0xFFF2B5E3U);
		}

		dev->EndScene();
		dev->Present(0, 0, 0, 0);
		dev->SetRenderTarget(1, 0);

		if (shot_target) {
			static size_t counter = 0;
			D3DXSaveSurfaceToFile(str(wformat(L"shot%05d.png") % counter++).c_str(), D3DXIFF_PNG, shot_target, 0, 0);
		}
	}
}

void game_update(u32 t) {
	while (io.poll_one());

	if (game->is_alive())
		game->step(t);
	game_draw();
}

template <typename Range>
void load_game_resources(Range static_art_sources) {

	// Load static art
	BOOST_FOREACH(static_art_description sad, static_art_sources) {
		insert(static_art_collection)(sad.z, compile_static_art(dev, sad));
	}

	// Load block art
	static_art_description block_desc(0, 0, 50, 24, 24, L"block.png");
	static_art_description block_decoration_desc(0, 0, 45, 24, 24, L"block-decoration.png");

	D3DCOLOR colors[] = {
#ifdef XTRIS_COLORS
		0xffbc0000, // I
		0xff00a900, // O
		0xff0060d8, // T
		0xff968000, // L
		0xff008080, // J
		0xffbc00bc, // S
		0xff7d00bc, // Z
#else
		0xff00f0f0, // I
		0xfff0f000, // O
		0xffa000f0, // T
		0xfff0a000, // L
		0xff0000f0, // J
		0xff00f000, // S
		0xfff00000, // Z
#endif
	};

	std::vector<D3DCOLOR> color_vec;
	BOOST_FOREACH(D3DCOLOR c, colors) {
		push_back(color_vec)(c)(c)(c)(c);
	}
	block_desc.color_source = color_vec;

	block_art = compile_static_art(dev, block_desc);
	block_decoration_art = compile_static_art(dev, block_decoration_desc);

#ifdef MOVIE
	dev->CreateRenderTarget(800, 600, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &shot_target, 0);
#endif

	D3DXCreateFont(dev, 24, 0, 0, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FF_DONTCARE, L"Impact", &stats_font);
	D3DXCreateFont(dev, 16, 0, 0, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY,
		FF_DONTCARE, L"Verdana", &chat_font);
}

void unload_game_resources() {
	if (shot_target)
		shot_target.Release();
	static_art_collection.clear();
	block_decoration_art = block_art = static_art();
}

void game_enter() {
	static_art_description mini_back, mini_front;
	static_art_description name_back(400, 420, 80, 128, 64,  L"nameplate-back.png");
	static_art_description name_front(400, 420, 10, 128, 64,  L"nameplate-front.png");

	static_art_description art[] = {
		static_art_description(400, 300, 90, 800, 600,  L"background.jpg"),
		mini_back = static_art_description(186, 310, 80, 512, 1024, L"play-area-back.png"),
		mini_front = static_art_description(186, 310, 10, 512, 1024, L"play-area-front.png"),
		static_art_description(564, 110, 80, 512, 256,  L"scorebox-back.png"),
		static_art_description(564, 110, 10, 512, 256,  L"scorebox-front.png"),
	};
	
	mini_back.w = mini_front.w /= 4;
	mini_back.h = mini_front.h /= 4;	
	::mini_back = compile_static_art(dev, mini_back);
	::mini_front = compile_static_art(dev, mini_front);
	::name_back = compile_static_art(dev, name_back);
	::name_front = compile_static_art(dev, name_front);

	load_game_resources(boost::make_iterator_range(art, art + ARRAY_COUNT(art)));

	D3DXMatrixTranslation(&playfield_to_world, 200.0f, 300.0f, 0.0f);
	D3DXMatrixTranslation(&preview_to_world, 380.0f, 100.0f, 0.0f);

	chat_messages.clear();
	chat_messages.add_text("S to start game, P to pause.");
	chat_messages.add_text("Arrows to move, shift to move far.");
	chat_messages.add_text("Space to drop, escape to end.");

	if (client) {
		event_handler = new network_event_handler_impl;
		game = new tetris::game(event_handler);
		protocol_handler.reset(new network_protocol_handler_impl(game, client.get(), &chat_messages));
		client->set_protocol_handler(protocol_handler);
		client->start_handshake();
		client->send_nick(nick);
		client->send_version();
	}
	else {
		event_handler = new local_event_handler_impl;
		game = new tetris::game(event_handler);
	}
}

void game_leave() {
	unload_game_resources();
	
	delete game;
	game = 0;

	if (client)
		client->disconnect();
	client.reset();
	protocol_handler.reset();
}

void game_key(WPARAM key, bool pressed) {
	static bool shifted_key = false;
	if (pressed) {
		switch (key) {
			case VK_SHIFT:
				shifted_key = true;
				break;
			case VK_LEFT:
			case VK_RIGHT: {
				if (game->is_alive()) {
					s32 amount = (key == VK_RIGHT) ? 1 : -1;
					if (shifted_key)
						game->move_full(amount);
					else
						game->move(amount);
				}
			} break;
			case VK_UP:
				if (game->is_alive())
					game->rotate();
				break;
			case VK_DOWN:
				if (game->is_alive())
					game->drop_one();
				break;
			case VK_SPACE:
				if (game->is_alive())
					game->drop();
				break;
		}
	}
	else {
		switch (key) {
			case VK_ESCAPE:
				change_state(&menu_state);
				break;
			case VK_SHIFT:
				shifted_key = false;
				break;
			case 'P': {
				if (client) {
					if (game->is_paused())
						client->send_cont();
					else
						client->send_pause();
				}
				else {
					if (game->is_alive()) game->pause(!game->is_paused());
				}
			} break;
			case 'S': {
				if (client) {
					client->send_play();
				}
				else {
					game->new_game();
				}
			} break;
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9': {
				if (client)
					client->send_level(key - '0');
				else
					game->set_level(key - '0');
			} break;
		}
	}
}

state game_state = {
	bind(&game_enter),
	bind(&game_leave),
	bind(&game_key, _1, true),
	bind(&game_key, _1, false),
	bind(&game_update, _1),
};