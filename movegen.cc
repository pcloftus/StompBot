#include "defines.h"
#include "board_util.h"

uint8_t movecount;

Move *m;

bool is_slider[5] = {0, 1, 1, 1, 0};
char dir_sizes[5] = {8, 8, 4, 4, 8};
char directions[5][8] = {
	{kSW, kSouth, kSE, kWest, kEast, kNW, kNorth, kNE},
	{kSW, kSouth, kSE, kWest, kEast, kNW, kNorth, kNE},
	{kSouth, kWest, kEast, kNorth},
	{kSW, kSE, kNW, kNE},
	{-33, -31, -18, -14, 14, 18, 31, 33}
};

// Updates the data for a move in the movelist
// Critically updates sort values for said move
void moveGenPush(char from, char to, uint8_t piece_from, uint8_t piece_cap, char flags) {
	m[movecount].from_sq = from;
	m[movecount].to_sq = to;
	m[movecount].from_piece = piece_from;
	m[movecount].to_piece = piece_from;
	m[movecount].piece_cap = piece_cap;
	m[movecount].flags = flags;
	m[movecount].ply = b.ply;
	m[movecount].castle = b.castle;
	m[movecount].en_passant = b.en_passant;
	m[movecount].id = movecount;

	// Below clauses only trigger on non-quiet moves
	// So we first sort based on history score
	// Which will be overwritten below for non-quiet
	m[movecount].score = sd.history[b.side_to_move][from][to];

	// Capture scoring
	// Add the sort value of the captured piece to the id of the attacking piece
	// (Ids are enumerated in reverse material value order; e.g.,
	// making a capture by a pawn better than a capture by a rook)
	if (piece_cap != NO_PIECE) {
		if(Blind(m[movecount]) == 0)
			m[movecount].score = e.SORT_VALUE[piece_cap] + piece_from;
		else
			m[movecount].score = kSortCapt + e.SORT_VALUE[piece_cap] + piece_from;
	}

	// Small extra sort bonus for en passant capture (5)
	if ((piece_from == PAWN) && (to == b.en_passant)) {
		m[movecount].score = kSortCapt + e.SORT_VALUE[PAWN] + 5;
		m[movecount].flags = MOVE_EPCAPTURE;
	}

	// For a single promotion move
	// Put all four possible piece promotions on the list
	// And score them based on their value and kSortProm
	// Results in queen promotions being searched first
	// But doesn't rule out the tasty knight promotion
	if ((piece_from == PAWN) && ((row(to) == kRow1)||(row(to) == kRow8))) {
		m[movecount].flags |= MOVE_PROMOTION;

		for (char prompiece = QUEEN; prompiece <= KNIGHT; ++prompiece) {
			m[movecount+prompiece-1] = m[movecount];
			m[movecount+prompiece-1].to_piece = prompiece;
			m[movecount+prompiece-1].score += kSortProm + e.SORT_VALUE[prompiece];
			m[movecount+prompiece-1].id = movecount+prompiece-1;
		}
		movecount += 3;
	}
	movecount++;
}

void moveGenPawnMove(int8_t sq, bool promotion_only) {
	if (b.side_to_move == WHITE) {
		if (promotion_only && (row(sq) != kRow7))
			return;

		if (b.pieces[sq+kNorth] == NO_PIECE) {
			moveGenPush(sq, sq+kNorth, PAWN, NO_PIECE, MOVE_NORMAL); 

			if ((row(sq) == kRow2)
				&& (b.pieces[sq+kNN] == NO_PIECE))
				moveGenPush(sq, sq+kNN, PAWN, NO_PIECE, MOVE_EP); 
		}
	} else {
		if (promotion_only && (row(sq) != kRow2))
			return;

		if (b.pieces[sq+kSouth] == NO_PIECE) {
			moveGenPush(sq, sq+kSouth, PAWN, NO_PIECE, MOVE_NORMAL); 

			if ((row(sq) == kRow7)
				&& (b.pieces[sq+kSS] == NO_PIECE))
				moveGenPush(sq, sq+kSS, PAWN, NO_PIECE, MOVE_EP); 
		}
	}
}

