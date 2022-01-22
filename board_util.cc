#include "defines.h"
#include "board_util.h"
#include "tt.h"

Board b;

// Returns a number representing a square
int8_t toSq(uint8_t row, uint8_t col) {
	return (row * 16 + col);
}

// Checks if a given number is a valid square
// The & will result in 0 for all numbers on the board
// Except those beyond it's range, and those in the gaps
// of the 0x88 board
bool isSq(int8_t sq) {
	return sq & 0x88 ? false : true;
}

// Returns column of square
int8_t col(int8_t sq) {
	return sq & 7;
}

// Returns row of square
int8_t row(int8_t sq) {
	return sq >> 4;
}

void clearBoard() {
	for (int sq = 0; sq < 128; sq++) {
		b.pieces[sq] = NO_PIECE;
		b.colors[sq] = NO_COLOR;

		b.pawn_control[WHITE][sq] = 0;
		b.pawn_control[BLACK][sq] = 0;
	}

	b.castle = 0;
	b.en_passant = -1;
	b.ply = 0;
	b.hash = 0;
	b.phash = 0;
	b.side_to_move = 0;
	b.repetition_i = 0;

	b.piece_material[WHITE] = 0;
	b.piece_material[BLACK] = 0;
	b.pawn_material[WHITE] = 0;
	b.pawn_material[BLACK] = 0;
	b.middle_pcsq[WHITE] = 0;
	b.middle_pcsq[BLACK] = 0;
	b.end_pcsq[WHITE] = 0;
	b.end_pcsq[BLACK] = 0;

	for (int i = 0; i < 6; i++) {
		b.piece_count[WHITE][i] = 0;
		b.piece_count[BLACK][i] = 0;
	}

	for (int i = 0; i < 8; i++) {
		b.pawns_files[WHITE][i] = 0;		
		b.pawns_files[BLACK][i] = 0;		
		b.pawns_rank[WHITE][i] = 0;
		b.pawns_rank[BLACK][i] = 0;
	}
}

// Puts a given piece on a given square
// Also updates the hash, king_location, material tables,
// pawn control squares, piece-square tables, and piece count
// If applicable
void fillSq(uint8_t color, uint8_t piece, int8_t sq) {
	b.pieces[sq] = piece;
	b.colors[sq] = color;

	if (piece == KING) {
		b.king_location[color] = sq;
	}

	if (piece == PAWN) {
		b.pawn_material[color] += e.PIECE_VALUE[piece];

		b.phash ^= zobrist.pieceSquare[piece][color][sq];

		b.pawns_files[color][col(sq)]++;
		b.pawns_rank[color][row(sq)]++;

		if (color == WHITE) {
			if (isSq(sq + kNE))
				b.pawn_control[WHITE][sq + kNE]++;
			if (isSq(sq + kNW))
				b.pawn_control[WHITE][sq + kNW]++;

		} else {
			if (isSq(sq + kSE))
				b.pawn_control[BLACK][sq + kSE]++;
			if (isSq(sq + kSW))
				b.pawn_control[BLACK][sq + kSW]++;
		}
	} else {
		b.piece_material[color] += e.PIECE_VALUE[piece];
	}

	b.piece_count[color][piece]++;

	b.middle_pcsq[color] += e.middle_table[piece][color][sq];
	b.end_pcsq[color] += e.end_table[piece][color][sq];

	b.hash ^= zobrist.pieceSquare[piece][color][sq];
}

// Inverse of fillSq
void clearSq(int8_t sq) {
	uint8_t color = b.colors[sq];
	uint8_t piece = b.pieces[sq];

	b.hash ^= zobrist.pieceSquare[piece][color][sq];

	if (piece == PAWN) {
		if (color == WHITE) {
			if (isSq(sq + kNE))
				b.pawn_control[WHITE][sq + kNE]--;
			if (isSq(sq + kNW))
				b.pawn_control[WHITE][sq + kNW]--;
		} else {
			if (isSq(sq + kSE))
				b.pawn_control[BLACK][sq + kSE]--;
			if (isSq(sq + kSW))
				b.pawn_control[BLACK][sq + kSW]--;
		}

		b.pawns_files[color][col(sq)]--;
		b.pawns_rank[color][row(sq)]--;

		b.pawn_material[color] -= e.PIECE_VALUE[piece];
		b.phash ^= zobrist.pieceSquare[piece][color][sq];
	} else {
		b.piece_material[color] -= e.PIECE_VALUE[piece];
	}

	b.middle_pcsq[color] -= e.middle_table[piece][color][sq];
	b.end_pcsq[color] -= e.end_table[piece][color][sq];

	b.piece_count[color][piece]--;

	b.pieces[sq] = NO_PIECE;
	b.colors[sq] = NO_COLOR;
}

