#include "defines.h"
#include "tt.h"

void moveMake(Move move) {
	b.side_to_move = !b.side_to_move;
	b.hash ^= zobrist.color;

	// Clear ply for captures and pawn moves
	b.ply++;
	if ((move.from_piece == PAWN) || moveIsCapt(move))
		b.ply = 0;

	// Clear the destination square for a capture
	if (b.pieces[move.to_sq] != NO_PIECE)
		clearSq(move.to_sq);

	clearSq(move.from_sq);
	fillSq(!b.side_to_move, move.to_piece, move.to_sq);

	// Castling rights if king or rook moves from starting square
	switch (move.from_sq) {
	case H1:
		b.castle &= ~CFLAG_WK;
		break;
	case E1:
		b.castle &= ~(CFLAG_WK|CFLAG_WQ);
		break;
	case A1:
		b.castle &= ~CFLAG_WQ;
		break;
	case H8:
		b.castle &= ~CFLAG_BK;
		break;
	case E8:
		b.castle &= ~(CFLAG_BK|CFLAG_BQ);
		break;
	case A8:
		b.castle &= ~CFLAG_BQ;
		break;
	}
	switch (move.to_sq) {
	case H1:
		b.castle &= ~CFLAG_WK;
		break;
	case E1:
		b.castle &= ~(CFLAG_WK|CFLAG_WQ);
		break;
	case A1:
		b.castle &= ~CFLAG_WQ;
		break;
	case H8:
		b.castle &= ~CFLAG_BK;
		break;
	case E8:
		b.castle &= ~(CFLAG_BK|CFLAG_BQ);
		break;
	case A8:
		b.castle &= ~CFLAG_BQ;
		break;
	}
	// update hash based on the current moves affect on castling
	b.hash ^= zobrist.castling[move.castle];
	// then update based on general board state
	b.hash ^= zobrist.castling[b.castle];

	// If it was a castle, the king move and flags were set above
	// Just have to move the rook now
	if (move.flags & MOVE_CASTLE) {
		if (move.to_sq == G1) {
			clearSq(H1);
			fillSq(WHITE, ROOK, F1);
		} else if (move.to_sq == C1) {
			clearSq(A1);
			fillSq(WHITE, ROOK, D1);
		} else if (move.to_sq == G8) {
			clearSq(H8);
			fillSq(BLACK, ROOK, F8);
		} else if (move.to_sq == C8) {
			clearSq(A8);
			fillSq(BLACK, ROOK, D8);
		}
	}

	// Reset en_passant flag
	// Reminder: ^= the hash's en_passant the set b.en_passant resets it
	if (b.en_passant != -1) {
		b.hash ^= zobrist.ep[b.en_passant];
		b.en_passant = -1;
	}
	// Check for new en_passant value
	// Update board and hash if so
	if ((move.from_piece == PAWN) && (abs(move.from_sq - move.to_sq) == 32)
		&& (b.pawn_control[b.side_to_move][(move.from_sq + move.to_sq) / 2])) {
		b.en_passant = (move.from_sq + move.to_sq) / 2;
		b.hash ^= zobrist.ep[b.en_passant];
	}

	// Remove a pawn captured en passant
	if (move.flags & MOVE_EPCAPTURE) {
		if ((!b.side_to_move) == WHITE) {
			clearSq(move.to_sq - 16);
		} else {
			clearSq(move.to_sq + 16);
		}
	}

	// Update repetition checker
	// TODO rigorously test repetition conditions
	++b.repetition_i;
	b.repetition[b.repetition_i] = b.hash;
}

// Inverse of moveMake()
void moveUnmake(Move move) {
	b.side_to_move = !b.side_to_move;
	b.hash ^= zobrist.color;

	b.ply = move.ply;

	// Re-set en_passant
	// Undoing en_passant if we are unmaking an en_passant
	// Or re-setting en_passant if we are unmaking a move after
	// a move that allowed en passant
	if (b.en_passant != -1)
		b.hash ^=  zobrist.ep[b.en_passant];
	if (move.en_passant != -1)
		b.hash ^= zobrist.ep[move.en_passant];
	b.en_passant = move.en_passant;
	
	clearSq(move.to_sq);
	fillSq(b.side_to_move, move.from_piece, move.from_sq);

	// Uncapture
	if (moveIsCapt(move))
		fillSq(!b.side_to_move, move.piece_cap, move.to_sq);

	// Uncastle
	if (move.flags & MOVE_CASTLE) {
		if (move.to_sq == G1) {
			clearSq(F1);
			fillSq(WHITE, ROOK, H1);
		} else if (move.to_sq == C1) {
			clearSq(D1);
			fillSq(WHITE, ROOK, A1);
		} else if (move.to_sq == G8) {
			clearSq(F8);
			fillSq(WHITE, ROOK, H8);
		} else if (move.to_sq == C8) {
			clearSq(D8);
			fillSq(WHITE, ROOK, A8);
		}
	}

	b.hash ^= zobrist.castling[move.castle];
	b.hash ^= zobrist.castling[b.castle];
	b.castle = move.castle;

	// Finish undoing en passant capture if applicable
	if (move.flags & MOVE_EPCAPTURE) {
		if (b.side_to_move == WHITE) {
			fillSq(BLACK, PAWN, move.to_sq - 16);
		} else {
			fillSq(WHITE, PAWN, move.to_sq + 16);
		}
	}

	--b.repetition_i;
}

// Move type helpers
bool moveIsCapt(Move m) {
	return (m.piece_cap != NO_PIECE);
}

bool moveIsProm(Move m) {
	return (m.from_piece != m.to_piece);
}

bool moveCanSimplify(Move m) {
	if (m.piece_cap == PAWN
		|| b.piece_material[!b.side_to_move] - e.PIECE_VALUE[m.piece_cap] > e.ENDGAME_MAT)
		return false;
	else
		return true;
}

// Generate all moves from the current position
// Check first that we have a match to the passed move
// Then check if it is legal
// TODO trim the duplicated work between this and moveCountLegal
// Or simplify the only call to it in util.cc
bool moveIsLegal(Move m) {
	Move movelist[256];
	int movecount = moveGen(movelist, 0xFF);

	for (int i = 0; i < movecount; ++i) {
		if (movelist[i].from_sq == m.from_sq
			&& movelist[i].to_sq == m.to_sq) {
			int result = true;

			moveMake(movelist[i]);
			if (isAttacked(b.side_to_move, b.king_location[!b.side_to_move]))
				result = false;
			moveUnmake(movelist[i]);

			return result;
		}
	}

	return false;
}

// Calls moveGen then filters out illegal moves
// Returns the number of legal moves from the current board position
int moveCountLegal() {
	Move mlist[256];
	int mcount = moveGen(mlist, 0xFF);
	int result = 0;

	for (int i = 0; i < mcount; ++i) {
		moveMake(mlist[i]);

		if (!isAttacked(b.side_to_move, b.king_location[!b.side_to_move]))
			++result;

		moveUnmake(mlist[i]);
	}

	return result;
}
