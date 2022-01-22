// A lot of specific eval numbers gleaned from open source engines
// Including Crafty, CPW, and Rebel
// Used here and in eval_init.cc

#include "defines.h"
#include "board_util.h"
#include "eval.h"
#include "tt.h"

static const int kRank7[2] = {row(A7), row(A2)};
static const int kRank8[2] = {row(A8), row(A1)};
static const int kOneUp[2] = {kNorth, kSouth};
static const int kOneBack[2] = {kSouth, kNorth};

static const int black_squares[128] = {
	A8, B8, C8, D8, E8, F8, G8, H8, -1, -1, -1, -1, -1, -1, -1, -1,
	A7, B7, C7, D7, E7, F7, G7, H7, -1, -1, -1, -1, -1, -1, -1, -1,
	A6, B6, C6, D6, E6, F6, G6, H6, -1, -1, -1, -1, -1, -1, -1, -1,
	A5, B5, C5, D5, E5, F5, G5, H5, -1, -1, -1, -1, -1, -1, -1, -1,
	A4, B4, C4, D4, E4, F4, G4, H4, -1, -1, -1, -1, -1, -1, -1, -1,
	A3, B3, C3, D3, E3, F3, G3, H3, -1, -1, -1, -1, -1, -1, -1, -1,
	A2, B2, C2, D2, E2, F2, G2, H2, -1, -1, -1, -1, -1, -1, -1, -1,
	A1, B1, C1, D1, E1, F1, G1, H1, -1, -1, -1, -1, -1, -1, -1, -1
};

// Alters piece value depending on number of pawns
int pawns_to_knight[9] = {-20, -16, -12, -8, -4, 0, 4, 8, 12};
int pawns_to_rook[9] = {15, 12, 9, 6, 3, 0, -3, -6, -9};

static const int safety_table[100] = {
	0, 0, 1, 2, 3, 5, 7, 9, 12, 15,
	18, 22, 26, 30, 35, 39, 44, 50, 56, 62,
	68, 75, 82, 85, 89, 97, 105, 113, 122, 131,
	140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
	260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
	377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
	494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
	500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};

struct EvalVector {
	int gamePhase; // depends on material count
	int mgMob[2]; 
	int egMob[2];
	int attCnt[2]; // number of pieces that attack near enemy king
	int attWeight[2];
	int mgTropism[2]; // midgame distance from king to enemy pieces
	int egTropism[2]; // endgame of the same
	int kingShield[2];
	int adjustMaterial[2];
	int blockages[2];
	int positionalThemes[2];
} v;

// Returns the mirrored square if black is passed
int8_t mirrorSquare(char cl, int8_t sq) {
	return cl == WHITE ? sq : black_squares[sq];	
}

