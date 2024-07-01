#ifndef ACTOR_H_
#define ACTOR_H_

#include "GraphObject.h"
#include "StudentWorld.h"

// Students:  Add code to this file, Actor.cpp, StudentWorld.h, and StudentWorld.cpp

class StudentWorld;

class Actor : public GraphObject {
public:
    // Constructor
    Actor(StudentWorld* world, int imageID, int hp, double startX, double startY, int dir = none)
    : GraphObject(imageID, startX, startY, dir, 1.0), m_world(world), m_hp(hp) { setVisible(true); }
    virtual void doSomething() {}
    virtual bool takeDamage() { return false; }

    // Getters and Setters
    StudentWorld* world() const { return m_world; }
    int hp() const { return m_hp; }
    void sethp(int newhp) { m_hp = newhp; }
    virtual void setStolen(bool stolen) {}

    // Transparency
    virtual bool playerTransparent() const { return false; }
    virtual bool marbleTransparent() const { return false; }
    virtual bool peaTransparent() const { return false; }
    virtual bool robotTransparent() const { return false; }

    // Attributes
    virtual bool pushable() const { return false; }
    virtual bool stealable() const { return false; }
    virtual bool canSteal() const { return false; }
    virtual bool canSpawn() const { return false; }
    virtual bool isStolen() const { return false; }
private:
    StudentWorld* m_world;
    int m_hp;
};

class Wall : public Actor {
public:
    Wall(StudentWorld* world, double startX, double startY)
    : Actor(world, IID_WALL, 100, startX,  startY) {}
};

class Player : public Actor {
public:
    Player(StudentWorld* world, double startX, double startY)
    : Actor(world, IID_PLAYER, 20, startX, startY, right), m_peas(20) {}
    virtual void doSomething();
    virtual bool takeDamage() {
        sethp(hp() - 2);
        if(hp() > 0) {
            world()->playSound(SOUND_PLAYER_IMPACT);
        } else {
            world()->playSound(SOUND_PLAYER_DIE);
        }
        return true;
    }
    void incPeas(int n) { m_peas += n; }
    int peas() const { return m_peas; }
private:
    int m_peas;
    void shoot();
};

class Marble : public Actor {
public:
    Marble(StudentWorld* world, double startX, double startY)
    : Actor(world, IID_MARBLE, 10, startX, startY, 1) {}
    virtual bool takeDamage() { sethp(hp() - 2); return true; }
    virtual bool pushable() const { return true; }
};

class Pea : public Actor {
public:
    Pea(StudentWorld* world, double startX, double startY, int dir)
    : Actor(world, IID_PEA, 100, startX, startY, dir), m_moved(false) {}
    virtual void doSomething();
    virtual bool playerTransparent() const { return true; }
    virtual bool marbleTransparent() const { return true; }
    virtual bool peaTransparent() const { return true; }
    virtual bool robotTransparent() const { return true; }
private:
    bool m_moved;
};

class Pit : public Actor {
public:
    Pit(StudentWorld* world, double startX, double startY)
    : Actor(world, IID_PIT, 100, startX, startY) {}
    virtual void doSomething();
    virtual bool marbleTransparent() const { return true; }
    virtual bool peaTransparent() const { return true; }
};

class Exit : public Actor {
public:
    Exit(StudentWorld* world, double startX, double startY)
    : Actor(world, IID_EXIT, 100, startX, startY), m_isOpen(false) { setVisible(false); }
    virtual void doSomething();

    virtual bool playerTransparent() const { return true; }
    virtual bool peaTransparent() const { return true; }
    virtual bool robotTransparent() const { return true; }
private:
    bool m_isOpen;
};

class Pickup : public Actor {
public:
    Pickup(StudentWorld* world, int imageID, double startX, double startY)
    : Actor(world, imageID, 100, startX, startY), m_isStolen(false) {}
    virtual void doSomething();

    virtual bool playerTransparent() const { return true; }
    virtual bool peaTransparent() const { return true; }
    virtual bool robotTransparent() const { return true; }

    virtual bool isStolen() const { return m_isStolen; }
    virtual void setStolen(bool stolen) { m_isStolen = stolen; }
private:
    virtual void pickUp() = 0;

    bool m_isStolen;
};

class Crystal : public Pickup {
public:
    Crystal(StudentWorld* world, double startX, double startY)
    : Pickup(world, IID_CRYSTAL, startX, startY) {}
private:
    virtual void pickUp();
};

class ExtraLife : public Pickup {
public:
    ExtraLife(StudentWorld* world, double startX, double startY)
    : Pickup(world, IID_EXTRA_LIFE, startX, startY) {}
    virtual bool stealable() const { return true; }
private:
    virtual void pickUp();
};

class RestoreHealth : public Pickup {
public:
    RestoreHealth(StudentWorld* world, double startX, double startY)
    : Pickup(world, IID_RESTORE_HEALTH, startX, startY) {}
    virtual bool stealable() const { return true; }
private:
    virtual void pickUp();
};

class Ammo : public Pickup {
public:
    Ammo(StudentWorld* world, double startX, double startY)
    : Pickup(world, IID_AMMO, startX, startY) {}
    virtual bool stealable() const { return true; }
private:
    virtual void pickUp();
};

class Bot : public Actor {
public:
    Bot(StudentWorld* world, int imageID, int hp, double startX, double startY, int dir)
    : Actor(world, imageID, hp, startX, startY, dir) {}
    virtual void shoot();
};

class RageBot : public Bot {
public:
    RageBot(StudentWorld* world, double startX, double startY, int dir)
    : Bot(world, IID_RAGEBOT, 10, startX, startY, dir) {}
    virtual void doSomething();
    virtual bool takeDamage();
};

class ThiefBot : public Bot {
public:
    ThiefBot(StudentWorld* world, int imageID, int hp, double startX, double startY, bool isMean)
    : Bot(world, imageID, hp, startX, startY, right), m_isMean(isMean), m_stolenGoods(nullptr), m_spacesMoved(0) {
        m_distanceBeforeTurning = (rand() % 6) + 1;
    }
    virtual void doSomething();
    virtual bool takeDamage();
    virtual bool canSteal() const { return true; }

    void steal(Actor* toSteal);
    void turn();

    int spacedMoved() const { return m_spacesMoved; }
    void setSpacesMoved(int spaces) { m_spacesMoved = spaces; }

    int distanceBeforeTurning() const { return m_distanceBeforeTurning; }
    void setDistanceBeforeTurning(int newDist) { m_distanceBeforeTurning = newDist; }

    Actor* stolenGoods() { return m_stolenGoods; }
private:
    bool m_isMean;
    Actor* m_stolenGoods;
    int m_distanceBeforeTurning;
    int m_spacesMoved;
};

class ThiefBotFactory : public Actor {
public:
    ThiefBotFactory(StudentWorld* world, double startX, double startY, bool isMean)
    : Actor(world, IID_ROBOT_FACTORY, 100, startX, startY), m_isMean(isMean) {}
    virtual void doSomething();
    virtual bool canSpawn() { return true; }
private:
    bool m_isMean;
};

#endif // ACTOR_H_
