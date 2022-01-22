#include "defines.h"
#include "board_util.h"

EvalData e;

int index_white[64] = {
	A8, B8, C8, D8, E8, F8, G8, H8,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A1, B1, C1, D1, E1, F1, G1, H1,
};

int index_black[64] = {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8,
};

// Bonus for captures towards center
// 2nd rank
// a3/h3
// Center occupation
// Penalty for center pawns on starting square
int pawn_middle_pcsq[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	-6,  -4,   1,   1,   1,   1,  -4,  -6,
	-6,  -4,   1,   2,   2,   1,  -4,  -6,
	-6,  -4,   2,   8,   8,   2,  -4,  -6,
	-6,  -4,   5,  10,  10,   5,  -4,  -6,
	-4,  -4,   1,   5,   5,   1,  -4,  -4,
	-6,  -4,   1, -24, -24,   1,  -4,  -6,
	 0,   0,   0,   0,   0,   0,   0,   0
};

int pawn_end_pcsq[64] = {
	 0,   0,   0,   0,   0,   0,   0,   0,
	-6,  -4,   1,   1,   1,   1,  -4,  -6,
	-6,  -4,   1,   2,   2,   1,  -4,  -6,
	-6,  -4,   2,   8,   8,   2,  -4,  -6,
	-6,  -4,   5,  10,  10,   5,  -4,  -6,
	-4,  -4,   1,   5,   5,   1,  -4,  -4,
	-6,  -4,   1, -24, -24,   1,  -4,  -6,
	 0,   0,   0,   0,   0,   0,   0,   0
};


// Bonus for being near center
// Penalty for being on any edge
int knight_middle_pcsq[64] = {
	-8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,
	-8,   0,   0,   0,   0,   0,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   1,   2,   2,   1,   0,  -8,
       -16, -12,  -8,  -8,  -8,  -8, -12, -16
};

int knight_end_pcsq[64] = {
	-8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,
	-8,   0,   0,   0,   0,   0,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   6,   8,   8,   6,   0,  -8,
	-8,   0,   4,   6,   6,   4,   0,  -8,
	-8,   0,   1,   2,   2,   1,   0,  -8,
       -16, -12,  -8,  -8,  -8,  -8, -12, -16
};

// Bonuses for being on own side
// Center (smaller than knight)
// Penalty for undeveloped
int bishop_middle_pcsq[64] = {
	-4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
	-4,   0,   0,   0,   0,   0,   0,  -4,
	-4,   0,   2,   4,   4,   2,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   1,   2,   4,   4,   2,   1,  -4,
	-4,  -4, -12,  -4,  -4, -12,  -4,  -4
};

int bishop_end_pcsq[64] = {
	-4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
	-4,   0,   0,   0,   0,   0,   0,  -4,
	-4,   0,   2,   4,   4,   2,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   0,   4,   6,   6,   4,   0,  -4,
	-4,   1,   2,   4,   4,   2,   1,  -4,
	-4,  -4, -12,  -4,  -4, -12,  -4,  -4
};

// Bonuses for 7th and 8th rank
// Small one for center
// Penalty for being on undeveloped columns
// TODO investigate this penalty
int rook_middle_pcsq[64] = {
	5,   5,   5,   5,   5,   5,   5,   5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
        0,   0,   0,   2,   2,   0,   0,   0
};

int rook_end_pcsq[64] = {
	5,   5,   5,   5,   5,   5,   5,   5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
        0,   0,   0,   2,   2,   0,   0,   0
};

// Small center bonus
// 1st rank penalty
int queen_middle_pcsq[64] = {
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   1,   1,   1,   1,   0,   0,
	0,   0,   1,   2,   2,   1,   0,   0,
	0,   0,   2,   3,   3,   2,   0,   0,
	0,   0,   2,   3,   3,   2,   0,   0,
	0,   0,   1,   2,   2,   1,   0,   0,
	0,   0,   1,   1,   1,   1,   0,   0,
       -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5
};