void loadFromFen(const char *fen) {
	clearBoard();
	clearHistoryTable();

	const char *f = fen;

	uint8_t col = 0;
	uint8_t row = 7;

	do {
		switch(f[0]) {
		case 'K':
			fillSq(WHITE, KING, toSq(row, col));
			col++;
			break;
		case 'Q':
			fillSq(WHITE, QUEEN, toSq(row, col));
			col++;
			break;
		case 'R':
			fillSq(WHITE, ROOK, toSq(row, col));
			col++;
			break;
		case 'B':
			fillSq(WHITE, BISHOP, toSq(row, col));
			col++;
			break;
		case 'N':
			fillSq(WHITE, KNIGHT, toSq(row, col));
			col++;
			break;
		case 'P':
			fillSq(WHITE, PAWN, toSq(row, col));
			col++;
			break;
		case 'k':
			fillSq(BLACK, KING, toSq(row, col));
			col++;
			break;
		case 'q':
			fillSq(BLACK, QUEEN, toSq(row, col));
			col++;
			break;
		case 'r':
			fillSq(BLACK, ROOK, toSq(row, col));
			col++;
			break;
		case 'b':
			fillSq(BLACK, BISHOP, toSq(row, col));
			col++;
			break;
		case 'n':
			fillSq(BLACK, KNIGHT, toSq(row, col));
			col++;
			break;
		case 'p':
			fillSq(BLACK, PAWN, toSq(row, col));
			col++;
			break;
		case '/':
			row--;
			col = 0;
			break;
		case '1':
			col += 1;
			break;
		case '2':
			col += 2;
			break;
		case '3':
			col += 3;
			break;
		case '4':
			col += 4;
			break;
		case '5':
			col += 5;
			break;
		case '6':
			col += 6;
			break;
		case '7':
			col += 7;
			break;
		case '8':
			col += 8;
			break;
		}

		f++;
	} while (f[0] != ' ');

	f++;

	if (f[0] == 'w') {
		b.side_to_move = WHITE;
	} else {
		b.side_to_move = BLACK;
		b.hash ^= zobrist.color;
	}

	f += 2;

	do {
		switch(f[0]) {
		case 'K':
			b.castle |= CFLAG_WK;
			break;
		case 'Q':
			b.castle |= CFLAG_WQ;
			break;
		case 'k':
			b.castle |= CFLAG_BK;
			break;
		case 'q':
			b.castle |= CFLAG_BQ;
			break;
		}
		f++;
	} while (f[0] != ' ');

	b.hash ^= zobrist.castling[b.castle];

	f++;

	if (f[0] != '-') {
		b.en_passant = convertTo0x88(f);
		b.hash ^= zobrist.ep[b.en_passant];
	}

	do {
		f++;
	} while (f[0] != ' ');

	f++;

	int ply = 0;
	sscanf(f, "%d", &ply);
	b.ply = (unsigned char) ply;

	b.repetition_i = 0;
	b.repetition[b.repetition_i] = b.hash;
}

void boardDisplay() {
	int8_t sq;

	char piece_icons[3][7] = {
		{'K','Q','R','B','N','P'},
		{'k','q','r','b','n','p'},
		{ 0 , 0 , 0 , 0 , 0 , 0 ,'.'}
	};	

	std::cout<<"   a b c d e f g h\n\n";

	for (int row = 7; row >= 0; row--) {
		std::cout<<row+1<<' ';

		for (int col = 0; col < 8; col++) {
			sq = toSq(row, col);
			std::cout<<' '<<piece_icons[b.colors[sq]][b.pieces[sq]];
		}

		std::cout<<"  "<<row+1<<'\n';
	}

	std::cout<<"\n   a b c d e f g h\n\n";
}
