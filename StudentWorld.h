#ifndef STUDENTWORLD_H_
#define STUDENTWORLD_H_

#include "GameWorld.h"
#include "Level.h"
#include <string>
#include <sstream>
#include <list>
// Students:  Add code to this file, StudentWorld.cpp, Actor.h, and Actor.cpp

class Actor;
class Player;
class Pea;
class Bot;
class RageBot;
class ThiefBot;

class StudentWorld : public GameWorld {
public:
    // Constructor and Destructor
    StudentWorld(std::string assetPath)
    : GameWorld(assetPath), m_player(nullptr), m_curTick(0), m_bonus(1000) {}
    ~StudentWorld() { cleanUp(); }

    // Required implementations
    virtual int init();
    virtual int move();
    virtual void cleanUp();

    // Modifiers
    void setDisplayText();
    void addActor(Actor* actor) { m_actors.push_back(actor); }
    void decCrystals() { m_crystals--; }
    void setComplete(bool complete) { m_levelComplete = complete; }

    // Getters
    int crystalsLeft() const { return m_crystals; }
    int curTick() const { return m_curTick; }
    void targetCoords(double& x, double& y, int dir) const;
    Actor* actorAtCoords(Actor* caller, double x, double y) const;
    Player* player() const { return m_player; }
    bool canShootAtPlayer(Bot* bot) const;
    bool botCanMoveHere(double x, double y) const;
    Actor* canSteal(ThiefBot* bot) const;
    int countThiefBots(double x, double y) const;

    // Helper Functions
    void movePlayer();
    void movePea(Pea* pea);
    void moveRageBot(RageBot* bot);
    void moveThiefBot(ThiefBot *bot);
private:
    Player* m_player;
    std::list<Actor*> m_actors;

    int m_curTick;
    int m_bonus;
    int m_crystals;
    bool m_levelComplete;
};

#endif // STUDENTWORLD_H_
