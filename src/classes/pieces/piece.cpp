#include "piece.h"

Piece::Piece(square position, bool white) {
    this->position = position;
    this->white = white;
}

void Piece::print_piece() {
    std::string color = "black";
    if (this->white)
        color = "white";

    std::cout << color << " " << this->name << " " <<
              this->position.row << this->position.line << std::endl;
}

bool Piece::isWhite() {
    return this->white;
}

bool Piece::checkIfPieceIsTakeable(Piece* p) {
    if (checkIfPiece(p)) {
        bool A = this->white;
        bool B = p->isWhite();
        return A != B; // XOR
    } else return false;
}

int squareToIdx(square c) {
    int row = c.row - 97;
    int line = 7 - (c.line - 1);
    return line * 8 + row;
}

bool checkIfPiece(Piece *p) {
    return p->getName() != "Null";
}