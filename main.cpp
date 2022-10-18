#include <glut.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#define PI 3.1415926
#define EPSILON 0.0001

class Color
{
public:
	float r;
	float g;
	float b;
	Color(float r, float g, float b):r(r),g(g),b(b)
	{}
	Color()
	{
		r = 0;
		g = 0;
		b = 0;
	}
};

class Point
{
public:
	float x;
	float y;
	Point(float x, float y): x(x), y(y)
	{}
	Point()
	{
		x = 0;
		y = 0;
	}
	friend Point operator*(Point p,float scalar)
	{
		return Point(p.x* scalar, p.y * scalar);
	}
	Point& operator+=(const Point& rhs)
	{
		this->x += rhs.x;
		this->y += rhs.y;
		return *this;
	}
};

const float rotationSpeed = 0.1; // radians
float speed = 3; // pixels
class Player
{
public:
	Point position;
	Point lookDirection;
	float lookAngle = 0; 
	Color color;
	Player(Color c)
	{
		position.x = 0;
		position.y = 0;
		color = c;
	}
	void Draw()
	{
		glColor3f(color.r, color.g, color.b);
		glPointSize(8);
		glBegin(GL_POINTS);
		glVertex2i(position.x, position.y);
		glEnd();
	}
};
Player* player;

int layout[] = {1,1,1,1,1,1,1,1,
				1,0,0,0,0,0,0,1,
				1,0,0,0,0,0,0,1,
				1,0,0,0,0,1,1,1,
				1,0,0,0,0,0,0,1,
				1,0,0,0,0,0,0,1,
				1,0,0,0,0,0,0,1,
				1,1,1,1,1,1,1,1 };	
class Map	// 2D map representation is a sizeX by sizeY grid;
{
public:
	int sizeX;	
	int sizeY;
	int blockSize; // size of squares on grid map
	Map(int x, int y, int scale) : sizeX(x), sizeY(y), blockSize(scale)
	{}
	void Draw()
	{
		int x0, y0;
		for (int i = 0; i < sizeY; i++)
		{
			for (int j = 0; j < sizeX; j++)
			{
				if (layout[i*sizeX +j] == 1)
				{
					glColor3f(0.3, 0.3, 1);
				}
				else
				{
					glColor3f(0.5, 0.5, 0.5);
				}
				y0 = i * blockSize;
				x0 = j * blockSize;
				glBegin(GL_QUADS);
				glVertex2i(x0,y0);
				glVertex2i(x0,y0+blockSize);
				glVertex2i(x0+blockSize,y0+blockSize);
				glVertex2i(x0+blockSize,y0);
				glEnd();
			}
		}
	}
};
Map* map;

//returns equivalent angle in the interval [0;2pi]
float clampAngle(float angle)
{
	while (angle > 2 * PI)
	{
		angle -= 2 * PI;
	}
	while (angle < 0)
	{
		angle += 2 * PI;
	}
	return angle;
}

