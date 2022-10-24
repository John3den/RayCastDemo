#include <glut.h>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#define PI 3.1415926
#define EPSILON 0.0001
using namespace std;

int textureRes = 8;
int texture1[] = {
	1,0,0,0,1,0,0,1,
	1,1,1,1,1,1,1,1,
	0,0,0,1,0,0,0,1,
	1,1,1,1,1,1,1,1,
	1,0,0,0,1,0,0,0,
	1,1,1,1,1,1,1,1,
	0,0,0,1,0,0,0,1,
	1,1,1,1,1,1,1,1
};

int texture2[] = {
	1,1,1,1,1,1,1,1,
	1,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,1,
	1,0,0,0,1,1,0,1,
	1,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,1,
	1,0,0,0,0,0,0,1,
	1,1,1,1,1,1,1,1
};
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

class inputState
{
public:
	int w = 0;
	int a = 0;
	int s = 0;
	int d = 0;

}; inputState inputs;

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



class Enemy
{
	int x, y;
	float lookAngle = 0;
	Enemy(int spawnX, int spawnY)
	{
		x = spawnX;
		y = spawnY;
	}
};

const float rotationSpeed = 0.06; // radians
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
				1,0,0,0,0,2,1,1,
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
				else if (layout[i * sizeX + j] == 2)
				{
					glColor3f(0.6, 0.3, 1);
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

float raycast(Point origin, float angle, float& lastCollisionX, float& lastCollisionY, int& hor, int& wiv)
{
	float distance; // "true" distance between player and  first collision with a wall
	float distanceV; // distance between player and first collision with vertical wall
	float distanceH; // distance between player and first collision with horizontal wall
	float deltaX, deltaY;
	int depth; // we can get maximum of MAX(map->sizeX, map->sizeY) amount of intersections with the grid
	float raySlope;
	int mapCoordX, mapCoordY, mapIndex;
	int HorCollisionX, HorCollisionY; // remember coords of first collision with horizontal line in case its the nearest one
	int wih = 0;
	hor = 0;
	depth = 0;
	raySlope = -1 / tan(angle); // coordinate origin is top left -> inverted slope
	if (angle > PI) // looking up (again, inverted angles)
	{
		lastCollisionY = (float)(((int)player->position.y / map->blockSize) * map->blockSize) - EPSILON;
		lastCollisionX = (player->position.y - lastCollisionY) * raySlope + player->position.x;
		deltaY = -map->blockSize;
		deltaX = -deltaY * raySlope;
	}
	if (angle < PI) // looking down
	{
		lastCollisionY = (float)(((int)player->position.y / map->blockSize) * map->blockSize) + (float)map->blockSize;
		lastCollisionX = (player->position.y - lastCollisionY) * raySlope + player->position.x;
		deltaY = map->blockSize;
		deltaX = -deltaY * raySlope;
	}
	if (angle == PI || angle == 0) // looking horizontally
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
		if (mapIndex >= map->sizeX * map->sizeY || mapIndex < 0)
		{
			depth = 8; // out of range <=> out of map
		}
		else if (mapIndex < map->sizeX * map->sizeY && layout[mapIndex] !=0)
		{
			wih = layout[mapIndex];
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
	raySlope = -tan(angle); // coordinate origin is top left -> inverted slope
	if (angle > PI / 2 && angle < 3 * PI / 2) // looking left
	{
		lastCollisionX = (float)(((int)player->position.x / map->blockSize) * map->blockSize) - EPSILON;
		lastCollisionY = (player->position.x - lastCollisionX) * raySlope + player->position.y;
		deltaX = -map->blockSize;
		deltaY = -deltaX * raySlope;
	}
	if (angle < PI / 2 || angle>3 * PI / 2) // looking right
	{
		lastCollisionX = (float)(((int)player->position.x / map->blockSize) * map->blockSize) + (float)map->blockSize;
		lastCollisionY = (player->position.x - lastCollisionX) * raySlope + player->position.y;
		deltaX = map->blockSize;
		deltaY = -deltaX * raySlope;
	}
	if (angle == PI / 2 || angle == 3 * PI / 2) // looking vertically
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
		else if (mapIndex < map->sizeX * map->sizeY && layout[mapIndex] != 0)
		{
			depth = 8;
			wiv = layout[mapIndex];
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
		wiv = wih;
		lastCollisionX = HorCollisionX;
		lastCollisionY = HorCollisionY;
		hor = 1;
	}
	return distance;
}

void render()
{
	for (int rays = -120; rays < 120; rays++)
	{
		float rayAngle = clampAngle(player->lookAngle + rays * PI / 720);
		float lastCollisionX, lastCollisionY;
		int hor, wiv;
		float distance = raycast(player->position, rayAngle, lastCollisionX, lastCollisionY, hor ,wiv);
		glColor3f(1, 1, 0);
		glLineWidth(1);
		glBegin(GL_LINES);
		glVertex2i(player->position.x, player->position.y);
		glVertex2i(lastCollisionX, lastCollisionY);
		glEnd();
		glColor3f(0, 0, 1);
		// 3d conversion
		distance = distance * cos(player->lookAngle - rayAngle); // remove distortion
		float h = (float)512 * (float)map->blockSize / (float)distance;
		float s = h;
		if (h > 512)
		{
			h = 512;
		}
		int texX;
		if (hor)
		{
			texX = (int)(lastCollisionX / 8.0) % 8;
			if (rayAngle < PI) texX = 7 - texX;
		}
		else
		{
			texX = (int)(lastCollisionY / 8.0) % 8;
			if (rayAngle > PI / 2 && rayAngle < PI * 3 / 2) texX = 7 - texX;
		}
		int texY = 0;
		float shade = hor * 0.5 + 0.5;
		for(int y = 0; y < h; y++)
		{
			if (s > 512) {
				texY = (((float)y + (s - h) / 2) / s) * textureRes;
			}
			else
				texY = (int)((float)y / (h)*textureRes);
			glBegin(GL_POINTS);
			if (wiv == 1)
			{
				
				if (texture1[texY * textureRes + texX])
					glColor3f(0.2 * shade, 0.2 * shade, 0.2 * shade);
				else
					glColor3f(0.4 * shade, 0.3 * shade, 0.2 * shade);
			}
			else 
			{
				if (texture2[texY * textureRes + texX])
					glColor3f(0.6 * shade, 0.5 * shade, 0.3 * shade);
				else
					glColor3f(0.5 * shade, 0.3 * shade, 0.2 * shade);
			}
				
			glVertex2i(512 + 256 + rays * 2, 256 + y - h / 2);
			glEnd();
		}

		for (int y = h; y < 512; y++)
		{
			glBegin(GL_POINTS);
			glColor3f(0.3, 0.3, 0.3);
			glVertex2i(512 + 256 + rays * 2, 256 + y - h / 2);
			glEnd();
		}

		for (int y = 0; y < 256 - h / 2; y++)
		{
			glBegin(GL_POINTS);
			glColor3f(0.3, 0.3, 0.7);
			glVertex2i(512 + 256 + rays * 2, y);
			glEnd();
		}
	}
}


void keyDown(unsigned char key, int x, int y)
{
	if (key == 'w') { inputs.w = 1; }
	if (key == 'a') { inputs.a = 1; }
	if (key == 's') { inputs.s = 1; }
	if (key == 'd') { inputs.d = 1; }
	glutPostRedisplay();
}
void keyUp(unsigned char key, int x, int y)
{
	if (key == 'w') { inputs.w = 0;  }
	if (key == 'a') { inputs.a = 0; }
	if (key == 's') { inputs.s = 0; }
	if (key == 'd') { inputs.d = 0; }
	glutPostRedisplay();
}



float getFPS()
{
	static float prevFrame;
	float thisFrame;
	float fps;
	thisFrame = glutGet(GLUT_ELAPSED_TIME);
	fps = thisFrame - prevFrame;
	prevFrame = thisFrame;
	cout << fps << endl;
	return fps;
}


void ProcessInput()
{
	float delta = getFPS()/10;
	if (inputs.a) 
	{
		
		player->lookAngle = clampAngle(player->lookAngle - rotationSpeed*delta);
		player->lookDirection = Point(cos(player->lookAngle), sin(player->lookAngle));
	}
	if (inputs.d)
	{
		player->lookAngle = clampAngle(player->lookAngle + rotationSpeed*delta);
		player->lookDirection = Point(cos(player->lookAngle), sin(player->lookAngle));
	}
	if (inputs.w)
	{
		player->position += player->lookDirection * speed * delta;
	}
	if (inputs.s)
	{
		player->position += player->lookDirection * (-speed)*delta;
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
	ProcessInput();
	glClear(GL_COLOR_BUFFER_BIT);
	map->Draw();
	player->Draw();
	render();

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
	glutKeyboardFunc(keyDown);
	glutKeyboardUpFunc(keyUp);
	Initialize();
	glutMainLoop();
	return 0;
}
