#include "engine.h"
/*************** End parse expression functions ***************/

vector<string> split (const string &s, char delim) {
    vector<string> result;
    stringstream ss (s);
    string item;

    while (getline (ss, item, delim)) {
        result.push_back (item);
    }

    return result;
}

void Engine::parseExpr(string expr) {
    ofstream log_file;
    log_file.open(this->logs_path, ios::app);
    log_file << expr << endl;
    log_file.close();

    vector<string> res = split(expr, ' ');

    // parse expression of type : position fen [fen] moves [moves]
    if (res.size() > 1 && res[0] == "position") {

        delete this->board;
        this->board = new Board ();

        this->positions.clear();
        this->moves.clear();

        int moves_idx = 8;

        if (res[1] == "fen") {
            string fen = "";
            for (int i = 2; i<8; i++) {
                fen += res[i] + " ";
            }
            replace(fen.begin(), fen.end(), 'A', 'K');
            replace(fen.begin(), fen.end(), 'H', 'Q');
            replace(fen.begin(), fen.end(), 'a', 'k');
            replace(fen.begin(), fen.end(), 'h', 'q');

            this->board->init(fen);

            this->startpos = fen == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ";


        } else if (res[1] == "startpos") {
            this->board->init();
            moves_idx = 2;

            this->startpos = true;
        }

        this->not_in_opening_table = !this->startpos;

        addInHashMap(generateHashKey(this->board));

        if (res.size() > moves_idx+1 && res[moves_idx] == "moves") {
            for (int i = moves_idx+1; i<res.size(); i++) {
                this->board->computeLegalMoves();
                this->board->play_move(res[i]);
                this->moves.push_back(Board::StringToPly(res[i]));
                addInHashMap(generateHashKey(this->board));
            }
        }

        this->zobrist_hash_key = generateHashKey(this->board);
    }
    
    else if (res.size() > 0 && res[0] == "go") {
        parseGoCommand(res);
    }

    else if (expr == "uci") {
        cout << "id name " << this->name << endl;
        cout << "id author Gaëtan Serré" << endl;
        cout << "uciok" << endl;
    }

    else if (expr == "isready") {
        cout << "readyok" << endl;
    }

    else if (res.size() >= 3 && res[0] == "set" && res[1] == "openings") {
        for (int i = 2; i<res.size(); i++) {
            this->opening_table_path = res[i];
        }
    }

    /************ debug commands ************/

    else if (res.size() == 2 && res[0] == "play") {
        
        this->board->computeLegalMoves();
        this->board->play_move(res[1]);

        addInHashMap(generateHashKey(this->board));

        this->board->printPieces();

        print_bitboard(this->board->occupancy);

        this->moves.push_back(Board::StringToPly(res[1]));

        this->board->computeLegalMoves();
        int size = this->board->getLegalMoves().size();
        if (this->board->isCheckmate(size)) {
            cout << (this->board->isWhite() ? "black" : "white") << " wins." << endl;
        }

        if (this->board->isStalemate(size)) {
            cout << "Stalemate." << endl;
        }

        this->zobrist_hash_key = generateHashKey(this->board);
    }

    else if (expr == "undo") {
        this->board->undo_move();
        this->board->printPieces();
        this->moves.pop_back();

        this->zobrist_hash_key = generateHashKey(this->board);
    }

    else if (expr == "legal") {
        u_int64_t start = millis();
        this->board->computeLegalMoves();
        u_int64_t dur = millis() - start;
        this->board->printLegalMoves();
        cout << dur << " ms" << endl;
    }

    else if (expr == "eval") {
        definePartGame();
        this->board->computeLegalMoves();
        u_int64_t start = micros();
        Score s = this->evaluator.evalPosition(this->board, this->early_game, this->end_game);
        u_int64_t dur = micros() - start;
        float score = this->board->isWhite() ? (float) s.score / 100.f : (float) -s.score / 100.f;
        cout << "Took " << dur << " µs" << endl;
        cout << "Final evaluation: " << score << " (white side)" << endl;
    }

    else if (expr == "sort") {
        vector<Move> res = this->sortMoves();
        for (Move m : res) {
            cout << m.ply.toString() << " : " << m.score << endl;
        }
    }

    else if (expr == "fen") {

        cout << this->board->getFen() << endl;
    }

    else if (expr == "hash") {
        cout << this->zobrist_hash_key << endl;
    }

    else if (expr == "hashMap") {
        for (auto pair : this->positions) {
            cout << pair.first << " : " << pair.second << endl;
        }
    }

    else if (res[0] == "trans") {
        if (res[1] == "all") {

            for (int i = 0; i<transposition_table_size; i++) {
                if (this->transposition_table[i].score.score != 0) {
                    cout << i << endl;
                    cout << this->transposition_table[i].score.score << endl;
                }
            }
            cout << "end" << endl;

        } else {
            int idx = stoi (res[1]);

            cout << idx << endl;
            cout << transposition_table[idx].score.score << endl;
        }
    }

    else if (expr != "quit") {
        cout << "Unknown command: " << expr << endl;
    }
}

