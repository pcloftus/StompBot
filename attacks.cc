#include "defines.h"
#include "board_util.h"

bool knightAttack(char byColor, int8_t sq, char byPiece) {
	int8_t nextSq;

	for (int dir = 0; dir < 8; ++dir) {
		nextSq = sq + directions[byPiece][dir];
		if (isSq(nextSq)
			&& isPiece(byColor, byPiece, nextSq))
			return true;
	}

	return false;
}

bool straightAttack(char byColor, int8_t sq, int vect) {
	int nextSq = sq + vect;

	while(isSq(nextSq)) {
		if (b.colors[nextSq] != NO_COLOR) {
			if ((b.colors[nextSq] == byColor)
				&& (b.pieces[nextSq] == ROOK || b.pieces[nextSq] == QUEEN))
				return true;
			return false;
		}
		nextSq = nextSq + vect;
	}
	return false;
}

bool diagAttack(char byColor, int8_t sq, int vect) {
	int nextSq = sq + vect;

	while(isSq(nextSq)) {
		if (b.colors[nextSq] != NO_COLOR) {
			if ((b.colors[nextSq] == byColor)
				&& (b.pieces[nextSq] == BISHOP || b.pieces[nextSq] == QUEEN))
				return true;
			return false;
		}
		nextSq = nextSq + vect;
	}
	return false;
}

bool isAttacked(char byColor, int8_t sq) {
	// pawns
	if (byColor == WHITE && b.pawn_control[WHITE][sq])
		return true;

	if (byColor == BLACK && b.pawn_control[BLACK][sq])
		return true;

	// knights
	if (knightAttack(byColor, sq, KNIGHT))
		return true;

	// king
	if (knightAttack(byColor, sq, KING))
		return true;

	// rooks and queens
	if (straightAttack(byColor, sq, kNorth)
		|| straightAttack(byColor, sq, kSouth)
		|| straightAttack(byColor, sq, kEast)
		|| straightAttack(byColor, sq, kWest))
		return true;

	// bishops and queens
	if (diagAttack(byColor, sq, kNE)
		|| diagAttack(byColor, sq, kSE)
		|| diagAttack(byColor, sq, kNW)
		|| diagAttack(byColor, sq, kSW))
		return true;

	return false;
}
