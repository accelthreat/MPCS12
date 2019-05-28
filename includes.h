#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using namespace std;

enum class LimbType { HAND, FEET };

enum class PlayerType { human = 0, alien = 1, zombie = 2, dog = 3 };

string playerTypeToString(PlayerType a) {
  if (a == PlayerType::human) return "human";
  if (a == PlayerType::alien) return "alien";
  if (a == PlayerType::zombie) return "zombie";
  if (a == PlayerType::dog) return "dog";
  return "error";
}

class Limb {
 protected:
  int cur_digits;
  int max_digits;
  bool dead_state;

 public:
  bool isDead() { return dead_state; }
  void set_digits(int x) { cur_digits = x; }
  int get_digits() { return cur_digits; }
  int getMaxDigits() { return max_digits; }
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
    if (cur_digits > max_digits) cur_digits -= max_digits;
    if (cur_digits == max_digits) dead_state = true;
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
  onPlayerSkip,
  onTeamSkip
};

class IObserver {
 public:
  virtual void update(EventType type) = 0;
};

class ISubject {
 public:
  virtual void attach(IObserver* observer) = 0;
  virtual void detach(IObserver* observer) = 0;
  virtual void notify(EventType type) = 0;
};

class Subject : public ISubject {
 protected:
  unordered_set<IObserver*> observers;

 public:
  void attach(IObserver* observer) { observers.insert(observer); }
  void detach(IObserver* observer) { observers.erase(observer); }
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
  bool skip = false;
  PlayerType type;
  vector<Limb*> hands;
  vector<Limb*> feet;
  int turns;

 public:
  Player(int p_team, int p_order) {
    team = p_team;
    order = p_order;
  }
  int getNumberOfLivingLimbs(LimbType type) {
    int counter = 0;
    if (type == LimbType::HAND) {
      for (int i = 0; i < hands.size(); i++) {
        if (!hands[i]->isDead()) counter++;
      }
    } else if (type == LimbType::FEET) {
      for (int i = 0; i < hands.size(); i++) {
        if (!hands[i]->isDead()) counter++;
      }
    }
    return counter;
  }
  Limb* getHand(int i) { return hands[i]; }
  Limb* getFoot(int i) { return feet[i]; }
  int getTurns() { return turns; }
  int getTeamNumber() { return team; }
  int get_ord() { return order; }
  int get_hands() { return hands.size(); }
  int get_feet() { return feet.size(); }
  PlayerType get_type() { return type; }
  void skipTurn() { skip = true; }
  bool ready() { return !skip && !isDead(); }
  void distribute(vector<int> params, LimbType limbType) {
    int limbCounter = 0;
    if (limbType == LimbType::HAND) {
      for (int i = 0; i < hands.size(); i++) {
        Limb* hand = hands[i];
        if (!hand->isDead()) hand->set_digits(params[limbCounter++]);
      }
    } else if (limbType == LimbType::FEET) {
      for (int i = 0; i < feet.size(); i++) {
        Limb* foot = feet[i];
        if (!foot->isDead()) foot->set_digits(params[limbCounter++]);
      }
    }
  }

  int getLimbDigits(string limb) { return limb_ptr(limb)->get_digits(); }

  virtual void attacked(string tgt_limb, int add) {
    Limb* l = limb_ptr(tgt_limb);
    l->attacked(add);
    if (dynamic_cast<Foot*>(l) != nullptr && l->isDead())
      notify(EventType::onFootDie);
    notify(EventType::onAttacked);
  }

  bool isDead() {
    bool dead = true;
    for (int i = 0; i < hands.size(); i++) {
      if (!hands[i]->isDead()) dead = false;
    }
    for (int i = 0; i < feet.size(); i++) {
      if (!feet[i]->isDead()) dead = false;
    }
    return dead;
  }

  string getPrintableStatus() {
    stringstream ss;
    ss << "P" << order << playerTypeToString(type)[0] << " (";
    for (int i = 0; i < hands.size(); i++) {
      string digits = "X";
      if (!hands[i]->isDead()) digits = to_string(hands[i]->get_digits());
      ss << digits;
    }

    ss << ":";

    for (int i = 0; i < feet.size(); i++) {
      string digits = "X";
      if (!feet[i]->isDead()) digits = to_string(feet[i]->get_digits());
      ss << digits;
    }
    // D: Adds another message if a player is skipped.
    if (skip) ss << ":SKIP";
    ss << ")";
    return ss.str();
  }

  bool turn_skip() {
    if (!skip) return false;
    skip = false;
    return true;
  }

  Limb* limb_ptr(string limb) {
    char l = limb[0];
    int i = limb[1] - 65;
    if (l == 'H') {
      if (type == PlayerType::dog) return nullptr;
      return hands[i];
    } else if (l == 'F') {
      if (type == PlayerType::zombie) return nullptr;
      return feet[i];
    } else {
      return nullptr;
    }
  }
};

class IPlayerObserver : public IObserver {
 protected:
  EventType type;
  Player* p;
  IPlayerObserver(Player* p) { this->p = p; }
};

class OnFootDie : public IPlayerObserver {
 public:
  OnFootDie(Player* p) : IPlayerObserver(p) { type = EventType::onFootDie; }
  void update(EventType type) {
    if (this->type == type) {
      p->skipTurn();
    }
  }
};

class OnTapDog : public IPlayerObserver {
 public:
  OnTapDog(Player* p) : IPlayerObserver(p) { type = EventType::hasAttackedDog; }
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
    turns = 1;
    type = PlayerType::human;
    attach(new OnFootDie(this));
    attach(new OnTapDog(this));
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
    turns = 1;
    type = PlayerType::alien;
    attach(new OnTapDog(this));

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
  void attacked(string tgt_limb, int add) {
    Limb* l = limb_ptr(tgt_limb);
    l->attacked(add);
    if (l->isDead() && !hasRegenerated) regenerate();
    if (dynamic_cast<Foot*>(l) != nullptr && l->isDead())
      notify(EventType::onFootDie);
    notify(EventType::onAttacked);
  }
};

Zombie::Zombie(int p_team, int p_order) : Player(p_team, p_order) {
  turns = 2;
  type = PlayerType::zombie;
  attach(new OnTapDog(this));
  Limb* h1 = new Hand(4);

  hands.push_back(h1);
}

class Doggo : public Player {
 public:
  Doggo(int p_team, int p_order) : Player(p_team, p_order) {
    turns = 1;
    type = PlayerType::dog;
    attach(new OnFootDie(this));
    for (int i = 0; i < 4; i++) {
      Limb* f = new Foot(4);
      feet.push_back(f);
    }
  }
};
