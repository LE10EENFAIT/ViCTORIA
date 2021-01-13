#pragma once

#include "../board/board.h"

const int mate_value = 99999;

class Score {
    public:
        Score (int score) {
            this->score = score;
        };

        Score() {
            this->score=0;
        };

        bool operator == (Score s);
        bool operator != (Score s);

        static Score max (Score s1, Score s2);
        static Score min (Score s1, Score s2);

        void print();
        void print_info(int depth, int nodes, int time_ms, bool white);
        static void print_ply (Ply p);

        int score;
        vector<Ply> plies;
};