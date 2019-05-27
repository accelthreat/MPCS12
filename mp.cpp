#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include "game.h"
#include "socketstream/socketstream.hh"

using namespace std;
using namespace swoope;

class ICommand {
   public:
    virtual void execute() = 0;
};

enum class Commands {
    InputPlayerType = 'a',
    InputTeamNumber = 'b',
    Standby = 'c',
    Acknowledged = 'd',
    InputPlayerAction = 'e',
    GameOver = 'f'
};

char commandToChar(Commands c) {
    return static_cast<char>(c);
}

Commands charToCommand(char c) {
    return static_cast<Commands>(c);
}

class Game {
   private:
    int n_players;
    int n_teams;
    int curr_index;
    vector<Team*> teams;
    vector<Player*> players;
    Player* currentPlayer;
    Server* server;

   public:
    Game() {
        curr_index = 0;
        cin >> n_players >> n_teams;

        for (int i = 1; i <= n_teams; i++) {
            Team* t = new Team(i);
            teams.push_back(t);
        }

        for (int i = 0; i < n_players; i++) {
            int p_team;
            string p_type;
            cin >> p_type >> p_team;
            Player* p = create_player(p_type, p_team, i + 1);
            teams[p_team - 1]->add_player(p);
            players.push_back(p);
        }
    }

    Game(Server* server, vector<int> teamNumbers, vector<string> playerTypes) {
        this->server = server;
        n_teams = unordered_set<int>(teamNumbers.begin(), teamNumbers.end()).size();
        for (int i = 1; i <= n_teams; i++) {
            Team* t = new Team(i);
            teams.push_back(t);
        }
        n_players = playerTypes.size();
        for (int i = 0; i < n_players; i++) {
            Player* p = create_player(playerTypes[i], teamNumbers[i], i + 1);
            teams[teamNumbers[i] - 1]->add_player(p);
            players.push_back(p);
        }
    }

    Player* create_player(string type, int team, int num) {
        Player* p;
        if (type == "human")
            p = new Human(team, num);
        else if (type == "alien")
            p = new Alien(team, num);
        else if (type == "doggo")
            p = new Doggo(team, num);
        else if (type == "zombie")
            p = new Zombie(team, num);
        return p;
    }

    string status() {
        stringstream ss;
        for (int t = 0; t < n_teams; t++) {
            ss << teams[t]->getPrintableStatus() << "\n";
        }
        ss << "\n";
        return ss.str();
    }

    bool OneLeft() {
        int counter = 0;
        for (int i = 0; i < teams.size(); i++) {
            if (!teams[i]->isDead()) counter++;
        }
        return counter == 1;
    }

    void updateTeamStatus() {
        for (int i = 0; i < teams.size(); i++) teams[i]->checkDead();
    }

    void declareWinner() {
        //status();
        server->broadcastStatus(status());
        int winner = 0;
        for (int i = 0; i < teams.size(); i++) {
            if (!teams[i]->isDead()) {
                winner = i + 1;
                cout << "Team " << i + 1 << " wins!" << endl;
            }
        }
        server->declareWinner(winner);
    }

    Player* getPlayer() {
        while (true) {
            if (!teams[curr_index]->isDead()) {
                Player* p = teams[curr_index]->getCurrentPlayer();
                if (p != nullptr) {
                    curr_index = (curr_index + 1) % n_teams;
                    currentPlayer = p;
                    return p;
                }
            }
            curr_index = (curr_index + 1) % n_teams;
        }
        // cout << "NullTeam" << endl;
        return nullptr;
    }

    void playerTurn(Player* p) {
        for (int i = 0; i < p->getTurns(); i++) {
            string act = server->getPlayerAction(p->get_ord());
            action(p, act);
            if (OneLeft()) {
                declareWinner();
                return;
            }
        }
        //cout << status();
        server->broadcastStatus(status());
    }