int queen_end_pcsq[64] = {
	0,   0,   0,   0,   0,   0,   0,   0,
	0,   0,   1,   1,   1,   1,   0,   0,
	0,   0,   1,   2,   2,   1,   0,   0,
	0,   0,   2,   3,   3,   2,   0,   0,
	0,   0,   2,   3,   3,   2,   0,   0,
	0,   0,   1,   2,   2,   1,   0,   0,
	0,   0,   1,   1,   1,   1,   0,   0,
       -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5
};

// Penalty for being anywhere not tucked away
// Except in endgame, with a center bonus
int king_middle_pcsq[64] = {
	-40, -30, -50, -70, -70, -50, -30, -40,
	-30, -20, -40, -60, -60, -40, -20, -30,
	-20, -10, -30, -50, -50, -30, -10, -20,
	-10,   0, -20, -40, -40, -20,   0, -10,
	  0,  10, -10, -30, -30, -10,  10,   0,
	 10,  20,   0, -20, -20,   0,  20,  10,
	 30,  40,  20,   0,   0,  20,  40,  30,
	 40,  50,  30,  10,  10,  30,  50,  40
};

int king_end_pcsq[64] = {
	-72, -48, -36, -24, -24, -36, -48, -72,
	-48, -24, -12,   0,   0, -12, -24, -48,
	-36, -12,   0,  12,  12,   0, -12, -36,
	-24,   0,  12,  24,  24,  12,   0, -24,
	-24,   0,  12,  24,  24,  12,   0, -24,
	-36, -12,   0,  12,  12,   0, -12, -36,
	-48, -24, -12,   0,   0, -12, -24, -48,
	-72, -48, -36, -24, -24, -36, -48, -72
};

// Larger penalty in the center
int weak_pawn_pcsq[64] = {
	0,   0,   0,   0,   0,   0,   0,   0,
      -10, -12, -14, -16, -16, -14, -12, -10,
      -10, -12, -14, -16, -16, -14, -12, -10,
      -10, -12, -14, -16, -16, -14, -12, -10,
      -10, -12, -14, -16, -16, -14, -12, -10,
      -10, -12, -14, -16, -16, -14, -12, -10,
      -10, -12, -14, -16, -16, -14, -12, -10,
	0,   0,   0,   0,   0,   0,   0,   0,
};

int passed_pawn_pcsq[64] = {
	0,   0,   0,   0,   0,   0,   0,   0,
      140, 140, 140, 140, 140, 140, 140, 140,
       92,  92,  92,  92,  92,  92,  92,  92,
       56,  56,  56,  56,  56,  56,  56,  56,
       32,  32,  32,  32,  32,  32,  32,  32,
       20,  20,  20,  20,  20,  20,  20,  20,
        0,   0,   0,   0,   0,   0,   0,   0
};

void setDefaultEval() {
	setBasicValues();
	setNearKing();
	setPcsq();
}

void setBasicValues() {
	// Material values by IM Larry Kaufman
	e.PIECE_VALUE[KING] = 0;
	e.PIECE_VALUE[QUEEN] = 975;
	e.PIECE_VALUE[ROOK] = 500;
	e.PIECE_VALUE[BISHOP] = 335;
	e.PIECE_VALUE[KNIGHT] = 325;
	e.PIECE_VALUE[PAWN] = 100;

	e.BISHOP_PAIR = 30;
	e.P_KNIGHT_PAIR = 8;
	e.P_ROOK_PAIR = 16;

	// Sort value is equal to piece value except for the king
	for (int i = 0; i < 6; ++i) {
		e.SORT_VALUE[i] = e.PIECE_VALUE[i];
	}
	e.SORT_VALUE[KING] = kSortKing;

	e.P_KING_BLOCKS_ROOK = 24;
	e.P_BLOCK_CENTRAL_PAWN = 24;
	e.P_BISHOP_TRAPPED_A7 = 150;
	e.P_BISHOP_TRAPPED_A6 = 50;
	e.P_KNIGHT_TRAPPED_A8 = 150;
	e.P_KNIGHT_TRAPPED_A7 = 100;

	e.SHIELD_2 = 10;
	e.SHIELD_3 = 5;
	e.P_NO_SHIELD = 10;

	e.ROOK_OPEN = 10;
	e.ROOK_HALF = 5;

	e.ENDGAME_MAT = 1300;
}

