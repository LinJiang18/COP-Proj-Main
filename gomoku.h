#ifndef GOMOKU
#define GOMOKU

#include <string>
#include <vector>
#include "endpoint.h"

using namespace std;

class Gomoku
{
public:
    Gomoku();
    Gomoku(string black, string white, int me_first);
    int in_board(int x, int y);
    int set(int x, int y, int who);
    int judge(int x, int y, int who);
    void win(int who);
    void tie();
    void table_init();
    void print();
    int is_end();
    void turnover();

private:
    int my_turn;
    string my_color;
    int XY = 9;
    string first_line = "\n0  1  2  3  4  5  6  7  8";
    vector<vector<string>> table;
    vector<vector<int>> direction = {{-1, -1}, {0, -1}, {1, 1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    string black_name;
    string white_name;
    int turn;
    string empty = ".";
    string black_set = "●";
    string white_set = "○";
    int game_end = 0;
};

class Status
{
public:
    Status();
    int is_gaming();
    void accept(string black, string white, int who_);
    void refuse();
    void invited();
    void add_peer(endpoint_t p);
    void delete_peer();
    endpoint_t get_peer();
    void set_address(vector<string> add);
    void print();
    string get(int i);
    int invite_status();
    int set(int x, int y);
    int set_(int x, int y);
    int me();
    void resign();

private:
    int who = 0;
    int game_status = 0;
    int accept_ = 0;
    int peer_valid = 0;
    endpoint_t peer;
    vector<string> address;
    vector<Gomoku> game;
};

#endif