void Engine::launchTimeThread (u_int64_t dur, bool direct_analysis) {
    thread t;

    if (!direct_analysis)
        t = thread(&Engine::searchOpeningBook, this, this->maxDepth);
    else
        t = thread(&Engine::IterativeDepthAnalysis, this, this->maxDepth, -1);
        
    u_int64_t start = millis();
    u_int64_t elapsed = millis() - start;
    while (elapsed < dur  && !this->is_terminated) {
        elapsed = millis() - start;

        // Useful for not using the CPU for nothing
        this_thread::sleep_for(chrono::microseconds(100));
    }

    this->terminate_thread = true;
    t.join();
}

void Engine::launchDepthSearch (int depth, bool direct_analysis, int nodes_max) {
    if (!direct_analysis)
        searchOpeningBook(depth);
    else
        IterativeDepthAnalysis(depth, nodes_max);
}

/*
 * Define if the position is early game, mid game or end game
 */
void Engine::definePartGame() {
    if (this->moves.size() < 20) {
        this->early_game = true;
        this->end_game = false;
    } else {
        int nb_piece_min = 0;
        for (auto & square : this->board->squares) {
            if (checkIfPiece(square)) {
                char name_piece = square->getName();
                // If the piece is not a pawn or a king
                if (name_piece != 'k' && name_piece != 'K' && name_piece != 'p' && name_piece != 'P')
                    nb_piece_min ++;
            }
        }
        this->early_game = false;
        this->end_game = nb_piece_min <= 4;
    }


}

double total_time = 0;
void Engine::parseGoCommand (vector<string> args) {

    /*
    * We reset the "thread killers"
    */
    this->terminate_thread = false;
    this->is_terminated = false;



    /*
    * We reset the old best move
    */
    this->best_move = Score();

    definePartGame();

    /*
    * We check if we will search for the next move in the opening book
    */
    bool direct_analysis = !(this->moves.size()/2 < 10 && !this->not_in_opening_table && !this->opening_table_path.empty());

    /*
    * command: go infinite
    */
           

    if (args.size() >= 2 && (args[1] == "infinite" || args[1] == "ponder")) {
        thread t;
        if (direct_analysis)
            t = thread(&Engine::IterativeDepthAnalysis, this, this->maxDepth, -1);
        else
            t = thread(&Engine::searchOpeningBook, this, this->maxDepth);

        string input = "";
        while (input != "stop" && !this->is_terminated) {
            getline(cin, input);
        }

        this->terminate_thread = true;
        t.join();

        this->best_move.print();
    }

    /*
    * command: go movetime n
    */

    else if (args.size() == 3 && args[1] == "movetime") {
        launchTimeThread (stoi(args[2]), direct_analysis);
        this->best_move.print();
    }

    /*
    * command: go depth n
    */
   else if (args.size() == 3 && args[1] == "depth") {
       int depth = stoi(args[2]);
       launchDepthSearch (depth, direct_analysis);
       this->best_move.print();
   }

   /*
   * command: go wtime n1 btime n2 [winc n3 binc n4]
   * I assume the engine need to pass 2/6 of the total time in the early game,
   * 3/6 in the mid game and the rest in the end game
   */

   else if (args.size() >= 5 && args[1] == "wtime") {
       if (total_time == 0)
           total_time = stod (args[this->board->isWhite() ? 2 : 4]);

       double time_search;

       double increment = 0;
        if (args.size() >= 8)
            increment = stod (args[this->board->isWhite() ? 6 : 8]);
        int nb_move = this->moves.size() / 2;

       if (this->early_game) {
           double total_early = total_time/6.0 * 2;
           int move_majorant = 10;
           time_search = total_early / move_majorant;
       }

       else if (! this->end_game) {
           double total_mid = total_time/6.0 * 3;

           // We remove the moves of the early game
           nb_move -= 10;

           int move_majorant = 20;
           if (nb_move < move_majorant - 5)
               time_search = total_mid / move_majorant;
           else
               time_search = stod (args[this->board->isWhite() ? 2 : 4]) / 20;
       }

       else {
           time_search = stod (args[this->board->isWhite() ? 2 : 4]) / 20;
       }

       /*if (nb_move < move_majorant - 10) {
           total_time /= move_majorant - nb_move;
       } else {
           total_time /= 20;
       }*/

       /*
        * If we have a increment of x seconds, then we can use x seconds to compute a move
        */
       if (time_search < increment) time_search = increment;

       cout << "searching for: " << time_search << " ms" << endl;

       launchTimeThread ((u_int64_t) time_search, direct_analysis);
       this->best_move.print();
   }

   /*
    * commmand: go nodes [nb_nodes]
    */
   else if (args.size() == 3 && args[1] == "nodes") {
       int nb_nodes = stoi(args[2]);
       cout << nb_nodes << endl;
        launchDepthSearch(this->maxDepth, direct_analysis, nb_nodes);

   }

    /*
    * command: go and all others
    */
    else {
        int depth = end_game ? 8 : 6;
        launchDepthSearch (depth, direct_analysis);
        this->best_move.print();
    }
}


/*************** End parse expression functions ***************/
