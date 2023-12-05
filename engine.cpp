#include <algorithm>
#include <random>
#include <iostream>
#include <thread>
#include <fstream>
#include <random>

#include "board.hpp"
#include "engine.hpp"
#include "butils.hpp"

using namespace std;
PlayerColor player;
Engine * e;
// std::ofstream OutFile("Moves.txt");
// ROOK, ROOK, KING, BISHOP,KNIGHT,KNIGHT,PAWN, PAWN,PAWN, PAWN
int weights[] = {700,700,900,650,500,500,300,300,300,300};
std::deque<U16> history;
int moves_cnt=0;
bool timeout;
int search_time=0;
int nodes_explored=0;

const int INTMAX = INT32_MAX/100;

int utility(Board *state, PlayerColor &start);
bool taken(Board board, U8 pos){
    auto b=board.data.board_0[pos];
    if (b==EMPTY){
        return false;
    }
    return true;
}

int inverse_dist(U16 move, Board* state){   
    U8 p0=getp0(move);
    U8 p1=getp1(move);
    int x0=getx(p0);
    int x1=getx(p1);
    int y0=gety(p0);
    int y1=gety(p1);
    bool kill=taken(*state,p1);
    if (kill){
        float score=ceil(7-(double)(abs(x0-x1)+abs(y0-y1)));
        return score;
    }
    return 0;
}


class Tree{
    public:
    // Val , Node* , move
    vector<pair<int,pair<Tree*,U16> > > children;
    Tree(){
        children.resize(0);
    }
    Tree(Board* b){
        children.resize(0);
        auto move_set = b->get_legal_moves(); 
        for(auto &move:move_set){
            children.push_back(make_pair(0,make_pair(new Tree(),move)));
        }
    }
    int expand(Board* b,int ply,bool isMax,int alpha,int beta,time_t& start_time,int& end_time){
        if (ply == 0 ){
            auto move_set = b->get_legal_moves(); 
            return utility(b,player)+2*move_set.size();}
        if ((time(NULL)-start_time >= end_time)){
            // OutFile<< "Timeout at "<<ply<<endl;
            timeout = true;
            auto move_set = b->get_legal_moves(); 
            return utility(b,player)+2*move_set.size();
        }
        if (children.size() == 0){
            auto move_set = b->get_legal_moves(); 
            if (move_set.size() == 0){
                if (b->in_check()){
                        if (isMax)
                            return (-1.0*INTMAX);
                        else{
                            return (INTMAX)/(ply +1);
                        }
                    }
                return 0;
            }
            for(auto &move:move_set){
                Board* newBoard = new Board(*b);
                newBoard->do_move_(move);
                nodes_explored++;
                children.push_back(make_pair(utility(newBoard,player),make_pair(new Tree(newBoard),move)));
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                shuffle (children.begin(), children.end(), std::default_random_engine(seed));
                sort(children.begin(),children.end());
                if (isMax)
                    reverse(children.begin(),children.end());
            }
            int best_max=INTMAX;
            int best_min=-1*INTMAX;
            for(int i=0;i<children.size();i++){
                auto child = children[i];
                int val=child.first;
                Tree* node=child.second.first;
                U16 move =child.second.second;
                Board* newBoard = new Board(*b);
                newBoard->do_move_(move);
                int newVal = node->expand(newBoard,ply-1,isMax^true,alpha,beta,start_time,end_time)+inverse_dist(children[i].second.second,b);
                children[i].first = newVal;
                if (isMax){
                    best_min = std::max(best_min,newVal);
                    alpha = std::max(alpha, newVal);
                    if (alpha >= beta){
                        sort(children.begin(),children.end());
                        reverse(children.begin(),children.end());
                        return newVal;
                        }
                    }
                else{
                    best_max = std::min(best_max,newVal);
                    beta = std::min(beta,newVal);
                    if (alpha >= beta){
                        sort(children.begin(),children.end());
                        return newVal;
                        }
                    }
                }
              unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
              shuffle (children.begin(), children.end(), std::default_random_engine(seed));
            sort(children.begin(),children.end());
            if (isMax){
                reverse(children.begin(),children.end());
            }
            return best_max*(!isMax) +isMax*best_min;
        }
        else{
            int best_max=INTMAX;
            int best_min=-1*INTMAX;
            for(int i=0;i<children.size();i++){
                auto child = children[i];
                int val=child.first;
                Tree* node=child.second.first;
                U16 move =child.second.second;
                Board* newBoard = new Board(*b);
                newBoard->do_move_(move);
                int newVal = node->expand(newBoard,ply-1,isMax^true,alpha,beta,start_time,end_time)+inverse_dist(children[i].second.second,b);
                children[i].first = newVal;
                if (isMax){
                    best_min = std::max(best_min,newVal);
                    alpha = std::max(alpha, newVal);
                    if (alpha >= beta){
                        sort(children.begin(),children.end());
                        reverse(children.begin(),children.end());
                        return newVal;
                        }
                    }
                else{
                    best_max = std::min(best_max,newVal);
                    beta = std::min(beta,newVal);
                    if (alpha >= beta){
                        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                        shuffle (children.begin(), children.end(), std::default_random_engine(seed));
                        sort(children.begin(),children.end());
                        return newVal;
                        }
                    }
                }
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            shuffle (children.begin(), children.end(), std::default_random_engine(seed));
            sort(children.begin(),children.end());
            if (isMax){
                reverse(children.begin(),children.end());
            }
            return best_max*(!isMax) +isMax*best_min;
        }
    }
    ~Tree(){
            for(auto &child: children){
                delete child.second.first;
            }
    }
};


