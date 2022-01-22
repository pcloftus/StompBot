#include "defines.h"
#include "board_util.h"

// BLIND based on Harm Geert Mueller's work
// "Better, or Lower If Not Defended"
// Detects obviously good captures
bool Blind(Move move) {
	int sq_to = move.to_sq;
	int sq_fr = move.from_sq;
	int pc_fr = b.pieces[sq_fr];
	int pc_to = b.pieces[sq_to];
	int val = e.SORT_VALUE[pc_to];

	// Pawn captures good
	if (pc_fr == PAWN)
		return true;

	// Lower value piece taking a higher good
	if (e.SORT_VALUE[pc_to] >= e.SORT_VALUE[pc_fr] - 50)
		return true;

	// Reveal any x-ray defenders
	clearSq(sq_fr);

	// Captures of undefended pieces are good by definition
	if (!isAttacked(!b.side_to_move, sq_to)) {
		fillSq(b.side_to_move, pc_fr, sq_fr);
		return true;
	}

	fillSq(b.side_to_move, pc_fr, sq_fr);
	return false;
}