// Main eval loop - calls eval functions for each piece as well
int eval(int alpha, int beta, int use_hash) {
	int result = 0, mgScore = 0, egScore = 0;

	// First check the hash table
	int probeval = ettAccess();
	if (probeval != kInvalid && use_hash)
		return probeval;

	// Clear all eval data
	v.gamePhase = b.piece_count[WHITE][KNIGHT] + b.piece_count[WHITE][BISHOP]
	       	+ b.piece_count[WHITE][ROOK] + b.piece_count[WHITE][QUEEN]
       		+ b.piece_count[BLACK][KNIGHT] + b.piece_count[BLACK][BISHOP]
		+ b.piece_count[BLACK][ROOK] + b.piece_count[BLACK][QUEEN];		

	for (int i = 0; i < 2; ++i) {
		v.mgMob[i] = 0;
		v.egMob[i] = 0;
		v.attCnt[i] = 0;
		v.attWeight[i] = 0;
		v.mgTropism[i] = 0;
		v.egTropism[i] = 0;
		v.adjustMaterial[i] = 0;
		v.blockages[i] = 0;
		v.positionalThemes[i] = 0;
		v.kingShield[i] = 0;
	}

	// Start with material scores and piece/square scores
	mgScore = b.piece_material[WHITE] + b.pawn_material[WHITE] + b.middle_pcsq[WHITE]
		- b.piece_material[BLACK] - b.pawn_material[BLACK] - b.middle_pcsq[BLACK];
	egScore = b.piece_material[WHITE] + b.pawn_material[WHITE] + b.end_pcsq[WHITE]
		- b.piece_material[BLACK] - b.pawn_material[BLACK] - b.end_pcsq[BLACK];

	// Continue with pawn shields around king and potentially blocked pieces
	v.kingShield[WHITE] = kingShield(WHITE);
	v.kingShield[BLACK] = kingShield(BLACK);
	blockedPieces(WHITE);
	blockedPieces(BLACK);
	mgScore += (v.kingShield[WHITE] - v.kingShield[BLACK]);

	// Add bonuses/penalties for piece combos	
	if (b.piece_count[WHITE][BISHOP] > 1) 
		v.adjustMaterial[WHITE] += e.BISHOP_PAIR;
	if (b.piece_count[BLACK][BISHOP] > 1) 
		v.adjustMaterial[BLACK] += e.BISHOP_PAIR;
	if (b.piece_count[WHITE][KNIGHT] > 1) 
		v.adjustMaterial[WHITE] += e.P_KNIGHT_PAIR;
	if (b.piece_count[BLACK][KNIGHT] > 1) 
		v.adjustMaterial[BLACK] += e.P_KNIGHT_PAIR;	
	if (b.piece_count[WHITE][ROOK] > 1) 
		v.adjustMaterial[WHITE] += e.P_ROOK_PAIR;
	if (b.piece_count[BLACK][ROOK] > 1) 
		v.adjustMaterial[BLACK] += e.P_ROOK_PAIR;

	v.adjustMaterial[WHITE] += pawns_to_knight[b.piece_count[WHITE][PAWN]] * b.piece_count[WHITE][KNIGHT];
	v.adjustMaterial[BLACK] += pawns_to_knight[b.piece_count[BLACK][PAWN]] * b.piece_count[BLACK][KNIGHT];
	v.adjustMaterial[WHITE] += pawns_to_rook[b.piece_count[WHITE][PAWN]] * b.piece_count[WHITE][ROOK];
	v.adjustMaterial[BLACK] += pawns_to_rook[b.piece_count[BLACK][PAWN]] * b.piece_count[BLACK][ROOK];

	// Do pawn business
	result += getPawnScore();

	// Evaluate pieces
	for (uint8_t row = 0; row < 8; ++row) {
		for (uint8_t col = 0; col < 8; ++col) {
			int8_t sq = toSq(row, col);

			if (b.colors[sq] != NO_COLOR) {
				switch (b.pieces[sq]) {
				case PAWN:
					break;
				case KNIGHT:
					evalKnight(sq, b.colors[sq]);
					break;
				case BISHOP:
					evalBishop(sq, b.colors[sq]);
					break;
				case ROOK:
					evalRook(sq, b.colors[sq]);
					break;
				case QUEEN:
					evalQueen(sq, b.colors[sq]);
					break;
				case KING:
					break;
				}
			}
		}
	}

	// Combine midgame and endgame scores
	mgScore += (v.mgMob[WHITE] - v.mgMob[BLACK]);
	egScore += (v.egMob[WHITE] - v.egMob[BLACK]);
	mgScore += (v.mgTropism[WHITE] - v.mgTropism[BLACK]);
	egScore += (v.egTropism[WHITE] - v.egTropism[BLACK]);
	if (v.gamePhase > 24) 
		v.gamePhase = 24;
	int mgWeight = v.gamePhase;
	int egWeight = 24 - mgWeight;
	result += ((mgScore * mgWeight) + (egScore * egWeight)) / 24;

	// Add non-phase-dependent scores
	result += (v.blockages[WHITE] - v.blockages[BLACK]);
	result += (v.positionalThemes[WHITE] - v.positionalThemes[BLACK]);
	result += (v.adjustMaterial[WHITE] - v.adjustMaterial[BLACK]);

	// Index the safety table based on attack weight
	// Not done if less than two attackers
	if (v.attCnt[WHITE] < 2) 
		v.attWeight[WHITE] = 0;
	if (v.attCnt[BLACK] < 2) 
		v.attWeight[BLACK] = 0;
	result += safety_table[v.attWeight[WHITE]];
	result -= safety_table[v.attWeight[BLACK]];

	// Return score relative to the side to move
	if (b.side_to_move == BLACK) result = -result;

	ettSave(result);

	return result;
}

