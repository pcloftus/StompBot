#include "defines.h"
#include "tt.h"
#include "board_util.h"

Task task;

int main() {
	init();
	setDefaultEval();
	ttInit();
	ttSize(0x4000000); // 64m
	pttSize(0x1000000); // 16m
	ettSize(0x2000000); // 32m

	loadFromFen(STARTFEN);

	while (true) {
		// TODO Employ isRepetition()/stalemate detection
		if (moveCountLegal() == 0) {
			std::cout<<"Game over!\n";
			if (b.side_to_move == WHITE)
				std::cout<<"Black wins!\n";
			if (b.side_to_move == BLACK)
				std::cout<<"White wins!\n";
			std::cout<<"Resetting the board...\n";
			loadFromFen(STARTFEN);
		} else if (task == TASK_NOTHING) {
			input();
		} else {
			searchRun();
			task = TASK_NOTHING;
		}
	}
}
