#include <cstdlib>
#include <cstring>
#include <stdint.h>

#include <iostream>
#include <string>
#include <vector>

// Search "extremely high" evaluation constant
const int kInf = 10000;
// Transposition table access not found constant
// (can be anything above kInf)
const int kInvalid = 42754;
// Max search depth constant
const int kMaxDepth = 100;

// Move sorting values for different types of moves:
// King - If able to be "captured"
// Hash - Stored best move in transposition table
// Captures - Only good ones (used in movegen in conjunction with BLIND)
// Promotion - Usually a good idea
// Killer move - quiet move good in a transposition of the position
//               which research shows is often good further up the tree
const int kSortKing = 400000000;
const int kSortHash = 200000000;
const int kSortCapt = 100000000;
const int kSortProm = 90000000;
const int kSortKill = 80000000;

// Starting position
const char STARTFEN[] = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

enum Colors {
	WHITE,
	BLACK,
	NO_COLOR
};

enum Pieces {
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
	NO_PIECE
};

// 0x88 board
enum Square {
	A1 = 0  , B1, C1, D1, E1, F1, G1, H1,
	A2 = 16 , B2, C2, D2, E2, F2, G2, H2,
	A3 = 32 , B3, C3, D3, E3, F3, G3, H3,
	A4 = 48 , B4, C4, D4, E4, F4, G4, H4,
	A5 = 64 , B5, C5, D5, E5, F5, G5, H5,
	A6 = 80 , B6, C6, D6, E6, F6, G6, H6,
	A7 = 96 , B7, C7, D7, E7, F7, G7, H7,
	A8 = 112, B8, C8, D8, E8, F8, G8, H8
};

// Bit masking idiom followed here and with CastleFlags
enum MoveFlags {
	MOVE_NORMAL = 0,
	MOVE_CAPTURE = 1,
	MOVE_EPCAPTURE = 2,
	MOVE_CASTLE = 4,
	MOVE_EP = 8,
	MOVE_PROMOTION = 16
};

enum CastleFlags {
	CFLAG_WK = 1,
	CFLAG_WQ = 2,
	CFLAG_BK = 4,
	CFLAG_BQ = 8
};

struct Board {
	uint8_t pieces[128];
	uint8_t colors[128];
	char side_to_move; // 0: white, 1: black
	char castle; // from CastleFlag
	char en_passant;
	uint8_t ply;
	uint64_t hash;
	uint64_t phash;
	int repetition_i;
	uint64_t repetition[1024];
	int8_t king_location[2];
	int middle_pcsq[2];
	int end_pcsq[2];
	int piece_material[2];
	int pawn_material[2];
	uint8_t piece_count[2][6];
	uint8_t pawns_files[2][8];
	uint8_t pawns_rank[2][8];
	uint8_t pawn_control[2][128];
};
extern Board b;

struct Move {
	char id; // originates as an index to a movegen list
	char from_sq;
	char to_sq;
	uint8_t from_piece;
	uint8_t to_piece;
	uint8_t piece_cap;
	char flags;
	char castle;
	char ply;
	char en_passant;
	int score;
};

struct SearchData {
	int my_side;
	uint8_t depth;
	int history[2][128][128]; // implements history heuristic
	Move killers[1024] [2]; // killer move storage (stores only two per ply)
	uint64_t nodes;
	int32_t move_time;
	uint64_t q_nodes;
	unsigned long start_time; // taken from system time
} extern sd;

enum Task {
	TASK_NOTHING, // awaiting input
	TASK_SEARCH // searching
} extern task;

struct Timer {
	int movestogo;
	int depth;
	int nodes;
	int mate;
	int move_time;
	uint8_t flags;
};

struct EvalData {
	// Data tables
	int PIECE_VALUE[6];
	int SORT_VALUE[6];

	// Piece-square tables
	int middle_table[6][2][128];
	int end_table[6][2][128];

	// Piece-square tables for pawn structure
	int weak_pawn[2][128];
	int passed_pawn[2][128];
	int protected_passer[2][128];
	int near_king[2][128][128];

	// Single values, p indicates penalty
	int BISHOP_PAIR;
	int P_KNIGHT_PAIR;
	int P_ROOK_PAIR;
	int ROOK_OPEN;
	int ROOK_HALF;
	int P_BISHOP_TRAPPED_A7;
	int P_BISHOP_TRAPPED_A6;
	int P_KNIGHT_TRAPPED_A8;
	int P_KNIGHT_TRAPPED_A7;
	int P_BLOCK_CENTRAL_PAWN;
	int P_KING_BLOCKS_ROOK;

	int SHIELD_2;
	int SHIELD_3;
	int P_NO_SHIELD;

	int ENDGAME_MAT;
};
extern EvalData e;

// Vectors for iterating piece movement
extern char directions[5][8];
extern bool is_slider[5];
extern char dir_sizes[5];

// Board functions
void boardDisplay();
void clearBoard();
void fillSq(uint8_t color, uint8_t piece, int8_t sq);
void clearSq(int8_t sq);
void loadFromFen(const char *fen);

// GUI functions
void sendMove(Move m);
void cliInput(std::string command);
void input();
void init();
bool isMove(std::string command);
void checkInput();

// Movegen
uint8_t moveGen(Move *moves, uint8_t tt_move);
uint8_t moveGenQs(Move *moves);
void moveGenSort(uint8_t move_count, Move *m, uint8_t current);

// Notation conversions
void convertFrom0x88(int8_t sq, char *a);
uint8_t convertTo0x88(const char *a);
char* algebraicWrite(Move m, char *a);
bool algebraic(char *a);
Move strToMove(char *a);

// Main move-doing/undoing
void moveMake(Move move);
void moveUnmake(Move move);

// Move type detection
bool moveIsCapt(Move m);
bool moveIsProm(Move m);
bool moveCanSimplify(Move m);
int moveCountLegal();
bool moveIsLegal(Move m);

// Main search interfaces
void searchRun(); 
void clearHistoryTable();

// Evaluation functions
int eval(int alpha, int beta, int use_hash);
bool isPiece(uint8_t color, uint8_t piece, int8_t sq);
int getTropism(int sq1, int sq2);

// Eval data functions
void setDefaultEval();
void setBasicValues();
void setNearKing();
void setPcsq();

// Misc. evaluation helpers
int Quiesce( int alpha, int beta );
bool Blind(Move move);

// Attack detection
bool isAttacked(char byColor, int8_t sq);
bool knightAttack( char byColor, int8_t sq, char byPiece );
bool straightAttack(char byColor, int8_t sq, int vect);
bool diagAttack(char byColor, int8_t sq, int vect);

// Utilities
void doPV(char *pv);
uint64_t perft(uint8_t depth);

// Internal timer functions
void go();
unsigned int getTime();
void moveTime();
bool timeCheckRoot();
bool timeCheck();

// (Used in search)
bool isRepetition();

// GUI helpers
void printWelcome();
void printStats();
void printHelp();
void printSearchHeader();
