/*
 * File:   wave.c
 *
 * Created on April 7, 2011, 9:57 AM
 *
 * TODO Graphics:
 * Ironically, enabling line antialiasing causes some pretty bad aliasing in wireframe mode
 *
 * TODO Logic:
 * Allow grid deformation with right mouse
 * Allow backstep
 *
 * TODO Bugs:
 * Increasing viscosity too much changes height coefficient?
 * Increasing speed to much (decreasing viscosity) makes things blow up
 *
 * Connections may show in only two directions even if there are connections in the other two
 * Move all declarations to top of file for consistency
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>

int w, h; // X and Y resolution of the grid
float scale; // How much to scale up the grid
int run; // Whether to run
int vwireframe, vsolid, vnormals, vheights; // Whether to draw a wireframe, solid view, face normals, and heights
float atten; // How much waves attenuate
float xbase, ybase, xspace, yspace; // Adjusted coordinates and intervals of grid cells
/*float diag1y, diag2y, dx, dy, dz; // For calculating normals*/
float diag1, diag2, centx, centy, centz; // For calculating and displaying normals
float c1[3], c2[3], v[3]; // For calculating normals
int colorMode, modelNum; // The coloring scheme to use and current model number
float x, y, z, rx, ry; // X, Y, and Z position, and rotation around X and Y axes
float keys[128]; // Whether a given key is being pressed or was recently pressed
int mox, moy, mnx, mny, mousedown; // Mouse original X and Y and new X and Y, and whether the mouse is pressed
float viscosity, keySpeed, mouseSpeed; // Speed-controlling factors
float heightCoef; // Height exaggeration factor
float **depth, **vector, **save; // Array of depths and speeds of the nodes and accessory array for temporarily saving state
int **connect; // Connectivity of the nodes
int a; // Iteration number
int ux1, ux2, uy1, uy2; // User node selections
int x1, x2, y1_, y2; // Filtered node selections
int selecting; // Indicator for what is being selected
float selectedHeight; // User height selection
int heightModifier; // Whether we're in negative or positive height
int selectingNode, lastSelectingNode, selectingHeight, adjustingHeight, selectionUpdate, rangeSelect, selectingPreset; // Other user selections
char message[100]; // Message to be displayed
int ti1, ti2, tj1, tj2, tk1, tk2; // Temporary
int i; // This is the loop variable for setting the keys[] array in init().  Allocating it on the stack (i.e., declaring it in the function itself) causes shading to work improperly.  Somebody please explain this to me.

float torad(float x) { return x * 3.14159 / 180; }
float todeg(float x) { return x * 180 / 3.14159; }
int imax(int a, int b) { return (a > b) ? a : b; }
int imin(int a, int b) { return (a > b) ? b : a; }
int mod(int a, int b) { int r = a % b; return r < 0 ? r + b : r; }
void crossprod(float *a, float *b, float *ret)
{
	ret[0] = a[1] * b[2] - a[2] * b[1];
	ret[1] = a[2] * b[0] - a[0] * b[2];
	ret[2] = a[0] * b[1] - a[1] * b[0];
}
void normalize(float *v, float factor)
{
	float mag = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]) / factor;
	v[0] /= mag;
	v[1] /= mag;
	v[2] /= mag;
}

void print(float **array)
{
	for (int i = 0; i < w; i++)
	{
		for (int j = 0; j < h; j++) printf("%f\t", array[i][j]);
		printf("\n");
	}
	printf("\n");
}

void step()
{
	float avg;
	float depth2[w][h];
	for (int i = 1; i < w - 1; i++) for (int j = 1; j < h - 1; j++)
	{
		int conn = connect[i-1][j-1] + connect[i+1][j-1] + connect[i+1][j] + connect[i-1][j] +
			connect[i][j+1] + connect[i][j-1] + connect[i+1][j+1] + connect[i-1][j+1];
		if (conn == 0) continue;
		avg = (depth[i+1][j] * connect[i+1][j] + depth[i-1][j] * connect[i-1][j] +
			depth[i][j+1] * connect[i][j+1] + depth[i][j-1] * connect[i][j-1] +
			depth[i+1][j+1] * connect[i+1][j+1] + depth[i+1][j-1] * connect[i+1][j-1] +
			depth[i-1][j+1] * connect[i-1][j+1] + depth[i-1][j-1] * connect[i-1][j-1]) /
			conn;
		if (! (avg < 200 && avg > -200)) printf("%f\n", avg);
		vector[i][j] -= (depth[i][j] - avg) / viscosity;
		depth2[i][j] = depth[i][j] + vector[i][j];
		if (depth2[i][j] > 0) depth2[i][j] -= atten;
		else if (depth2[i][j] < 0) depth2[i][j] += atten;
	}
	for (int i = 1; i < w - 1; i++) for (int j = 1; j < h - 1; j++) depth[i][j] = depth2[i][j];
	a += 1; // Update the iteration number.
}

