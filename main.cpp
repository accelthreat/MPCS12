#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#define DEBUG 0
#define debug_print(args) \
    if (DEBUG) cout << args << endl

using namespace std;

#define HAND 0
#define FEET 1

enum class PlayerType {
    human = 0,
    alien = 1,
    zombie = 2,
    dog = 3
};

string playerTypeToString(PlayerType a) {
    if (a == PlayerType::human)
        return "human";
    if (a == PlayerType::alien)
        return "alien";
    if (a == PlayerType::zombie)
        return "zombie";
    if (a == PlayerType::dog)
        return "dog";
}

class Limb {
   protected:
    int cur_digits;
    int max_digits;
    bool dead_state;

   public:
    void get_data() {
        if (dead_state)
            cout << "X";
        else
            cout << cur_digits;
    }
    bool isDead() {
        return dead_state;
    }
    void set_digits(int x) {
        cur_digits = x;
    }
    int get_digits() {
        return cur_digits;
    }
    int getMaxDigits() {
        return max_digits;
    }
    virtual void attacked(int x) = 0;
};

class Hand : public Limb {
   public:
    Hand(int d, bool dead = false) {
        cur_digits = 1;
        max_digits = d;
        dead_state = dead;
    }

    void attacked(int x) {
        cur_digits += x;
        if (cur_digits > max_digits)
            cur_digits -= max_digits;
        if (cur_digits == max_digits) dead_state = true;
        debug_print("hand attacked");
    }
};

class Foot : public Limb {
   public:
    Foot(int d, bool dead = false) {
        cur_digits = 1;
        max_digits = d;
        dead_state = dead;
    }
    void attacked(int x) {
        cur_digits += x;
        if (cur_digits >= max_digits) dead_state = true;
    }
};

enum class EventType {
    onFootDie,
    onAttacked,
    hasAttacked,
    hasAttackedDog,
    onZombieHandDie
};

class IObserver {
   public:
    virtual void update(EventType type) = 0;

   protected:
    EventType type;
};

//Add skipping when foot is dead

class Subject {
   protected:
    unordered_set<IObserver*> observers;

   public:
    void attach(IObserver* observer) {
        observers.insert(observer);
    }
    void detach(IObserver* observer) {
        observers.erase(observer);
    }
    void notify(EventType type) {
        for (auto iter = observers.begin(); iter != observers.end(); ++iter) {
            (**iter).update(type);
        }
    }
};

class Player : public Subject {
   protected:
    int team;
    int order;
    bool skip;
    PlayerType type;
    vector<Limb*> hands;
    vector<Limb*> feet;

   public:
    Player(int p_team, int p_order) {
        team = p_team;
        order = p_order;
        skip = false;
    }

    int get_ord() { return order; }
    int get_hands() { return hands.size(); }
    int get_feet() { return feet.size(); }
    PlayerType get_type() { return type; }
    void skipTurn() { skip = true; }

    void distribute(vector<int> params, int limbType) {
        if (limbType == HAND) {
            for (int i = 0; i < hands.size(); i++) {
                hands[i]->set_digits(params[i]);
            }
        } else if (limbType == FEET) {
            for (int i = 0; i < feet.size(); i++) {
                feet[i]->set_digits(params[i]);
            }
        }
    }

    int getLimbDigits(string limb) {
        return limb_ptr(limb)->get_digits();
    }

    void attacked(string tgt_limb, int add) {
        Limb* l = limb_ptr(tgt_limb);
        l->attacked(add);
        if (dynamic_cast<Foot*>(l) != nullptr && l->isDead())
            notify(EventType::onFootDie);
        notify(EventType::onAttacked);
    }

    bool isDead() {
        bool dead = true;
        for (int i = 0; i < hands.size(); i++) {
            if (!hands[i]->isDead())
                dead = false;
        }
        for (int i = 0; i < feet.size(); i++) {
            if (!feet[i]->isDead())
                dead = false;
        }
        return dead;
    }
    void print() {
        cout << "P" << order << playerTypeToString(type)[0] << " (";
        int h = hands.size();
        for (int i = 0; i < h; i++)
            hands[i]->get_data();
        cout << ":";

        int f = feet.size();
        for (int i = 0; i < f; i++)
            feet[i]->get_data();
        cout << ")";
    }

