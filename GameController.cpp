#if defined(__APPLE__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
#include "freeglut.h"
#include "GameController.h"
#include "GameWorld.h"
#include "GameConstants.h"
#include "GraphObject.h"
#include "SoundFX.h"
#include "SpriteManager.h"
#include <iostream>
#include <string>
#include <map>
#include <utility>
#include <cstdlib>
#include <algorithm>
#include <thread>
#include <chrono>
using namespace std;

/*
spriteWidth = .67
spritesPerRow = 16

RowWidth = spriteWidth*spritesPerRow = 10.72
PixelWidth = RowWidth/256 = .041875
newSpriteWidth = PixelWidth * NumPixels

spriteHeight = .54
spritesPerRow = 16

RowHeight = spriteHeight*spritesPerCol = 8.64

PixelHeight = RowHeight/256 = .03375

newSpriteHeight = PixelHeight * NumPixels
*/

static const int WINDOW_WIDTH = 768; //1024;
static const int WINDOW_HEIGHT = 768;

static const int PERSPECTIVE_NEAR_PLANE = 4;
static const int PERSPECTIVE_FAR_PLANE	= 22;

static const double VISIBLE_MIN_X = -2.39;
static const double VISIBLE_MAX_X = 2.1; // 2.39;
static const double VISIBLE_MIN_Y = -2.1;
static const double VISIBLE_MAX_Y = 1.9;
static const double VISIBLE_MIN_Z = -20;
// static const double VISIBLE_MAX_Z = -6;

static const double FONT_SCALEDOWN = 760.0;

static const double SCORE_Y = 3.8;
static const double SCORE_Z = -10;

struct SpriteInfo
{
	unsigned int imageID;
	unsigned int frameNum;
	std::string	 tgaFileName;
	std::string	 imageName;
	int			 depth;
};

static void convertToGlutCoords(double x, double y, double& gx, double& gy, double& gz);
static void drawPrompt(string mainMessage, string secondMessage);
static void drawScoreAndLives(string);

enum GameController::GameControllerState : int {
    welcome, init, makemove, animate, contgame, finishedlevel, gameover, cleanup, quit, prompt, not_applicable
};

int GameController::m_msPerTick;