// PIECE/PAWN EVAL FUNCTIONS
// Critical that anything altering result score is updated frequently
// for any minor change in position, and based on as many factors as possible
// Otherwise risk getting stuck in a particular evaluation for several moves
// Which also leads to a loop when the engine plays itself
//
// Each eval*PIECE/PAWN*() function returns an evaluation value
// for a given side's *PIECE/PAWN*

// PAWN EVAL
// First check the pawn hash table
// Call evalPawnStructure() if no hash yet
int getPawnScore() {
	int result;

	int probeval = pttAccess();
	if (probeval != kInvalid)
		return probeval;

	result = evalPawnStructure();
	pttSave(result);
	return result;
}

// Calls evalPawn() for each pawn on the board
int evalPawnStructure() {
	int result = 0;

	for (uint8_t row = 0; row < 8; ++row) {
		for (uint8_t col = 0; col < 8; ++col) {
			int8_t sq = toSq(row, col);

			if (b.pieces[sq] == PAWN) {
				if (b.colors[sq] == WHITE)
					result += evalPawn(sq, WHITE);
				else
					result -= evalPawn(sq, BLACK);
			}
		}
	}	

	return result;
}

int evalPawn(int8_t sq, int8_t side) {
	int result = 0;
	int flagIsPassed = 1;
	int flagIsWeak = 1;
	int flagIsOpposed = 0;


	// Checks for doubled and opposed pawns
	// Also updates if not a passer
	int8_t nextSq = sq;
	while (isSq(nextSq)) {
		if (b.pieces[nextSq] == PAWN) {
			flagIsPassed = 0;
			if (b.colors[nextSq] == side)	
				result -= 20;
			else
				flagIsOpposed = 1;
		}

		if (b.pawn_control[!side][nextSq])
			flagIsPassed = 0;

		nextSq += kOneUp[side];
	}

	// Start one ahead of current pawn
	// Such that the backwards loop will not
	// flag a pawn duo as weak
	nextSq = sq + kOneUp[side];

	// Backwards check for pawn support
	// (One is enough)
	while (isSq(nextSq)) {
		if (b.pawn_control[side][nextSq]) {
			flagIsWeak = 0;
			break;
		}

		nextSq += kOneBack[side];
	}

	// Passed pawn scoring
	// Higher for protected passer or potential protected passer
	if (flagIsPassed) {
		if (isPawnSupported(sq, side))
			result += e.protected_passer[side][sq];
		else
			result += e.passed_pawn[side][sq];
	}

	// Weak pawn scoring
	if (flagIsWeak) {
		result += e.weak_pawn[side][sq];
		if (!flagIsOpposed)
			result -= 4;
	}

	return result;
}

// TODO Incorporate this helper into the check for pawn support
// in evalPawn() (line 260)
bool isPawnSupported(int8_t sq, int8_t side) {
	int step;
	if (side == WHITE)
		step = kSouth;
	else
		step = kNorth;

	if (isSq(sq + kWest) && isPiece(side, PAWN, sq + kWest))
		return true;
	if (isSq(sq + kEast) && isPiece(side, PAWN, sq + kEast))
		return true;
	if (isSq(sq + step + kWest) && isPiece(side, PAWN, sq + step + kWest))
		return true;
	if (isSq(sq + step + kEast) && isPiece(side, PAWN, sq + step + kEast))
		return true;

	return false;
}