    bool turn_skip() {
        if (!skip)
            return false;
        skip = false;
        return true;
    }

    Limb* limb_ptr(string limb) {
        char l = limb[0];
        int i = limb[1] - 65;
        if (l == 'H') {
            return hands[i];
        } else {
            return feet[i];
        }
    }
};

class IPlayerObserver : public IObserver {
   protected:
    Player* p;
    IPlayerObserver(Player* p) {
        p->attach(this);
        this->p = p;
    }
};
class OnFootDie : public IPlayerObserver {
   public:
    OnFootDie(Player* p) : IPlayerObserver(p) {
        type = EventType::onFootDie;
    }
    void update(EventType type) {
        if (this->type == type) {
            // cout << "Wow may namatay na foot" << endl;
            p->skipTurn();
        }
    }
};

class OnTapDog : public IPlayerObserver {
   public:
    OnTapDog(Player* p) : IPlayerObserver(p) {
        type = EventType::hasAttackedDog;
    }
    void update(EventType type) {
        if (this->type == type) {
            // cout << "Wow may ng hit ng dog" << endl;
            p->skipTurn();
        }
    }
};

class Human : public Player {
   public:
    Human(int p_team, int p_order) : Player(p_team, p_order) {
        type = PlayerType::human;
        observers.insert(new OnFootDie(this));
        observers.insert(new OnTapDog(this));
        for (int i = 0; i < 2; i++) {
            Limb* h = new Hand(5);
            hands.push_back(h);
        }

        for (int i = 0; i < 2; i++) {
            Limb* f = new Foot(5);
            feet.push_back(f);
        }
    }
};

class Alien : public Player {
   public:
    Alien(int p_team, int p_order) : Player(p_team, p_order) {
        type = PlayerType::alien;
        observers.insert(new OnTapDog(this));
        for (int i = 0; i < 4; i++) {
            Limb* h = new Hand(3);
            hands.push_back(h);
        }

        for (int i = 0; i < 2; i++) {
            Limb* f = new Foot(2);
            feet.push_back(f);
        }
    }
};

class Zombie : public Player {
   public:
    bool hasRegenerated = false;
    Zombie(int p_team, int p_order);
    void regenerate() {
        Limb* h2 = new Hand(4);
        hands.push_back(h2);
        hasRegenerated = true;
    }
};

class OnZombieHandDie : public IPlayerObserver {
   public:
    OnZombieHandDie(Player* p) : IPlayerObserver(p) {
        type = EventType::onAttacked;
    }
    void update(EventType type) {
        if (this->type == type) {
            if (p->get_type() == PlayerType::zombie && p->getLimbDigits("HA") == p->limb_ptr("HA")->getMaxDigits()) {
                Zombie* z = static_cast<Zombie*>(p);
                z->regenerate();
                z->detach(this);
            }
        }
    }
};

Zombie::Zombie(int p_team, int p_order) : Player(p_team, p_order) {
    type = PlayerType::zombie;
    observers.insert(new OnTapDog(this));
    observers.insert(new OnZombieHandDie(this));
    Limb* h1 = new Hand(4);

    hands.push_back(h1);
}

class Doggo : public Player {
   public:
    Doggo(int p_team, int p_order) : Player(p_team, p_order) {
        type = PlayerType::dog;
        observers.insert(new OnFootDie(this));
        for (int i = 0; i < 4; i++) {
            Limb* f = new Foot(4);
            feet.push_back(f);
        }
    }
};

class Team {
   private:
    int team_num;
    int n_players;
    int curr_index;
    vector<Player*> players;
    bool dead = false;