void GameController::initDrawersAndSounds()
{
	SpriteInfo drawers[] = {
		{ IID_PLAYER      , 0, "dude_1.tga", "PLAYER", 0 },
		{ IID_PLAYER      , 1, "dude_2.tga", "PLAYER", 0 },
		{ IID_PLAYER      , 2, "dude_3.tga", "PLAYER", 0 },
		{ IID_THIEFBOT    , 0, "thiefbot-1.tga", "THIEFBOT", 0 },
		{ IID_THIEFBOT    , 1, "thiefbot-2.tga", "THIEFBOT", 0 },
		{ IID_THIEFBOT    , 2, "thiefbot-3.tga", "THIEFBOT", 0 },
		{ IID_MEAN_THIEFBOT  , 0, "thiefbot-1.tga", "MEAN_THIEFBOT", 0 },
		{ IID_MEAN_THIEFBOT  , 1, "thiefbot-2.tga", "MEAN_THIEFBOT", 0 },
		{ IID_MEAN_THIEFBOT  , 2, "thiefbot-3.tga", "MEAN_THIEFBOT", 0 },
		{ IID_RAGEBOT     , 0, "ragebot-1.tga", "RAGEBOT", 0  },
		{ IID_RAGEBOT     , 1, "ragebot-2.tga", "RAGEBOT", 0 },
		{ IID_RAGEBOT     , 2, "ragebot-3.tga", "RAGEBOT", 0 },
		{ IID_RAGEBOT     , 3, "ragebot-4.tga", "RAGEBOT", 0 },
		{ IID_PEA         , 0, "pea.tga", "PEA", 1 },
		{ IID_ROBOT_FACTORY   , 0, "factory.tga", "ROBOT_FACTORY", 2 },
		{ IID_CRYSTAL     , 0, "crystal.tga", "CRYSTAL", 2 },
		{ IID_RESTORE_HEALTH  , 0, "medkit.tga", "RESTORE_HEALTH", 2 },
		{ IID_EXTRA_LIFE  , 0, "extralife.tga", "EXTRA_LIFE", 2 },
		{ IID_AMMO        , 0, "ammo.tga", "AMMO", 2 },
		{ IID_EXIT        , 0, "exit.tga", "EXIT", 2 },
		{ IID_WALL        , 0, "wall.tga", "WALL", 2 },
		{ IID_MARBLE      , 0, "marble.tga", "MARBLE", 2 },
		{ IID_PIT         , 0, "pit.tga", "PIT", 2 }
	};

	m_soundMap = {
		{ SOUND_THEME         , "theme.wav" },
		{ SOUND_PLAYER_FIRE   , "torpedo.wav" },
		{ SOUND_ENEMY_FIRE    , "pop.wav" },
		{ SOUND_ROBOT_DIE     , "explode.wav" },
		{ SOUND_PLAYER_DIE    , "die.wav" },
		{ SOUND_GOT_GOODIE    , "goodie.wav" },
		{ SOUND_REVEAL_EXIT   , "revealexit.wav" },
		{ SOUND_FINISHED_LEVEL, "finished.wav" },
		{ SOUND_ROBOT_IMPACT  , "clank.wav" },
		{ SOUND_PLAYER_IMPACT , "ouch.wav" },
		{ SOUND_ROBOT_MUNCH   , "munch.wav" },
		{ SOUND_ROBOT_BORN    , "materialize.wav" },
	};

	for (const auto& d : drawers)
	{
		string path = m_gw->assetPath();
		if (!path.empty())
			path += '/';
		if (!m_spriteManager.loadSprite(path + d.tgaFileName, d.imageID, d.frameNum)) {
			cerr << "Error loading sprite: " << (path+d.tgaFileName) << endl;
			setGameState(quit);
		}
		m_imageNameMap[d.imageID] = d.imageName;
		m_imageDepthMap[d.imageID] = d.depth;
	}
}

bool GameController::passesThruWhenSingleStepping(int key) const
{
	static set<int> passThruKeys = {
		'a', 'd', 'w', 's', '2', '4', '6', '8', KEY_PRESS_LEFT, KEY_PRESS_RIGHT,
		KEY_PRESS_UP, KEY_PRESS_DOWN, KEY_PRESS_ENTER
	};
	return passThruKeys.find(key) != passThruKeys.end();
}

static void doSomethingCallback()
{
	Game().doSomething();
}

static void reshapeCallback(int w, int h)
{
	Game().reshape(w, h);
}

static void keyboardEventCallback(unsigned char key, int x, int y)
{
	Game().keyboardEvent(key, x, y);
}

static void specialKeyboardEventCallback(int key, int x, int y)
{
	Game().specialKeyboardEvent(key, x, y);
}

void GameController::timerFuncCallback(int)
{
	Game().doSomething();
	this_thread::sleep_for(chrono::milliseconds(m_msPerTick));
    glutTimerFunc(0, timerFuncCallback, 0);
}

void windowCloseCallback()
{
#if defined(__APPLE__)
	SoundFX().abortClip();
#endif
}

void GameController::run(int argc, char* argv[], GameWorld* gw, string windowTitle, int msPerTick)
{
	gw->setController(this);
	m_gw = gw;
	m_msPerTick = msPerTick;
	setGameState(welcome);
	m_lastKeyHit = INVALID_KEY;
	m_singleStep = false;
	m_curIntraFrameTick = 0;
	m_playerWon = false;

	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	glutInitWindowPosition(0, 0);
	glutCreateWindow(windowTitle.c_str());

	initDrawersAndSounds();  // won't work unless *after* window created

	glutKeyboardFunc(keyboardEventCallback);
	glutSpecialFunc(specialKeyboardEventCallback);
	glutReshapeFunc(reshapeCallback);
	glutDisplayFunc(doSomethingCallback);
	glutTimerFunc(0, timerFuncCallback, 0);
	glutWMCloseFunc(windowCloseCallback);

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
	delete m_gw;
	reportLeakedGraphObjects();
}

