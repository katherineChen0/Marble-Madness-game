#ifndef GRAPHOBJ_H_
#define GRAPHOBJ_H_

#include "SpriteManager.h"
#include "GameConstants.h"

#include <set>
#include <cmath>

const int ANIMATION_POSITIONS_PER_TICK = 1;

class GraphObject
{
  public:

  static const int none = -1;
	static const int right = 0;
	static const int left = 180;
	static const int up = 90;
	static const int down = 270;

	GraphObject(int imageID, double startX, double startY, int dir = 0, double size = 1.0)
	 : m_imageID(imageID), m_visible(true), m_x(startX), m_y(startY),
	   m_destX(startX), m_destY(startY), m_brightness(1.0),
	   m_animationNumber(0), m_direction(dir), m_size(size)
	{
		if (m_size <= 0)
			m_size = 1;

		getGraphObjects().insert(this);
		setVisible(true);
	}

	virtual ~GraphObject()
	{
		getGraphObjects().erase(this);
	}

	void setVisible(bool shouldIDisplay)
	{
		m_visible = shouldIDisplay;
	}

	void setBrightness(double brightness)
	{
		m_brightness = brightness;
	}

	double getX() const
	{
		  // If already moved but not yet animated, use new location anyway.
		return m_destX;
	}

	double getY() const
	{
		  // If already moved but not yet animated, use new location anyway.
		return m_destY;
	}

	virtual void moveTo(double x, double y)
	{
		m_destX = x;
		m_destY = y;
		increaseAnimationNumber();
	}

	virtual void moveAngle(int angle, int units = 1)
	{
		double newX;
		double newY;
		getPositionInThisDirection(angle, units, newX, newY);
		moveTo(newX, newY);
		increaseAnimationNumber();
	}

	virtual void getPositionInThisDirection(int angle, int units, double& newX, double& newY)
	{
		static const double PI = 4 * atan(1.0);
		newX = (getX() + units * cos(angle*1.0 / 360 * 2 * PI));
		newY = (getY() + units * sin(angle*1.0 / 360 * 2 * PI));
	}

	void moveForward(int units = 1)
	{
		moveAngle(getDirection(), units);
	}

	int getDirection() const
	{
		return m_direction;
	}

	void setDirection(int d)
	{
		while (d < 0)
			d += 360;

		m_direction = d % 360;
	}

	void setSize(double size)
	{
		m_size = size;
	}

	double getSize() const
	{
		return m_size;
	}

	double getRadius() const
	{
		const int kRadiusPerUnit = 8;
		return kRadiusPerUnit * m_size;
	}

	  // The following should be used by only the framework, not the student

	bool isVisible() const
	{
		return m_visible;
	}

	double getBrightness() const
	{
		return m_brightness;
	}

	unsigned int getAnimationNumber() const
	{
		return m_animationNumber;
	}

	void getAnimationLocation(double& x, double& y) const
	{
		x = m_x;
		y = m_y;
	}

	void animate()
	{
		m_x = m_destX;
		m_y = m_destY;
		//moveALittle(m_x, m_destX);
		//moveALittle(m_y, m_destY);
	}

	static std::set<GraphObject*>& getGraphObjects()
	{
		static std::set<GraphObject*> graphObjects;
		return graphObjects;
	}

	void increaseAnimationNumber()
	{
		m_animationNumber++;
	}


private:
	friend class GameController;
	unsigned int getID() const
	{
		return m_imageID;
	}

  private:
	  // Prevent copying or assigning GraphObjects
	GraphObject(const GraphObject&);
	GraphObject& operator=(const GraphObject&);

	static const int NUM_DEPTHS = 4;
	int		m_imageID;
	bool	m_visible;
	double	m_x;
	double	m_y;
	double	m_destX;
	double	m_destY;
	double	m_brightness;
	int	m_animationNumber;
	int	m_direction;
	double	m_size;

	void moveALittle(double& from, double& to)
	{
		static const double DISTANCE = 1.0/ANIMATION_POSITIONS_PER_TICK;
		if (to - from >= DISTANCE)
			from += DISTANCE;
		else if (from - to >= DISTANCE)
			from -= DISTANCE;
		else
			from = to;
	}


};

#endif // GRAPHOBJ_H_
