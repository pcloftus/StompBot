int getPawnScore();
int evalPawnStructure();
int evalPawn(int8_t sq, int8_t side);
bool isPawnSupported(int8_t sq, int8_t side);
void evalKnight(int8_t sq, int8_t side);
void evalBishop(int8_t sq, int8_t side);
void evalRook(int8_t sq, int8_t side);
void evalQueen(int8_t sq, int8_t side);
int kingShield(int8_t side);
void blockedPieces(int side);
