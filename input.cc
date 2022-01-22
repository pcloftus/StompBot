#include "defines.h"
#include "tt.h"

static std::string command;

void init() {
	Timer timer;

	timer.move_time = 5000;
}

// Takes the engine's Move and prints 
// As well as critically making the move
// (All prior search and evaluation moveMake()s
// should be undone soon after in that context)
void sendMove(Move m) {
	int promotion = 0;
	char piece_symbols[5] = {'\0', 'q', 'r', 'b', 'n'};

	char command[20];
	char move[6];

	convertFrom0x88(m.from_sq, move);
	convertFrom0x88(m.to_sq, move+2);

	// Adds a piece symbol to the end of the sent move
	// to indicate promotion, if applicable
	if (m.to_piece != m.from_piece) {
		promotion = m.to_piece;
	}
	move[4] = piece_symbols[promotion];
	move[5] = '\0';

	std::cout<<"Stomp moves: "<<move<<'\n';

	moveMake(m);
}

// Stops the search and checks for input
void input() {
	if (task == TASK_SEARCH) {
		task = TASK_NOTHING;
		return;
	}

	std::getline(std::cin, command);

	cliInput(command);
}

// Handles command inputted by user
// Including algebraic moves
void cliInput(std::string command) {
	Timer timer;
	int converted;
	const char *c_command = command.c_str();
	if (command == "stat") printStats();
	else if (command == "b") boardDisplay();
	else if (command == "new") loadFromFen(STARTFEN);
	else if (!strncmp(c_command, "pos", 3)) {
		loadFromFen(c_command+4);
	} else if (command == "move") go();
	else if (command == "quit") exit(0);
	else if (command == "help") printHelp();
	else if (isMove(command)) {
		char converted_command[command.size()];
		std::size_t length = command.copy(converted_command, command.size());
		converted_command[length] = '\0';
		if (algebraic(converted_command))
			go();
		else
			std::cout<<"Not a legal move\n";
	}  else if (command[0] == '\n') { }
	else {
		std::cout<<command<<" - INVALID COMMAND\n";
	}
}

// Utility for checking if a given string is a valid algebraic move
bool isMove(std::string command) {
	if (command[0] >= 'a' && command[0] <= 'h' &&
		command[1] >= '1' && command[1] <= '8' &&
		command[2] >= 'a' && command[2] <= 'h' &&
		command[3] >= '1' && command[3] <= '8') {
		if (command[4] == ' ' || command[4] == '\n' ||
			command[4] == '\0' || command[4] == '-' ||
			command[4] == 'q' || command[4] == 'r' ||
			command[4] == 'b' || command[4] == 'n')
			return true;
	}
	return false;
}