void raycast()
{
	float distance; // "true" distance between player and  first collision with a wall
	float distanceV; // distance between player and first collision with vertical wall
	float distanceH; // distance between player and first collision with horizontal wall
	float rayAngle;
	float lastCollisionX, lastCollisionY, deltaX, deltaY;
	int depth; // we can get maximum of MAX(map->sizeX, map->sizeY) amount of intersections with the grid
	float raySlope;
	int mapCoordX, mapCoordY, mapIndex;
	int HorCollisionX, HorCollisionY; // remember coords of first collision with horizontal line in case its the nearest one
	
	for (int rays = -120; rays < 120; rays++)
	{
		rayAngle = clampAngle(player->lookAngle + rays*PI/720);
		depth = 0;
		
		raySlope = -1 / tan(rayAngle); // coordinate origin is top left -> inverted slope
		if (rayAngle > PI) // looking up (again, inverted angles)
		{
			lastCollisionY = (float)(((int)player->position.y / map->blockSize) * map->blockSize) - EPSILON;
			lastCollisionX = (player->position.y - lastCollisionY) * raySlope + player->position.x;
			deltaY = -map->blockSize;
			deltaX = -deltaY * raySlope;
		}
		if (rayAngle < PI) // looking down
		{
			lastCollisionY = (float)(((int)player->position.y / map->blockSize) * map->blockSize) + (float)map->blockSize;
			lastCollisionX = (player->position.y - lastCollisionY) * raySlope + player->position.x;
			deltaY = map->blockSize;
			deltaX = -deltaY * raySlope;
		}
		if (rayAngle == PI || rayAngle == 0) // looking horizontally
		{
			lastCollisionY = player->position.y;
			lastCollisionX = player->position.x;
			depth = 8; // no expected collisions
		}
		while (depth < 8)
		{
			mapCoordX = (int)(lastCollisionX / map->blockSize);
			mapCoordY = (int)(lastCollisionY / map->blockSize);
			mapIndex = mapCoordY * map->sizeX + mapCoordX;
			if (mapIndex >= map->sizeX * map->sizeY || mapIndex<0)
			{
				depth = 8; // out of range <=> out of map
			}
			else if (mapIndex < map->sizeX * map->sizeY && layout[mapIndex] == 1)
			{
				depth = 8; // ray hits wall
			}
			else
			{
				lastCollisionX += deltaX;
				lastCollisionY += deltaY;
				depth++;
			}
		}
		distanceH = sqrt((player->position.x - lastCollisionX) * (player->position.x - lastCollisionX) +
						 (player->position.y - lastCollisionY) * (player->position.y - lastCollisionY));
		HorCollisionX = lastCollisionX;
		HorCollisionY = lastCollisionY;

		// VERTICAL LINE INTERSECTION
		depth = 0;
		raySlope = -tan(rayAngle); // coordinate origin is top left -> inverted slope
		if (rayAngle > PI/2 && rayAngle < 3*PI/2) // looking left
		{
			lastCollisionX = (float)(((int)player->position.x / map->blockSize) * map->blockSize) - EPSILON;
			lastCollisionY = (player->position.x - lastCollisionX) * raySlope + player->position.y;
			deltaX = -map->blockSize;
			deltaY = -deltaX * raySlope;
		}
		if (rayAngle < PI/2 || rayAngle>3*PI/2) // looking right
		{
			lastCollisionX = (float)(((int)player->position.x / map->blockSize) * map->blockSize) + (float)map->blockSize;
			lastCollisionY = (player->position.x - lastCollisionX) * raySlope + player->position.y;
			deltaX = map->blockSize;
			deltaY = -deltaX * raySlope;
		}
		if (rayAngle == PI/2 || rayAngle == 3*PI/2) // looking vertically
		{
			lastCollisionX = player->position.x;
			lastCollisionY = player->position.y;
			depth = 8; // no expected collisions
		}
		while (depth < 8)
		{
			mapCoordY = (int)(lastCollisionY / map->blockSize);
			mapCoordX = (int)(lastCollisionX / map->blockSize);
			mapIndex = mapCoordY * map->sizeX + mapCoordX;
			if (mapIndex >= map->sizeX * map->sizeY || mapIndex < 0)
			{
				depth = 8;
			}
			else if (mapIndex < map->sizeX * map->sizeY && layout[mapIndex] == 1)
			{
				depth = 8;
			}
			else
			{
				lastCollisionX += deltaX;
				lastCollisionY += deltaY;
				depth++;
			}
		}
		distanceV = sqrt((player->position.x - lastCollisionX) * (player->position.x - lastCollisionX) +
						 (player->position.y - lastCollisionY) * (player->position.y - lastCollisionY));
		distance = distanceH < distanceV ? distanceH : distanceV;
		if (distanceH < distanceV)
		{
			lastCollisionX = HorCollisionX;
			lastCollisionY = HorCollisionY;
		}
		glColor3f(1, 1, 0);
		glLineWidth(1);
		glBegin(GL_LINES);
		glVertex2i(player->position.x, player->position.y);
		glVertex2i(lastCollisionX, lastCollisionY);
		glEnd();
		glColor3f(0, 0, 1);
		if (distanceH < distanceV) // pseudo-lightning
		{
			glColor3f(0.3, 0.3, 1);
		}
		// 3d conversion
		distance = distance *cos(player->lookAngle - rayAngle); // remove distortion
		float stripHeight = (float)512*(float)map->blockSize/(float)distance;
		if (stripHeight > 512)stripHeight = 512;
		glLineWidth(2);
		glBegin(GL_LINES);
		glVertex2i(512+256+rays*2, 256- stripHeight/2);
		glVertex2i(512+256+rays*2, 256+ stripHeight/2);
		glEnd();
	}

}

void ProcessInput(unsigned char key, int x, int y)
{
	if (key == 'a') 
	{
		player->lookAngle = clampAngle(player->lookAngle - rotationSpeed);
		player->lookDirection = Point(cos(player->lookAngle), sin(player->lookAngle));
	}
	if (key == 'd')
	{
		player->lookAngle = clampAngle(player->lookAngle + rotationSpeed);
		player->lookDirection = Point(cos(player->lookAngle), sin(player->lookAngle));
	}
	if (key == 'w')
	{
		player->position += player->lookDirection * speed;
	}
	if (key == 's')
	{
		player->position += player->lookDirection * (-speed);
	}
	glutPostRedisplay();
}

void DrawBorder()
{
	glColor3f(0.05, 0.05, 0.05);
	glLineWidth(16);
	glBegin(GL_LINES);
	glVertex2i(512+8,0);
	glVertex2i(512 + 8, 512);
	glEnd();
	glBegin(GL_LINES);
	glVertex2i(1024-8, 0);
	glVertex2i(1024-8, 512);
	glEnd();
	glBegin(GL_LINES);
	glVertex2i(512, 8);
	glVertex2i(1024, 8);
	glEnd();
	glBegin(GL_LINES);
	glVertex2i(512, 512-8);
	glVertex2i(1024, 512-8);
	glEnd();
}
void Display() 
{
	glClear(GL_COLOR_BUFFER_BIT);
	map->Draw();
	player->Draw();
	raycast();
	DrawBorder();
	glFlush();
}

void Initialize() 
{
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0, 1024, 512, 0);
}

int main(int argc, char** argv) 
{
	Color playerColor(1,0,0);
	player = &Player(playerColor);
	player->position.x = 100;
	player->position.y = 100;
	player->lookAngle = 2*PI;
	map = &Map(8,8,64);
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
	glutInitWindowSize(1024, 512);
	glutInitWindowPosition(100, 200);
	glutCreateWindow("HLDemake");
	glutDisplayFunc(Display);
	glutKeyboardFunc(ProcessInput);
	Initialize();
	glutMainLoop();
	return 0;
}
