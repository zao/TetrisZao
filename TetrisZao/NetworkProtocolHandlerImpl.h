#pragma once
#include "Xtris.h"
#include "Tetris.h"

struct chat {
	explicit chat(u32 buffered_lines);
	void add_text(std::string const& msg);
	void clear();

	typedef std::deque<std::string>::const_iterator iterator;
	iterator begin() const;
	iterator end() const;

private:
	u32 buffered_lines;
	std::deque<std::string> messages;
};

struct network_protocol_handler_impl : xtris::client::protocol_handler {
	void on_nick(u8 sender, std::string const& nick);
	void on_play(u8 sender);
	void on_fall(u8 sender, std::vector<u8> const& lines);
	void on_draw(u8 sender, std::vector<xtris::block> const& blocks);

	void on_lost(u8 sender);
	void on_gone(u8 sender);
	void on_clear(u8 sender);
	void on_new(u8 sender, u16 wins);

	void on_lines(u8 sender, u8 n);
	void on_grow(u8 sender);
	void on_mode(u8 sender, u8 mode);
	void on_level(u8 sender, u8 level);

	void on_pause(u8 sender);
	void on_cont(u8 sender);
	void on_badvers(u8 sender, u8 major, u8 minor, u8 subminor);
	void on_msg(u8 sender, std::string const& message);

	void on_youare(u8 id);
	void on_linesto(u8 sender, u8 count, u8 victim);
	void on_won(u8 sender);
	void on_zero(u8 sender);

	// Game state
	struct state {
		struct player {
			player() : alive(true), wins(0) { }
			bool alive;
			tetris::board board;
			std::string nick;
			u16 wins;
		};
		std::map<u8, player> players;
		u8 own_id;
		bool playing;
	};

	state s;
	tetris::board board;
	tetris::game* game;
	xtris::client* client;
	::chat* chat;

	network_protocol_handler_impl(tetris::game* game, xtris::client* client, ::chat* c)
	: game(game), client(client), chat(c) { }
};