void initView()
{
	run = 0;
	a = 0;
	depth = malloc(w * sizeof(float *));
	for (int i = 0; i < w; i++) depth[i] = malloc(h * sizeof(float));
	vector = malloc(w * sizeof(float *));
	for (int i = 0; i < w; i++) vector[i] = malloc(h * sizeof(float));
	save = malloc(w * sizeof(float *));
	for (int i = 0; i < w; i++) save[i] = malloc(h * sizeof(float));
	connect = malloc(w * sizeof(int *));
	for (int i = 0; i < w; i++) connect[i] = malloc(h * sizeof(int));
	for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) depth[i][j] = 0;
	for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) save[i][j] = 0;
	for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) connect[i][j] = 1;
	float m, n;
	if (modelNum == 1) // Spike
	{
		depth[w / 2][h / 2] = 50;
	}
	else if (modelNum == 2) // Two-way sine
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			m = i * 10.0 / w;
			n = j * 10.0 / h;
			depth[i][j] = sin(m) + sin(n);
		}
	}
	else if (modelNum == 3) // One-way sine
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			m = i * 10.0 / w;
			n = j * 10.0 / h;
			depth[i][j] = sin(m);
		}
	}
	else if (modelNum == 4) // Cliff
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			if (i + j > (h + w) / 2) depth[i][j] = 2;
			else depth[i][j] = 0;
		}
	}
	else if (modelNum == 5) // Wall
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			if (i + j > (h + w) / 2 - 2 && i + j < (h + w) / 2 + 2) depth[i][j] = 4;
			else depth[i][j] = 0;
		}
	}
	else if (modelNum == 6) // Block

	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			if (i > 0.4 * w && i < 0.6 * w && j > 0.4 * h && j < 0.6 * h) depth[i][j] = 2;
			else depth[i][j] = 0;
		}
	}
	else if (modelNum == 7) // Hole
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			if (i > 0.4 * w && i < 0.6 * w && j > 0.4 * h && j < 0.6 * h) depth[i][j] = -2;
			else depth[i][j] = 0;
		}
	}
	else if (modelNum == 8) // Missing block
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
		{
			if (i > h / 2 || j > h / 2) depth[i][j] = 0;
			else depth[i][j] = -2;
		}
	}
	else if (modelNum == 9) // Table
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) depth[i][j] = 2;
	}
	else if (modelNum == 0) // Flat
	{
		for (int i = 0; i < w; i++) for (int j = 0; j < h; j++) depth[i][j] = 0;
	}
	for (int i = 0; i < w; i++) for (int j = 0; j < h; j++)
	{
		
		if (i * j * (w - i - 1) * (h - j - 1) == 0) depth[i][j] = 0;
		vector[i][j] = 0;
	}
}

void init()
{
	glClearColor(0, 0, 0, 0);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, 1, 0.01, 10000);
	glMatrixMode(GL_MODELVIEW);
	float ambient0[] = {0.2, 0.2, 0.2, 1.0};
	float diffuse0[] = {0.8, 0.8, 0.8, 1.0};
	float position0[] = {1, 1, 1};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse0);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient0);
	glLightfv(GL_LIGHT0, GL_POSITION, position0);
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient0);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_LIGHT0);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffuse0);
	glShadeModel(GL_SMOOTH);
	glPointSize(4);
	for (i = 0; i < 128; i++) keys[i] = 0; // Initialize the keys.
	x = 0; y = 0; z = -60; rx = 20; ry = -20;
	w = 100; h = 100; scale = 40;
	viscosity = 100; keySpeed = 0.2; mouseSpeed = 0.5;
	vwireframe = 0; vsolid = 1; vnormals = 0; vheights = 1;
	atten = 0;
	colorMode = 0; modelNum = 0;
	heightCoef = 2;
	selectingNode = 0; selectingHeight = 0; adjustingHeight = 0; selectionUpdate = 0; selectingPreset = 0;
	heightModifier = 1;
	initView();
}

