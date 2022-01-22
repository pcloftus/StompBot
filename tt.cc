#include "defines.h"
#include "tt.h"

extern bool time_over;

Zobrist zobrist;

TT *tt;
PawnTT *ptt;
EvalTT *ett;

int tt_size = 0;
int ptt_size = 0;
int ett_size = 0;

// Random 64-int generator from Sungorous engine
uint64_t rand64() {
	static uint64_t next = 1;

	next = next * 1103515245 + 12345;
	return next;
}

// Fills the zobrist structure
void ttInit() {
	for (int i = 0; i < 6; i++) {
		for (int j = 0; j < 2; j++) {
			for (int k = 0; k < 128; k++) {
				zobrist.pieceSquare[i][j][k] = rand64();
			}
		}
	}

	zobrist.color = rand64();

	for (int i = 0; i < 16; i++) {
		zobrist.castling[i] = rand64();
	}

	for (int i = 0; i < 128; i++) {
		zobrist.ep[i] = rand64();
	}
}

// Dynamically updates size of the tt to consume less overall memory
// As the tt is the source of most of the memory footprint
//
// If size is not a power of 2 - make it so (lower)
// Allows us to do hash_value & (tt_size - 1)
// To get a number between 0 and number of entries quickly
// Which becomes our index
void ttSize(int size) {
	free(tt);

	if (size & (size - 1)) {
		size--;
		for (int i = 1; i < 32; i = i * 2) {
			size |= size >> i;
		}		
		size++;
		size >>= 1;
	}	
	
	if (size < 16) {
		tt_size = 0;
		return;
	}

	tt_size = (size / sizeof(TT)) - 1;
	tt = (TT*) malloc(size);
}

// If nothing is stored/entry doesn't match, return kInvalid
// Otherwise, sets best to the stored best move
// And if stored depth is greater than passed depth
// Return stored search data (val or alpha or beta)
int ttAccess(uint8_t depth, int alpha, int beta, char *best) {
	if (!tt_size) 
		return kInvalid;	

	TT *entry = &tt[b.hash & tt_size];

	// Match
	if (entry->hash == b.hash) {
		*best = entry->best_move;
	
		if (entry->depth >= depth) {
			if (entry->flags == TT_EXACT)
				return entry->val;
			if ((entry->flags == TT_ALPHA) && (entry->val <= alpha))
				return alpha;
			if ((entry->flags == TT_BETA) && (entry->val >= beta))
				return beta;
		}
	}

	return kInvalid;
}

// Only saves an entry if:
// It is a new board position and
// The depth to save is greater than the entry
// (which is 0 if not already present)
void ttSave(uint8_t depth, int val, char flags, char best) {
	if (!tt_size)
		return;
	if (time_over)
		return;

	TT *entry = &tt[b.hash & tt_size];

	if ((entry->hash == b.hash) && (entry->depth > depth))
		return;

	entry->hash = b.hash;
	entry->val = val;
	entry->flags = flags;
	entry->depth = depth;
	entry->best_move = best;
}

// Same as ttSize()
void pttSize(int size) {
	free(ptt);

	if (size & (size - 1)) {
		size--;
		for (int i = 1; i < 32; i = i * 2) {
			size |= size >> i;
		}
		size++;
		size >>= 1;
	}

	if (size < 8) {
		ptt_size = 0;
		return;
	}

	ptt_size = (size / sizeof(PawnTT)) - 1;
	ptt = (PawnTT*) malloc(size);
}

// Same as ttAccess() sans evaluation
int pttAccess() {
	if (!ptt_size)
		return kInvalid;

	PawnTT *entry = &ptt[b.phash & ptt_size];

	if (entry->hash == b.hash)
		return entry->val;

	return kInvalid;
}

// Same as ttSave()
void pttSave(int val) {
	if (!ptt_size)
		return;

	PawnTT *entry = &ptt[b.phash & ptt_size];

	entry->hash = b.phash;
	entry->val = val;
}

void ettSize(int size) {
	free(ett);

	if (size & (size - 1)) {
		size--;
		for (int i = 1; i < 32; i = i * 2) {
			size |= size >> i;
		}
		size++;
		size >>= 1;
	}

	if (size < 16) {
		ett_size = 0;
		return;
	}

	ett_size = (size / sizeof(EvalTT)) - 1;
	ett = (EvalTT*) malloc(size);
}

int ettAccess() {
	if (!ett_size)
		return kInvalid;
	
	EvalTT *entry = &ett[b.hash & ett_size];

	if (entry->hash == b.hash)
		return entry->val;

	return kInvalid;
}

void ettSave(int val) {
	if (!ett_size)
		return;

	EvalTT *entry = &ett[b.hash & ett_size];

	entry->hash = b.hash;
	entry->val = val;
}
