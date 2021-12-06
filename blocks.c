/*
 * main.c (c) Shish 2006
 * 
 * Blocks; a digital version of a block puzzle found at the 2006 BIO
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#define WIDTH 640
#define HEIGHT 480

typedef int list_id; // OGL display list

typedef struct {
	int x, y, z;
	float r, g, b;
	int cubes[7][7][7];
} block;

// camera position
static GLfloat zoom = -10;
static GLfloat yaw = 0;
static GLfloat pitch = 0;

// things to render
list_id grid;
block blocks[6];
int space[16][16][16];

// the selected block
static int selected = 0;

// as taken from a photo :)
float colours[7][3] = {
	{0.0, 0.0, 0.0}, // empty
	{0.4, 0.7, 1.0}, // blue
	{1.0, 1.0, 0.5}, // yellow
	{1.0, 0.6, 0.2}, // orange
	{0.5, 1.0, 1.0}, // cyan
	{0.6, 1.0, 0.5}, // green
	{1.0, 0.3, 0.5}, // red
};


/*
 * copy a block from it's 7-cube into the 16-cube
 */
static void drawBlock(int id) {
	int x, y, z;
	block *b = &blocks[id];
	for(x=0; x<7; x++)
		for(y=0; y<7; y++)
			for(z=0; z<7; z++)
				if(b->cubes[x][y][z])
					space[b->x+x][b->y+y][b->z+z] = b->cubes[x][y][z];	
}


/*
 * Render a unit of space[][][]
 */
void renderCube(int x, int y, int z) {
	int colour = space[x][y][z];
	
	x -= 8; // space is a 16-cube, from 0 - 16
	y -= 8; // the display is -10 - +10
	z -= 8; // so we -8 to align them

    glBegin(GL_QUADS);
    glColor3f(
		colours[colour][0], 
		colours[colour][1], 
		colours[colour][2]
	);

	// bottom
	glVertex3f(x+0, y+0, z+0);
	glVertex3f(x+1, y+0, z+0);
	glVertex3f(x+1, y+1, z+0);
	glVertex3f(x+0, y+1, z+0);

	// top
	glVertex3f(x+0, y+0, z+1);
	glVertex3f(x+1, y+0, z+1);
	glVertex3f(x+1, y+1, z+1);
	glVertex3f(x+0, y+1, z+1);

	// front
	glVertex3f(x+0, y+0, z+0);
	glVertex3f(x+1, y+0, z+0);
	glVertex3f(x+1, y+0, z+1);
	glVertex3f(x+0, y+0, z+1);

	// back
	glVertex3f(x+0, y+1, z+0);
	glVertex3f(x+1, y+1, z+0);
	glVertex3f(x+1, y+1, z+1);
	glVertex3f(x+0, y+1, z+1);

	// left
	glVertex3f(x+0, y+0, z+0);
	glVertex3f(x+0, y+1, z+0);
	glVertex3f(x+0, y+1, z+1);
	glVertex3f(x+0, y+0, z+1);

	// right
	glVertex3f(x+1, y+0, z+0);
	glVertex3f(x+1, y+1, z+0);
	glVertex3f(x+1, y+1, z+1);
	glVertex3f(x+1, y+0, z+1);

    glEnd();
}


/*
 * get OGL to draw the grid and the cubes
 */
int doRender(SDL_Window *window) {
	int i, x, y, z;

	// set some sane limits
	if(zoom < -30) zoom = -30;
	else if(zoom > -1) zoom = -1;

	// OGL stuff...
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

	// Move the camera
    glTranslatef(0.0f, 0.0f, zoom);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(yaw,   0.0f, 1.0f, 0.0f);

	// call the grid macro
    glCallList(grid);

	// clear the space
	for(x=0; x<16; x++)
		for(y=0; y<16; y++)
			for(z=0; z<16; z++)
			    if(space[x][y][z]) space[x][y][z] = 0;

	// put the blocks into the space
	for(i=0; i<6; i++)
		drawBlock(i);
    
	// render the space
	for(x=0; x<16; x++)
		for(y=0; y<16; y++)
			for(z=0; z<16; z++)
			    if(space[x][y][z]) renderCube(x, y, z);

	// gogogo~
    SDL_GL_SwapWindow(window);
    
    return 1;
}


#define between(x, min, max) ((x >= min) && (x <= max))
/*
 * Check if a 7-cube can be translated within
 * the 16-cube without bumping into anything
 */
