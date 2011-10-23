#include "Pch.h"
#include "Tetris.h"
#include "Helpers.h"

//#define BLOCK_DRAW_DEBUG

namespace tetris {
	board::board()
	: grid(boost::extents[range(-3, 13)][range(-3,24)]) {
		std::fill_n(grid.data(), grid.num_elements(), piece::no_piece);
		for (index c = -3; c < 13; ++c) {
			for (index r = -3; r < 24; ++r) {
				if (c < 0 || c >= 10 || r < 0)
					grid[c][r] = piece::border_piece;
			}
		}
#ifdef BLOCK_DRAW_DEBUG
		srand(42);
		for (index r = 0; r < 10; ++r) {
			for (index c = 0; c < 10; ++c)
				if (rand() % 4 == 0)
					grid[c][r] = (piece::kind)(rand() % piece::piece_count);
		}
#endif
		std::fill_n(full_line, ARRAY_COUNT(full_line), false);
	}

	board::field_view::type board::get_field() const {
		return grid[boost::indices[index_range(0, 10)][index_range(0, 24)]];
	}

	bool board::has_overlap(piece::block_view::type & blocks, index c, index r) const {
		if (c < -3 || c > 9) return true;
		field_view::type view = grid[boost::indices[index_range(c, c+4)][index_range(r, r+4)]];
		for (size_t x = 0; x < 4; ++x)
			for (size_t y = 0; y < 4; ++y)
				if (view[x][y] != piece::no_piece && blocks[x][y] != piece::no_piece)
					return true;
		return false;
	}

	void board::set_block(piece::kind kind, index x, index y) {
		assert(x >= 0 && x < 10 && y >= 0 && y < 20);
		grid[x][y] = kind;
	}

	void board::commit_blocks(piece::block_view::type & blocks, index c, index r) {
		field_slice::type view = grid[boost::indices[index_range(c, c+4)][index_range(r, r+4)]];
		for (size_t y = 0; y < 4; ++y) {
			for (size_t x = 0; x < 4; ++x)
				if (blocks[x][y] != piece::no_piece)
					view[x][y] = blocks[x][y];
			bool full = true;
			for (size_t x = 0; x < 10; ++x) {
				if (grid[x][r + y] == piece::no_piece) {
					full = false;
					break;
				}
			}
			if (full && (r + y >= 0) && (r + y < 20))
				full_line[r + y] = true;
		}
	}

	void board::insert_lines(int n) {
		index_range width(0, 10);
		boost::multi_array<piece::kind, 1> empty_line(boost::extents[10]);
		std::fill_n(empty_line.data(), 10, piece::no_piece);
		for (index r = 22; r >= 0; --r) {
			line_slice::type target = grid[boost::indices[width][r]];
			if (r >= n) {
				target = grid[boost::indices[width][r - n]];
				if (r < 20) full_line[r] = full_line[r - n];
			}
			else {
				target = empty_line;
				full_line[r] = false;
			}
		}
	}

	std::vector<board::index> board::get_full_lines() const {
		std::vector<board::index> ret;
		for (int i = 0; i < 20; ++i)
			if (full_line[i]) ret += i;
		return ret;
	}

	void board::flatten() {
		boost::multi_array<piece::kind, 1> empty_line(boost::extents[10]);
		std::fill_n(empty_line.data(), 10, piece::no_piece);
		int assign_target = 0;
		for (int r = 0; r != 20; ++r) {
			if (assign_target != r) {
				grid[boost::indices[index_range(0, 10)][assign_target]] = grid[boost::indices[index_range(0, 10)][r]];
			}
			if (full_line[r]) {
				full_line[r] = false;
			}
			else {
				++assign_target;
			}
		}
		for (; assign_target != 20; ++assign_target)
			grid[boost::indices[index_range(0, 10)][assign_target]] = empty_line;
	}

	void board::clear_line(index y) {
		boost::multi_array<piece::kind, 1> empty_line(boost::extents[10]);
		std::fill_n(empty_line.data(), 10, piece::no_piece);
		grid[boost::indices[index_range(0, 10)][y]] = empty_line;
		full_line[y] = true;
	}

	void find_field_difference(std::vector<block>& diff, board::field_view::type& before, board::field_view::type& after) {
		for (board::index r = 0; r < 20; ++r)
			for (board::index c = 0; c < 10; ++c)
				if (after[c][r] != before[c][r]) {
					block b = { c, r, after[c][r] };
					diff += b;
				}
	}
}