    void Start() {
        //cout << status();
        server->broadcastStatus(status());
        while (!OneLeft()) {
            Player* p = getPlayer();
            playerTurn(p);
        }
    }
    bool verifyAction(int playerIndex, string action) {
        stringstream ss(action);
        string command;
        ss >> command;
        Player* currPlayer = players[playerIndex];
        if (command == "tap") {
            int tgt_player;
            string limb, tgt_limb;
            ss >> limb >> tgt_player >> tgt_limb;
            Player* target = players[tgt_player - 1];
            if (target->getTeamNumber() == currPlayer->getTeamNumber())
                return false;
            if (target->isDead())
                return false;
            Limb* attack_limb = currPlayer->limb_ptr(limb);
            Limb* target_limb = target->limb_ptr(tgt_limb);
            if (attack_limb == nullptr || attack_limb->isDead())
                return false;
            if (target_limb == nullptr || target_limb->isDead())
                return false;
        } else if (command == "disthands") {
            vector<int> params;
            int x;
            while (ss >> x) {
                params.push_back(x);
            }
            if (params.size() > currPlayer->getNumberOfLivingLimbs(LimbType::HAND))
                return false;
            for (int i = 0; i < params.size(); i++) {
                //mali to
                if (currPlayer->getHand(i)->get_digits() == params[i])
                    return false;
            }
            if (currPlayer->getNumberOfLivingLimbs(LimbType::HAND) == 1)
                return false;
        } else {
            return false;
        }
        return true;
    }
    void action(Player* p, string action) {
        stringstream ss(action);
        string command;
        ss >> command;
        if (command == "tap") {
            int tgt_player;
            string limb, tgt_limb;
            ss >> limb >> tgt_player >> tgt_limb;
            attack(p, limb, tgt_player, tgt_limb);
        } else if (command == "disthands") {
            vector<int> params;
            int x;
            while (ss >> x) {
                params.push_back(x);
            }
            p->distribute(params, LimbType::HAND);
        }
        updateTeamStatus();
    }

    void action(Player* p) {
        debug_print("action");
        string command;
        cin >> command;
        if (command == "tap") {
            int tgt_p;
            string l, tgt_l;
            cin >> l >> tgt_p >> tgt_l;
            attack(p, l, tgt_p, tgt_l);
        } else {
            string limb = command.substr(4, 4);
            vector<int> params;
            int index;
            LimbType type;
            if (limb == "feet") {
                type = LimbType::FEET;
                index = p->get_feet();
            } else {
                type = LimbType::HAND;
                index = p->get_hands();
            }
            for (int i = 0; i < index; i++) {
                int x;
                cin >> x;
                params.push_back(x);
            }
            p->distribute(params, type);
        }
        updateTeamStatus();
    }

    void attack(Player* atk_player, string atk_limb, int tgt_p, string tgt_limb) {
        Player* tgt_player = players[tgt_p - 1];
        tgt_player->attacked(tgt_limb, atk_player->getLimbDigits(atk_limb));
        atk_player->notify(EventType::hasAttacked);
        if (tgt_player->get_type() == PlayerType::dog)
            atk_player->notify(EventType::hasAttackedDog);
    }
};

class Common {
   protected:
    int n;
    int port;
    int playerNumber = 0;
    int teamNumber;
    string playerType;
    Player* player;

   public:
    int getTeamNumber() {
        return teamNumber;
    }
    bool validPlayerType(string a) {
        if (a == "human" || a == "alien" || a == "zombie" || a == "doggo")
            return true;
        return false;
    }
    void inputPlayerType() {
        cout << "Ready" << endl;
        string playerType;
        while (!cin.good() || !validPlayerType(playerType)) {
            cout << "Pick a player type: " << endl;
            cout << "human" << endl
                 << "alien" << endl
                 << "doggo" << endl
                 << "zombie" << endl;
            cin >> playerType;
        }
    }
    void inputTeamNumber() {
        while (!cin.good() || teamNumber < 1 || teamNumber > n) {
            cout << "Input your team number from 1-" << n << endl;
            cin >> teamNumber;
        }
    }
};

class Server : public Common {
   private:
    socketstream listener;
    socketstream* clients;
    Game* game;