void GameController::keyboardEvent(unsigned char key, int /* x */, int /* y */)
{
	switch (key)
	{
		case 'a': case '4': m_lastKeyHit = KEY_PRESS_LEFT;	break;
		case 'd': case '6': m_lastKeyHit = KEY_PRESS_RIGHT; break;
		case 'w': case '8': m_lastKeyHit = KEY_PRESS_UP;	break;
		case 's': case '2': m_lastKeyHit = KEY_PRESS_DOWN;	break;
		case 't':			m_lastKeyHit = KEY_PRESS_TAB;	break;
		case 'f':			m_singleStep = true;			break;
		case 'r':			m_singleStep = false;			break;
		case 'q': case 'Q': case '\x03':  // CTRL-C
							setGameState(quit);				break;
		default:			m_lastKeyHit = key;				break;
	}
}

void GameController::specialKeyboardEvent(int key, int /* x */, int /* y */)
{
	switch (key)
	{
		case GLUT_KEY_LEFT:	 m_lastKeyHit = KEY_PRESS_LEFT;	 break;
		case GLUT_KEY_RIGHT: m_lastKeyHit = KEY_PRESS_RIGHT; break;
		case GLUT_KEY_UP:	 m_lastKeyHit = KEY_PRESS_UP;	 break;
		case GLUT_KEY_DOWN:	 m_lastKeyHit = KEY_PRESS_DOWN;	 break;
		default:			 m_lastKeyHit = INVALID_KEY;	 break;
	}
}

void GameController::playSound(int soundID)
{
	if (soundID == SOUND_NONE)
		return;

	auto p = m_soundMap.find(soundID);
	if (p != m_soundMap.end())
	{
		string path = m_gw->assetPath();
		if (!path.empty())
			path += '/';
		SoundFX().playClip(path + p->second);
	}
}

void GameController::setGameState(GameControllerState s)
{
    if (m_gameState != quit)
        m_gameState = s;
}

void GameController::quitGame()
{
    setGameState(quit);
}

