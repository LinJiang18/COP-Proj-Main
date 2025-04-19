#include "gomoku.h"
#include <iostream>
#include <algorithm>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <string.h>
#include <sstream>
#include <unordered_map>
#include <map>
#include <fstream>
#include "endpoint.h"

using namespace std;

Gomoku::Gomoku() {}

Gomoku::Gomoku(string black, string white, int me_first)
{
    black_name = black;
    white_name = white;

    my_turn = me_first;
    // if (my_turn)
    // {
    //     black_name = "*" + black_name;
    // }
    // else
    // {
    //     white_name = "*" + white_name;
    // }
    table_init();
    print();
}

int Gomoku::in_board(int x, int y)
{
    if ((x <= XY) & (y <= XY) & (x >= 0) & (y >= 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int Gomoku::set(int x, int y, int who) // 0: black 1: white
{
    // cout << "turn: " << my_turn << endl;
    // cout << "who: " << who << endl;
    if (my_turn == who)
    {
        if (in_board(x, y))
        {
            if (table[x][y] == empty)
            {
                if (who == 0)
                {
                    table[x][y] = black_set;
                }
                else
                {
                    table[x][y] = white_set;
                }
                print();
                if (judge(x, y, who))
                {
                    return 2; // I win.
                }
                else
                {
                    turnover();
                    return 1;
                }
            }
            else
            {
                cout << "Conflict." << endl;
            }
        }
        else
        {
            cout << "Out of board." << endl;
        }
    }
    else
    {
        // cout << who << endl;
        cout << "This is not your turn." << endl;
    }
    return 0;
}

int Gomoku::judge(int x, int y, int who)
{
    string myset = (who == 0) ? black_set : white_set;
    for (int i = 0; i < 8; i++)
    {
        vector<int> d = direction[i];
        int x_ = d[0];
        int y_ = d[1];
        int flag = 1;

        for (int j = 1; j <= 4; j++)
        {
            if (in_board(x + j * x_, y + j * y_))
            {
                string t = table[x + j * x_][y + j * y_];
                if ((t != myset))
                {
                    flag = 0;
                }
            }
            else
            {
                flag = 0;
            }
        }

        if (flag)
        {
            win(who);
            return 1;
        }
    }
    return 0;
}

void Gomoku::win(int who)
{
    game_end = 1;
    if (who == 0)
    {
        cout << black_name + "(black) wins." << endl;
    }
    else
    {
        cout << white_name + "(white) wins." << endl;
    }
}

void Gomoku::tie()
{
    game_end = 1;
    cout << "The game ties." << endl;
}

void Gomoku::table_init()
{
    vector<string> l;
    first_line = "\n   ";
    for (int line = 0; line <= XY; line++)
    {
        l = {empty, empty, empty, empty, empty, empty, empty, empty, empty, empty};
        table.push_back(l);
        first_line += (to_string(line) + "  ");
    }

    if (my_turn)
    {
        my_color = black_set;
    }
    else
    {
        my_color = white_set;
    }
}

void Gomoku::print()
{
    string info = "\nBlack:\t" + black_name + "\tWhite:\t" + white_name;
    string table_status = info + first_line;

    for (int i = 0; i <= XY; i++)
    {
        table_status += ("\n" + to_string(i) + "  ");
        for (int j = 0; j <= XY; j++)
        {
            table_status += (table[i][j] + "  ");
        }
    }
    // table_status += "\n";

    cout << table_status << endl;
}

int Gomoku::is_end()
{
    return game_end;
}

void Gomoku::turnover()
{
    my_turn = 1 - my_turn;
}

Status::Status() {}

int Status::is_gaming()
{
    return game_status;
}

void Status::accept(string black, string white, int who_)
{
    game_status = 1;
    accept_ = 0;
    who = who_;
    Gomoku g = Gomoku(black, white, 0);
    game.push_back(g);
}

void Status::refuse()
{
    game_status = 0;
    accept_ = 0;
    who = 0;
    if (game.size() > 0)
    {
        game.pop_back();
    }
}

void Status::invited()
{
    accept_ = 1;
}

void Status::add_peer(endpoint_t p)
{
    peer = p;
    peer_valid = 1;
}
void Status::delete_peer()
{
    peer_valid = 0;
}

endpoint_t Status::get_peer()
{
    return peer;
}

void Status::set_address(vector<string> add)
{
    address = add;
}

void Status::print()
{
    for (long unsigned int i = 0; i < address.size(); i++)
    {
        cout << i << ": " << address[i] << endl;
    }
}

string Status::get(int i)
{
    return address[i];
}

int Status::invite_status()
{
    return accept_;
}

int Status::set(int x, int y)
{
    return game[0].set(x, y, who);
}

int Status::set_(int x, int y)
{
    return game[0].set(x, y, 1 - who);
}

int Status::me()
{
    return who;
}

void Status::resign()
{
    game[0].win(1 - who);
    refuse();
}

// int main(int argc, char **argv)
// {
//     // Gomoku g = Gomoku("123.123.123.123:12345", "123.123.123.123:54321", 0);
//     // g.set(0, 0, 0);
//     // g.set(1, 1, 1);
//     // g.set(0, 1, 0);
//     // g.set(2, 1, 1);
//     // g.set(0, 2, 0);
//     // g.set(3, 1, 1);
//     // g.set(0, 3, 0);
//     // g.set(4, 1, 1);
//     // g.set(0, 4, 0);
//     // g.set(5, 1, 1);
//     char c[] = "set 7,9";
//     char *cmd = strtok(c, " ");

//     cmd = strtok(NULL, ",");
//     char *cmd2 = strtok(NULL, ",");

//     cout << cmd2 << endl;

//     return 0;
// }