   public:
    Server(int port, int n) {
        this->port = port;
        this->n = n;
    }
    bool validTeamNumber(vector<int> teamNumbers) {
        vector<int> uniqueNumbers(6);
        vector<int>::iterator ip = std::unique_copy(teamNumbers.begin(), teamNumbers.begin() + 12, uniqueNumbers.begin());
        uniqueNumbers.resize(distance(uniqueNumbers.begin(), ip));
        sort(uniqueNumbers.begin(), uniqueNumbers.end());
        int n_teams = uniqueNumbers.size();
        if (n_teams < 2)
            return false;
        for (int i = 1; i <= n_teams; i++) {
            if (uniqueNumbers[i] != i) {
                return false;
            }
        }
        return true;
    }
    void initialize() {
        vector<string> playerTypes;
        vector<int> teamNumbers;
        listener.open(port, n - 1);

        cout << "Waiting for connection...\n";
        clients = new socketstream[n];

        for (int i = 1; i < n; i++) {
            cout << "Waiting for P" << (i + 1) << "...\n";
            listener.accept(clients[i]);
            clients[i] << i << endl;
            cout << "P" << (i + 1) << " ready.\n";
        }
        inputPlayerType();
        playerTypes.push_back(playerType);
        for (int i = 1; i < n; i++) {
            clients[i] << commandToChar(Commands::InputPlayerType) << endl;
            string s;
            getline(clients[i], s);
            playerTypes.push_back(s);
        }
        while (!validTeamNumber(teamNumbers)) {
            inputTeamNumber();
            teamNumbers.push_back(this->teamNumber);
            for (int i = 1; i < n; i++) {
                clients[i] << commandToChar(Commands::InputTeamNumber) << endl;
                int t;
                clients[i] >> t;
                clients[i].ignore();
                teamNumbers.push_back(t);
            }
        }
        for (int i = 1; i < n; i++) {
            clients[i] << commandToChar(Commands::Acknowledged) << endl;
        }
        game = new Game(this, teamNumbers, playerTypes);
        game->Start();
    }
    string getPlayerAction(int playerIndex) {
        string action;
        if (playerIndex == 0) {
            while (!game->verifyAction(playerIndex, action)) {
                getline(cin, action);
            }
        } else {
            while (!game->verifyAction(playerIndex, action)) {
                clients[playerIndex] << commandToChar(Commands::InputPlayerAction) << endl;
                getline(clients[playerIndex], action);
            }
            clients[playerIndex] << commandToChar(Commands::Acknowledged) << endl;
        }
        return action;
    }
    void broadcastStatus(string status) {
        cout << status;
        for (int i = 1; i < n; i++) {
            clients[i] << status << endl;
        }
    }
    void declareWinner(int winner) {
        if (teamNumber == winner) {
            cout << "Congratulations: Team " << winner << " wins!" << endl;
        } else {
            cout << "You Lose; Team " << winner << " wins!" << endl;
        }
        for (int i = 1; i < n; i++) {
            clients[i] << commandToChar(Commands::GameOver) << endl;
            clients[i] << winner << endl;
        }
    }
    void start() {
        game->Start();
    }
};

class Client : public Common {
   private:
    string ip;
    socketstream server;

   public:
    Client(int port, string ip) {
        this->port = port;
        this->ip = ip;
    }
    bool initialize() {
        char cmd;
        server.open(ip, port);
        if (!server.good()) return false;
        server >> playerNumber;
        server.ignore();
        cout << "Your player number is: " << playerNumber + 1 << endl;
        bool ready = false;
        cout << "Waiting for other players" << endl;
        server >> ready;
        server.ignore();
        cout << "Ready" << endl;
        server >> cmd;
        server.ignore();
        if (charToCommand(cmd) == Commands::InputPlayerType) {
            inputPlayerType();
            server << playerType << endl;
        }
        while (charToCommand(cmd) != Commands::Acknowledged) {
            server >> cmd;
            server.ignore();
            if (charToCommand(cmd) == Commands::InputTeamNumber) {
                inputTeamNumber();
                server << playerType << endl;
            }
        }
        while (true) {
            string status;
            getline(server, status);
            cout << status << endl;
            server >> cmd;
            server.ignore();
            if (charToCommand(cmd) == Commands::InputPlayerAction) {
                while (charToCommand(cmd) == Commands::InputPlayerAction) {
                    string action;
                    getline(cin, action);
                    server << action << endl;
                    server >> cmd;
                    server.ignore();
                }
            } else if (charToCommand(cmd) == Commands::Standby) {
                //Do nothing
            } else if (charToCommand(cmd) == Commands::GameOver) {
                int winner;
                server >> winner;
                server.ignore();
                stringstream ss;
                if (teamNumber == winner) {
                    cout << "Congratulations: Team " << winner << " wins!\n";
                } else {
                    cout << "You Lose; Team " << winner << " wins!\n";
                }
                break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    int p = atoi(argv[1]);
    if (p < 1024 || p > 65535) {
        cout << "ERROR: Port must be within range of 1024-65535" << endl;
        return 0;
    }
    if (argc == 2) {
        int n;
        while (!cin.good() || n < 2 || n > 6) {
            cin >> n;
        }
        Server* server = new Server(p, n);
        server->initialize();
    } else if (argc == 3) {
        string ip = argv[2];
        Client* client = new Client(p, ip);
        if (!client->initialize()) {
            cout << "ERROR: IP INVALID" << endl;
            return 0;
        }
    }
}