void GameController::doSomething()
{
	switch (m_gameState)
	{
		case not_applicable:
			break;
		case welcome:
			playSound(SOUND_THEME);
			m_mainMessage = "Welcome to Marble Madness!";
			m_secondMessage = "Press Enter to begin play...";
			setGameState(prompt);
			m_nextStateAfterPrompt = init;
			break;
		case init:
			{
				int status = m_gw->init();
				m_postInitPreCleanup = true;
				SoundFX().abortClip();
				switch (status)
				{
				  case GWSTATUS_CONTINUE_GAME:
					setGameState(makemove);
					break;
				  case GWSTATUS_PLAYER_WON:  // only if no levels at all
					m_playerWon = true;
					setGameState(gameover);
					break;
				  case GWSTATUS_LEVEL_ERROR:
					m_mainMessage = "Error in level data file encoding!";
					m_secondMessage = "Press Enter to quit...";
					setGameState(prompt);
					m_nextStateAfterPrompt = quit;
					break;
				  default:
					m_mainMessage = "StudentWorld::init returned a wrong status!";
					m_secondMessage = "Press Enter to quit...";
					setGameState(prompt);
					m_nextStateAfterPrompt = quit;
					break;
				}
			}
			break;
		case makemove:
			m_curIntraFrameTick = ANIMATION_POSITIONS_PER_TICK;
			m_nextStateAfterAnimate = not_applicable;
			{
				int status = m_gw->move();
				switch (status)
				{
				  case GWSTATUS_PLAYER_DIED:
					  // animate one last frame so we can see what happened
					m_nextStateAfterAnimate = (m_gw->isGameOver() ? gameover : contgame);
					break;
				  case GWSTATUS_FINISHED_LEVEL:
					m_gw->advanceToNextLevel();
					  // animate one last frame so we can see what happened
					m_nextStateAfterAnimate = finishedlevel;
					break;
				  case GWSTATUS_PLAYER_WON:
					m_playerWon = true;
					m_nextStateAfterAnimate = gameover;
					break;
				}
			}
			setGameState(animate);
			break;
		case animate:
			displayGamePlay();
			if (m_curIntraFrameTick-- <= 0)
			{
				if (m_nextStateAfterAnimate != not_applicable)
					setGameState(m_nextStateAfterAnimate);
				else if (!m_singleStep)
					setGameState(makemove);
				else
				{
					int key;
					if (getKeyIfAny(key))
					{
						if (passesThruWhenSingleStepping(key))
							putBackKey(key);
						setGameState(makemove);
					}
				}
			}
			break;
		case contgame:
			m_mainMessage = "You lost a life!";
			m_secondMessage = "Press Enter to continue playing...";
			setGameState(prompt);
			m_nextStateAfterPrompt = cleanup;
			break;
		case finishedlevel:
			m_mainMessage = "Woot! You finished the level!";
			m_secondMessage = "Press Enter to continue playing...";
			setGameState(prompt);
			m_nextStateAfterPrompt = cleanup;
			break;
		case cleanup:
			if (m_postInitPreCleanup)  // should aways be true here
			{
				m_gw->cleanUp();
				m_postInitPreCleanup = false;
			}
			setGameState(init);
			break;
		case gameover:
			{
				ostringstream oss;
				oss << (m_playerWon ? "You won the game!" : "Game Over!")
					<< " Final score: " << m_gw->getScore() << "!";
				m_mainMessage = oss.str();
			}
			m_secondMessage = "Press Enter to quit...";
			setGameState(prompt);
			m_nextStateAfterPrompt = quit;
			break;
		case quit:
			if (m_postInitPreCleanup)  // might be false if aborted game
			{
				m_gw->cleanUp();
				m_postInitPreCleanup = false;
			}
            SoundFX().abortClip();
			glutLeaveMainLoop();
			break;
		case prompt:
			drawPrompt(m_mainMessage, m_secondMessage);
			{
				int key;
				if (getKeyIfAny(key) && key == '\r')
					setGameState(m_nextStateAfterPrompt);
			}
			break;
	}
}


void GameController::displayGamePlay()
{
	glEnable(GL_DEPTH_TEST); // must be done each time before displaying graphics or gets disabled for some reason
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifdef _MSC_VER
    gluLookAt(0, 0, 0, 0, 0, -1, 0, 1, 0);
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gluLookAt(0, 0, 0, 0, 0, -1, 0, 1, 0);
#pragma GCC diagnostic pop
#endif

  std::set<GraphObject*> &graphObjects = GraphObject::getGraphObjects();

	for (int i = GraphObject::NUM_DEPTHS - 1; i >= 0; --i)
	{
		for (auto it = graphObjects.begin(); it != graphObjects.end(); it++)
		{
			GraphObject* cur = *it;
			if (m_imageDepthMap.at(cur->getID()) == i && cur->isVisible())
			{
				cur->animate();

				double x, y, gx, gy, gz;
				cur->getAnimationLocation(x, y);
				convertToGlutCoords(x, y, gx, gy, gz);

				int angle = cur->getDirection();
				int imageID = cur->getID();

				m_spriteManager.plotSprite(imageID, cur->getAnimationNumber() % m_spriteManager.getNumFrames(imageID), gx, gy, gz, angle, cur->getSize());
			}
		}
	}

	drawScoreAndLives(m_gameStatText);

	glutSwapBuffers();
}

