#include "defines.h"
#include "board_util.h"
#include "tt.h"

extern bool time_over;

unsigned int getTime();

// Utility function that first checks the tt for a saved best
// If no saved best do nothing
// Otherwise check through moveGen for this best
// Which both converts the char best to a Move
// As well as ensures that the position from which the best move comes
// is reachable from our current position
// Then make the move, and write the algebraic notation to pv
// This is used in PV() within searchRoot() to do PV related functions
void doPV(char *pv) {
	// Temp board to make moves on
	Board rootb = b;

	char best;
	Move m[256];
	int mcount = 0;

	for (uint8_t depth = 1; depth <= sd.depth; depth++) {
		best = -1;
		ttAccess(0, 0, 0, &best);

		if (best == -1)
			break;

		mcount = moveGen(m, 0xFF);

		for (int i = 0; i < mcount; ++i) {
			if (m[i].id == best) {
				moveMake(m[i]);

				pv = algebraicWrite(m[i], pv);
				pv[0] = ' ';
				pv++;

				break;
			}
		}
	}

	pv[0] = '\0';
	b = rootb;
}

// perft included at the behest of the greater
// Chess programming community
//
// Walks the move generation tree (of legal moves)
// and counts move paths of a given depth
// Ignores draw rules
uint64_t perft(uint8_t depth) {
	uint64_t nodes = 0;

	if (depth == 0)
		return 1;

	Move m[256];
	int mcount = moveGen(m, 0xFF);

	for (int i = 0; i < mcount; ++i) {
		moveMake(m[i]);

		if (!isAttacked(b.side_to_move, b.king_location[!b.side_to_move]))
			nodes += perft(depth - i);

		moveUnmake(m[i]);
	}

	return nodes;
}

// Utility functions taking an std::string
// and converting to a Move
// Interstitially converting to a char*
// for convertTo0x88
// Assumes a valid algebraic move passed to it
Move strToMove(std::string &a) {
	Move m;

	char converted_a[a.size()];
	std::size_t length = a.copy(converted_a, a.size());
	converted_a[length] = '\0';
	m.from_sq = convertTo0x88(converted_a);
	m.to_sq = convertTo0x88(converted_a+2);

	m.from_piece = b.pieces[m.from_sq];
	m.to_piece = b.pieces[m.from_sq];
	m.piece_cap = b.pieces[m.to_sq];

	m.flags = 0;
	m.castle = 0;
	m.en_passant = -1;
	m.ply = 0;
	m.score = 0;

	// Promotes to a Queen by default
	if ((m.to_piece == PAWN) &&
		(row(m.to_sq) == kRow1 || row(m.to_sq) == kRow8))
		m.to_piece = QUEEN;

	int i = 4;
	switch (a[i]) {
	case 'q':
		m.to_piece = QUEEN;
		i++;
		break;
	case 'r':
		m.to_piece = ROOK;
		i++;
		break;
	case 'b':
		m.to_piece = BISHOP;
		i++;
		break;
	case 'n':
		m.to_piece = KNIGHT;
		i++;
		break;
	}
	a = a.substr(i);

	// Detects castling
	if ((m.from_piece == KING) &&
		((m.from_sq == E1 && (m.to_sq == G1 || m.to_sq == C1)) ||
		 (m.from_sq == E8 && (m.to_sq == G8 || m.to_sq == C8))))
		m.flags = MOVE_CASTLE;

	// Detects en passant
	if ((m.from_piece == PAWN) &&
		(m.piece_cap == NO_PIECE) &&
		((abs(m.from_sq-m.to_sq) == 15) || (abs(m.from_sq-m.to_sq) == 17)))
		m.flags = MOVE_EPCAPTURE;

	if ((m.from_piece == PAWN) && (abs(m.from_sq - m.to_sq) == 32))
		m.flags |= MOVE_EP;

	return m;
}

// Takes a char* representing a move, assumed to be valid
// (checked by isMove() prior, and returns false if somehow entirely invalid)
// and makes the move if it is legal
// Also resets repetition index depending on the move
bool algebraic(char *a) {
	Move m;
	bool found_match = false;

	while (a[0]) {
		if (!((a[0] >= 'a') && (a[0] <= 'h'))) {
			a++;
			continue;
		}

		if (a == nullptr)
			return false;

		std::string converted_a(a);
		m = strToMove(converted_a);

		found_match = moveIsLegal(m);

		if (found_match) {
			moveMake(m);

			if ((m.from_piece == PAWN) ||
				(moveIsCapt(m)) ||
				(m.flags == MOVE_CASTLE))
				b.repetition_i = 0;
		} else {
			break;
		}

		a += 4;
		if (a[0] == '\0')
			break;
		if (a[0] != ' ')
			a++;
	}

	return found_match;
}

// Takes a move and a char* and stores the
// move as algebraic notation in the char*,
// which is then returned
char* algebraicWrite(Move m, char *a) {
	char piece_icons[5] = {0, 'q', 'r', 'b', 'n'};

	convertFrom0x88(m.from_sq, a);
	convertFrom0x88(m.to_sq, a+2);
	a += 4;
	// Promotion
	if (m.to_piece != m.from_piece) {
		a[0] = piece_icons[m.to_piece];
		a++;
	}
	a[0] = '\0';
	return a;
}

// Converts an int8_t representing a square
// to algebraic stored in a char* (size 2)
void convertFrom0x88(int8_t sq, char *a) {
	a[0] = col(sq) + 'a';
	a[1] = row(sq) + '1';
	a[2] = '\0';
}

// Converts the first two elements of a
// char* to a int8_t representing a square
uint8_t convertTo0x88(const char *a) {
	int8_t sq;
	sq = a[0] - 'a';
	sq += (a[1] - '1') * 16;

	return sq;
}