void setPcsq() {
	for (int i = 0; i < 64; ++i) {
		e.weak_pawn[WHITE][index_white[i]] = weak_pawn_pcsq[i];
		e.weak_pawn[BLACK][index_black[i]] = weak_pawn_pcsq[i];
		e.passed_pawn[WHITE][index_white[i]] = passed_pawn_pcsq[i];
		e.passed_pawn[BLACK][index_black[i]] = passed_pawn_pcsq[i];

		e.protected_passer[WHITE][index_white[i]] = (passed_pawn_pcsq[i] * 10) / 8;
		e.protected_passer[BLACK][index_black[i]] = (passed_pawn_pcsq[i] * 10) / 8;

		e.middle_table[PAWN][WHITE][index_white[i]] = pawn_middle_pcsq[i];
		e.middle_table[PAWN][BLACK][index_black[i]] = pawn_middle_pcsq[i];
		e.middle_table[KNIGHT][WHITE][index_white[i]] = knight_middle_pcsq[i];
		e.middle_table[KNIGHT][BLACK][index_black[i]] = knight_middle_pcsq[i];
		e.middle_table[BISHOP][WHITE][index_white[i]] = bishop_middle_pcsq[i];
		e.middle_table[BISHOP][BLACK][index_black[i]] = bishop_middle_pcsq[i];
		e.middle_table[ROOK][WHITE][index_white[i]] = rook_middle_pcsq[i];
		e.middle_table[ROOK][BLACK][index_black[i]] = rook_middle_pcsq[i];
		e.middle_table[QUEEN][WHITE][index_white[i]] = queen_middle_pcsq[i];
		e.middle_table[QUEEN][BLACK][index_black[i]] = queen_middle_pcsq[i];
		e.middle_table[KING][WHITE][index_white[i]] = king_middle_pcsq[i];
		e.middle_table[KING][BLACK][index_black[i]] = king_middle_pcsq[i];

		// TODO, change to update endgame tables
		e.middle_table[PAWN][WHITE][index_white[i]] = pawn_end_pcsq[i];
		e.middle_table[PAWN][BLACK][index_black[i]] = pawn_end_pcsq[i];
		e.middle_table[KNIGHT][WHITE][index_white[i]] = knight_end_pcsq[i];
		e.middle_table[KNIGHT][BLACK][index_black[i]] = knight_end_pcsq[i];
		e.middle_table[BISHOP][WHITE][index_white[i]] = bishop_end_pcsq[i];
		e.middle_table[BISHOP][BLACK][index_black[i]] = bishop_end_pcsq[i];
		e.middle_table[ROOK][WHITE][index_white[i]] = rook_end_pcsq[i];
		e.middle_table[ROOK][BLACK][index_black[i]] = rook_end_pcsq[i];
		e.middle_table[QUEEN][WHITE][index_white[i]] = queen_end_pcsq[i];
		e.middle_table[QUEEN][BLACK][index_black[i]] = queen_end_pcsq[i];
		e.middle_table[KING][WHITE][index_white[i]] = king_end_pcsq[i];
		e.middle_table[KING][BLACK][index_black[i]] = king_end_pcsq[i];
	}
}

void setNearKing() {
	for (int i = 0; i < 128; ++i) {
		for (int j = 0; j < 128; ++j) {
			e.near_king[WHITE][i][j] = 0;
			e.near_king[BLACK][i][j] = 0;

			if (isSq(i) && isSq(j)) {
				// squares right in front
				if (j == i + kNorth || j == i + kSouth
						|| j == i + kEast || j == i + kWest
						|| j == i + kNW || j == i + kNE
						|| j == i + kSW || j == i + kSE) {
					e.near_king[WHITE][i][j] = 1;
					e.near_king[BLACK][i][j] = 1;
				}

				// squares in front of right in front (WHITE)
				if (j == i + kNorth + kNorth
						|| j == i + kNorth + kNE
						|| j == i + kNorth + kNW)
					e.near_king[WHITE][i][j] = 1;

				// squares in front of right in front (BLACK)
				if (j == i + kSouth + kSouth
						|| j == i + kSouth + kSE
						|| j == i + kSouth + kSW)
					e.near_king[BLACK][i][j] = 1;
			}
		}
	}
}
