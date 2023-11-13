#include <algorithm>
#include <random>
#include <iostream>
#include <set>
#include <map>
#include<unistd.h> 

#include "board.hpp"
#include "engine.hpp"
#include "butils.hpp"


using namespace std;

int weights[] = {70,70,90,55,30,30};
PlayerColor start;

int utility(Board *state){
    int white=0,black=0;
    U8* pieces = (U8*)(&(state->data));

    for(int i=0;i<6;i++){
        black += weights[i]*(pieces[i] != DEAD);
    }
    pieces += 6;
    for(int i=0;i<6;i++){
        white += weights[i]*(pieces[i] != DEAD);
    }
    
    if (state->in_check()){
        if (state->data.player_to_play == WHITE){black+=10;}
        else{white+=10;}
    }
    if (start == WHITE){
        white -= (state->data.w_pawn_1 == pos(0,0) || state->data.w_pawn_1 == pos(6,6) || state->data.w_pawn_1 == pos(0,6) || state->data.w_pawn_1 ==pos(6,0));
        white -= (state->data.w_pawn_2 == pos(0,0) || state->data.w_pawn_2 == pos(6,6) || state->data.w_pawn_2 == pos(0,6) || state->data.w_pawn_2 == pos(6,0));
        return (white-black);
    }
    else{
        black -= (state->data.b_pawn_1 == pos(0,0) || state->data.b_pawn_1 == pos(6,6) || state->data.b_pawn_1 == pos(0,6) || state->data.b_pawn_1 ==pos(6,0));
        black -= (state->data.b_pawn_2 == pos(0,0) || state->data.b_pawn_2 == pos(6,6) || state->data.b_pawn_2 == pos(0,6) || state->data.b_pawn_2 == pos(6,0));
        return (black-white);
    }
}




class MonteCarloNode{
    public:
    Board state;
    MonteCarloNode* parent;
    U16 parent_move;
    vector<MonteCarloNode*> children;
    int visits;
    map<int,int> results;
    vector<U16> untried_actions;
    MonteCarloNode(Board state,MonteCarloNode* parent=nullptr,U16 parent_move=0){
        cout<<"Creating node\n";
        this->state = state;
        this->parent = parent;
        this->parent_move = parent_move;
        children.clear();
        untried_actions.clear();
        results.clear();
        visits = 0;
        results[1] = results[-1] = 0;
        this->generate_actions();
        cout<<"Node created\n";
        return;
    }
    ~MonteCarloNode(){
        untried_actions.clear();
        children.clear();
        results.clear();
        return;
    }

    void generate_actions(){
        this->untried_actions= vector<U16> (state.get_legal_moves().begin(),state.get_legal_moves().end());
    }

    int q(){
        return (this->results[1]-this->results[-1]);
    }
    int n(){
        return visits;
    }
  
    MonteCarloNode* expand(){
        U16 action = this->untried_actions[this->untried_actions.size()-1];
        untried_actions.pop_back();
        Board newState = this->state;
        newState.do_move_(action);
        MonteCarloNode* childNode = new MonteCarloNode(
            newState,parent=this,parent_move=action
        );
        this->children.push_back(childNode);
        return childNode;
    }
    bool isTerminal(){
        return this->state.get_legal_moves().size() == 0;
    }

    int rollout(){
        Board current_state = this->state;
        int roll = 0;
        while (current_state.get_legal_moves().size() && roll < 500)
        {
            unordered_set<U16> possible_moves = current_state.get_legal_moves();
            U16 action = this->rollout_policy(possible_moves);
            current_state.do_move_(action);
        }
        int val =utility(&current_state);
        if (val > 0){
            return 1;
        }
        else{
            return -1;
        }
    }

    U16 rollout_policy(unordered_set<U16>& possible_moves){
        std::vector<U16> moves;
        std::sample(
            possible_moves.begin(),
            possible_moves.end(),
            std::back_inserter(moves),
            1,
            std::mt19937{std::random_device{}()}
        );
        return moves[0];
    }
    
    void backpropagate(int result,int n_){
        if (n_ > 100){
            return;
        }
        this->visits += 1;
        this->results[result] += 1;
        if (this->parent != nullptr && this->parent != this){
            this->parent->backpropagate(result,n_+1);
        }
    }

    MonteCarloNode* best_child(double c_=0.1){
        int argmax=0;
        double max_=-INT32_MAX;
        for(int i=0;i<this->children.size();i++){
            double c = (double)this->children[i]->q()/(double)this->children[i]->n() + 
                        c_ *sqrt(2*log((double)this->n()/(double)this->children[i]->n()));
            if (max_ < c){
                max_ = c;
                argmax = i;
            }
        }
        return this->children[argmax];
    }

    MonteCarloNode* tree_policy_construct(){
        MonteCarloNode* current_node = this;
        int depth = 0;
        while (!current_node->isTerminal())
        {
            if (!current_node->isExpanded()){
                return current_node->expand();
            }
            else{
                current_node = current_node->best_child();
            }
        }
        return current_node;
    }

    bool isExpanded(){
        return this->untried_actions.size() == 0;
    }
    
    U16 best_move(){
        int sim=1000;
        while (sim--)
        {
            MonteCarloNode* v= this->tree_policy_construct();
            int reward = v->rollout();
            v->backpropagate(reward,1);
        }
        
        return this->best_child()->parent_move; 
    }
};




void Engine::find_best_move(const Board& b) {

    // pick a random move
    start = b.data.player_to_play;
    auto moveset = b.get_legal_moves();
    if (moveset.size() == 0) {
        std::cout << "Could not get any moves from board!\n";
        std::cout << board_to_str(&b.data);
        this->best_move = 0;
    }
    else {
        MonteCarloNode root(b);
        this->best_move = root.best_move();
    }
    sleep(1);
}