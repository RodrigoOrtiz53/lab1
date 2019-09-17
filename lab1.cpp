//modified by: Rodrigo Ortiz
//date:
//
//3350 Spring 2019 Lab-1
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment is to modify this program.
//You will follow along with your instructor.
//
//Elements to be learned in this lab...
// .general animation framework
// .animation loop
// .object definition and movement
// .collision detection
// .mouse/keyboard interaction
// .object constructor
// .coding style
// .defined constants
// .use of static variables
// .dynamic memory allocation
// .simple opengl components
// .git
//
//elements we will add to program...
//   .Game constructor
//   .multiple particles
//   .gravity
//   .collision detection
//   .more objects


#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <ctime>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/Xdbe.h>
#include <GL/glx.h>
#include "fonts.h"
#include "log.h"


// test every particle for collison
// make an array of boxes

// use glRotate to make the boxes have weight when particles touch


const int MAX_PARTICLES = 5000;
const float GRAVITY     = .1;

//some structures

struct Vec {
    float x, y, z;
};

struct Shape {
    float width, height;
    float radius;
    Vec center;
};

struct Particle {
    Shape s;
    Vec velocity;
};

class Global {
    public:
        int xres, yres;
        Shape box[5];
        Shape circle;
        int moveBox[5];
        char boxText[5][20];

        Particle particle[MAX_PARTICLES];
        int n;
        Global();
} g;

class X11_wrapper {
    private:
        Display *dpy;
        Window win;
        GLXContext glc;
    public:
        ~X11_wrapper();
        X11_wrapper();
        void set_title();
        bool getXPending();
        XEvent getXNextEvent();
        void swapBuffers();
} x11;

//Function prototypes
void init_opengl(void);
void check_mouse(XEvent *e);
int check_keys(XEvent *e);
void movement();
void render();
void makeParticle(int x, int y);

//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
    srand(time(NULL));
    init_opengl();
    //Main animation loop
    int done = 0;
    while (!done) {
        //Process external events.
        while (x11.getXPending()) {
            XEvent e = x11.getXNextEvent();
            check_mouse(&e);
            done = check_keys(&e);
        }

        // Make particles without any input
        for (int i = 0; i < 5; i++) {
            makeParticle(150, 640);
        }

        movement();
        render();
        x11.swapBuffers();
    }
    return 0;
}

//-----------------------------------------------------------------------------
//Global class functions
//-----------------------------------------------------------------------------
Global::Global()
{
    xres = 800;
    yres = 600;

    //define a box shape
    for (int i = 0; i < 5; i++) {
        box[i].width = 80;
        box[i].height = 10;
        box[i].center.x = 100 + 65 * i;
        box[i].center.y = 500 - 60 * i;
    }

    //define circle
    circle.radius = 130;
    circle.center.x = 500;
    circle.center.y = 0;

    strcpy(boxText[0], "Requirements");
    strcpy(boxText[1], "Design");
    strcpy(boxText[2], "Coding");
    strcpy(boxText[3], "Testing");
    strcpy(boxText[4], "Maintenance");

    moveBox[0] = 38;
    moveBox[1] = 18;
    moveBox[2] = 17;
    moveBox[3] = 19;
    moveBox[4] = 32;

    n = 0;
}

//-----------------------------------------------------------------------------
//X11_wrapper class functions
//-----------------------------------------------------------------------------
X11_wrapper::~X11_wrapper()
{
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
    GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
    int w = g.xres, h = g.yres;
    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        cout << "\n\tcannot connect to X server\n" << endl;
        exit(EXIT_FAILURE);
    }
    Window root = DefaultRootWindow(dpy);
    XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
    if (vi == NULL) {
        cout << "\n\tno appropriate visual found\n" << endl;
        exit(EXIT_FAILURE);
    } 
    Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
    XSetWindowAttributes swa;
    swa.colormap = cmap;
    swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask | SubstructureNotifyMask;
    win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
            InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
    set_title();
    glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
    glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
    //Set the window title bar.
    XMapWindow(dpy, win);
    XStoreName(dpy, win, "3350 Lab1");
}

bool X11_wrapper::getXPending()
{
    //See if there are pending events.
    return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
    //Get a pending event.
    XEvent e;
    XNextEvent(dpy, &e);
    return e;
}

void X11_wrapper::swapBuffers()
{
    glXSwapBuffers(dpy, win);
}
//-----------------------------------------------------------------------------

void init_opengl(void)
{
    //OpenGL initialization
    glViewport(0, 0, g.xres, g.yres);
    //Initialize matrices
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    //Set 2D mode (no perspective)
    glOrtho(0, g.xres, 0, g.yres, -1, 1);
    //Set the screen background color
    glClearColor(0.1, 0.1, 0.1, 1.0);
    //Allow fonts to be shown
    glEnable(GL_TEXTURE_2D);
    initialize_fonts();

}

void makeParticle(int x, int y)
{
    //Add a particle to the particle system.
    //
    if (g.n >= MAX_PARTICLES)
        return;
    //set position of particle
    Particle *p = &g.particle[g.n];
    p->s.center.x = x;
    p->s.center.y = y;
    p->velocity.y = ((double)rand() / (double)RAND_MAX);
    p->velocity.x = ((double)rand() / (double)RAND_MAX);
    ++g.n;
}