void moveGenPawnCapt(int8_t sq) {
	if (b.side_to_move == WHITE) {
		if (isSq(sq+kNW) && ((b.en_passant == sq+kNW) || (b.colors[sq+kNW] == (b.side_to_move^1)))) {
			moveGenPush(sq, sq+kNW, PAWN, b.pieces[sq+kNW], MOVE_CAPTURE);
		}
		if (isSq(sq+kNE) && ((b.en_passant == sq+kNE) || (b.colors[sq+kNE] == (b.side_to_move^1)))) {
			moveGenPush(sq, sq+17, PAWN, b.pieces[sq+kNE], MOVE_CAPTURE);
		}
	} else {
		if (isSq(sq+kSE) && ((b.en_passant == sq+kSE) || (b.colors[sq+kSE] == (b.side_to_move^1)))) {
			moveGenPush(sq, sq+kSE, PAWN, b.pieces[sq+kSE], MOVE_CAPTURE);
		}
		if (isSq(sq+kSW) && ((b.en_passant == sq+kSW) || (b.colors[sq+kSW] == (b.side_to_move^1)))) {
			moveGenPush(sq, sq+kSW, PAWN, b.pieces[sq+kSW], MOVE_CAPTURE);
		}
	}
}
uint8_t moveGen(Move *moves, uint8_t tt_move) {
	m = moves;

	movecount = 0;

	// Castling
	if (b.side_to_move == WHITE) {
		if (b.castle & CFLAG_WK) {
			if ((b.pieces[F1] == NO_PIECE)
				&& (b.pieces[G1] == NO_PIECE)
				&& (!isAttacked(!b.side_to_move, E1))
				&& (!isAttacked(!b.side_to_move, F1))
				&& (!isAttacked(!b.side_to_move, G1)))
				moveGenPush(E1, G1, KING, NO_PIECE, MOVE_CASTLE);
		}

		if (b.castle & CFLAG_WQ) {
			if ((b.pieces[B1] == NO_PIECE)
				&& (b.pieces[C1] == NO_PIECE)
				&& (b.pieces[D1] == NO_PIECE)
				&& (!isAttacked(!b.side_to_move, E1))
				&& (!isAttacked(!b.side_to_move, D1))
				&& (!isAttacked(!b.side_to_move, C1)))
				moveGenPush(E1, C1, KING, NO_PIECE, MOVE_CASTLE);
		}
	} else {
		if (b.castle & CFLAG_BK) {
			if ((b.pieces[F8] == NO_PIECE)
				&& (b.pieces[G8] == NO_PIECE)
				&& (!isAttacked(!b.side_to_move, E8))
				&& (!isAttacked(!b.side_to_move, F8))
				&& (!isAttacked(!b.side_to_move, G8)))
				moveGenPush(E8, G8, KING, NO_PIECE, MOVE_CASTLE);
		}

		if (b.castle & CFLAG_BQ) {
			if ((b.pieces[B8] == NO_PIECE)
				&& (b.pieces[C8] == NO_PIECE)
				&& (b.pieces[D8] == NO_PIECE)
				&& (!isAttacked(!b.side_to_move, E8))
				&& (!isAttacked(!b.side_to_move, D8))
				&& (!isAttacked(!b.side_to_move, C8)))
				moveGenPush(E8, C8, KING, NO_PIECE, MOVE_CASTLE);
		}
	}

	for (int8_t sq = 0; sq < 120; ++sq) {
		if (b.colors[sq] == b.side_to_move) {
			if (b.pieces[sq] == PAWN) {
				moveGenPawnMove(sq, 0);
				moveGenPawnCapt(sq);
			} else {
				for (char dir = 0; dir < dir_sizes[b.pieces[sq]]; ++dir) {
					for (char pos = sq;;) {
						pos = pos + directions[b.pieces[sq]][dir];

						if (!isSq(pos))
							break;

						if (b.pieces[pos] == NO_PIECE) {
							moveGenPush(sq, pos, b.pieces[sq], NO_PIECE, MOVE_NORMAL);
						} else {
							if (b.colors[pos] != b.side_to_move)
								moveGenPush(sq, pos, b.pieces[sq], b.pieces[pos], MOVE_CAPTURE);
							break;
						}

						if (!is_slider[b.pieces[sq]])
							break;
					}
				}
			}
		}
	}

	// If tt_move is within the total movecount possibles
	// Then set the sort score for hash move
	if (tt_move < movecount)
		moves[tt_move].score = kSortHash;

	return movecount;
}

// Movegen for quiet moves
// Exclusively used in quiescence
uint8_t moveGenQs(Move *moves) {
	m = moves;

	movecount = 0;

	for (int8_t sq = 0; sq < 120; ++sq) {
		if (b.colors[sq] == b.side_to_move) {
			if (b.pieces[sq] == PAWN) {
				moveGenPawnMove(sq, 1);
				moveGenPawnCapt(sq);
			} else {
				for (char dir = 0; dir < dir_sizes[b.pieces[sq]]; ++dir) {
					for (char pos = sq;;) {
						pos = pos + directions[b.pieces[sq]][dir];

						if (!isSq(pos))
							break;

						if (b.pieces[pos] != NO_PIECE) {
							if (b.colors[pos] != b.side_to_move)
								moveGenPush(sq, pos, b.pieces[sq], b.pieces[pos], MOVE_CAPTURE);
							break; 
						}

						if (!is_slider[b.pieces[sq]])
							break;
					}
				}
			}
		}
	}
	return movecount;
}

// Puts the highest scoring move at the position passed to it
void moveGenSort(uint8_t movecount, Move *m, uint8_t current) {
	int high = current;

	for (int i = current+1; i < movecount; ++i) {
		if (m[i].score > m[high].score)
			high = i;
	}

	Move temp = m[high];
	m[high] = m[current];
	m[current] = temp;
}
