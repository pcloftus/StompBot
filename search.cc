#include "defines.h"
#include "search.h"
#include "tt.h"

SearchData sd;

bool time_over = false;

// The best move so far
// Passed between iterations
uint8_t best_move;
// Backup move in case we run out of time mid-search
Move move_to_make;

void searchClearDriver() {
	sd.my_side = b.side_to_move; 
	sd.start_time = getTime();
	sd.move_time = 0;
	sd.depth = 0;

	sd.nodes = 0;
	sd.q_nodes = 0;
}

// Interface to the rest of the search functions
void searchRun() {
	Timer timer;

	searchClearDriver();
	moveTime();
	ageHistoryTable();
	printSearchHeader();

	searchIterate();
}

// Implements Iterative Deepening
// Essentially just calls searchWiden() increasing depth each iteration
// Assuming we still have time and there is more than one reply
void searchIterate() {
	int val;

	int move_count = moveCountLegal();

	// Iterative Deepening
	// Layer 1
	sd.depth = 1;
	val = searchRoot(sd.depth, -kInf, kInf);

	// Iterative Deepening
	// Layers 2 - MAX
	for (sd.depth = 2; sd.depth <= kMaxDepth; sd.depth += 1) {
		// Time allotted to search expired
		if (timeCheckRoot() || time_over)
			break;
		// Only one legal reply
		if (move_count == 1 && sd.depth == 5)
			break;

		val = searchWiden(sd.depth, val);
	}

	sendMove(move_to_make);
}

int searchWiden(int depth, int val) {
	int temp = val;
	int alpha = val - 50;
	int beta = val + 50;

	// Do the main search
	temp = searchRoot(sd.depth, alpha, beta);

	// If we have a cutoff
	// We attribute it to the aspiration window (+/- 50)
	// And do a full window search
	if (temp <= alpha || temp >= beta)
		temp = searchRoot(sd.depth, -kInf, kInf);
	return temp;
}

int searchRoot(uint8_t depth, int alpha, int beta) {
	int flagInCheck;
	Move movelist[256];
	int val = 0;
	int best = -kInf;

	uint8_t currmove_legal = 0;

	// We extend the current depth if we are in check
	flagInCheck = isAttacked(!b.side_to_move, b.king_location[b.side_to_move]);
	if (flagInCheck)
		++depth;

	uint8_t mcount = moveGen(movelist, best_move);

	for (uint8_t i = 0; i < mcount; ++i) {
		int cl = b.side_to_move;
		moveGenSort(mcount, movelist, i);

		// If the current move is a check??
		// Or if it's mate??
		// Or if we can capture the enemy king??
		// TODO figure out if this is necessary
		if (movelist[i].piece_cap == KING) {
			alpha = kInf;
			best_move = movelist[i].id;
		}

		moveMake(movelist[i]);

		// Immediately unmake any moves that leave us in check
		// And go to the next move
		if (isAttacked(b.side_to_move, b.king_location[!b.side_to_move])) {
			moveUnmake(movelist[i]);
			continue;
		}

		currmove_legal++;

		// If we haven't tried any moves yet
		// Begin recursive DFS at root
		// And, relying on previous move ordering, assume that this is the PV
		//
		// Otherwise, start a PV search if we have an alpha cutoff
		if (best == -kInf) {
			val = -Search(depth - 1, 0, -beta, -alpha, 1);
		} else
			if (-Search(depth - 1, 0, -alpha - 1, -alpha, 0) > alpha)
				val = -Search(depth - 1, 0, -beta, -alpha, 1);

		if (val > best)
			best = val;

		moveUnmake(movelist[i]);

		if (time_over)
			break;

		// At this point the main search is done
		// And we can check for cutoffs
		// First checking that we don't have a fail low (val > alpha)
		// Then checking if we have a fail high
		// If we don't, set alpha, and save to the TT
		//
		// In either case, PV prints info and calls doPV which makes the PV move
		if (val > alpha) {
			best_move = movelist[i].id;
			move_to_make = movelist[i];

			if (val > beta) {
				ttSave(depth, beta, TT_BETA, best_move);
				PV(beta);
				return beta;
			}

			alpha = val;
			ttSave(depth, alpha, TT_ALPHA, best_move);

			PV(val);
		}
	}

	// This saves the exact evaluation to the TT
	// Can occur in conjunction with the above clause
	// As we are only looking for beta cutoffs via the Negamax framework
	// Also as per Negamax, we are unconcerned if evaluation goes below alpha
	ttSave(depth, alpha, TT_EXACT, best_move);
	return alpha;
}