static int moveBlock(int id, int xo, int yo, int zo) {
	int x, y, z;
	int currentUser;
	block *b = &blocks[id];

	// check that there's space in the places the block wants to be
	for(x=0; x<7; x++) {
		for(y=0; y<7; y++) {
			for(z=0; z<7; z++) {
				if(!b->cubes[x][y][z]) continue; // this space isn't wanted

				// make sure we stay within the 16-cube
				if(
					!between(b->x+xo+x, 0, 15) ||
					!between(b->y+yo+y, 0, 15) ||
					!between(b->z+zo+z, 0, 15)
				) {
					return 0;
				}

				// see what's in the space we want to use
				currentUser = space[b->x+xo+x][b->y+yo+y][b->z+zo+z];
				if((currentUser != 0) && (currentUser != b->cubes[x][y][z])) {
					return 0;
				}
			}
		}
	}

	// if we're still here, we can move
	b->x += xo;
	b->y += yo;
	b->z += zo;

	return 1;
}


/*
 * Check if a 7-cube can be rotated within
 * the 16-cube without bumping into anything
 *
 * FIXME: allow rotations about a point other
 * than the center...
 *
 * pitch = rotate y-z about x
 * yaw = rotate x-z about y
 * roll = rotate x-y about z
 */
static int rotateBlock(int id, int yaw, int pitch, int roll) {
	int x, y, z;
	int currentUser;
	int slice, p, q;
	block *b = &blocks[id]; // the current block
	block *n = (block *)malloc(sizeof(block)); // the new block (post-rotate)

	if(!n) return 0;
	memcpy(n, b, sizeof(block));

	// create a ghost 7-cube
	for(slice=0; slice<7; slice++) {
		for(p=0; p<7; p++) {
			for(q=0; q<7; q++) {
				if(pitch == -1) n->cubes[slice][p][q] = b->cubes[slice][q][6-p];
				if(pitch ==  1) n->cubes[slice][p][q] = b->cubes[slice][6-q][p];
				if(yaw   == -1) n->cubes[p][slice][q] = b->cubes[q][slice][6-p];
				if(yaw   ==  1) n->cubes[p][slice][q] = b->cubes[6-q][slice][p];
				if(roll  ==  1) n->cubes[p][q][slice] = b->cubes[q][6-p][slice];
				if(roll  == -1) n->cubes[p][q][slice] = b->cubes[6-q][p][slice];
			}
		}
	}

	// see if the ghost bumps into anything
	for(x=0; x<7; x++) {
		for(y=0; y<7; y++) {
			for(z=0; z<7; z++) {
				if(!n->cubes[x][y][z]) continue; // this space isn't wanted
			
				// make sure we stay within the 16-cube
				if(
					!between(n->x+x, 0, 15) ||
					!between(n->y+y, 0, 15) ||
					!between(n->z+z, 0, 15)
				) {
					free(n);
					return 0;
				}

				currentUser = space[n->x+x][n->y+y][n->z+z];
				if((currentUser != 0) && (currentUser != n->cubes[x][y][z])) {
					free(n); // put the ghost to rest
					return 0;
				}
			}
		}
	}

	// if the ghost is free, apply to the main block
	memcpy(b, n, sizeof(block));
	free(n);

	return 1;
}

		
/*
 * handle SDL input
 */