void cleanup()
{
	free(depth);
	free(vector);
	free(save);
	free(connect);
}

void color(int i, int j) // Sets glColor appropriately
{
	float r, g, b, a = 0.9;
	if (colorMode == 1) { r = 0; g = (i + 10) / (2.0 * w); b = (j + 10) / (2.0 * h); }
	else if (colorMode == 2)
	{
		float c = depth[i][j];
		if (c < 0 && c >= -1) { b = r = 1 + c; g = 1; }
		else if (c < -1 && c >= -3) { b = 1 - ((c + 3) / 2); g = (c + 3) / 2; r = 0; }
		else if (c < -3) { b = 1; r = g = 0; }
		else if (c > 0 && c <= 1) { r = 1; g = b = 1 - c; }
		else if (c > 1 && c <= 3) { r = 1; g = 0; b = (c - 1) / 2; }
		else if (c > 3) { r = b = 1; g = 0; }
		else { r = 1; g = 1; b = 1; }
	}
	else if (colorMode == 3) r = g = b = (i + j) * 1.0 / (w + h);
	else r = g = b = 0.8;
	glColor4f(r, g, b, a);
}

void inputHandler()
{
	if (keys['w'] > 0) z -= keys['w'] * z / (80 / keySpeed);
	if (keys['s'] > 0) z += keys['s'] * z / (80 / keySpeed);
	if (keys['p'] > 0) rx += keys['p'] * keySpeed;
	if (keys[';'] > 0) rx -= keys[';'] * keySpeed;
	if (keys['l'] > 0 || keys['a'] > 0) ry += (keys['l'] + keys['a']) * keySpeed;
	if (keys[39] > 0 || keys['d'] > 0) ry -= (keys[39] + keys['d']) * keySpeed;
	for (int i = 0; i < 127; i++) if (keys[i] > 0 && keys[i] < 1) keys[i] -= 0.1 * keySpeed;
	if (mousedown == 1)
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
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(x, y, z);
	glRotatef(rx, 1, 0, 0);
	glRotatef(ry, 0, 1, 0);
	float height = heightCoef * vheights;
	xspace = scale / w;
	yspace = scale / h;
	int angle = mod((int) ry, 360);
	float norm = fmin(xspace, yspace) * 0.5;
	int i, j, idiff, jdiff, imax_, jmax, *iprox, *jprox;
	if (angle >= 315 || angle < 45) { iprox = &j; imax_ = h; idiff = 1; jprox = &i; jmax = w; jdiff = 1; }
	else if (angle >= 45 && angle < 135) { iprox = &i; imax_ = w; idiff = -1; jprox = &j; jmax = h; jdiff = 1; }
	else if (angle >= 135 && angle < 225) { iprox = &j; imax_ = h; idiff = -1; jprox = &i; jmax = w; jdiff = 1; }
	else if (angle >= 225 && angle < 315) { iprox = &i; imax_ = w; idiff = 1; jprox = &j; jmax = h; jdiff = 1; }
	for (*iprox = (idiff > 0 ? 0 : imax_ - 2); *iprox != (idiff > 0 ? imax_ - 1 : -1); *iprox += idiff) for (*jprox = (jdiff > 0 ? 0 : jmax - 2); *jprox != (jdiff > 0 ? jmax - 1 : -1); *jprox += jdiff)
	{
		if (! (connect[i][j] && connect[i+1][j] && connect[i][j+1] && connect[i+1][j+1])) continue;
		xbase = (i - w / 2.0) / w * scale;
		ybase = (j - h / 2.0) / h * scale;
		if (vwireframe)
		{
			glDisable(GL_LIGHTING);
			glBegin(GL_LINE_LOOP);
			// Point i, j
			color(i, j);
			glVertex3f(xbase, depth[i][j] * height, ybase);
			// Point i + 1, j
			color(i + 1, j);
			glVertex3f(xbase + xspace, depth[i + 1][j] * height, ybase);
			// Point i + 1, j + 1
			color(i + 1, j + 1);
			glVertex3f(xbase + xspace, depth[i + 1][j + 1] * height, ybase + yspace);
			// Point i, j + 1
			color(i, j + 1);
			glVertex3f(xbase, depth[i][j + 1] * height, ybase + yspace);
			glEnd();
			glEnable(GL_LIGHTING);
		}
		if (vsolid)
		{
			c1[0] = xspace;
			c2[2] = yspace;
			c1[2] = c2[0] = 0;
			glBegin(GL_QUAD_STRIP);
			// Point i, j
			c1[1] = (depth[i+1][j] - depth[i][j]) * height;
			c2[1] = (depth[i][j+1] - depth[i][j]) * height;
			crossprod(c1, c2, v);
			normalize(v, 1.0);
			glNormal3fv(v);
			color(i, j);
			glVertex3f(xbase, depth[i][j] * height, ybase);
			// Point i + 1, j
			c1[1] = (depth[i+1][j] - depth[i][j]) * height;
			c2[1] = (depth[i+1][j+1] - depth[i+1][j]) * height;
			crossprod(c1, c2, v);
			normalize(v, 1.0);
			glNormal3fv(v);
			color(i + 1, j);
			glVertex3f(xbase + xspace, depth[i+1][j] * height, ybase);
			// Point i, j + 1
			c1[1] = (depth[i+1][j+1] - depth[i][j+1]) * height;
			c2[1] = (depth[i][j+1] - depth[i][j]) * height;
			crossprod(c1, c2, v);
			normalize(v, 1.0);
			glNormal3fv(v);
			color(i, j + 1);
			glVertex3f(xbase, depth[i][j+1] * height, ybase + yspace);
			// Point i + 1, j + 1
			c1[1] = (depth[i+1][j+1] - depth[i][j+1]) * height;
			c2[1] = (depth[i+1][j+1] - depth[i+1][j]) * height;
			crossprod(c1, c2, v);
			normalize(v, 1.0);
			glNormal3fv(v);
			color(i + 1, j + 1);
			glVertex3f(xbase + xspace, depth[i+1][j+1] * height, ybase + yspace);
			glEnd();
		}
		if (vnormals)
		{
			glDisable(GL_LIGHTING);
			c1[0] = xspace;
			c2[0] = -xspace;
			c1[2] = c2[2] = yspace;
			c1[1] = (depth[i+1][j+1] - depth[i][j]) * height;
			c2[1] = (depth[i][j+1] - depth[i+1][j]) * height;
			crossprod(c1, c2, v);
			normalize(v, norm);
			centx = xbase + xspace / 2.0;
			centy = (depth[i][j] + depth[i][j+1] + depth[i+1][j] + depth[i+1][j+1]) / 4.0 * height;
			centz = ybase + yspace / 2.0;
			glBegin(GL_LINES);
			glColor3f(0.6, 0, 0);
			glVertex3f(centx, centy, centz);
			glVertex3f(centx - v[0], centy - v[1], centz - v[2]);
			glEnd();
			glEnable(GL_LIGHTING);
		}
	}
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	if (keys['u'] == 1)
	{
		glColor3f(0.6, 0, 0);
		glBegin(GL_POINTS);
		if (rangeSelect) for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) glVertex3f((i - w / 2.0) / w * scale, depth[i][j] * height, (j - h / 2.0) / h * scale);
		else glVertex3f((x1 - w / 2.0) / w * scale, depth[x1][y1_] * height, (y1_ - h / 2.0) / h * scale);
		glEnd();
	}
	if (selectingNode || selectingHeight || adjustingHeight || selectingPreset)
	{
		run = 0;
		if (selectingNode)
		{
			if (selectionUpdate)
			{
				if (rangeSelect)
				{
					sprintf(message, "Selected range:  (%i, %i) - (%i, %i)", ux1, uy1, ux2, uy2);
					ti1 = imax(imin(ux1, ux2), 0);
					ti2 = imin(imax(ux1, ux2), w - 1);
					tj1 = imax(imin(uy1, uy2), 0);
					tj2 = imin(imax(uy1, uy2), h - 1);
				}
				else
				{
					sprintf(message, "Selected node:  (%i, %i)", ux1, uy1);
					ti2 = ti1 = imax(imin(ux1, w - 1), 0);
					tj2 = tj1 = imax(imin(uy1, h - 1), 0);
				}
			}
			glColor3f(0.6, 0, 0);
			glBegin(GL_POINTS);
			for (int i = ti1; i <= ti2; i++) for (int j = tj1; j <= tj2; j++) glVertex3f((i - w / 2.0) / w * scale, depth[i][j] * height, (j - h / 2.0) / h * scale);
			glEnd();
		}
		else if (selectingHeight || adjustingHeight)
		{
			if (selectionUpdate)
			{
				if (selectingHeight)
				{
					sprintf(message, "Node height:  %.2f", heightModifier * selectedHeight);
					for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) depth[i][j] = heightModifier * selectedHeight;
				}
				else if (adjustingHeight)
				{
					sprintf(message, "Node height adjustment:  %.2f", heightModifier * selectedHeight);
					for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) depth[i][j] = save[i][j] + heightModifier * selectedHeight;
				}
			}
			glColor3f(0.6, 0, 0);
			glBegin(GL_POINTS);
			if (rangeSelect) for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) glVertex3f((i - w / 2.0) / w * scale, depth[i][j] * height, (j - h / 2.0) / h * scale);
			else glVertex3f((x1 - w / 2.0) / w * scale, depth[x1][y1_] * height, (y1_ - h / 2.0) / h * scale);
			glEnd();
		}
		else if (selectingPreset)
		{
			sprintf(message, "Preset:  ");
		}
		glColor3f(0.6, 0, 0);
		glRotatef(-ry, 0, 1, 0);
		if (vheights)
		{
			/*diag1 = (depth[i][j+1] - depth[i][j]);*/
			/*diag2 = (depth[i+1][j] - depth[i][j]);*/
			/*dx = - diag2y * heightCoef;*/
			/*dz = - diag1y * heightCoef;*/
			
		}
		/*else dx = dz = 0;*/
		/*dy = 0.1;*/
		glRotatef(-rx, 1, 0, 0);
		glTranslatef(-x, -y, -z - 0.1);
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
	if (selectingNode)
	{
		int current;
		if (selectingNode == 1) current = ux1;
		else if (selectingNode == 2) current = uy1;
		else if (selectingNode == 3) current = ux2;
		else if (selectingNode == 4) current = uy2;
		lastSelectingNode = selectingNode;
		selectionUpdate = 1;
		if (c == 27 || c == 'q')
		{
			ux1 = 0; ux2 = 0; uy1 = 0; uy2 = 0;
			selectingNode = 0;
		}
		else
		{
			if (c == '\b')
			{
				if (current == 0) selectingNode -= 1;
				else current /= 10;
				if (selectingNode < 3) rangeSelect = 0;
			}
			else if (c > 47 && c < 58)
			{
				current = current * 10 + c - 48;
			}
			else if (c == '-' && selectingNode < 3)
			{
				ux2 = 0; uy2 = 0;
				rangeSelect = 1;
				selectingNode = 3;
			}
			else if (c == ',' && selectingNode == 1) selectingNode = 2;
			else if (c == ',' && selectingNode == 3) selectingNode = 4;
			else if (c == 13)
			{
				if (rangeSelect)
				{
					x1 = imax(imin(ux1, ux2), 0);
					x2 = imin(imax(ux1, ux2), w - 1);
					y1_ = imax(imin(uy1, uy2), 0);
					y2 = imin(imax(uy1, uy2), h - 1);
				}
				else
				{
					x2 = x1 = imax(imin(ux1, w - 1), 0);
					y2 = y1_ = imax(imin(uy1, h - 1), 0);
				}
				selectingNode = 0;
			}
			if (lastSelectingNode == 1) ux1 = current;
			else if (lastSelectingNode == 2) uy1 = current;
			else if (lastSelectingNode == 3) ux2 = current;
			else if (lastSelectingNode == 4) uy2 = current;
		}
	}
	else if (selectingHeight || adjustingHeight)
	{
		if (c == '\b')
		{
			if (selectedHeight == 0) heightModifier = 1;
			else selectedHeight /= 10;
			if (selectedHeight < 0.0099) selectedHeight = 0; // Stupid
		}
		else if (c > 47 && c < 58) selectedHeight = selectedHeight * 10 + 0.01 * (c - 48);
		else if (c == '-') heightModifier *= -1;
		else if (c == 13) adjustingHeight = selectingHeight = 0;
		else if (c == 27 || c == 'q' || (c == 'h' && selectingHeight) || (c == 'j' && adjustingHeight))
		{
			selectingHeight = 0;
			adjustingHeight = 0;
			for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) depth[i][j] = save[i][j];
		}
		selectionUpdate = 1;
	}
	else if (selectingPreset)
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
		if (c == 'q')
		{
			cleanup();
			exit(0);
		}
		else if (c == 13)
		{
			selectingNode = 1;
			selectionUpdate = 1;
			ux1 = 0; ux2 = 0; uy1 = 0; uy2 = 0;
			rangeSelect = 0;
		}
		else if (c == 'h')
		{
			selectingHeight = 1;
			selectionUpdate = 1;
			selectedHeight = 0;
			heightModifier = 1;
			for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) save[i][j] = depth[i][j];
		}
		else if (c == 'j')
		{
			adjustingHeight = 1;
			selectionUpdate = 1;
			selectedHeight = 0;
			heightModifier = 1;
			for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++) save[i][j] = depth[i][j];
		}
		else if (c == 'o')
		{
			for (int i = x1; i <= x2; i++) for (int j = y1_; j <= y2; j++)
			{
				if (connect[i][j] > 0) save[i][j] = depth[i][j];
				else depth[i][j] = save[i][j];
				connect[i][j] = (connect[i][j] + 1) % 2;
			}
			
		}
		else if (c == 'f')
		{
			cleanup();
			init();
		}
		else if (c == 'r') initView();
		else if (c == ' ') run = (run + 1) % 2;
		else if (c == '=' || c == '+') viscosity /= 1.5;
		else if (c == '-' || c == '_') viscosity *= 1.5;
		else if (c == '[' && heightCoef >= 0.1) heightCoef -= 0.1;
		else if (c == ']') { heightCoef += 0.1; }
		else if (c == '.') { step(); run = 0; }
		else if (c == 'x') { vwireframe = (vwireframe + 1) % 2; vsolid = (vwireframe + 1) % 2; }
		else if (c == 'v') vnormals = (vnormals + 1) % 2;
		else if (c == 'c') colorMode = (colorMode + 1) % 4;
		else if (c == 'b') vheights = (vheights + 1) % 2;
		else if (c == 'n' && atten > 0) atten -= 0.00001;
		else if (c == 'm') atten += 0.00001;
		else if (c == 'i') selectingPreset = 1;
	}
	glutPostRedisplay();
}