void evalKnight(int8_t sq, int8_t side) {
	int att = 0;
	int mob = 0;
	int pos;

	// Collect mobility and attack data
	for (uint8_t i = 0; i < 8; ++i) {
		pos = sq + directions[KNIGHT][i];
		if (isSq(pos) && b.colors[pos] != side) {
			if (!b.pawn_control[!side][pos]) 
				mob++;
			if (e.near_king[!side][b.king_location[!side]][pos])
				att++;
		}
	}
	
	v.mgMob[side] += 4 * (mob - 4);
	v.egMob[side] += 4 * (mob - 4);
	
	// Save king attack data
	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 2 * att;
	}

	int tropism = getTropism(sq, b.king_location[!side]);
	v.mgTropism[side] += 3 * tropism;
	v.egTropism[side] += 3 * tropism;
}

void evalBishop(int8_t sq, int8_t side) {
	int att = 0;
	int mob = 0;

	for (char i = 0; i < dir_sizes[BISHOP]; ++i) {
		for (char pos = sq;;) {
			pos = pos + directions[BISHOP][i];
			if (!isSq(pos))
				break;
			if (b.pieces[pos] == NO_PIECE) {
				if (!b.pawn_control[!side][pos])
					mob++;
				if (e.near_king[!side][b.king_location[!side]][pos])
					att++;
				else {
					mob++;
					if (e.near_king[!side][b.king_location[!side]][pos])
						++att;
					break;
				}
			}

		}
	}

	v.mgMob[side] += 3 * (mob - 7);
	v.egMob[side] += 3 * (mob - 7);

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 2 * att;
	}

	int tropism = getTropism(sq, b.king_location[!side]);
	v.mgTropism[side] += 2 * tropism;
	v.egTropism[side] += 1 * tropism;
}

void evalRook(int8_t sq, int8_t side) {
	int att = 0;
	int mob = 0;

	// Seventh rank bonus for attacking pawns or cutting king
	if (row(sq) == kRank7[side]
		&& (b.pawns_rank[!side][kRank7[side]] 
		|| row(b.king_location[!side]) == kRank8[side])) {
		v.mgMob[side] += 20;
		v.egMob[side] += 30;
	}

	// Bonus for open and semi-open files added to mobility score
	// Bonus for open files targetting enemy king added to attWeight
	if (b.pawns_files[side][col(sq)] == 0) {
		if (b.pawns_files[!side][col(sq)] == 0) {
			v.mgMob[side] += e.ROOK_OPEN;
			v.egMob[side] += e.ROOK_OPEN;
			if (abs(col(sq) - col(b.king_location[!side])) < 2)
				v.attWeight[side] += 1;
		} else {
			v.mgMob[side] += e.ROOK_HALF;
			v.egMob[side] += e.ROOK_HALF;
			if (abs(col(sq) - col(b.king_location[!side])) < 2)
				v.attWeight[side] += 2;
		}
	}

	// Mobility and king attacks
	for (char i = 0; i < dir_sizes[ROOK]; ++i) {
		for (char pos = sq;;) {
			pos = pos + directions[ROOK][i];
			if (!isSq(pos))
				break;

			if (b.pieces[pos] == NO_PIECE) {
				mob++;
				if (e.near_king[!side][b.king_location[!side]][pos])
					++att;
				else {
					mob++;
					if (e.near_king[!side][b.king_location[!side]][pos])
						++att;
					break;
				}
			}
		}
	}

	v.mgMob[side] += 2 * (mob - 7);
	v.egMob[side] += 4 * (mob - 7);

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 3*att;
	}

	int tropism = getTropism(sq, b.king_location[!side]);
	v.mgTropism[side] += 2 * tropism;
	v.egTropism[side] += 1 * tropism;
}