int doInput() {
	static int animate = 0;
	int ok = 0;
    SDL_Event event;

	if(animate) {
	    SDL_PollEvent(&event);
	}
	else {
		SDL_WaitEvent(&event);
	}

    switch(event.type) {
	    case SDL_QUIT:
			return 0;
		case SDL_MOUSEWHEEL:
			if(event.wheel.y > 0) zoom += 2;
			if(event.wheel.y < 0) zoom -= 2;
			break;
	    case SDL_MOUSEMOTION:
			if(event.motion.state & SDL_BUTTON(1)) {
				yaw += event.motion.xrel;
				pitch += event.motion.yrel;
			}
			else if(event.motion.state & (SDL_BUTTON(2)|SDL_BUTTON(3))) {
				zoom -= (float)event.motion.yrel/4.0f;
			}
			break;
	    case SDL_KEYDOWN:
			// global keybindings
	        switch(event.key.keysym.sym) {
				case SDLK_q: case SDLK_ESCAPE: return 0;
				case SDLK_KP_5: animate = !animate; break;
				case SDLK_KP_9: selected--; break;
				case SDLK_KP_3: selected++; break;
				default: break;
			}
			// ctrl-numpad = rotate
			if(event.key.keysym.mod & KMOD_CTRL) {
		        switch(event.key.keysym.sym) {
					case SDLK_KP_8: ok = rotateBlock(selected,  0,  1,  0); break;
					case SDLK_KP_2: ok = rotateBlock(selected,  0, -1,  0); break;
					case SDLK_KP_4: ok = rotateBlock(selected, -1,  0,  0); break;
					case SDLK_KP_6: ok = rotateBlock(selected,  1,  0,  0); break;
					case SDLK_KP_7: ok = rotateBlock(selected,  0,  0, -1); break;
					case SDLK_KP_1: ok = rotateBlock(selected,  0,  0,  1); break;
					default: break;
				}
			}
			// numpad = move
			else {
		        switch(event.key.keysym.sym) {
					case SDLK_KP_8: ok = moveBlock(selected,  0,  1,  0); break;
					case SDLK_KP_2: ok = moveBlock(selected,  0, -1,  0); break;
					case SDLK_KP_4: ok = moveBlock(selected, -1,  0,  0); break;
					case SDLK_KP_6: ok = moveBlock(selected,  1,  0,  0); break;
					case SDLK_KP_7: ok = moveBlock(selected,  0,  0, -1); break;
					case SDLK_KP_1: ok = moveBlock(selected,  0,  0,  1); break;
					default: break;
				}
			}
			// sanitise
			if(selected < 0) selected = 5;
			if(selected > 5) selected = 0;
			break;
	}

	if(ok) {/* make woosh noise */}
	else {/* make bump noise */}
	
	return 1;
}

/*
 * See how long it took to render a frame,
 * invert to get FPS, cap if necessary
 */
int doFPS() {
	static int start = 0;
		
	int now = SDL_GetTicks();
	int delta = now-start;
	start = now;
	
	// anti-aliasing off and no fps cap = 200FPS on slow hardware~
	if(1000/30-delta > 0) SDL_Delay(1000/30-delta);
	
	return 1;
}


/*
 * Boilerplate OGL init
 */
int initOpenGL() {
    float aspect = (float)WIDTH / (float)HEIGHT;
    glViewport(0, 0, WIDTH, HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, aspect, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glClearColor(1.0, 1.0, 1.0, 0);
    glEnable(GL_DEPTH_TEST);
//    glEnable(GL_LINE_SMOOTH); // antialiasing, only works on ATI cards?
    glDisable(GL_CULL_FACE); // remove faces that don't face the camera
	return 1;
}


/*
 * Make the grid into a display list
 */
int initGrid() {
	grid = glGenLists(1);
	if(!grid) return 0;

	glNewList(grid,GL_COMPILE);
	glBegin(GL_LINES);

	// x = red
	for(int y=0; y<21; y++) {
		if(y == 10) glColor3f(1.0f, 0.0f, 0.0f);
		else glColor3f(0.75f, 0.75f, 0.75f);

		glVertex3f(-10, -10+y, 0);
		glVertex3f(+10, -10+y, 0);
	}
	
	// y = green
	for(int x=0; x<21; x++) {
		if(x == 10) glColor3f(0.0f, 1.0f, 0.0f);
		else glColor3f(0.75f, 0.75f, 0.75f);

		glVertex3f(-10+x, -10, 0);
		glVertex3f(-10+x, +10, 0);
	}
	
	// z = blue
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(0, 0, -10);
	glVertex3f(0, 0, +10);
	
	glEnd();
	glEndList();

	return 1;
}


/*
 * load a block from a file
 */
static int loadBlock(int id) {
	blocks[id].x = 0;
	blocks[id].y = 0;
	blocks[id].z = 2 * id;
	
	char fname[32];
	sprintf(fname, "data/block_%i.txt", id);
	FILE *fp = fopen(fname, "r");
	if(!fp) return 0;
	// load into the center of the 7-cube
	for(int x=0; x<7; x++)
		for(int y=1; y<6; y++)
			fscanf(fp, "%d", &blocks[id].cubes[x][y][3]);
	fclose(fp);
	return 1;
}

/*
 * load the blocks from files
 */
int initBlocks() {
	return 
		loadBlock(0) &&
		loadBlock(1) &&
		loadBlock(2) &&
		loadBlock(3) &&
		loadBlock(4) &&
		loadBlock(5);
}

/*
 * start, main loop, end
 */
int main(int argc, char *argv[]) {
	SDL_Window *window = SDL_CreateWindow("Block Game 0.2", 0, 0, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if(window && initOpenGL() && initGrid() && initBlocks())
		while(doRender(window) && doFPS() && doInput());

	SDL_Quit();
	
	return 1;
}
