#pragma once

namespace tetris {

	struct piece {
		enum kind { i_piece, o_piece, t_piece, l_piece, j_piece, s_piece, z_piece, no_piece, piece_count = no_piece, border_piece };

		typedef boost::multi_array<kind, 3> storage;
		typedef storage::const_array_view<2> block_view;
		typedef storage::index index;
		typedef storage::index_range index_range;

		explicit piece(kind k = no_piece);

		block_view::type get_blocks() const { return get_block_view(orientation); }
		block_view::type get_rotated() const { index o = orientation + 1; return get_block_view(o % 4); }
		void rotate() { ++orientation; orientation %= 4; }

		kind piece_kind;

	private:
		static u8 piece_source[piece_count][4][4];
		block_view::type get_block_view(index n) const;
		u32 orientation;
		storage blocks;
	};

	struct board {
		typedef boost::multi_array<piece::kind, 2> storage;
		typedef storage::const_array_view<2> field_view;
		typedef storage::array_view<2> field_slice;
		typedef storage::const_array_view<1> line_view;
		typedef storage::array_view<1> line_slice;
		typedef storage::extent_range range;
		typedef storage::index index;
		typedef storage::index_range index_range;

		board();
		field_view::type get_field() const;

		bool has_overlap(piece::block_view::type& p, index x, index y) const;
		void set_block(piece::kind, index x, index y);
		void commit_blocks(piece::block_view::type& p, index x, index y);
		void insert_lines(int n);
		void flatten();
		void clear_line(index y);
		std::vector<index> get_full_lines() const;

	private:
		storage grid;
		bool full_line[20];
	};

	struct block { u8 x, y; piece::kind k; };
	void find_field_difference(std::vector<block>& diff, board::field_view::type& before, board::field_view::type& after);

	struct game {
		struct event_handler {
			virtual ~event_handler() { }
			virtual void on_lines_removed(std::vector<board::index>) = 0;
			virtual void on_updated_blocks(std::vector<block> blocks) = 0;
			virtual void on_lost_game() = 0;
			virtual void on_clear() = 0;
			virtual void on_score_increased(int) = 0;
			virtual void on_line_added() = 0;
		};

		explicit game(event_handler* eh);
		void new_game();
		bool is_alive();
		bool is_paused();
		void step(u32 t);

		board& get_board();
		piece get_current_piece();
		piece get_next_piece();
		s32 get_current_x();
		s32 get_current_y();

		void move(s32 sign);
		void move_full(s32 sign);
		void rotate();
		void drop_one();
		void drop();

		void penalty_lines(u8 n);
		void pause(bool pause);
		void set_level(u8 level);

	private:
		event_handler* eh;

		static s32 const spawn_delay;
		static s32 const fall_delay[9];

		void spawn_piece();
		bool attempt_move(bool left);
		bool attempt_drop();
		bool attempt_rotation();
		board& commit_current_to(board& b);

		struct action {
			enum kind { move_one, move_full, drop_one, drop_full, rotate } op;
			s32 val;
		};

		u8 level;
		bool paused;

		struct round_data {
			board board;
			u32 last_time;
			s32 spawn_delta, fall_delta;
			
			piece current_piece, next_piece;
			s32 current_x, current_y;

			s32 pending_penalty;
			bool paused;
			std::queue<action> actions;
		
			s32 score, lines;

			explicit round_data(u8 level, bool paused)
			: last_time(0), spawn_delta(spawn_delay), fall_delta(fall_delay[level-1]), pending_penalty(0), paused(paused), score(0), lines(0) { }
		};

		shared_ptr<round_data> data;

		boost::mt19937 rng;
		boost::uniform_int<> kind_distribution;
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > kind_generator;
		boost::uniform_int<> hole_distribution;
		boost::variate_generator<boost::mt19937&, boost::uniform_int<> > hole_generator;
	};
}