void evalQueen(int8_t sq, int8_t side) {
	int att = 0;
	int mob = 0;

	// Same file bonus as rooks
	if (row(sq) == kRank7[side]
		&& (b.pawns_rank[!side][kRank7[side]] 
		|| row(b.king_location[!side]) == kRank8[side])) {
		v.mgMob[side] += 5;
		v.egMob[side] += 10;
	}

	// Penalty for developing too early
	// Can't be based on gamephase as it changes too infrequently
	if ((side == WHITE && row(sq) > kRow2) || (side == BLACK && row(sq) < kRow7)) {
		if (isPiece(side, KNIGHT, mirrorSquare(side, B1))) 
			v.positionalThemes[side] -= 2;
		if (isPiece(side, BISHOP, mirrorSquare(side, C1))) 
			v.positionalThemes[side] -= 2;
		if (isPiece(side, BISHOP, mirrorSquare(side, F1))) 
			v.positionalThemes[side] -= 2;
		if (isPiece(side, KNIGHT, mirrorSquare(side, G1))) 
			v.positionalThemes[side] -= 2;
	}

	// Collect mobility and attacks data
	for (char i = 0; i < dir_sizes[QUEEN]; ++i) {
		for (char pos = sq;;) {
			pos = pos + directions[QUEEN][i];
			if (!isSq(pos))
				break;

			if (b.pieces[pos] == NO_PIECE) {
				mob++;
				if (e.near_king[!side][b.king_location[!side]][pos])
					++att;
				else {
					mob++;
					if (e.near_king[!side][b.king_location[!side]][pos])
						++att;
					break;
				}
			}
		}
	}

	v.mgMob[side] += 1 * (mob - 14);
	v.egMob[side] += 2 * (mob - 14);

	if (att) {
		v.attCnt[side]++;
		v.attWeight[side] += 4 * att;
	}

	int tropism = getTropism(sq, b.king_location[!side]);
	v.mgTropism[side] += 2 * tropism;
	v.egTropism[side] += 4 * tropism;
}

// Checks for pawns adjacent to or near a given side's King
// Returning a number to incoporate into overall evaluation
int kingShield(int8_t side) {
	int result = 0;
	if (side == WHITE) {
		// kingside
		if (col(b.king_location[WHITE]) > kColE) {
			if (isPiece(WHITE, PAWN, F2)) result += e.SHIELD_2;
			else if (isPiece(WHITE, PAWN, F3)) result += e.SHIELD_3;
	
			if (isPiece(WHITE, PAWN, G2)) result += e.SHIELD_2;
			else if (isPiece(WHITE, PAWN, G3)) result += e.SHIELD_3;
	
			if (isPiece(WHITE, PAWN, H2)) result += e.SHIELD_2;
			else if (isPiece(WHITE, PAWN, H3)) result += e.SHIELD_3;
		// queenside
		} else if (col(b.king_location[WHITE]) < kColD) {
			if (isPiece(WHITE, PAWN, A2)) result += e.SHIELD_2;
			else if (isPiece(WHITE, PAWN, A3)) result += e.SHIELD_3;
	
			if (isPiece(WHITE, PAWN, B2)) result += e.SHIELD_2;
			else if (isPiece(WHITE, PAWN, B3)) result += e.SHIELD_3;
	
			if (isPiece(WHITE, PAWN, C2)) result += e.SHIELD_2;
			else if (isPiece(WHITE, PAWN, C3)) result += e.SHIELD_3;
		}
	} else {
		if (col(b.king_location[BLACK]) > kColE) {
			if (isPiece(BLACK, PAWN, F7)) result += e.SHIELD_2;
			else if (isPiece(BLACK, PAWN, F6)) result += e.SHIELD_3;
	
			if (isPiece(BLACK, PAWN, G7)) result += e.SHIELD_2;
			else if (isPiece(BLACK, PAWN, G6)) result += e.SHIELD_3;
	
			if (isPiece(BLACK, PAWN, H7)) result += e.SHIELD_2;
			else if (isPiece(BLACK, PAWN, H6)) result += e.SHIELD_3;
		} else if (col(b.king_location[BLACK]) < kColD) {
			if (isPiece(BLACK, PAWN, A7)) result += e.SHIELD_2;
			else if (isPiece(BLACK, PAWN, A6)) result += e.SHIELD_3;
		
			if (isPiece(BLACK, PAWN, B7)) result += e.SHIELD_2;
			else if (isPiece(BLACK, PAWN, B6)) result += e.SHIELD_3;
	
			if (isPiece(BLACK, PAWN, C7)) result += e.SHIELD_2;
			else if (isPiece(BLACK, PAWN, C6)) result += e.SHIELD_3;
		}
	}

	return result;
}