int rook_king_eval(int xp,int yp,int xk,int yk,BoardType board){
    int val=15;
    //cout<<"ROOK: "<<xp<<" "<<yp<<" "<<xk<<" "<<yk<<endl;
    if (board==SEVEN_THREE){
        if (yk>=5){
            if (xp<=1 || (yp>=5 && xp<xk)){
                //std:://cout<<"helloooooooooooooooooooooo"<<endl;
                return val;
            }
        }
        else if (xk>=5){
            if (yp>=5 || (xp>=5 && yp>yk)){
                return val;
            }
        }
        else if (yk<=1){
            if (xp>=5 || (yp<=1 && xp>xk)){
                return val;
            }
        }
        else{
            if (yp<=1 || (xp<=1 && yp<yk)){
                return val;
            }
        }
    }
    else if(board==EIGHT_TWO){
        if (yk>=5){
            if (xp<=2 || (yp>=5 && xp<xk)){
                return val;
            }
        }
        else if (xk>=5){
            if (yp>=5 || (xp>=5 && yp>yk)){
                return val;
            }
        }
        else if (yk<=2){
            if (xp>=5 || (yp<=2 && xp>xk)){
                return val;
            }
        }
        else{
            if (yp<=2 || (xp<=2 && yp<yk)){
                return val;
            }
        }
    }
    else if(board==EIGHT_FOUR){
        if (yk>=6){
            if (xp<=1 || (yp>=6 && xp<xk)){
                return val;
            }
        }
        else if (xk>=6){
            if (yp>=6 || (xp>=6 && yp>yk)){
                return val;
            }
        }
        else if (yk<=1){
            if (xp>=6 || (yp<=1 && xp>xk)){
                return val;
            }
        }
        else{
            if (yp<=1 || (xp<=1 && yp<yk)){
                return val;
            }
        }
    }
    return 0;
}

int Pawn_promotion(int xp,int yp, BoardType board, PlayerColor start){
    int val=4;
    //cout<<"PAWN: "<<xp<<" "<<yp<<endl;
    if (board==SEVEN_THREE){
        int dis=0;
        if (start==WHITE){
            dis=min(abs(5-yp),abs(6-yp));
        }
        else{
            dis=min(abs(1-yp),abs(yp));
        }
        return val-dis;
    }
    else if(board==EIGHT_TWO){
        int dis=0;
        if (start==WHITE){
            dis=min(min(abs(5-yp),abs(6-yp)),abs(7-yp));
        }
        else{
            dis=min(min(abs(2-yp),abs(1-yp)),abs(yp));
        }
        return val-dis;
    }
    else if(board==EIGHT_FOUR){
        int dis=0;
        if (start==WHITE){
            dis=min(abs(6-yp),abs(7-yp));
        }
        else{
            dis=min(abs(1-yp),abs(yp));
        }
        return val-dis;
    }
    return 0;
}
int Pawn_club(vector<int> &pawns, BoardType board){
    int val=4;
    int value=0;
    for (int i=0;i<pawns.size();i+=2){
        for (int j=i+2;j<pawns.size();j+=2){
            int dis=abs(pawns[i]-pawns[j])+abs(pawns[i+1]-pawns[j+1]);
            value+=(val-dis);
        }
    }
    return value;
}

