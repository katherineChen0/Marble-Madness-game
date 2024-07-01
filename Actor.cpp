#include "Actor.h"
#include "StudentWorld.h"

// Students:  Add code to this file, Actor.h, StudentWorld.h, and StudentWorld.cpp

// MARK: Player
void Player::doSomething() {
    // if dead do nothing
    if(hp() <= 0) return;
    
    // get user input
    int dir;
    if(world()->getKey(dir)) {
        switch(dir) {
            case KEY_PRESS_UP:
                dir = up;
                break;
            case KEY_PRESS_DOWN:
                dir = down;
                break;
            case KEY_PRESS_LEFT:
                dir = left;
                break;
            case KEY_PRESS_RIGHT:
                dir = right;
                break;
            case KEY_PRESS_SPACE:
                shoot();
                return;
            case KEY_PRESS_ESCAPE:
                sethp(0);
                return;
            default:
                return;
        }
        setDirection(dir);
        world()->movePlayer();
    }
}

void Player::shoot() {
    if(m_peas > 0) {
        double x = getX();
        double y = getY();
        world()->targetCoords(x, y, getDirection());
        world()->addActor(new Pea(world(), x, y, getDirection()));
        world()->playSound(SOUND_PLAYER_FIRE);
        m_peas--;
    }
}

// MARK: Pea
void Pea::doSomething() {
    // if dead do nothing
    if(hp() <= 0) return;
    
    // do not move if just spawned
    if(!m_moved) {
        m_moved = true;
        return;
    }
    // attempt to move pea
    world()->movePea(this);
}

// MARK: Pit
void Pit::doSomething() {
    // if dead do nothing
    if(hp() <= 0) return;

    // Detect Marble and Swallow
    Actor* actor = world()->actorAtCoords(this, getX(), getY());
    if(actor != nullptr && actor->pushable()) {
        sethp(0);
        actor->sethp(0);

        setVisible(false);
        actor->setVisible(false);
    }
}

// MARK: Exit
void Exit::doSomething() {
    // update visibility
    if(world()->crystalsLeft() == 0) {
        setVisible(true);
        m_isOpen = true;
    }

    if(m_isOpen && world()->player()->getX() == getX() && world()->player()->getY() == getY()) {
        world()->increaseScore(2000);
        world()->playSound(SOUND_FINISHED_LEVEL);
        world()->setComplete(true);
    }
}

// MARK: Pickup
void Pickup::doSomething() {
    // if dead or invisible do nothing
    if(hp() <= 0 || isStolen()) return;

    // if player is on square, get picked up
    if(world()->player()->getX() == getX() && world()->player()->getY() == getY()) {
        sethp(0);
        setVisible(false);
        world()->playSound(SOUND_GOT_GOODIE);
        pickUp();
    }
}

// MARK: Crystal
void Crystal::pickUp() {
    world()->increaseScore(50);
    world()->decCrystals();
}

// MARK: ExtraLife
void ExtraLife::pickUp() {
    world()->increaseScore(1000);
    world()->incLives();
}

// MARK: RestoreHealth
void RestoreHealth::pickUp() {
    world()->increaseScore(500);
    world()->playSound(SOUND_GOT_GOODIE);
    world()->player()->sethp(20);
}

// MARK: Ammo
void Ammo::pickUp() {
    world()->increaseScore(100);
    world()->playSound(SOUND_GOT_GOODIE);
    world()->player()->incPeas(20);
}

// MARK: Bot
void Bot::shoot() {
    double x = getX();
    double y = getY();
    world()->targetCoords(x, y, getDirection());
    world()->addActor(new Pea(world(), x, y, getDirection()));
    world()->playSound(SOUND_ENEMY_FIRE);
}

// MARK: RageBot
void RageBot::doSomething() {
    // if dead do nothing
    if(hp() <= 0) return;

    // calculate number of ticks to wait
    int ticks = (28 - world()->getLevel()) / 4;
    if(ticks < 3) ticks = 3;
    
    if(world()->curTick() % ticks == 0) {
        if(world()->canShootAtPlayer(this)) {
            shoot();
        } else {
            world()->moveRageBot(this);
        }
    }
}

bool RageBot::takeDamage() {
    sethp(hp() - 2);
    if(hp() > 0) {
        world()->playSound(SOUND_ROBOT_IMPACT);
    } else {
        // if dead
        setVisible(false);
        world()->playSound(SOUND_ROBOT_DIE);
        world()->increaseScore(100);
    }
    return true;
}

// MARK: ThiefBot
void ThiefBot::doSomething() {
    // if dead do nothing
    if(hp() <= 0) return;

    // calculate number of ticks to wait
    int ticks = (28 - world()->getLevel()) / 4;
    if(ticks < 3) ticks = 3;

    if(world()->curTick() % ticks == 0) {
        Actor* toSteal = world()->canSteal(this);
        if(m_isMean && world()->canShootAtPlayer(this)) {
            shoot();
        } else if(toSteal != nullptr && !toSteal->isStolen() && rand() % 10 == 0) {
            toSteal->setVisible(false);
            toSteal->setStolen(true);
            m_stolenGoods = toSteal;
            world()->playSound(SOUND_ROBOT_MUNCH);
        } else {
            world()->moveThiefBot(this);
        }
    }
}

bool ThiefBot::takeDamage() {
    sethp(hp() - 2);
    if(hp() > 0) {
        world()->playSound(SOUND_ROBOT_IMPACT);
    } else {
        // if dead
        setVisible(false);
        world()->playSound(SOUND_ROBOT_DIE);
        // increment score differently for different meanness
        if(m_isMean) {
            world()->increaseScore(20);
        } else {
            world()->increaseScore(10);
        }
        // drop stolen goods
        if(stolenGoods() != nullptr) {
            stolenGoods()->moveTo(getX(), getY());
            stolenGoods()->setVisible(true);
            stolenGoods()->setStolen(false);
        }
    }
    return true;
}

void ThiefBot::turn() {
    int directions[] = {up, down, left, right};

    double x = getX();
    double y = getY();

    int index = rand() % 4;
    setDirection(directions[index]);

    // attempt to set random direction
    for(int i = 1; i <= 4; i++) {
        world()->targetCoords(x, y, directions[index]);
        if(world()->botCanMoveHere(x, y)) {
            setDirection(directions[index]);
            moveTo(x, y);
            return;
        }

        index = (index + 1) % 4;
    }
}

// MARK: ThiefBotFactory
void ThiefBotFactory::doSomething() {
    if(world()->countThiefBots(getX(), getY()) < 3) {
        if(rand() % 50 == 0) {
            if(m_isMean) {
                world()->addActor(new ThiefBot(world(), IID_MEAN_THIEFBOT, 5, getX(), getY(), true));
            } else {
                world()->addActor(new ThiefBot(world(), IID_THIEFBOT, 8, getX(), getY(), false));
            }
            world()->playSound(SOUND_ROBOT_BORN);
        }
    }
}