// Helper function that detects a few forms of piece-blocking
// Or otherwise restricted movement
void blockedPieces(int side) {
	// pawn blocking bishop development
	if (isPiece(side, BISHOP, mirrorSquare(side, C1))
		&& isPiece(side, PAWN, mirrorSquare(side, D2))
		&& b.colors[mirrorSquare(side, D3)] != NO_COLOR)
		v.blockages[side] -= e.P_BLOCK_CENTRAL_PAWN;

	if (isPiece(side, BISHOP, mirrorSquare(side, F1))
		&& isPiece(side, PAWN, mirrorSquare(side, E2))
		&& b.colors[mirrorSquare(side, E3)] != NO_COLOR)
		v.blockages[side] -= e.P_BLOCK_CENTRAL_PAWN;

	// trapped knight
	if (isPiece(side, KNIGHT, mirrorSquare(side, A8))
		&& isPiece(!side, PAWN, mirrorSquare(side, A7))
		|| isPiece(!side, PAWN, mirrorSquare(side, C7)))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A8;

	if (isPiece(side, KNIGHT, mirrorSquare(side, H8))
		&& isPiece(!side, PAWN, mirrorSquare(side, H7))
		|| isPiece(!side, PAWN, mirrorSquare(side, F7)))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A8;

	if (isPiece(side, KNIGHT, mirrorSquare(side, A7))
		&& isPiece(!side, PAWN, mirrorSquare(side, A6))
		&& isPiece(!side, PAWN, mirrorSquare(side, B7)))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A7;

	if (isPiece(side, KNIGHT, mirrorSquare(side, H7))
		&& isPiece(!side, PAWN, mirrorSquare(side, H6))
		&& isPiece(!side, PAWN, mirrorSquare(side, G7)))
		v.blockages[side] -= e.P_KNIGHT_TRAPPED_A7;

	// trapped bishop
	if (isPiece(side, BISHOP, mirrorSquare(side, A7))
		&& isPiece(!side, PAWN, mirrorSquare(side, B6)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, mirrorSquare(side, H7))
		&& isPiece(!side, PAWN, mirrorSquare(side, G6)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, mirrorSquare(side, B8))
		&& isPiece(!side, PAWN, mirrorSquare(side, C7)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, mirrorSquare(side, G8))
		&& isPiece(!side, PAWN, mirrorSquare(side, F7)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A7;

	if (isPiece(side, BISHOP, mirrorSquare(side, A6))
		&& isPiece(!side, PAWN, mirrorSquare(side, B5)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A6;

	if (isPiece(side, BISHOP, mirrorSquare(side, H6))
		&& isPiece(!side, PAWN, mirrorSquare(side, G5)))
		v.blockages[side] -= e.P_BISHOP_TRAPPED_A6;

	// King blocking rook
	if ((isPiece(side, KING, mirrorSquare(side, F1)) || isPiece(side, KING, mirrorSquare(side, G1))
		&& isPiece(side, ROOK, mirrorSquare(side, H1)) || isPiece(side, ROOK, mirrorSquare(side, G1))))
		v.blockages[side] -= e.P_KING_BLOCKS_ROOK;

	if ((isPiece(side, KING, mirrorSquare(side, C1)) || isPiece(side, KING, mirrorSquare(side, B1))
		&& isPiece(side, ROOK, mirrorSquare(side, A1)) || isPiece(side, ROOK, mirrorSquare(side, B1))))
		v.blockages[side] -= e.P_KING_BLOCKS_ROOK;
}

// Accesses current board and returns whether a given
// color, piece, and square is represented on said board
bool isPiece(uint8_t color, uint8_t piece, int8_t sq) {
	return ((b.pieces[sq] == piece) && (b.colors[sq] == color));
}

// Returns a value representing a form of distance between two squares
// Used to calculate attackers near a King
int getTropism(int sq1, int sq2) {
	return 7 - (abs(row(sq1) - row(sq2)) + abs(col(sq1) - col(sq2)));
}
