#include "Pch.h"
#include "Tetris.h"

namespace tetris {
	s32 const game::spawn_delay = 200;
	s32 const game::fall_delay[9] = { 1500, 1200, 900, 700, 500, 350, 200, 120, 80 };

	game::game(event_handler* eh)
	: eh(eh), rng((u32)time(0)), level(5), paused(false), kind_distribution(0, piece::z_piece), kind_generator(rng, kind_distribution), hole_distribution(0, 9), hole_generator(rng, hole_distribution) {
	}

	void game::new_game() {
		data.reset(new round_data(level, paused));
		spawn_piece();
	}

	bool game::is_alive() { 
		return data;
	}

	bool game::is_paused() {
		return paused;
	}

	void game::spawn_piece() {
		data->current_piece = data->next_piece;
		data->current_x = 3;
		data->current_y = 17;
		data->next_piece = piece((piece::kind)kind_generator());
		data->spawn_delta = fall_delay[level - 1];
	}

	bool game::attempt_move(bool left) {
		s32 shift = (left ? -1 : 1);
		bool success;
		if (success = !data->board.has_overlap(data->current_piece.get_blocks(), data->current_x + shift, data->current_y))
			data->current_x += shift;
		return success;
	}

	bool game::attempt_drop() {
		bool success;
		if (success = !data->board.has_overlap(data->current_piece.get_blocks(), data->current_x, data->current_y - 1))
			--data->current_y;
		return success;
	}

	bool game::attempt_rotation() {
		bool success;
		if (success = !data->board.has_overlap(data->current_piece.get_rotated(), data->current_x, data->current_y))
			data->current_piece.rotate();
		return success;
	}

	struct board_difference {
		explicit board_difference(board& b) : b(b) { }

		std::vector<block> difference(board& a);
	private:
		board b;
	};

	std::vector<block> board_difference::difference(board& a) {
		std::vector<block> ret;
		board::field_view::type const& before = b.get_field();
		board::field_view::type const& after  = a.get_field();
		for (board::index c = 0; c < 10; ++c) {
			for (board::index r = 0; r < 20; ++r) {
				if (before[c][r] != after[c][r]) {
					block b = { c, r, after[c][r] };
					ret += b;
				}
			}
		}
		return ret;
	}

	board& game::commit_current_to(board& b) {
		b.commit_blocks(data->current_piece.get_blocks(), data->current_x, data->current_y);
		return b;
	}

	void game::step(u32 t) {
		if (!data->last_time) data->last_time = t;
		s32 dt = t - data->last_time;
		data->last_time = t;
		if (paused)
			return;

		// Spawn new piece if suitable
		if (data->current_piece.piece_kind == piece::no_piece) {
			data->spawn_delta -= dt;
			if (data->spawn_delta <= 0) {
				spawn_piece();
				data->fall_delta = fall_delay[level - 1];
				std::queue<action> filtered;
				while (!data->actions.empty()) {
					action& a = data->actions.front();
					if (a.op != action::drop_one && a.op != action::drop_full)
						filtered.push(a);
					data->actions.pop();
				}
				data->actions = filtered;
			}
			return;
		}

		board_difference diff(commit_current_to(board(data->board)));

		while (!data->actions.empty()) {
			action a = data->actions.front();
			data->actions.pop();
			switch (a.op) {
				case action::move_one: attempt_move(a.val < 0); break;
				case action::move_full: while (attempt_move(a.val < 0)); break;
				case action::drop_one: attempt_drop(); break;
				case action::drop_full: while (attempt_drop()); break;
				case action::rotate: attempt_rotation(); break;
			}
		}

		bool stick = false;
		// Move piece down if suitable
		data->fall_delta -= dt;
		if (data->fall_delta < 0) {
			stick = !attempt_drop();
			data->fall_delta = fall_delay[level-1];
		}

		if (stick) {
			eh->on_updated_blocks(diff.difference(commit_current_to(data->board)));
			data->current_piece = piece(piece::no_piece);
		}
		else {
			eh->on_updated_blocks(diff.difference(commit_current_to((board)data->board)));
		}

		if (stick) {
			std::vector<board::index> scoring_lines = data->board.get_full_lines();
			if (!scoring_lines.empty()) {
				s32 score_table[] = { 40, 100, 300, 1200 };
				int score_increase = level * score_table[scoring_lines.size() - 1];
				data->score += score_increase;
				eh->on_score_increased(score_increase);
				eh->on_lines_removed(scoring_lines);
				data->board.flatten();
			}
		}

		// Check for overlap, lose if true
		if (data->board.has_overlap(data->current_piece.get_blocks(), data->current_x, data->current_y)) {
			eh->on_lost_game();
			data.reset();
			return;
		}

		if (stick && data->pending_penalty) {
			data->board.insert_lines(data->pending_penalty);
			board_difference diff(data->board);
			for (board::index r = 0; r < data->pending_penalty; ++r) {
				eh->on_line_added();	
				board::index skip = hole_generator();
				for (board::index c = 0; c < 10; ++c) {
					if (c != skip)
						data->board.set_block((piece::kind)kind_generator(), c, r);
				}
			}
			eh->on_updated_blocks(diff.difference(data->board));
			data->pending_penalty = 0;
		}
	}

	board& game::get_board() {
		return data->board;
	}

	piece game::get_current_piece() {
		return data->current_piece;
	}

	piece game::get_next_piece() {
		return data->next_piece;
	}

	s32 game::get_current_x() {
		return data->current_x;
	}

	s32 game::get_current_y() {
		return data->current_y;
	}

	void game::move(s32 sign) {
		if (paused) return;
		action a = { action::move_one, sign };
		data->actions += a;
	}

	void game::move_full(s32 sign) {
		if (paused) return;
		action a = { action::move_full, sign };
		data->actions += a;
	}

	void game::rotate() {
		if (paused) return;
		action a = { action::rotate };
		data->actions += a;
	}

	void game::drop_one() {
		if (paused) return;
		action a = { action::drop_one };
		data->actions += a;
	}

	void game::drop() {
		if (paused) return;
		action a = { action::drop_full };
		data->actions += a;
	}

	void game::penalty_lines(u8 n) {
		data->pending_penalty += n;
	}

	void game::pause(bool pause) {
		this->paused = pause;
		if (data)
			data->paused = pause;
	}

	void game::set_level(u8 level) {
		this->level = level;
	}
}