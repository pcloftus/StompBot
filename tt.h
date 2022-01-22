struct Zobrist {
	uint64_t pieceSquare[6][2][128];
	uint64_t color;
	uint64_t castling[16];
	uint64_t ep[128];
} extern zobrist;

enum  TTFlag {
	TT_EXACT,
	TT_ALPHA,
	TT_BETA
};

struct TT {
	uint64_t hash;
	int val;
	uint8_t depth;
	uint8_t flags;
	uint8_t best_move;
} extern *tt;

struct PawnTT {
	uint64_t hash;
	int val;
} extern *ptt;

struct EvalTT {
	uint64_t hash;
	int val;
} extern *ett;

extern int tt_size;
extern int ptt_size;
extern int ett_size;

uint64_t rand64();
void ttInit();
void ttSize(int size);
int ttAccess(uint8_t depth, int alpha, int beta, char *best);
void ttSave(uint8_t depth, int val, char flags, char best);
void pttSize(int size);
int pttAccess();
void pttSave(int val);
void ettSize(int size);
int ettAccess();
void ettSave(int val);