   public:
    Team(int n) {
        team_num = n;
        n_players = 0;
        curr_index = 0;
    }
    int getTeamNumber() {
        return team_num;
    }
    bool isDead() {
        return dead;
    }
    void add_player(Player* p) {
        n_players++;
        players.push_back(p);
    }

    Player* getPlayer(int i) {
        return players[i];
    }
    void checkDead() {
        for (int i = 0; i < players.size(); i++) {
            if (!players[i]->isDead()) {
                dead = false;
                return;
            }
        }
        dead = true;
    }
    //[0,1]
    Player* getCurrentPlayer() {
        // cout << "Team " << team_num << endl;
        //cout << "Curr_index_before" << curr_index << endl;
        int orig_index = curr_index;
        for (int index = curr_index; index < players.size(); index++) {
            if (!players[index]->turn_skip() && !players[index]->isDead()) {
                curr_index = index + 1;
                return players[index];
                debug_print("dafuq");
            }
        }
        for (int index = 0; index < orig_index; index++) {
            if (!players[index]->turn_skip() && !players[index]->isDead()) {
                curr_index = index + 1;
                return players[index];
                debug_print("dafuq");
            }
        }
        debug_print("Null");
        return nullptr;
    }

    int get_size() { return n_players; }
};

class Game {
   private:
    int n_players;
    int n_teams;
    vector<Team*> teams;
    vector<Player*> players;

   public:
    Game() {
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
        status();
        Start();
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

    void status() {
        for (int t = 0; t < n_teams; t++) {
            int p_num = teams[t]->get_size();
            cout << "Team " << teams[t]->getTeamNumber() << ": ";
            for (int p = 0; p < p_num; p++) {
                teams[t]->getPlayer(p)->print();
                if (p < p_num - 1) cout << " | ";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    bool OneLeft() {
        int counter = 0;
        for (int i = 0; i < teams.size(); i++) {
            if (!teams[i]->isDead())
                counter++;
        }
        return counter == 1;
    }

    void updateTeamStatus() {
        for (int i = 0; i < teams.size(); i++)
            teams[i]->checkDead();
    }
    void Start() {
        int t = 0;
        while (!OneLeft()) {
            //Check if Team is dead; move this to an observer
            teams[t]->checkDead();
            if (!teams[t]->isDead()) {
                Player* p = teams[t]->getCurrentPlayer();
                if (p != nullptr) {
                    action(p);
                    updateTeamStatus();
                    if (OneLeft()) {
                        status();
                        break;
                    }

                    if (p->get_type() == PlayerType::zombie) {
                        action(p);
                        updateTeamStatus();
                        if (OneLeft()) {
                            status();
                            break;
                        }
                    }
                    debug_print("Finish Action");
                    status();
                } else {
                    debug_print("Skip team");
                }
            }
            t++;
            if (t == teams.size())
                t = 0;
        }
        for (int i = 0; i < teams.size(); i++) {
            if (!teams[i]->isDead()) {
                cout << "Team " << i + 1 << " wins!" << endl;
            }
        }
    }

    void action(Player* p) {
        //if (p->turn_skip()) return;
        //cout << "Player " << p->get_ord() << endl;
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
            if (limb == "feet") {
                for (int i = 0; i < p->get_feet(); i++) {
                    int x;
                    cin >> x;
                    params.push_back(x);
                }
                p->distribute(params, FEET);
            } else {
                for (int i = 0; i < p->get_hands(); i++) {
                    int x;
                    cin >> x;
                    params.push_back(x);
                }
                p->distribute(params, HAND);
            }
        }
    }

    void attack(Player* atk_player, string atk_limb, int tgt_p, string tgt_limb) {
        Player* tgt_player = players[tgt_p - 1];
        tgt_player->attacked(tgt_limb, atk_player->getLimbDigits(atk_limb));
        atk_player->notify(EventType::hasAttacked);
        if (tgt_player->get_type() == PlayerType::dog)
            atk_player->notify(EventType::hasAttackedDog);
    }
};

int main() {
    Game();
    return 0;
}