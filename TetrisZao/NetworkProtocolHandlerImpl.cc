#include "Pch.h"
#include "NetworkProtocolHandlerImpl.h"
#include "State.h"
#include "MenuState.h"

chat::chat(u32 buffered_lines)
: buffered_lines(buffered_lines) {
}

void chat::add_text(const std::string &msg) {
	if (messages.size() >= buffered_lines) {
		messages.pop_front();
	}
	messages.push_back(msg);
}

void chat::clear() {
	messages.clear();
}

chat::iterator chat::begin() const {
	return messages.begin();
}

chat::iterator chat::end() const {
	return messages.end();
}

void network_protocol_handler_impl::on_nick(u8 sender, std::string const& nick) {
	s.players[sender].nick = nick;
}

void network_protocol_handler_impl::on_play(u8 sender) {
	game->new_game();
	client->send_clear();
}

void network_protocol_handler_impl::on_fall(u8 sender, std::vector<u8> const& lines) {
	tetris::board& board = s.players[sender].board;
	BOOST_FOREACH(u8 y, lines)
		board.clear_line(19 - y);
	board.flatten();
}

void network_protocol_handler_impl::on_draw(u8 sender, std::vector<xtris::block> const& blocks) {
	tetris::board& board = s.players[sender].board;
	BOOST_FOREACH(xtris::block b, blocks) {
		board.set_block(b.c, b.x, 19 - b.y);
	}
}


void network_protocol_handler_impl::on_lost(u8 sender) {
	if (sender != s.own_id)
		s.players[sender].alive = false;
}

void network_protocol_handler_impl::on_gone(u8 sender) {
	s.players.erase(sender);
}

void network_protocol_handler_impl::on_clear(u8 sender) {
	if (sender != s.own_id)
		s.players[sender].board = tetris::board();
}

void network_protocol_handler_impl::on_new(u8 sender, u16 wins) {
	s.players[sender].wins = wins;
}

void network_protocol_handler_impl::on_lines(u8 sender, u8 n) {
	game->penalty_lines(n);
}

void network_protocol_handler_impl::on_grow(u8 sender) {
	s.players[sender].board.insert_lines(1);
}

void network_protocol_handler_impl::on_mode(u8 sender, u8 mode) {
	if (mode != 0) {
		client->disconnect();
		change_state(&menu_state);
	}
}

void network_protocol_handler_impl::on_level(u8 sender, u8 level) {
	game->set_level(level);
}

void network_protocol_handler_impl::on_pause(u8 sender) {
	game->pause(true);
}

void network_protocol_handler_impl::on_cont(u8 sender) {
	game->pause(false);
}

void network_protocol_handler_impl::on_badvers(u8 sender, u8 major, u8 minor, u8 subminor) {
	client->disconnect();
	change_state(&menu_state);
}

void network_protocol_handler_impl::on_msg(u8 sender, std::string const& message) {
	chat->add_text(str(format("<%s> %s") % s.players[sender].nick % message));
}


void network_protocol_handler_impl::on_youare(u8 id) {
	s.own_id = id;
}

void network_protocol_handler_impl::on_linesto(u8 sender, u8 count, u8 victim) {
	//TODO: Notify about lines added
}

void network_protocol_handler_impl::on_won(u8 sender) {
	if (sender != s.own_id)
		++s.players[sender].wins;
}

void network_protocol_handler_impl::on_zero(u8 sender) {
	for (std::map<u8, state::player>::iterator I = s.players.begin(); I != s.players.end(); ++I)
		I->second.wins = 0;
}