void check_mouse(XEvent *e)
{
    static int savex = 0;
    static int savey = 0;

    //Weed out non-mouse events
    if (e->type != ButtonRelease &&
            e->type != ButtonPress &&
            e->type != MotionNotify) {
        //This is not a mouse event that we care about.
        return;
    }
    //
    if (e->type == ButtonRelease) {
        return;
    }
    if (e->type == ButtonPress) {
        if (e->xbutton.button==1) {
            //Left button was pressed.
            int y = g.yres - e->xbutton.y;

            for (int i = 0; i < 10; i++)
            {
                makeParticle(e->xbutton.x, y);
            }
            return;
        }
        if (e->xbutton.button==3) {
            //Right button was pressed.
            return;
        }
    }
    if (e->type == MotionNotify) {
        //The mouse moved!
        if (savex != e->xbutton.x || savey != e->xbutton.y) {
            savex = e->xbutton.x;
            savey = e->xbutton.y;
            //Code placed here will execute whenever the mouse moves.

            int y = g.yres - e->xbutton.y;

            for (int i=0; i<1; i++)
            {
                makeParticle(e->xbutton.x, y);
            }

        }
    }
}

int check_keys(XEvent *e)
{
    if (e->type != KeyPress && e->type != KeyRelease)
        return 0;
    int key = XLookupKeysym(&e->xkey, 0);
    if (e->type == KeyPress) {
        switch (key) {
            case XK_1:
                //Key 1 was pressed
                break;
            case XK_a:
                //Key A was pressed
                break;
            case XK_Escape:
                //Escape key was pressed
                return 1;
        }
    }
    return 0;
}

void movement()
{
    if (g.n <= 0)
        return;

    for(int i = 0; i < g.n; i++)
    {

        Particle *p = &g.particle[i];
        p->s.center.x += p->velocity.x;
        p->s.center.y += p->velocity.y;
        p->velocity.y -= GRAVITY;

        //check for collision with shapes...
        //Shape *s;
        for (int j = 0; j < 5; j++) {
            Shape *s = &g.box[j];
			
			if (p->s.center.y < s->center.y + g.box[j].height && 
					p->s.center.x > g.box[j].center.x - g.box[j].width &&
					p->s.center.x < g.box[j].center.x + g.box[j].width &&
                    p->s.center.y > g.box[j].center.y - g.box[j].height) {
                p->velocity.y = -p->velocity.y;
                p->velocity.y *= (((float)rand() / 
                            (4 * (float) RAND_MAX)) + .4);
            }

            //check for off-screen
            if (p->s.center.y < 0.0) {
                g.particle[i] = g.particle[g.n-1];
                --g.n;
            }
        }

        /*
        // check for collision with circle
        float squared_x = (p->s.center.x - g.circle.center.x) *
            (p->s.center.x - g.circle.center.x);
        float squared_y = (p->s.center.y - g.circle.center.y) *
            (p->s.center.y - g.circle.center.y);

        if (sqrt(squared_x + squared_y) < g.circle.radius)
        {
            p->velocity.y = -p->velocity.y;
            p->velocity.y *= (((float)rand() / (4 * (float)RAND_MAX)) + 0.2);

            if (p->s.center.x < g.circle.center.x)
                p->velocity.x -= 0.1;
            else
                p->velocity.x += 0.1;
        }
        */

    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    //Draw shapes...
    //draw the box
    Shape *s;
    glColor3ub(90, 140, 90);
    s = &g.box[0];

    float w, h;
    w = s->width;
    h = s->height;

    for (int i = 0; i < 5; i++) {

        glColor3ub(90, 140, 90);
        s = &g.box[i];

        glPushMatrix();
        glTranslatef(s->center.x, s->center.y, s->center.z);

        glBegin(GL_QUADS);

        glVertex2i(-w, -h);
        glVertex2i(-w,  h);
        glVertex2i( w,  h);
        glVertex2i( w, -h);

        glEnd();
        glPopMatrix();

        //Write Requirements on the box
        Rect r;

        r.bot = s->center.y - 7;
        r.left = s->center.x - g.moveBox[i];
        r.center = 0;

        ggprint13(&r, 16, 0x00fffffff, g.boxText[i]);
    }

    /*
    // Draw circle
    float x, y;

    glColor3ub(90, 140, 90);
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < 180; i++)
    {
        x = g.circle.radius * cos(i) + g.circle.center.x;
        y = g.circle.radius * sin(i) + g.circle.center.y;
        glVertex3f(x, y, 0);

        x = g.circle.radius * cos(i + 0.1) + g.circle.center.x;
        y = g.circle.radius * sin(i + 0.1) + g.circle.center.y;
        glVertex3f(x, y, 0);
    }

    glEnd();
    */

    //Draw particles here
    //if (g.n > 0) {
    for(int i =0; i<g.n; i++) {
        //There is at least one particle to draw.
        glPushMatrix();
        glColor3ub(150, 160, 220);

        Vec *c = &g.particle[i].s.center;
        w = h = 2;

        glBegin(GL_QUADS);

        glVertex2i(c->x-w, c->y-h);
        glVertex2i(c->x-w, c->y+h);
        glVertex2i(c->x+w, c->y+h);
        glVertex2i(c->x+w, c->y-h);

        glEnd();
        glPopMatrix();
    }
    //Draw your 2D text here

}
