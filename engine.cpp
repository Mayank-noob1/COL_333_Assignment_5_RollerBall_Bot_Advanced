#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include <fstream>

#include "board.hpp"
#include "engine.hpp"
#include "butils.hpp"


Engine * e;
std::ofstream OutFile("Moves.txt");
// ROOK, ROOK, KING, BISHOP,KNIGHT,KNIGHT,PAWN, PAWN,PAWN, PAWN
int weights[] = {70,70,90,55,50,50,30,30,30,30};
std::deque<U16> history;
int moves_cnt=0;

int search_time=0;

const int INTMAX = INT32_MAX/100;
// Removed flux for now
inline int flux(Board *state, U16 moves,PlayerColor start){
    return 0;
}
// Terminal test
inline bool terminal(Board *state,size_t size){
    if (size == 0){
        return true;
    }
    return false;
}
// Piece count
inline int piece_count(Board *state,PlayerColor &start){
    int cnt=0;
    U8* pieces = (U8*)(&(state->data));
    for (int j=0;j<10;j++){
        if (pieces[j] != DEAD){cnt--;}
    }
    for (int j=10;j<20;j++){
        if (pieces[j] != DEAD){cnt++;}
    }
    if (start == WHITE){return cnt;}
    return -cnt;
}

inline int inverse_dist(Board *state){
    return {};
}

// Utility
int utility(Board *state, PlayerColor &start){
    int white=0,black=0;
    U8* pieces = (U8*)(&(state->data));

    for(int i=0;i<10;i++){
        black += weights[i]*(pieces[i] != DEAD);
    }
    pieces += 10;
    for(int i=0;i<10;i++){
        white += weights[i]*(pieces[i] != DEAD);
    }
    
    if (state->in_check()){
        if (state->data.player_to_play == WHITE){black+=10;}
        else{white+=10;}
    }
    if (start == WHITE){
        // white -= (state->data.w_pawn_1 == pos(0,0) || state->data.w_pawn_1 == pos(6,6) || state->data.w_pawn_1 == pos(0,6) || state->data.w_pawn_1 ==pos(6,0));
        // white -= (state->data.w_pawn_2 == pos(0,0) || state->data.w_pawn_2 == pos(6,6) || state->data.w_pawn_2 == pos(0,6) || state->data.w_pawn_2 == pos(6,0));
        return (white-black)+10*piece_count(state,start);
    }
    else{
        // black -= (state->data.b_pawn_1 == pos(0,0) || state->data.b_pawn_1 == pos(6,6) || state->data.b_pawn_1 == pos(0,6) || state->data.b_pawn_1 ==pos(6,0));
        // black -= (state->data.b_pawn_2 == pos(0,0) || state->data.b_pawn_2 == pos(6,6) || state->data.b_pawn_2 == pos(0,6) || state->data.b_pawn_2 == pos(6,0));
        return (black-white)+10*piece_count(state,start);
    }
}

int MaxVal(Board *, int , int, PlayerColor&, int, time_t&, int);
int MinVal(Board *, int , int, PlayerColor&, int, time_t&, int);

int MaxVal(Board *state, int alpha, int beta, PlayerColor &start, int ply,time_t &init,int plays){
    auto legal_moves = state->get_legal_moves();
    if (terminal(state,legal_moves.size())){
        if (state->in_check())
        return (-1.0*INTMAX)/(ply + 1);
        return 0;
    }
    // auto flux_ = flux(state,legal_moves,start);
    if (ply == 0 || (time(NULL)-init)>=search_time){
        return utility(state,start);
    }
    int best = -1.0*INTMAX;
    for (auto &move: legal_moves){
        Board * newState = new Board(*state);
        newState->do_move_(move);
        auto child = MinVal(newState,alpha,beta,start,ply-1,init,plays+1)+flux(newState,move,start)+plays;
        best = std::max(best,child);
        alpha = std::max(alpha, child);
        delete newState;
        if (alpha >= beta){
            return child;
        }
    }
    return best;
};

int MinVal(Board *state, int alpha, int beta, PlayerColor &start, int ply,time_t &init,int plays){
    auto legal_moves = state->get_legal_moves();
    if (terminal(state,legal_moves.size())){
        if (state->in_check())
        return INTMAX/(ply + 1);
        return 0;
    }
    // auto flux_ = flux(state,legal_moves,start);
    if (ply == 0 || (time(NULL)-init)>=search_time){
        return utility(state,start);
    }
    int best = INTMAX;
    for (auto &move: legal_moves){
        Board * newState = new Board(*state);
        newState->do_move_(move);
        auto child = MaxVal(newState,alpha,beta,start,ply-1,init,plays+1)+flux(newState,move,start)+plays;
        best = std::min(best,child);
        beta = std::min(beta,child);
        delete newState;
        if (alpha >= beta){
            return child;
        }
    }
    return best;
};

std::pair<U16,int> MiniMax(Board* root, PlayerColor &start, int ply,time_t &init){
    U16 best_move=0;
    int alpha=-1.0*INTMAX;
    int beta=INTMAX;
    int best_val=alpha;
    int moves_done=0;
    auto moves = root->get_legal_moves();
    std::vector<U16> best_same_moves;
    for(auto &move:moves){
        if (time(NULL)-init>= search_time){
            break;
        }
        moves_done++;
        Board * board = new Board(*root);
        board->do_move_(move);
        auto val = MinVal(board,alpha,beta,start,ply-1,init,1);
        OutFile<< move_to_str(move)<< ' '<<val<<std::endl;
        if (val >= best_val){
            if (history.size() == 2){
                auto front = history.front();
                if (front != move){
                    best_val = val;
                    best_move = move;
                }
                else{
                    if (moves.size() == 1){
                        best_val = val;
                        best_move = move;
                    }
                }
            }
            else{
                best_val = val;
                best_move = move;
            }
        }
        delete board;
    }
    if (history.size() < 2){
        history.push_back(best_move);}
    else if (history.size() == 2){
        auto front = history.front();
        if (front != best_move){
            history.pop_front();
            history.push_back(best_move);
        }
    }
    if (moves_done == moves.size())
    return {best_move,best_val};
    return {best_move,-INT16_MAX};
}


void Engine::find_best_move(const Board& b) {
    e = this;
    auto moveset = b.get_legal_moves();

    OutFile << show_moves(&b.data, moveset) << std::endl;
        for (auto m : moveset) {
            OutFile << move_to_str(m) << " ";
        }
       OutFile << std::endl;

    this->best_move = 0;
    if (moveset.size() != 0){

        time_t init = time(NULL);
        int best_val = -INTMAX;
        int ply = 2;
        if (e->time_left < std::chrono::milliseconds(5000)){
            search_time = 0;
            this->best_move = *moveset.begin();
        }
        else if (e->time_left < std::chrono::milliseconds(10000)){
            search_time = 1;
        }
        else if (e->time_left < std::chrono::milliseconds(20000)){
            search_time = 2;
        }
        else{
            search_time = 3;
        }
        while (time(NULL) - init < search_time)
        {  
            Board * board = new Board(b);
            auto player=b.data.player_to_play;
            auto best= MiniMax(board,player,ply,init);
            if (best.second != -INT16_MAX){    
                if (best_val <= best.second){
                    this->best_move = best.first;
                    best_val = best.second;
                }
                if (best.second >= INTMAX/(ply+1)){
                    break;
                }
            std::cout<<ply<<'\n';
            }
            ply += 2;
        }
        OutFile<<move_to_str(this->best_move)<<std::endl;
    }
    moves_cnt+=2;
}