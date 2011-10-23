#pragma once

#include "Tetris.h"

extern asio::io_service io;

namespace xtris {
	enum opcode {
		OP_NICK = 1, OP_PLAY, OP_FALL, OP_DRAW, OP_LOST, OP_GONE, OP_CLEAR, OP_NEW,
		OP_LINES = 9, OP_GROW, OP_MODE, OP_LEVEL, OP_BOT, OP_KILL, OP_PAUSE,
		OP_CONT = 16, OP_VERSION, OP_BADVERS, OP_MSG, OP_YOUARE, OP_LINESTO, OP_WON, OP_ZERO
	};

	static std::string opnames[] = { "",
		"OP_NICK", "OP_PLAY", "OP_FALL", "OP_DRAW", "OP_LOST", "OP_GONE", "OP_CLEAR", "OP_NEW",
		"OP_LINES", "OP_GROW", "OP_MODE", "OP_LEVEL", "OP_BOT", "OP_KILL", "OP_PAUSE",
		"OP_CONT", "OP_VERSION", "OP_BADVERS", "OP_MSG", "OP_YOUARE", "OP_LINESTO", "OP_WON", "OP_ZERO",
	};

	struct block {
		u8 x, y;
		tetris::piece::kind c;
	};

	struct client : enable_shared_from_this<client> {
		struct protocol_handler {
			virtual ~protocol_handler() { }
			virtual void on_nick(u8 sender, std::string const& nick) = 0;
			virtual void on_play(u8 sender) = 0;
			virtual void on_fall(u8 sender, std::vector<u8> const& lines) = 0;
			virtual void on_draw(u8 sender, std::vector<block> const&) = 0;

			virtual void on_lost(u8 sender) = 0;
			virtual void on_gone(u8 sender) = 0;
			virtual void on_clear(u8 sender) = 0;
			virtual void on_new(u8 sender, u16 wins) = 0;

			virtual void on_lines(u8 sender, u8 n) = 0;
			virtual void on_grow(u8 sender) = 0;
			virtual void on_mode(u8 sender, u8 mode) = 0;
			virtual void on_level(u8 sender, u8 level) = 0;

			virtual void on_pause(u8 sender) = 0;
			virtual void on_cont(u8 sender) = 0;
			virtual void on_badvers(u8 sender, u8 major, u8 minor, u8 subminor) = 0;
			virtual void on_msg(u8 sender, std::string const& message) = 0;

			virtual void on_youare(u8 id) = 0;
			virtual void on_linesto(u8 sender, u8 count, u8 victim) = 0;
			virtual void on_won(u8 sender) = 0;
			virtual void on_zero(u8 sender) = 0;
		};

		client(std::string nick, std::string host, std::string port = "19503");
		
		void start_handshake();
		void disconnect();
		void set_protocol_handler(weak_ptr<protocol_handler> ph);

		void send_nick(std::string nick);
		void send_play();
		void send_fall(std::vector<tetris::board::index> lines);
		void send_draw(std::vector<tetris::block> blocks);
		void send_lost();
		void send_clear();
		void send_lines(u8 lines);
		void send_grow();
		void send_level(u8 level);
		void send_pause();
		void send_cont();
		void send_version();
		void send_msg(std::string msg);
		void send_zero();

	private:
		struct message {
			static u32 const header_length = 6;
			static u32 const max_body_length = 255;
			u8 buf[260];

			u8* data() { return buf; }
			u32 data_length() { return header_length + body_length(); }
			u8* body() { return buf + header_length; }
			u32 body_length() {
				u32 len;
				std::copy(buf, buf + 4, (u8*)&len);
				return ntohl(len) - 2;
			}
			
			u8 sender() { return buf[4]; }
			opcode opcode() { return (xtris::opcode)buf[5]; }
		} msg;

		std::queue<message*> message_queue;

		void on_sent_message(sys::error_code const& err);
		void on_read_header(sys::error_code const& err);
		void on_read_body(sys::error_code const& err);

		message* make_message(u8 opcode, u8 const* payload_data, size_t payload_size);
		void enqueue_message(message* msg);
		void handle_message(message& msg);

		tcp::socket socket;
		tcp::resolver resolver;
		weak_ptr<protocol_handler> ph;

		asio::streambuf out_buf;
	};
}