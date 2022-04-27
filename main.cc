#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

void printBoard(string state) {
  for (int x = 0; x < 3; ++x) {
    for (int y = 0; y < 3; ++y) {
      cout << state[x * 3 + y];
    }
    cout << '\n';
  }
}

unsigned seed = chrono::system_clock::now().time_since_epoch().count();
default_random_engine rng(seed);

class Tic {
  char state[9];
  char turn;

 public:
  Tic() {
    for (int i = 0; i < 9; ++i) {
      state[i] = '-';
    }
    turn = 'X';
  }

  string toString() {
    string res;
    for (auto& square : state) {
      res += square;
    }
    return res;
  }

  char win() {
    for (int i = 0; i < 3; ++i) {
      // win by row
      if (state[3 * i] == state[3 * i + 1] &&
          state[3 * i] == state[3 * i + 2] && state[3 * i] != '-') {
        return state[3 * i];
      }
      // win by column
      if (state[i] == state[i + 3] && state[i] == state[i + 6] &&
          state[i] != '-') {
        return state[i];
      }
    }
    // win by diagonal
    if (state[0] == state[4] && state[0] == state[8] && state[0] != '-') {
      return state[0];
    }
    if (state[2] == state[4] && state[2] == state[6] && state[2] != '-') {
      return state[2];
    }
    for (int i = 0; i < 9; ++i) {
      if (state[i] == '-') {
        return '-';
      }
    }
    return '/';
  }

  bool place(int pos) {
    if (state[pos] == '-') {
      state[pos] = turn;
      if (turn == 'X') {
        turn = 'O';
      } else {
        turn = 'X';
      }
      return true;
    }
    return false;
  }

  void print() { printBoard(state); }

  char getTurn() { return turn; }
};

class TicAI {
  map<string, vector<int>> options;

 public:
  void read(string filename) {
    ifstream file(filename);
    string perm;
    int freq;
    while (file >> perm) {
      options.insert({perm, vector<int>(9)});
      for (int i = 0; i < 9; ++i) {
        file >> freq;
        options.at(perm)[i] = freq;
      }
    }
  }

  void write(string filename) {
    ofstream file{filename};
    for (auto& kvp : options) {
      file << kvp.first;
      for (int i : kvp.second) {
        file << ' ' << i;
      }
      file << '\n';
    }
  }

  void print() {
    for (auto& kvp : options) {
      printBoard(kvp.first);
      for (int i : kvp.second) {
        cout << ' ' << i;
      }
    }
  }

  void initState(string state) {
    try {
      options.at(state);
    } catch (out_of_range) {
      options.insert({state, vector<int>(9)});
      for (int i = 0; i < 9; ++i) {
        options.at(state)[i] = 10;
      }
    }
  }

  int chooseMove(string state) {
    initState(state);
    vector<int>& choices = options.at(state);
    int count = 0;
    for (int i = 0; i < 9; ++i) {
      count += choices[i];
    }
    if (count == 0) {
      return -1;
    }
    if (uniform_real_distribution<double>(0, 1)(rng) < 0.05) {
      return uniform_int_distribution<int>(0, 8)(rng);
    }
    int rand = uniform_int_distribution<int>(0, count - 1)(rng);

    for (int i = 0; i < 9; ++i) {
      rand -= choices[i];
      if (rand < 0) {
        if (count > 10000) {
          normalize(state);
          // recount
        }
        return i;
      }
    }
    return -1;
  }

  void set(string state, int square, int val) {
    initState(state);
    options.at(state)[square] = val;
  }

  void reinforce(string state, int square) {
    initState(state);
    options.at(state)[square]++;
  }

  void normalize(string state) {
    for (int i = 0; i < 9; ++i) {
      options.at(state)[i] = (options.at(state)[i] + 1) / 2;
    }
  }
};

void random_place(Tic& game) {
  vector<int> squares;
  for (int i = 0; i < 9; ++i) {
    squares.push_back(i);
  }
  shuffle(squares.begin(), squares.end(), rng);

  for (int i : squares) {
    if (game.place(i)) {
      return;
    }
  }
}

void player_place(Tic& game) {
  game.print();
  int s;
  cin >> s;
  while (!game.place(s)) {
    cin >> s;
  };
}

TicAI ai;

void simulate(int num_of_games) {
  int wins = 0;
  int lose = 0;
  for (int i = 0; i < num_of_games; ++i) {
    vector<pair<string, int>> ai_moves = {};
    vector<pair<string, int>> ai2_moves = {};
    Tic g;

    int chosen;
    while (true) {
      // player_place(g);
      // if (g.win() != '-') {
      //   break;
      // }

      string state = g.toString();
      chosen = ai.chooseMove(state);
      if (chosen == -1) {
        break;
      }

      while (!g.place(chosen)) {
        ai.set(g.toString(), chosen, 0);
        chosen = ai.chooseMove(state);
        if (chosen == -1) {
          break;
        }
      }
      if (chosen == -1) {
        break;
      }
      if (g.getTurn() == 'O') {
        ai_moves.push_back({state, chosen});
      } else {
        ai2_moves.push_back({state, chosen});
      }

      if (g.win() != '-') {
        break;
      }
    }

    char var = g.win();
    char winner = var;
    if (winner == '-') {
      if (g.getTurn() == 'X') {
        winner = 'O';
      } else {
        winner = 'X';
      }
    } else if (winner == '/') {
      continue;
    }
    if (winner == 'X') {
      for (auto& kvp : ai_moves) {
        ai.reinforce(kvp.first, kvp.second);
      }
      pair<string, int>& last_move = ai2_moves.back();
      ai.set(last_move.first, last_move.second, 0);
      wins++;
    } else {
      for (auto& kvp : ai2_moves) {
        ai.reinforce(kvp.first, kvp.second);
      }
      pair<string, int>& last_move = ai_moves.back();
      ai.set(last_move.first, last_move.second, 0);
      lose++;
    }
  }
  cout << "simulated " << num_of_games << " games: \n";
  cout << ">> won/lose/draw: " << wins << '/' << lose << '/'
       << (num_of_games - wins - lose);
}

int main() {
  ai.read("data");
  simulate(1);
  ai.write("data");
}
