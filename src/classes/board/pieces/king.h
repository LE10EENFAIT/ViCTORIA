#include "piece.h"

class King : public Piece {
    public:
        King (Square position, bool white);
        virtual ~King () {};
        bool check_move(Square goal, Piece* squares[], U64 occupancy) override;
        bool en_prise(Square goal, Piece* squares[], U64 occupancy) override
            {return check_move(goal, squares, occupancy);}
                
    private:
        bool is_check;
};