int manhattan(Board* state, PlayerColor player){
    U8* pieces = (U8*)(&(state->data));
    int p=(player==WHITE)?0:10;
    U8 OK;
    int value=0;
    // //cout<<"checking: "<<p<<endl;
    if (p==0){
        OK=(U8)pieces[12];
    }
    else{
        OK=(U8)pieces[2];
    }
    pieces+=p;
    int xok=getx(OK);
    int yok=gety(OK);
    if (pieces[0]!=DEAD){
        U8 R1=(U8)pieces[0];
        int xp=getx(R1);
        int yp=gety(R1);
        //cout<<board_to_str(&(state->data));
        value+=rook_king_eval(xp,yp,xok,yok,state->data.board_type);   
    }
    if (pieces[1]!=DEAD ){
        U8 R2=(U8)pieces[1];
        int xp=getx(R2);
        int yp=gety(R2);
        // cout<<board_to_str(&(state->data));
        value+=rook_king_eval(xp,yp,xok,yok,state->data.board_type);   
    }
    vector<int> pawns(0);

    if (pieces[6]!=DEAD && state->data.board_0[pieces[6]]&PAWN){
        U8 P1=(U8)pieces[6];
        int xp=getx(P1);
        int yp=gety(P1);
        pawns.push_back(xp);
        pawns.push_back(yp);
        // cout<<board_to_str(&(state->data));
        value+=Pawn_promotion(xp,yp,state->data.board_type, player);   
    }        
    if (pieces[7]!=DEAD && state->data.board_0[pieces[7]]&PAWN){
        U8 P1=(U8)pieces[7];
        int xp=getx(P1);
        int yp=gety(P1);
        pawns.push_back(xp);
        pawns.push_back(yp);
        // cout<<board_to_str(&(state->data));
        value+=Pawn_promotion(xp,yp,state->data.board_type, player);   
    }        
    if (pieces[8]!=DEAD && state->data.board_0[pieces[8]]&PAWN){
    
        U8 P1=(U8)pieces[8];
        int xp=getx(P1);
        int yp=gety(P1);
        pawns.push_back(xp);
        pawns.push_back(yp);
        // cout<<board_to_str(&(state->data));
        value+=Pawn_promotion(xp,yp,state->data.board_type, player);   
    }        
    if (pieces[9]!=DEAD && state->data.board_0[pieces[9]]&PAWN){
        U8 P1=(U8)pieces[9];
        int xp=getx(P1);
        int yp=gety(P1);
        pawns.push_back(xp);
        pawns.push_back(yp);
        // cout<<board_to_str(&(state->data));
        value+=Pawn_promotion(xp,yp,state->data.board_type, player);   
    }    
    value+=Pawn_club(pawns, state->data.board_type);
    return value;
}



int utility(Board *state, PlayerColor &start){
    int white=0,black=0;
    U8* pieces = (U8*)(&(state->data));

    for(int i=0;i<6;i++){
        white += weights[i]*(pieces[i] != DEAD);
    }
    for(int i=6;i<10;i++){
        if (pieces[i]==ROOK){
            white += weights[0]*(pieces[i] != DEAD);
        }
        else if (pieces[i]==BISHOP){
            white += weights[3]*(pieces[i] != DEAD);
        }
        else{
            white += weights[i]*(pieces[i] != DEAD);
        }
    }
    pieces += 10;
    for(int i=0;i<6;i++){
        black += weights[i]*(pieces[i] != DEAD);
    }
    for(int i=6;i<10;i++){
        if (pieces[i]==ROOK){
            white += weights[0]*(pieces[i] != DEAD);
        }
        else if (pieces[i]==BISHOP){
            white += weights[3]*(pieces[i] != DEAD);
        }
        else{
            white += weights[i]*(pieces[i] != DEAD);
        }
    }
    if (state->in_check()){
        if (state->data.player_to_play == WHITE){black+=80;}
        else{white+=80;}
    }
    if (start == WHITE){
        return (white-black+manhattan(state,start));
    }
    else{
        return (black-white+manhattan(state,start));
    }
}


void Engine::find_best_move(const Board& b) {
    e = this;
    // taken(b,0);
    player = b.data.player_to_play;
    auto moveset = b.get_legal_moves();
    // OutFile << show_moves(&b.data, moveset) << std::endl;
    //     for (auto m : moveset) {
    //         OutFile << move_to_str(m) << " ";
    //     }
    //    OutFile << std::endl;

    this->best_move = 0;
    if (moveset.size() != 0){

        time_t init = time(NULL);
        int best_val = -INTMAX;
        int ply = 2;
        if (e->time_left < std::chrono::milliseconds(4000)){
            this->best_move = *moveset.begin();
            return;
        }
        search_time = 10;
        Board* temp = new Board(b);
        Tree* tree = new Tree(temp);
        int prevVal=-INTMAX;
        nodes_explored= 0;
        while (time(NULL) - init < search_time)
        {  
            timeout = false;
            Board * board = new Board(b);
            int newval= tree->expand(board,ply,true,-INTMAX,INTMAX,init,search_time);
            // OutFile<<"Ply count :"<<ply<<std::endl;
            // for(auto &child:tree->children){
            // OutFile<<move_to_str(child.second.second)<<' '<<child.first<<std::endl;
            // }
            if (tree->children.size() == 0){
                this->best_move = 0;
            }
            else{
                    int max_ = -INTMAX;
                    int j=0;
                    for(int i=0;i<tree->children.size();i++){
                        if (max_ < tree->children[j].first){
                            j = i;
                            max_= tree->children[j].first;
                        }
                    }
                    if ( !timeout){
                        this->best_move = tree->children[j].second.second;
                    }
            }
            prevVal = newval;
            if ((newval >= INTMAX-2000) ||(newval <= -INTMAX + 2000) ){
                break;
            };
            if (ply==4){
                break;
            }
            ply += 2;
        }
        delete tree;
        // OutFile<<"Nodes explored: "<<nodes_explored<<' '<<move_to_str(this->best_move)<<std::endl;
    }
    moves_cnt+=2;
}