void GameController::reportLeakedGraphObjects() const
{
	//int totalLeaked = 0;
	auto& graphObjects = GraphObject::getGraphObjects();
	if (graphObjects.empty())
		cerr << "No memory leaks were detected." << endl;
	else
	{
		cerr << "***** " << graphObjects.size() << " leaked objects" << endl;
		for (GraphObject* go : graphObjects)
			cerr << "At (" << go->getX() << "," << go->getY() << "): "
						   <<  m_imageNameMap.at(go->m_imageID) << endl;
		//totalLeaked += graphObjects.size();
	}
	//if (totalLeaked > 0)
	//	cout << "***** Total leaked objects: " << totalLeaked << endl;
}

void GameController::reshape (int w, int h)
{
	glViewport (0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
#ifdef _MSC_VER
    gluPerspective(45.0, double(WINDOW_WIDTH) / WINDOW_HEIGHT, PERSPECTIVE_NEAR_PLANE, PERSPECTIVE_FAR_PLANE);
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    gluPerspective(45.0, double(WINDOW_WIDTH) / WINDOW_HEIGHT, PERSPECTIVE_NEAR_PLANE, PERSPECTIVE_FAR_PLANE);
#pragma GCC diagnostic pop
#endif
	glMatrixMode (GL_MODELVIEW);
}

static void convertToGlutCoords(double x, double y, double& gx, double& gy, double& gz)
{
	x /= VIEW_WIDTH;
	y /= VIEW_HEIGHT;
	gx = 2 * VISIBLE_MIN_X + .3 + x * 2 * (VISIBLE_MAX_X - VISIBLE_MIN_X);
	gy = 2 * VISIBLE_MIN_Y +	  y * 2 * (VISIBLE_MAX_Y - VISIBLE_MIN_Y);
	gz = .6 * VISIBLE_MIN_Z;
}

static void doOutputStroke(double x, double y, double z, double size, const char* str, bool centered)
{
	if (centered)
	{
		double len = glutStrokeLength(GLUT_STROKE_ROMAN, reinterpret_cast<const unsigned char*>(str)) / FONT_SCALEDOWN;
		x = -len / 2;
		size = 1;
	}
	GLfloat scaledSize = static_cast<GLfloat>(size / FONT_SCALEDOWN);
	glPushMatrix();
	glLineWidth(1);
	glLoadIdentity();
	glTranslatef(static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z));
	glScalef(scaledSize, scaledSize, scaledSize);
	for ( ; *str != '\0'; str++)
		glutStrokeCharacter(GLUT_STROKE_ROMAN, *str);
	glPopMatrix();
}

//static void outputStroke(double x, double y, double z, double size, const char* str)
//{
//	doOutputStroke(x, y, z, size, str, false);
//}

static void outputStrokeCentered(double y, double z, const char* str)
{
	doOutputStroke(0, y, z, 1, str, true);
}

static void drawPrompt(string mainMessage, string secondMessage)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f (1.0, 1.0, 1.0);
	glLoadIdentity ();
	outputStrokeCentered(1, -5, mainMessage.c_str());
	outputStrokeCentered(-1, -5, secondMessage.c_str());
	glutSwapBuffers();
}

static void drawScoreAndLives(string gameStatText)
{
	static int RATE = 1;
	static GLfloat rgb[3] =
		{ static_cast<GLfloat>(.6), static_cast<GLfloat>(.6), static_cast<GLfloat>(.6) };
	for (int k = 0; k < 3; k++)
	{
		double strength = rgb[k] + randInt(-RATE, RATE) / 100.0;
		if (strength < .6)
			strength = .6;
		else if (strength > 1.0)
			strength = 1.0;
		rgb[k] = static_cast<GLfloat>(strength);
	}
	glColor3f(rgb[0], rgb[1], rgb[2]);
	outputStrokeCentered(SCORE_Y, SCORE_Z, gameStatText.c_str());
}

#if defined(__APPLE__)
#pragma GCC diagnostic pop
#endif
