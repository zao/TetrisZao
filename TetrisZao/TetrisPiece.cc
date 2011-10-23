#include "Pch.h"
#include "Tetris.h"

namespace tetris {
	piece::piece(kind k)
	: piece_kind(k), orientation(0), blocks(boost::extents[4][4][4]) {
		if (k >= piece_count)
			std::fill_n(blocks.data(), blocks.num_elements(), k);
		else {
			for (index o = 0; o < 4; ++o)
				for (index r = 0; r < 4; ++r)
					for (index c = 0; c < 4; ++c)
						blocks[o][c][r] = (piece_source[k][o][r] & (1 << c)) ? k : no_piece;
		}
	}

	piece::block_view::type piece::get_block_view(index n) const {
		 return blocks[boost::indices[n][index_range()][index_range()]];
	}
	
	u8 piece::piece_source[piece::piece_count][4][4] = { {
		// i_piece
		{  0,  0, 15,  0 },
		{  4,  4,  4,  4 },
		{  0,  0, 15,  0 },
		{  4,  4,  4,  4 },
	}, {
		// o_piece
		{  0,  6,  6,  0 },
		{  0,  6,  6,  0 },
		{  0,  6,  6,  0 },
		{  0,  6,  6,  0 },
	}, {
		// t_piece
		{  0,  4, 14,  0 },
		{  0,  4, 12,  4 },
		{  0, 14,  4,  0 },
		{  0,  4,  6,  4 },
	}, {
		// l_piece
		{  0,  2, 14,  0 },
		{  0, 12,  4,  4 },
		{  0, 14,  8,  0 },
		{  0,  4,  4,  6 },
	}, {
		// j_piece
		{  0,  8, 14,  0 },
		{  0,  4,  4, 12 },
		{  0, 14,  2,  0 },
		{  0,  6,  4,  4 },
	}, {
		// s_piece
		{  0,  6, 12,  0 },
		{  0,  8, 12,  4 },
		{  0,  6, 12,  0 },
		{  0,  8, 12,  4 },
	}, {
		// z_piece
		{  0, 12,  6,  0 },
		{  0,  4, 12,  8 },
		{  0, 12,  6,  0 },
		{  0,  4, 12,  8 },
	} };
}