int Search(uint8_t depth, uint8_t ply, int alpha, int beta, int is_pv) {
	int val = -kInf;
	char best_move;
	char tt_move_index = (char) -1;
	char tt_flag = TT_ALPHA;
	int flagInCheck;
	int raised_alpha = 0;
	int moves_tried = 0;
	int new_depth;
	int mate_value = kInf - ply; // used in mate distance pruning
	Move movelist[256];
	Move move; // current move

	// Quick (not really) input/time check
	checkInput();
	if (time_over)
		return 0;

	// Cut off search early if we have mate
	// Also prevents looking for "better" mates
	if (alpha < -mate_value)
		alpha = -mate_value;
	if (beta > mate_value - 1)
		beta = mate_value - 1;
	if (alpha >= beta) // TODO is this necessary?
		return alpha;

	// If, through the recursive process, we are in check
	// Search one depth further
	flagInCheck = (isAttacked(!b.side_to_move, b.king_location[b.side_to_move]));
	if (flagInCheck)
		depth++;

	// Quiesce on leaf nodes
	if (depth < 1) {
		return Quiesce(alpha, beta);
	}

	// This begins searching in earnest
	sd.nodes++;

	if (isRepetition())
		return drawScore();

	// Check the TT and use it's value if the depth matches
	// And we are not currently searching a PV
	// Or we have an exact value stored in the TT
	// TODO Use the tt_move directly if found
	// Instead of indexing it in move generation later
	val = ttAccess(depth, alpha, beta, &tt_move_index);
	if (val != kInvalid) {
		if (!is_pv || (val > alpha && val < beta)) {
			// If we have mate
			// Convert val to the mate distance from root
			// Instead of mate distance from current node
			if (abs(val) > kInf - 100) {
				if (val > 0)
					val -= ply;
				else
					val += ply;
			}

			return val;
		}
	}
	
	// If by this point we don't have mate
	// But the straight-up evaluation of the current board
	// is high, return that score
	if (depth < 3
		&& !is_pv
		&& !flagInCheck
		&& abs(beta - 1) > -kInf + 100) {
		int temp_eval = eval(alpha, beta, 1) - 120 * depth;

		if (temp_eval >= beta)
			return temp_eval;
	}

	// Generate all moves
	uint8_t mcount = moveGen(movelist, tt_move_index);
	sortKillers(movelist, mcount, ply);
	best_move = movelist[0].id;

	for (int i = 0; i < mcount; ++i) {
		int cl = b.side_to_move;
		moveGenSort(mcount, movelist, i);
		move = movelist[i];
		moveMake(move);

		if (isAttacked(b.side_to_move, b.king_location[!b.side_to_move])) {
			moveUnmake(move);
			continue;
		}

		moves_tried++;
		new_depth = depth - 1;

		re_search:
		// Introduce PV search
		// If we found a move that raised alpha, do a null-window search to find anything better
		// If we find a better move, then do a full-window search
		// If we did not find a move that raised alpha, continue
		if (!raised_alpha)
			val = -Search(new_depth, ply + 1, -beta, -alpha, is_pv);
		else {
			if (-Search(new_depth, ply + 1, -alpha - 1, -alpha, 0) > alpha)
				val = -Search(new_depth, ply + 1, -beta, -alpha, 1);
		}

		moveUnmake(move);
		if (time_over)
			return 0;

		// Here we change node value if we can do better than alpha
		// Also set raised_alpha
		// And return alpha/beta immediately on beta cutoff
		// After setting killers/history table
		if (val > alpha) {
			best_move = movelist[i].id;
			if (val >= beta) {
				// Update killers/history table for quiet moves
				if (!moveIsCapt(move) && !moveIsProm(move)) {
					setKillers(movelist[i], ply);
					sd.history[b.side_to_move][move.from_sq][move.to_sq] += depth * depth;
				}
				tt_flag = TT_BETA;
				alpha = beta;
				break; // no need to search any further
			}
			raised_alpha = 1;
			tt_flag = TT_EXACT;
			alpha = val;
		}
	}

	// Check for stalemate or checkmate
	// If there are no legal moves and we are in check
	// Return the current node's mate score (-kInf + ply)
	// Otherwise, return the draw score
	if (!moves_tried) {
		best_move = -1;
		if (flagInCheck)
			alpha = -kInf + ply;
		else
			alpha = drawScore();
	}

	ttSave(depth, alpha, tt_flag, best_move);

	return alpha;
}