void keyUpHandler(unsigned char c, int mx, int my)
{
	keys[c] = 0.9;
	glutPostRedisplay();
}

void mouseDownHandler(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			mnx = mox = x;
			mny = moy = y;
			mousedown = 1;
		}
		else if (state == GLUT_UP)
		{
			mousedown = 0;
		}
	}
	// Zoom
	else if (button == 3) z += mouseSpeed;
	else if (button == 4) z -= mouseSpeed;
	glutPostRedisplay();
}

void mouseMoveHandler(int x, int y)
{
	mnx = x;
	mny = y;
	glutPostRedisplay();
}

void mouseWheelHandler(int wheel, int direction, int x, int y)
{
	printf("%d, %d, %d, %d", wheel, direction, x, y);
	/*z += direction * mouseSpeed;*/
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
 * x		toggle wireframe/solid
 * c		switch color mode
 * v		toggle face normals
 * b		enable/disable heights
 * m/n		increase/decrease attenuation
 * +/-		change simulation speed
 * [/]		change height exaggeration
 * enter	select node (enter comma-separated node coordinates and press enter again; backspace to delete; okay to enter a range of nodes with a hyphen)
 * u		display selected nodes
 * o		hide/show selected nodes
 * h		set height of selected node (enter height and press enter; backspace to delete)
 * j		adjust height of selected nodes (enter adjustment and press enter; backspace to delete)
 * escape	cancel operation
 * q		quit
 */

int main(int argc, char** argv)
{
	glutInit(&argc, argv);				// Initialize GLUT.
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);	// Display mode is dblbuf and RGB-alpha with depth enabled and antialiasing
	glutInitWindowSize(800, 800);			// Set the window size
	glutInitWindowPosition(0,0);			// Set the window position
	glutCreateWindow("Wave");			// Set the window title
	init();						// Initialize our view
	glutDisplayFunc(render);			// Set the drawing function
	glutIdleFunc(render);				// Set the idle function
	glutKeyboardFunc(keyDownHandler);		// Keydown handler
	glutKeyboardUpFunc(keyUpHandler);		// Keyup handler
	glutMouseFunc(mouseDownHandler);		// Mousedown handler
	glutMotionFunc(mouseMoveHandler);		// Mouse movement handler
	glutMainLoop();					// Enter the event processing loop
	return 0;
}

