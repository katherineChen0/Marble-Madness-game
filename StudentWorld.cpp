#include "StudentWorld.h"
#include "GameConstants.h"
#include "GraphObject.h"
#include "Actor.h"

#include <iostream>
#include <string>
#include <iomanip>
#include <cmath>
using namespace std;

GameWorld* createStudentWorld(string assetPath) {
    return new StudentWorld(assetPath);
}

// MARK: Required Implementations

int StudentWorld::init() {
    m_curTick = 0;
    m_bonus = 1000;
    m_levelComplete = false;
    m_crystals = 0;

    // format levelPath string
    string levelPath;
    if(getLevel() <= 9)
        levelPath = "level0" + to_string(getLevel()) + ".txt";
    else
        levelPath = "level" + to_string(getLevel()) + ".txt";

    // create level
    Level lev(assetPath());
    Level::LoadResult result = lev.loadLevel(levelPath);

    if(result == Level::load_fail_file_not_found || getLevel() >= 100) {
        return GWSTATUS_PLAYER_WON;
    }

    if (result == Level::load_fail_file_not_found)
        cerr << "Could not find " + levelPath + " data file" << endl;
    else if (result == Level::load_fail_bad_format)
        cerr << "Your level was improperly formatted" << endl;
    else if (result == Level::load_success) {
        // load actors by square
        for(int x = 0; x < VIEW_WIDTH; x++) {
            for(int y = 0; y < VIEW_HEIGHT; y++) {
                Level::MazeEntry ge = lev.getContentsOf(x,y);
                switch (ge) {
                    case Level::empty:
                        break;
                    case Level::player:
                        m_player = new Player(this, x, y);
                        m_actors.push_back(m_player);
                        break;
                    case Level::wall:
                        m_actors.push_back(new Wall(this, x, y));
                        break;
                    case Level::marble:
                        m_actors.push_back(new Marble(this, x, y));
                        break;
                    case Level::pit:
                        m_actors.push_back(new Pit(this, x, y));
                        break;
                    case Level::crystal:
                        m_actors.push_back(new Crystal(this, x, y));
                        m_crystals++;
                        break;
                    case Level::exit:
                        m_actors.push_back(new Exit(this, x, y));
                        break;
                    case Level::extra_life:
                        m_actors.push_back(new ExtraLife(this, x, y));
                        break;
                    case Level::restore_health:
                        m_actors.push_back(new RestoreHealth(this, x, y));
                        break;
                    case Level::ammo:
                        m_actors.push_back(new Ammo(this, x, y));
                        break;
                    case Level::vert_ragebot:
                        m_actors.push_back(new RageBot(this, x, y, GraphObject::up));
                        break;
                    case Level::horiz_ragebot:
                        m_actors.push_back(new RageBot(this, x, y, GraphObject::right));
                        break;
                    case Level::thiefbot_factory:
                        m_actors.push_back(new ThiefBotFactory(this, x, y, false));
                        break;
                    case Level::mean_thiefbot_factory:
                        m_actors.push_back(new ThiefBotFactory(this, x, y, true));
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return GWSTATUS_CONTINUE_GAME;
}

int StudentWorld::move() {
    setDisplayText();

    // tell actors do do something
    for(auto it = m_actors.begin(); !m_levelComplete && it != m_actors.end(); it++) {
        (*it)->doSomething();

        // check if player died
        if(m_player->hp() <= 0) {
            playSound(SOUND_PLAYER_DIE);
            decLives();
            return GWSTATUS_PLAYER_DIED;
        }
    }

    // remove dead actors
    for(auto it = m_actors.begin(); it != m_actors.end();) {
        if((*it)->hp() <= 0) {
            delete(*it);
            it = m_actors.erase(it);
        } else {
            it++;
        }
    }
    // check to see if level was completed
    if(m_levelComplete) {
        increaseScore(m_bonus);
        return GWSTATUS_FINISHED_LEVEL;
    }

    if(m_bonus > 0) m_bonus--;
    m_curTick++;
    return GWSTATUS_CONTINUE_GAME;
}

void StudentWorld::cleanUp() {
    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        delete *it;
    }
    m_actors.clear();
}

// MARK: Helper Functions

void StudentWorld::setDisplayText() {
    ostringstream oss;
    oss.fill('0');
    oss << "Score: " << setw(7) << getScore() << "  ";
    oss << "Level: " << setw(2) << getLevel() << "  ";
    oss.fill(' ');
    oss << "Lives: " << setw(2) << getLives() << "  ";
    oss << "Health: " << setw(3) << round(m_player->hp() / 0.2) << "%  ";
    oss << "Ammo: " << setw(2) << m_player->peas() << "  ";
    oss << "Bonus: " << setw(4) << m_bonus;

    setGameStatText(oss.str());
}

void StudentWorld::targetCoords(double& x, double& y, int dir) const {
    switch(dir) {
        case GraphObject::up:
            y += 1;
            break;
        case GraphObject::down:
            y -= 1;
            break;
        case GraphObject::right:
            x += 1;
            break;
        case GraphObject::left:
            x -= 1;
            break;
    }
}

void StudentWorld::movePlayer() {
    // get target location
    double x = m_player->getX();
    double y = m_player->getY();
    targetCoords(x, y, m_player->getDirection());

    // iterate through all actors
    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        // if an actor is here
        if((*it)->getX() == x && (*it)->getY() == y) {
            // if actor is transparent to players
            if((*it)->playerTransparent()) continue;

            // if actor is pushable
            if((*it)->pushable()) {
                // get coords of space to push into
                double nextX = x;
                double nextY = y;
                targetCoords(nextX, nextY, m_player->getDirection());
                Actor* actorAtTarget = actorAtCoords(nullptr, nextX, nextY);
                // if nothing at target coords, or marble transparent at target coords
                if(actorAtTarget == nullptr || actorAtTarget->marbleTransparent()) {
                    (*it)->moveTo(nextX, nextY);
                    m_player->moveTo(x, y);
                }
            }
            return;
        }
    }
    // valid movement, move player
    m_player->moveTo(x, y);
}

void StudentWorld::movePea(Pea* pea) {
    // get current location
    double x = pea->getX();
    double y = pea->getY();

    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        if((*it)->getX() == x && (*it)->getY() == y) {
            if(!(*it)->peaTransparent()) {
                pea->sethp(0);
                pea->setVisible(false);
                // If factory, continue without damaging anything;
                if((*it)->canSpawn()) {
                    continue;
                }
                (*it)->takeDamage();
                return;
            }
        }
    }

    targetCoords(x, y, pea->getDirection());
    pea->moveTo(x, y);

    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        if((*it)->getX() == x && (*it)->getY() == y) {
            if(!(*it)->peaTransparent()) {
                pea->sethp(0);
                pea->setVisible(false);
                // If factory, continue without damaging anything;
                if((*it)->canSpawn()) {
                    continue;
                }
                (*it)->takeDamage();
                return;
            }
        }
    }
}