// Quiet move evaluation
int Quiesce(int alpha, int beta) {
	checkInput();
	if (time_over)
		return 0;

	sd.nodes++;
	sd.q_nodes++;

	// Do a static evaluation for the stand_pat
	int val = eval(alpha, beta, 1);
	int stand_pat = val;

	if (val >= beta)
		return beta;

	// Update lower bound
	if (alpha < val)
		alpha = val;

	Move movelist[256];
	uint8_t mcount = moveGenQs(movelist);

	for (uint8_t i = 0; i < mcount; ++i) {
		moveGenSort(mcount, movelist, i);

		if (movelist[i].piece_cap == KING)
			return kInf;

		// So called "delta cutoff"
		// A move so bad there's no point in continuing to search it
		if ((stand_pat + e.PIECE_VALUE[movelist[i].piece_cap] + 200 < alpha)
			&& (b.piece_material[!b.side_to_move] - e.PIECE_VALUE[movelist[i].piece_cap] > e.ENDGAME_MAT)
			&& (!moveIsProm(movelist[i])))
			continue;

		// Regular move search (negamax)
		moveMake(movelist[i]);
		val = -Quiesce(-beta, -alpha);
		moveUnmake(movelist[i]);

		if (time_over)
			return 0;

		if (val > alpha) {
			if (val >= beta)
				return beta;
			alpha = val;
		}
	}
	return alpha;
}

// Prints info about the PV in a search
// Critically calls doPV()
// Which makes the best PV move on the board
int PV(int val) {
	char pv[2048];

	uint32_t nodes = (uint32_t)sd.nodes;
	uint32_t time = getTime() - sd.start_time;

	doPV(pv);

	std::cout<<(int)sd.depth<<"\t"<<nodes<<"\t\t"<<time/10<<"\t"<<val<<"\t"<<pv<<'\n';

	return 0;
}

void setKillers(Move m, uint8_t ply) {
	if (m.piece_cap == NO_PIECE) {
		// Move primary killer to secondary if we have a new killer
		if (m.from_sq != sd.killers[ply][0].from_sq
			|| m.to_sq != sd.killers[ply][0].to_sq)
			sd.killers[ply][1] = sd.killers[ply][0];

		sd.killers[ply][0] = m;
	}	
}

// Set killer move scores for moveGenSort
// kSortKill - 1 for secondary, and kSortKill for primary
void sortKillers(Move *m, uint8_t mcount, uint8_t ply) {
	for (int j = 0; j < mcount; ++j) {
		if ((m[j].from_sq == sd.killers[ply][1].from_sq)
			&& (m[j].to_sq == sd.killers[ply][1].to_sq)
			&& (m[j].score < kSortKill - 1)) {
			m[j].score = kSortKill - 1;
		}

		if ((m[j].from_sq == sd.killers[ply][0].from_sq)
			&& (m[j].to_sq == sd.killers[ply][0].to_sq)
			&& (m[j].score < kSortKill)) {
			m[j].score = kSortKill;
		}
	}
}

// Clears the history table! :)
void clearHistoryTable() {
	for (int cl = 0; cl < 2; ++cl) {
		for (int i = 0; i < 128; ++i) {
			for (int j = 0; j < 128; ++j) {
				sd.history[cl][i][j] = 0;
			}
		}
	}
}

// Used instead of clearing at the start of a new search
// So that we can benefit from history table-improved move ordering
// When we sort moves
void ageHistoryTable() {
	for (int cl = 0; cl < 2; ++cl) {
		for (int i = 0; i < 128; ++i) {
			for (int j = 0; j < 128; ++j) {
				sd.history[cl][i][j] = sd.history[cl][i][j] / 10;
			}
		}
	}
}

// Returns a score for a draw
// Allows the engine to avoid a draw in the midgame
// Or possibly play for one in the endgame
int drawScore() {
	int value = -10;

	if (b.piece_material[sd.my_side] < e.ENDGAME_MAT)
		value = 0;

	if (b.side_to_move == sd.my_side)
		return value;
	else
		return -value;
}

void checkInput() {
	if (!time_over)
		time_over = timeCheck();
}

// Checks (exlusively) if the current position has been reached before
bool isRepetition() {
	for (int i = 0; i < b.repetition_i; ++i) {
		if (b.repetition[i] == b.hash)
			return true;
	}

	return false;
}
