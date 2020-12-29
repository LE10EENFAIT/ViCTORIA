#include "pawn.h"

Pawn::Pawn(Square position, bool white, bool start_pos,
           bool* en_passant, Square* en_passant_square) : Piece(position, white) {
    this->name = "pawn";
    this->start_pos = start_pos;
    this->en_passant = en_passant;
    this->en_passant_square = en_passant_square;
}

int Pawn::forward (int n) {
    return (this->white ? n : -n);
}

bool Pawn::check_move(Square goal, Piece* squares[]) {
    if (this->position.row == goal.row) {
        if (this->position.line + forward(1) == goal.line)
            return ! checkIfPiece(squares[squareToIdx(goal)]);
        
        else if ((this->position.line + forward(2) == goal.line) && this->start_pos)
            return ! checkIfPiece(squares[squareToIdx(goal)]);
        
        else return false;

    } else if (this->PlusOrMinus(this->position.row, goal.row, 1)) {
        if (this->position.line + forward(1) == goal.line)
        
            return 
                this->checkIfPieceIsTakeable(squares[squareToIdx(goal)]) ||
                (*(this->en_passant) && (*this->en_passant_square) == goal);
            
        else return false;

    } else return false;
}

bool Pawn::en_prise(Square goal, Piece* squares[]) {
    if (this->PlusOrMinus(this->position.row, goal.row, 1)) {
        if (this->position.line + forward(1) == goal.line)
        
            return 
                this->checkIfPieceIsTakeable(squares[squareToIdx(goal)]) ||
                (*(this->en_passant) && (*this->en_passant_square) == goal);
            
        else return false;

    } else return false;
}