bool StudentWorld::botCanMoveHere(double x, double y) const {
    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        // if an actor is here
        if((*it)->getX() == x && (*it)->getY() == y) {
            // if actor is transparent to robots
            if((*it)->robotTransparent()) {
                continue;
            } else {
                return false;
            }
        }
    }
    return true;
}

void StudentWorld::moveRageBot(RageBot* bot) {
    double x = bot->getX();
    double y = bot->getY();
    targetCoords(x, y, bot->getDirection());

    if(botCanMoveHere(x, y)) {
        bot->moveTo(x, y);
    } else {
        bot->setDirection((bot->getDirection() + 180) % 360);
    }
}

void StudentWorld::moveThiefBot(ThiefBot* bot) {
    if(bot->spacedMoved() >= bot->distanceBeforeTurning()) {
        bot->turn();
        return;
    }

    double x = bot->getX();
    double y = bot->getY();
    targetCoords(x, y, bot->getDirection());

    if(botCanMoveHere(x, y)) {
        bot->moveTo(x, y);
    } else {
        bot->turn();
    }
}

bool StudentWorld::canShootAtPlayer(Bot* bot) const {
    double x = bot->getX();
    double y = bot->getY();

    int lower, upper;
    switch(bot->getDirection()) {
        case GraphObject::left:
            lower = m_player->getX();
            upper = x;
            if(y != m_player->getY() || x < lower) return false;
            for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
                if((*it)->getY() == y && (*it)->getX() > lower && (*it)->getX() < upper && !(*it)->peaTransparent()) {
                    return false;
                }
            }
            break;
        case GraphObject::right:
            lower = x;
            upper = m_player->getX();
            if(y != m_player->getY() || x > upper) return false;
            for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
                if((*it)->getY() == y && (*it)->getX() > lower && (*it)->getX() < upper && !(*it)->peaTransparent()) {
                    return false;
                }
            }
            break;
        case GraphObject::up:
            lower = y;
            upper = m_player->getY();
            if(x != m_player->getX() || y > upper) return false;
            for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
                if((*it)->getX() == x && (*it)->getY() > lower && (*it)->getY() < upper && !(*it)->peaTransparent()) {
                    return false;
                }
            }
            break;
        case GraphObject::down:
            lower = m_player->getY();
            upper = y;
            if(x != m_player->getX() || y < lower) return false;
            for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
                if((*it)->getX() == x && (*it)->getY() > lower && (*it)->getY() < upper && !(*it)->peaTransparent()) {
                    return false;
                }
            }
    }
    return true;
}

Actor* StudentWorld::canSteal(ThiefBot* bot) const {
    double x = bot->getX();
    double y = bot->getY();

    // iterate through until something stealable is found
    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        if((*it) != bot && (*it)->getX() == x && (*it)->getY() == y && (*it)->stealable() && !(*it)->isStolen()) {
            return *it;
        }
    }
    // nothing found, return nothing
    return nullptr;
}

int StudentWorld::countThiefBots(double x, double y) const {
    int count = 0;
    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        if((*it)->canSteal() && abs(x - (*it)->getX()) <= 3 && abs(y - (*it)->getY()) <= 3) {
            count++;
        }
        if((*it)->canSteal() && (*it)->getX() == x && (*it)->getY() == y) {
            return 4;
        }
    }
    return count;
}

Actor* StudentWorld::actorAtCoords(Actor* caller, double x, double y) const {
    for(auto it = m_actors.begin(); it != m_actors.end(); it++) {
        if((*it)->getX() == x && (*it)->getY() == y && (*it) != caller) {
            return *it;
        }
    }
    return nullptr;
}
