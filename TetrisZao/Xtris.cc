#include "Pch.h"
#include "Xtris.h"

asio::io_service io;

using asio::ip::tcp;

namespace xtris {
	client::client(std::string nick, std::string hostname, std::string port)
	: socket(io), resolver(io) {
		tcp::resolver::query query(hostname, port);
		sys::error_code err;
		tcp::resolver::iterator I = resolver.resolve(query, err), end;
		bool success = false;
		if (!err) {
			while (!success && I != end) {
				socket.close();
				socket.connect(*I, err);
				if (!err)
					success = true;
			}
		}
		if (!success)
			throw err;
	}

	void client::start_handshake() {
		async_read(socket,
			asio::buffer(msg.data(), message::header_length),
			bind(&client::on_read_header, shared_from_this(), _1));
	}

	void client::disconnect() {
		socket.close();
	}

	void client::set_protocol_handler(weak_ptr<client::protocol_handler> ph) {
		this->ph = ph;
	}

	client::message* client::make_message(u8 opcode, u8 const* payload_data, size_t payload_size) {
		asio::streambuf out;
		{
			std::ostream os(&out);
			u32 length_he = 1 + 1 + payload_size;
			u32 length_ne = htonl(length_he);
			os.write((char*)&length_ne, 4);
			os.put(0);
			os.put(opcode);
			if (payload_size)
				os.write((char const*)payload_data, payload_size);
		}
		message* ret = new message;
		u8 const* data = asio::buffer_cast<u8 const*>(out.data());
		size_t size = asio::buffer_size(out.data());
		std::copy(data, data + size, ret->data());
		return ret;
	}

	void client::handle_message(message& msg) {
		u8 sender = msg.sender();
		u32 bodylen = msg.body_length();
		u8 const* body = msg.body();
		OutputDebugStringA(str(format("Message (%s) from (%d)\n") % opnames[msg.opcode()] % +sender).c_str());
		shared_ptr<protocol_handler> ph = this->ph.lock();
		if (!ph) return;
		switch (msg.opcode()) {
			case OP_NICK: ph->on_nick(sender, std::string((char const*)body, bodylen)); break;
			case OP_PLAY: ph->on_play(sender); break;
			case OP_FALL: ph->on_fall(sender, std::vector<u8>(body, body + bodylen)); break;
			case OP_DRAW: {
				std::vector<block> v;
				for (size_t i = 0; i < bodylen; i += 3, body += 3) {
					block b = { body[0], body[1], (tetris::piece::kind)body[2] };
					v += b;
				}
				ph->on_draw(sender, v);
				break;
			}

			case OP_LOST: ph->on_lost(sender); break;
			case OP_GONE: ph->on_gone(sender); break;
			case OP_CLEAR: ph->on_clear(sender); break;
			case OP_NEW: {
				u16 wins = 0;
				if (bodylen >= 2)
					std::copy(body, body + 2, (u8*)&wins);
				ph->on_new(sender, ntohs(wins));
				break;
			}

			case OP_LINES: ph->on_lines(sender, body[0]); break;
			case OP_GROW: ph->on_grow(sender); break;
			case OP_MODE: ph->on_mode(sender, body[0]); break;
			case OP_LEVEL: ph->on_level(sender, body[0]); break;

			case OP_PAUSE: ph->on_pause(sender); break;
			case OP_CONT: ph->on_cont(sender); break;
			case OP_BADVERS: ph->on_badvers(sender, body[0], body[1], body[2]); break;
			case OP_MSG: ph->on_msg(sender, std::string((char const*)body, bodylen)); break;

			case OP_YOUARE: ph->on_youare(sender); break;
			case OP_LINESTO: ph->on_linesto(sender, body[0], body[1]); break;
			case OP_WON: ph->on_won(sender); break;
			case OP_ZERO: ph->on_zero(sender); break;
		}
	}

	void client::on_sent_message(sys::error_code const& err) {
		message* msg = message_queue.front();
		message_queue.pop();
		OutputDebugStringA(str(format("Message (%s) sent.\n") % opnames[msg->opcode()]).c_str());
		delete msg;
		if (!message_queue.empty()) {
			msg = message_queue.front();
			async_write(socket, asio::buffer(msg->data(), msg->data_length()),
				bind(&client::on_sent_message, shared_from_this(), _1));
		}
	}

	void client::on_read_header(sys::error_code const& err) {
		if (!err) {
			async_read(socket,
				asio::buffer(msg.body(), msg.body_length()),
				bind(&client::on_read_body, shared_from_this(), _1));
		}
	}

	void client::on_read_body(sys::error_code const& err) {
		if (!err) {
			handle_message(msg);
			async_read(socket,
				asio::buffer(msg.data(), message::header_length),
				bind(&client::on_read_header, shared_from_this(), _1));
		}
	}

	void client::enqueue_message(message* msg) {
		bool empty = message_queue.empty();
		message_queue.push(msg);
		if (empty) {
			message* msg = message_queue.front();
			async_write(socket, asio::buffer(msg->data(), msg->data_length()),
				bind(&client::on_sent_message, shared_from_this(), _1));
		}
	}

	void client::send_nick(std::string nick) {
		enqueue_message(make_message(OP_NICK, (u8 const*)nick.data(), nick.size()));
	}

	void client::send_play() {
		enqueue_message(make_message(OP_PLAY, 0, 0));
	}

	void client::send_fall(std::vector<tetris::board::index> lines) {
		std::vector<u8> xtris_lines;
		xtris_lines.reserve(lines.size());
		BOOST_FOREACH(tetris::board::index i, lines)
			xtris_lines.push_back(19 - i);
		enqueue_message(make_message(OP_FALL, &xtris_lines[0], xtris_lines.size()));
	}

	void client::send_draw(std::vector<tetris::block> blocks) {
		std::vector<u8> draws;
		draws.reserve(blocks.size());
		BOOST_FOREACH(tetris::block b, blocks)
			push_back(draws)(b.x)(19 - b.y)(b.k);
		enqueue_message(make_message(OP_DRAW, &draws[0], draws.size()));
	}

	void client::send_lost() {
		enqueue_message(make_message(OP_LOST, 0, 0));
	}

	void client::send_clear() {
		enqueue_message(make_message(OP_CLEAR, 0, 0));
	}

	void client::send_lines(u8 n) {
		enqueue_message(make_message(OP_LINES, &n, 1));
	}

	void client::send_grow() {
		enqueue_message(make_message(OP_GROW, 0, 0));
	}

	void client::send_level(u8 level) {
		enqueue_message(make_message(OP_LEVEL, &level, 1));
	}

	void client::send_pause() {
		enqueue_message(make_message(OP_PAUSE, 0, 0));
	}

	void client::send_cont() {
		enqueue_message(make_message(OP_CONT, 0, 0));
	}

	void client::send_version() {
		u8 version[] = { 1, 0, 1 };
		enqueue_message(make_message(OP_VERSION, version, 3));
	}

	void client::send_msg(std::string msg) {
		enqueue_message(make_message(OP_MSG, (u8 const*)msg.data(), msg.size()));
	}

	void client::send_zero() {
		enqueue_message(make_message(OP_ZERO, 0, 0));
	}
}