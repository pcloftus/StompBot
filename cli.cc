#include "defines.h"

void printWelcome() {
	std::cout<<"Lil Stomper 9000 Chess Engine\n";
	std::cout<<"\"help\" for commands\n";
}

void printHelp() {
	std::cout<<"b\t\t- show board\n";
	std::cout<<"stat\t\t- show search stats\n";
	std::cout<<"move\t\t- play a move for which side's turn it is\n";
	std::cout<<"new\t\t- reset the game and board\n";
	std::cout<<"quit\t\t- quit the program\n";
}

void printStats() {
	uint64_t nodes = sd.nodes + (sd.nodes == 0);

	std::cout<<"Nodes\t: "<<sd.nodes<<'\n';
	std::cout<<"Quiet nodes\t: "<<sd.q_nodes<<'\n';
}

void printSearchHeader() {
	std::cout<<"ply\tnodes\t\ttime\tscore\tpv\n";
}
