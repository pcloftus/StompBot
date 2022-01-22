// IMPLEMENTS THE FUN 0x88 REPRESENTATION MATH

// Divide by 16 to get row
const int8_t kRow1 = A1 >> 4;
const int8_t kRow2 = A2 >> 4;
const int8_t kRow3 = A3 >> 4;
const int8_t kRow4 = A4 >> 4;
const int8_t kRow5 = A5 >> 4;
const int8_t kRow6 = A6 >> 4;
const int8_t kRow7 = A7 >> 4;
const int8_t kRow8 = A8 >> 4;

// Effectively subtracts by however much
// needed to get it between 0-7
const int8_t kColA = A1 & 7;
const int8_t kColB = A2 & 7;
const int8_t kColC = A3 & 7;
const int8_t kColD = A4 & 7;
const int8_t kColE = A5 & 7;
const int8_t kColF = A6 & 7;
const int8_t kColG = A7 & 7;
const int8_t kColH = A8 & 7;

// Directional values
const char kNorth = 16;
const char kNN = 32;
const char kSouth = -16;
const char kSS = -32;
const char kEast = 1;
const char kWest = -1;
const char kNE = 17;
const char kSW = -17;
const char kNW = 15;
const char kSE = -15;

// Square conversions
int8_t toSq(uint8_t row, uint8_t col);
bool isSq(int8_t sq);
int8_t col(int8_t sq);
int8_t row(int8_t sq);
