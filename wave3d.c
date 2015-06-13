/*
 * File:   wave3d.c
 *
 * Created on August 13, 2011, 1:45 PM
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

int res[3]; // X, Y, and Z resolution of the grid
float scale; // How much to scale up the grid
int run; // Whether to run
int vwireframe; //, vsolid, vnormals, vheights; // Whether to draw a wireframe, solid view, face normals, and heights
int colorMode, modelNum; // The coloring scheme to use and current model number
float px, py, pz, rx, ry; // X, Y, and Z position, and rotation around X and Y axes
float keys[127]; // Whether a given key is being pressed or was recently pressed
int mox, moy, mnx, mny, mousedown; // Mouse original X and Y and new X and Y, and whether the mouse is pressed
float viscosity, keySpeed, mouseSpeed; // Speed-controlling factors
float exag; // How much to exaggerate the motion of the points
float ****disp, ****vel; // Array of displacements and veloceties of the nodes
int a; // Iteration number;
int selectingPreset, selectionUpdate;
char message[100]; // Message to be displayed
int ti1, ti2, tj1, tj2, tk1, tk2; // Temporary

double torad(double x) { return x * 3.14159 / 180; }
double todeg(double x) { return x * 180 / 3.14159; }

void step()
{
	float avg[3];
	float disp2[res[0]][res[1]][res[2]][3];
	for(int i = 1; i < res[0] - 1; i++) for(int j = 1; j < res[1] - 1; j++) for(int k = 1; k < res[2] - 1; k++) for(int l = 0; l < 3; l++)
	{
		avg[l] = (disp[i - 1][j][k][l] + disp[i + 1][j][k][l] + disp[i][j - 1][k][l] + disp[i][j + 1][k][l] + disp[i][j][k - 1][l] + disp[i][j][k + 1][l]) / 6;
		vel[i][j][k][l] -= (disp[i][j][k][l] - avg[l]) / viscosity;
		disp2[i][j][k][l] = disp[i][j][k][l] + vel[i][j][k][l];
	}
	for(int i = 1; i < res[0] - 1; i++) for(int j = 1; j < res[1] - 1; j++) for(int k = 1; k < res[2] - 1; k++) for(int l = 0; l < 3; l++) disp[i][j][k][l] = disp2[i][j][k][l]; // Copy the array over...can use pointers?
	a++; // Update the iteration number.
}

void initView()
{
	run = 0;
	a = 0;
	disp = malloc(res[0] * sizeof(float ***));
	vel = malloc(res[0] * sizeof(float ***));
	for(int i = 0; i < res[0]; i++)
	{
		disp[i] = malloc(res[1] * sizeof(float **));
		vel[i] = malloc(res[1] * sizeof(float **));
		for(int j = 0; j < res[1]; j++)
		{
			disp[i][j] = malloc(res[2] * sizeof(float *));
			vel[i][j] = malloc(res[2] * sizeof(float *));
			for(int k = 0; k < res[2]; k++)
			{
				disp[i][j][k] = malloc(3 * sizeof(float));
				disp[i][j][k][0] = i;
				disp[i][j][k][1] = j;
				disp[i][j][k][2] = k;
				vel[i][j][k] = malloc(3 * sizeof(float));
				vel[i][j][k][0] = vel[i][j][k][1] = vel[i][j][k][2] = 0;
			}
		}
	}
	float m, n;
	if (modelNum == 1) // Explosion
	{
		for(int i = 1; i < res[0] - 1; i++) for(int j = 1; j < res[1] - 1; j++) for(int k = 1; k < res[2] - 1; k++) for(int l = 0; l < 3; l++)
		{
			disp[i][j][k][l] = res[l] / 2 + (disp[i][j][k][l] - res[l] / 2) * 2;
		}
	}
	else if (modelNum == 2) // Implosion
	{
		for(int i = 1; i < res[0] - 1; i++) for(int j = 1; j < res[1] - 1; j++) for(int k = 1; k < res[2] - 1; k++) for(int l = 0; l < 3; l++)
		{
			disp[i][j][k][l]  = (disp[i][j][k][l] + res[l] / 2) / 2;
		}
	}
}

void init()
{
	glClearColor(0, 0, 0, 0);
	glEnable(GL_COLOR);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 0.01, 10000);
	glMatrixMode(GL_MODELVIEW);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glShadeModel(GL_SMOOTH);
	glPointSize(4);
	for (int i = 0; i < 128; i++) keys[i] = 0;	// Initialize the keys.
	px = 0; py = 0; pz = -60; rx = 20; ry = -20;
	res[0] = 11; res[1] = 11; res[2] = 11; scale = 3;
	viscosity = 1200; keySpeed = 0.2; mouseSpeed = 0.5;
	vwireframe = 0; //vsolid = 1; vnormals = 0; vheights = 1;
	colorMode = 0; modelNum = 0;
	exag = 2;
	initView();
}

void color(int i, int j, int k) // Sets glColor appropriately
{
	float r, g, b, a; a = 1;
	r = g = b = 1;
	float dist = sqrt(pow(res[0] / 2 - disp[i][j][k][0], 2) + pow(res[1] / 2 -  disp[i][j][k][1], 2) + pow(res[2] / 2 -  disp[i][j][k][2], 2));
	float reg = sqrt(pow(res[0] / 2 - i, 2) + pow(res[1] / 2 - j, 2) + pow(res[2] / 2 - k, 2));
	if (dist < reg) r = g = dist / reg;
	else if (dist < 2 * reg) g = b = (2 * reg - dist) / reg;
	else g = b = 0;
	glColor4f(r, g, b, a);
}

void inputHandler()
{
	if (keys['w'] > 0) pz -= keys['w'] * pz / (80 / keySpeed);
	if (keys['s'] > 0) pz += keys['s'] * pz / (80 / keySpeed);
	if (keys['p'] > 0) rx += keys['p'] * keySpeed;
	if (keys[';'] > 0) rx -= keys[';'] * keySpeed;
	if (keys['l'] > 0 || keys['a'] > 0) ry += (keys['l'] + keys['a']) * keySpeed;
	if (keys[39] > 0 || keys['d'] > 0) ry -= (keys[39] + keys['d']) * keySpeed;
	for (int i = 0; i < 127; i++) if (keys[i] > 0 && keys[i] < 1) keys[i] -= 0.1 * keySpeed;
	if(mousedown == 1)
	{
		ry += (mnx - mox) * mouseSpeed;
		rx += (mny - moy) * mouseSpeed;
		mox = mnx;
		moy = mny;
	}
	if (rx > 90) rx = 90; if (rx < -90) rx = -90;
}

void render()
{
	// DRAWING
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(px, py, pz);
	glRotatef(rx, 1, 0, 0);
	glRotatef(ry, 0, 1, 0);
	for (int i = 1; i < res[0] - 1; i++) for (int j = 1; j < res[1] - 1; j++) for (int k = 1; k < res[2] - 1; k++)
	{
		color(i, j, k);
		if (vwireframe)
		{
			glBegin(GL_POINTS);
			glVertex3f((disp[i][j][k][0] - res[0] / 2) * scale, (disp[i][j][k][1] - res[1] / 2) * scale, (disp[i][j][k][2] - res[2] / 2) * scale);
			glEnd();
		}
		else
		{
			glBegin(GL_LINES);
			if (i < res[0] - 2)
			{
				glVertex3f((disp[i][j][k][0] - res[0] / 2) * scale, (disp[i][j][k][1] - res[1] / 2) * scale, (disp[i][j][k][2] - res[2] / 2) * scale);
				glVertex3f((disp[i + 1][j][k][0] - res[0] / 2) * scale, (disp[i + 1][j][k][1] - res[1] / 2) * scale, (disp[i + 1][j][k][2] - res[2] / 2) * scale);
			}
			if (j < res[1] - 2)
			{
				glVertex3f((disp[i][j][k][0] - res[0] / 2) * scale, (disp[i][j][k][1] - res[1] / 2) * scale, (disp[i][j][k][2] - res[2] / 2) * scale);
				glVertex3f((disp[i][j + 1][k][0] - res[0] / 2) * scale, (disp[i][j + 1][k][1] - res[1] / 2) * scale, (disp[i][j + 1][k][2] - res[2] / 2) * scale);
			}
			if (k < res[2] - 2)
			{
				glVertex3f((disp[i][j][k][0] - res[0] / 2) * scale, (disp[i][j][k][1] - res[1] / 2) * scale, (disp[i][j][k][2] - res[2] / 2) * scale);
				glVertex3f((disp[i][j][k + 1][0] - res[0] / 2) * scale, (disp[i][j][k + 1][1] - res[1] / 2) * scale, (disp[i][j][k + 1][2] - res[2] / 2) * scale);
			}
			glEnd();
		}
	}
		if (selectingPreset)
		{
			run = 0;
			sprintf(message, "Preset:  ");
			glColor3f(1, 0, 0);
			glRotatef(-ry, 0, 1, 0);
			glRotatef(-rx, 1, 0, 0);
			glTranslatef(-px, -py, -pz - 0.1);
			glRasterPos2f(-0.04, -0.04);
			glutBitmapString(GLUT_BITMAP_8_BY_13, message);
			selectionUpdate = 0;
		}
	glutSwapBuffers(); // Send everything to the screen.
	inputHandler(); // Handle key and mouse presses.
	if (run) step(); // Update the grid for the next step of the animation.
}

void keyDownHandler(unsigned char c, int mx, int my) // Handles keyboard input
{
	keys[c] = 1;
	if (selectingPreset)
	{
		if (c > 47 && c < 58)
		{
			modelNum = c - 48;
			initView();
			run = 0;
		}
		selectingPreset = 0;
	}
	else
	{
		if (c == 'q') exit(0);
		else if (c == 'r') initView();
		else if (c == 'f') init();
		else if (c == ' ') run = (run + 1) % 2;
		else if (c == '.') { step(); run = 0; }
		else if (c == '=' || c == '+') viscosity /= 1.5; //{ if (viscosity > 40) viscosity -= 40; }
		else if (c == '-' || c == '_') viscosity *= 1.5; //viscosity += 40;
		else if (c == '[' && exag >= 0.1) exag -= 0.1;
		else if (c == ']') { exag += 0.1; }
		else if (c == 'z') { step(); run = 0; }
		else if (c == 'x') { vwireframe = (vwireframe + 1) % 2; }
		else if (c == 'i') selectingPreset = 1;
	}
	glutPostRedisplay();
}

void keyUpHandler(unsigned char c, int mx, int my)
{
	keys[c] = 0.9;
	glutPostRedisplay();
}

void mouseDownHandler(int button, int state, int px, int py)
{
	if(button == GLUT_LEFT_BUTTON)
	{
		if(state == GLUT_DOWN)
		{
			mnx = mox = px;
			mny = moy = py;
			mousedown = 1;
		}
		else if(state == GLUT_UP)
		{
			mousedown = 0;
		}
	}
	// Zoom
	else if (button == 3) pz += mouseSpeed;
	else if (button == 4) pz -= mouseSpeed;
	glutPostRedisplay();
}

void mouseMoveHandler(int px, int py)
{
	mnx = px;
	mny = py;
	glutPostRedisplay();
}

void mouseWheelHandler(int wheel, int direction, int x, int y)
{
	pz += direction * mouseSpeed;
}

/* KEYBINDINGS:
 * w/a/s/d	move in, out, left, right
 * p/l/;/'	rotate up, down, left, right
 * space	start/stop
 * .		step forward
 * ,		step backward (not implemented)
 * r		reset model
 * f		reset all
 * i		load preset pattern (enter pattern digit)
 * x		toggle wireframe/points
 * +/-		change simulation speed
 * q		quit
 */

int main(int argc, char** argv)
{
	glutInit(&argc, argv);				// Initialize GLUT
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);	// Display mode is dblbuf and RGB-alpha with depth enabled and antialiasing
	glutInitWindowSize(600, 600);			// Set the window size
	glutInitWindowPosition(0,0);			// Set the window position
	glutCreateWindow("Wave3D");			// Set the window title
	init();						// Initialize our view
	glutDisplayFunc(render);			// Set the drawing function
	glutIdleFunc(render);				// Set the idle function
	glutKeyboardFunc(keyDownHandler);		// Keydown handler
	glutKeyboardUpFunc(keyUpHandler);		// Keyup handler
	glutMouseFunc(mouseDownHandler);		// Mousedown handler
	glutMotionFunc(mouseMoveHandler);		// Mouse movement handler
	glutMouseWheelFunc(mouseWheelHandler);		// Mouse wheel handler
	glutMainLoop();					// Enter the event processing loop
	return 0;
}

