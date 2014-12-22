/*
 * An SDL replacement for BUILD's VESA code.
 *
 *  Written by Ryan C. Gordon. (icculus@clutteredmind.org)
 *
 * Please do NOT harrass Ken Silverman about any code modifications
 *  (including this file) to BUILD.
 */

/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file IS NOT A PART OF Ken Silverman's original release
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include "platform.h"
#include "amiga_display.h"

#include "build.h"
#include "display.h"
#include "pragmas.h"
#include "engine.h"

#include "a.h"
#include "cache1d.h"


/*
 * !!! remove the surface_end checks, for speed's sake. They are a
 * !!!  needed safety right now. --ryan.
 */


#define DEFAULT_MAXRESWIDTH  1600
#define DEFAULT_MAXRESHEIGHT 1200

AMIGA_Surface *surface = NULL;

int _argc = 0;
char **_argv = NULL;

    /* !!! move these elsewhere? */
long xres, yres, bytesperline, frameplace, imageSize, maxpages;
char *screen, vesachecked;
long buffermode, origbuffermode, linearmode;
char permanentupdate = 0, vgacompatible;

extern BOOL directaccess,dblbuf;

static long mouse_x = 0;
static long mouse_y = 0;
static long mouse_relative_x = 0;
static long mouse_relative_y = 0;
static short mouse_buttons = 0;
static unsigned int lastkey = 0;
/* so we can make use of setcolor16()... - DDOI */
static unsigned char drawpixel_color=0;
/* These hold the colors for the 256 color palette when in 16-color mode - DDOI */
static char backup_palette[48];
static int in_egapalette = 0;
/* The standard EGA palette in BUILD format - DDOI */
static char egapalette[] = { 0, 0, 0,
			    0, 0, 42,
			    0, 42, 0,
			    0, 42, 42,
			    42, 0, 0,
			    42, 0, 42,
			    42, 21, 0,
			    42, 42, 42,
			    21, 21, 21,
			    21, 21, 63,
			    21, 63, 21,
			    21, 63, 63,
			    63, 21, 21,
			    63, 21, 63,
			    63, 63, 21,
			    63, 63, 63
			   };

static unsigned int scancodes[256];

static long last_render_ticks = 0;
long total_render_time = 1;
long total_rendered_frames = 0;

static char *titlelong = NULL;
static char *titleshort = NULL;

void restore256_palette (void);
void set16color_palette (void);


/* lousy -ansi flag.  :) */
static char *string_dupe(const char *str)
{
    char *retval = malloc(strlen(str) + 1);
    if (retval != NULL)
	strcpy(retval, str);
    return(retval);
} /* string_dupe */

/*
 * !!! This is almost an entire copy of the original setgamemode().
 * !!!  Figure out what is needed for just 2D mode, and separate that
 * !!!  out. Then, place the original setgamemode() back into engine.c,
 * !!!  and remove our simple implementation (and this function.)
 * !!!  Just be sure to keep the non-DOS things, like the window's
 * !!!  titlebar caption.   --ryan.
 */
static char screenalloctype = 255;
static void init_new_res_vars(int davidoption)
{
    int i = 0;
    int j = 0;

    setupmouse();

    xdim = xres = surface->w;
    ydim = yres = surface->h;

    bytesperline = surface->pitch;
    vesachecked = 1;
    vgacompatible = 1;
    linearmode = 1;
    qsetmode = surface->h;
    activepage = visualpage = 0;
    horizlookup = horizlookup2 = NULL;

    frameplace = NULL;

    if (screen != NULL)
    {
	if (screenalloctype == 0) kkfree((void *)screen);
	    if (screenalloctype == 1) suckcache((long *)screen);
		screen = NULL;
    } /* if */

    if (davidoption != -1)
    {
	switch(vidoption)
	{
		case 1:i = xdim*ydim; break;
		case 2: xdim = 320; ydim = 200; i = xdim*ydim; break;
		case 6: xdim = 320; ydim = 200; i = 131072; break;
		default: assert(0);
	}
	j = ydim*4*sizeof(long);  /* Leave room for horizlookup&horizlookup2 */

	screenalloctype = 0;

	if ((screen = (char *)kkmalloc(i+(j<<1))) == NULL)
	{
		 allocache((long *)&screen,i+(j<<1),&permanentlock);
		 screenalloctype = 1;
	}

	if(!directaccess)
	{
		frameplace = FP_OFF(screen);
	}
	else
	{
	    frameplace=AMIGA_LockBackbuffer(&bytesperline);
	}

	horizlookup = (long *)(frameplace+i);
	horizlookup2 = (long *)(frameplace+i+j);
    } /* if */

    j = 0;
    for(i = 0; i <= ydim; i++)
    {
	ylookup[i] = j;
	j += bytesperline;
    } /* for */

    horizycent = ((ydim*4)>>1);

    /* Force drawrooms to call dosetaspect & recalculate stuff */
    oxyaspect = oxdimen = oviewingrange = -1;

    setvlinebpl(bytesperline);

    if (davidoption != -1)
    {
	setview(0L,0L,xdim-1,ydim-1);
	clearallviews(0L);
    } /* if */

    setbrightness((char) curbrightness, (unsigned char *) &palette[0]);

    if (searchx < 0) { searchx = halfxdimen; searchy = (ydimen>>1); }
} /* init_new_res_vars */


static void go_to_new_vid_mode(int vidoption, int w, int h)
{
    getvalidvesamodes();
    surface = AMIGA_SetVideoMode(w, h, FALSE,titlelong);
    if (surface == NULL)
    {
	printf("AMIGA: Failed to set %dx%d video mode!\n",w,h);
	AMIGA_Quit();
	exit(13);
    } /* if */

    init_new_res_vars(vidoption);
} /* go_to_new_vid_mode */


static __inline int amiga_mouse_button_filter(AMIGA_Event const *event)
{
    if(event->type==AMIGA_MOUSEBUTTONDOWN)
    {
	if(event->lbutton)
	    mouse_buttons |= 0x01;

	if(event->rbutton)
	    mouse_buttons |= 0x02;

	if(event->mbutton)
	    mouse_buttons |= 0x04;
    }
    else if(event->type==AMIGA_MOUSEBUTTONUP)
    {
	if(event->lbutton)
	    mouse_buttons &= 0xfe;

	if(event->rbutton)
	    mouse_buttons &= 0xfd;

	if(event->mbutton)
	    mouse_buttons &= 0xfb;
    }

    return(0);
} /* sdl_mouse_up_filter */


static int amiga_mouse_motion_filter(AMIGA_Event const *event)
{
    if (surface == NULL)
	return(0);

    mouse_relative_x = event->mousex;
    mouse_relative_y = event->mousey;
    mouse_x += mouse_relative_x;
    mouse_y += mouse_relative_y;

    if (mouse_x < 0) mouse_x = 0;
    if (mouse_x > surface->w) mouse_x = surface->w;
    if (mouse_y < 0) mouse_y = 0;
    if (mouse_y > surface->h) mouse_y = surface->h;

    return(0);
} /* sdl_mouse_motion_filter */


static int amiga_key_filter(const AMIGA_Event *event)
{
    int extended;

    lastkey = scancodes[event->keycode];

    if (lastkey == 0x0000)   /* No DOS equivalent defined. */
	return(0);

    extended = ((lastkey & 0xFF00) >> 8);
    if (extended != 0)
    {
	lastkey = extended;
	keyhandler();
	lastkey = (scancodes[event->keycode] & 0xFF);
    } /* if */

    if (event->keyup)
	lastkey += 128;  /* +128 signifies that the key is released in DOS. */

    keyhandler();
    return(0);
} /* amiga_key_filter */


static int root_amiga_event_filter(const AMIGA_Event *event)
{
    switch (event->type)
    {
	case AMIGA_KEYUP: case AMIGA_KEYDOWN:
	    return(amiga_key_filter(event));
	break;

	case AMIGA_MOUSEMOTION:
	    return(amiga_mouse_motion_filter(event));
	break;
	
	case AMIGA_MOUSEBUTTONUP:
	case AMIGA_MOUSEBUTTONDOWN:
	    return(amiga_mouse_button_filter(event));
	break;

	case AMIGA_QUIT:
	    /* !!! rcg TEMP */
	    AMIGA_Quit();
	    exit(0);   
    } /* switch */

    return(1);
} /* root_amiga_event_filter */


static void handle_events(void)
{
    AMIGA_Event event;

    //if(directaccess)
	//AMIGA_UnlockBackbuffer();

    while (AMIGA_PollEvent(&event))
	root_amiga_event_filter(&event);

    //if(directaccess)
	//frameplace=AMIGA_LockBackbuffer(&bytesperline);
} /* handle_events */


/* bleh...public version... */
void _handle_events(void)
{
    handle_events();
} /* _handle_events */


//static SDL_Joystick *joystick = NULL;
void _joystick_init(void)
{
/*    const char *envr = getenv(BUILD_SDLJOYSTICK);
    int favored = 0;
    int numsticks;
    int i;

    if (joystick != NULL)
    {
	sdldebug("Joystick appears to be already initialized.");
	sdldebug("...deinitializing for stick redetection...");
	_joystick_deinit();
    }

    if ((envr != NULL) && (strcmp(envr, "none") == 0))
    {
	sdldebug("Skipping joystick detection/initialization at user request");
	return;
    }

    sdldebug("Initializing SDL joystick subsystem...");
    sdldebug(" (export environment variable BUILD_SDLJOYSTICK=none to skip)");

    if (SDL_Init(SDL_INIT_JOYSTICK) != 0)
    {
	sdldebug("SDL_Init(SDL_INIT_JOYSTICK) failed: [%s].", SDL_GetError());
	return;
    }

    numsticks = SDL_NumJoysticks();
    sdldebug("SDL sees %d joystick%s.", numsticks, numsticks == 1 ? "" : "s");
    if (numsticks == 0)
	return;

    for (i = 0; i < numsticks; i++)
    {
	const char *stickname = SDL_JoystickName(i);
	if ((envr != NULL) && (strcmp(envr, stickname) == 0))
	    favored = i;

	sdldebug("Stick #%d: [%s]", i, stickname);
    }

    sdldebug("Using Stick #%d.", favored);
    if ((envr == NULL) && (numsticks > 1))
	sdldebug("Set BUILD_SDLJOYSTICK to one of the above names to change.");

    joystick = SDL_JoystickOpen(favored);
    if (joystick == NULL)
    {
	sdldebug("Joystick #%d failed to init: %s", favored, SDL_GetError());
	return;
    }

    sdldebug("Joystick initialized. %d axes, %d buttons, %d hats, %d balls.",
	      SDL_JoystickNumAxes(joystick), SDL_JoystickNumButtons(joystick),
	      SDL_JoystickNumHats(joystick), SDL_JoystickNumBalls(joystick));

    SDL_JoystickEventState(SDL_QUERY);*/
} /* _joystick_init */


void _joystick_deinit(void)
{
/*    if (joystick != NULL)
    {
	sdldebug("Closing joystick device...");
	SDL_JoystickClose(joystick);
	sdldebug("Joystick device closed. Deinitializing SDL subsystem...");
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	sdldebug("SDL joystick subsystem deinitialized.");
	joystick = NULL;
    }*/
} /* _joystick_deinit */


int _joystick_update(void)
{
/*    if (joystick == NULL)
	return(0);

    SDL_JoystickUpdate();*/
    return(1);
} /* _joystick_update */


int _joystick_axis(int axis)
{
/*    if (joystick == NULL)
	return(0);

    return(SDL_JoystickGetAxis(joystick, axis));*/
    return 0;
} /* _joystick_axis */


int _joystick_button(int button)
{
/*    if (joystick == NULL)
	return(0);

    return(SDL_JoystickGetButton(joystick, button) != 0);*/
    return 0;
} /* _joystick_button */


unsigned char _readlastkeyhit(void)
{
    return(lastkey);
} /* _readlastkeyhit */




void _platform_init(int argc, char **argv, const char *title, const char *icon)
{
    _argc = argc;
    _argv = argv;

    if (title == NULL)
	title = "BUILD";

    if (icon == NULL)
	icon = "BUILD";

    titlelong = string_dupe(title);
    titleshort = string_dupe(icon);

    memset(scancodes, '\0', sizeof (scancodes));
    scancodes[AMIKB_ESCAPE]          = 0x01;
    scancodes[AMIKB_1]               = 0x02;
    scancodes[AMIKB_2]               = 0x03;
    scancodes[AMIKB_3]               = 0x04;
    scancodes[AMIKB_4]               = 0x05;
    scancodes[AMIKB_5]               = 0x06;
    scancodes[AMIKB_6]               = 0x07;
    scancodes[AMIKB_7]               = 0x08;
    scancodes[AMIKB_8]               = 0x09;
    scancodes[AMIKB_9]               = 0x0A;
    scancodes[AMIKB_0]               = 0x0B;
    scancodes[AMIKB_MINUS]           = 0x0C; /* was 0x4A */
    scancodes[AMIKB_EQUALS]          = 0x0D; /* was 0x4E */
    scancodes[AMIKB_BACKSPACE]       = 0x0E;
    scancodes[AMIKB_TAB]             = 0x0F;
    scancodes[AMIKB_Q]               = 0x10;
    scancodes[AMIKB_W]               = 0x11;
    scancodes[AMIKB_E]               = 0x12;
    scancodes[AMIKB_R]               = 0x13;
    scancodes[AMIKB_T]               = 0x14;
    scancodes[AMIKB_Y]               = 0x15;
    scancodes[AMIKB_U]               = 0x16;
    scancodes[AMIKB_I]               = 0x17;
    scancodes[AMIKB_O]               = 0x18;
    scancodes[AMIKB_P]               = 0x19;
    scancodes[AMIKB_LEFTBRACKET]     = 0x1A;
    scancodes[AMIKB_RIGHTBRACKET]    = 0x1B;
    scancodes[AMIKB_RETURN]          = 0x1C;
    scancodes[AMIKB_LCTRL]           = 0x1D;
    scancodes[AMIKB_A]               = 0x1E;
    scancodes[AMIKB_S]               = 0x1F;
    scancodes[AMIKB_D]               = 0x20;
    scancodes[AMIKB_F]               = 0x21;
    scancodes[AMIKB_G]               = 0x22;
    scancodes[AMIKB_H]               = 0x23;
    scancodes[AMIKB_J]               = 0x24;
    scancodes[AMIKB_K]               = 0x25;
    scancodes[AMIKB_L]               = 0x26;
    scancodes[AMIKB_SEMICOLON]       = 0x27;
    scancodes[AMIKB_QUOTE]           = 0x28;
    scancodes[AMIKB_BACKQUOTE]       = 0x29;
    scancodes[AMIKB_LSHIFT]          = 0x2A;
    scancodes[AMIKB_BACKSLASH]       = 0x2B;
    scancodes[AMIKB_Z]               = 0x2C;
    scancodes[AMIKB_X]               = 0x2D;
    scancodes[AMIKB_C]               = 0x2E;
    scancodes[AMIKB_V]               = 0x2F;
    scancodes[AMIKB_B]               = 0x30;
    scancodes[AMIKB_N]               = 0x31;
    scancodes[AMIKB_M]               = 0x32;
    scancodes[AMIKB_COMMA]           = 0x33;
    scancodes[AMIKB_PERIOD]          = 0x34;
    scancodes[AMIKB_SLASH]           = 0x35;
    scancodes[AMIKB_RSHIFT]          = 0x36;
    scancodes[AMIKB_LALT]            = 0x38;
    scancodes[AMIKB_SPACE]           = 0x39;
    scancodes[AMIKB_CAPSLOCK]        = 0x3A;
    scancodes[AMIKB_F1]              = 0x3B;
    scancodes[AMIKB_F2]              = 0x3C;
    scancodes[AMIKB_F3]              = 0x3D;
    scancodes[AMIKB_F4]              = 0x3E;
    scancodes[AMIKB_F5]              = 0x3F;
    scancodes[AMIKB_F6]              = 0x40;
    scancodes[AMIKB_F7]              = 0x41;
    scancodes[AMIKB_F8]              = 0x42;
    scancodes[AMIKB_F9]              = 0x43;
    scancodes[AMIKB_F10]             = 0x44;
    scancodes[AMIKB_NUMLOCK]         = 0x45;
    scancodes[AMIKB_SCROLLOCK]       = 0x46;
    scancodes[AMIKB_KP8]             = 0x48;
    scancodes[AMIKB_KP_MINUS]        = 0x4A;
    scancodes[AMIKB_KP4]             = 0x4B;
    scancodes[AMIKB_KP5]             = 0x4C;
    scancodes[AMIKB_KP6]             = 0x4D;
    scancodes[AMIKB_KP_PLUS]         = 0x4E;
    scancodes[AMIKB_KP2]             = 0x50;

    scancodes[AMIKB_KP_ENTER]        = 0xE01C;
    scancodes[AMIKB_RCTRL]           = 0xE01D;
    scancodes[AMIKB_KP_DIVIDE]       = 0xE035;
    scancodes[AMIKB_PRINT]           = 0xE037; /* SBF - technically incorrect */
    scancodes[AMIKB_RALT]            = 0xE038;
    scancodes[AMIKB_HOME]            = 0xE047;
    scancodes[AMIKB_UP]              = 0xE048;
    scancodes[AMIKB_PAGEUP]          = 0xE049;
    scancodes[AMIKB_LEFT]            = 0xE04B;
    scancodes[AMIKB_RIGHT]           = 0xE04D;
    scancodes[AMIKB_END]             = 0xE04F;
    scancodes[AMIKB_DOWN]            = 0xE050;
    scancodes[AMIKB_PAGEDOWN]        = 0xE051;
    scancodes[AMIKB_INSERT]          = 0xE052;
    scancodes[AMIKB_DELETE]          = 0xE053;

    if(!AMIGA_Init())
	exit(1);
    
} /* _platform_init */


int setvesa(long x, long y)
{
    fprintf(stderr, "setvesa() called in AMIGA driver!\n");
    exit(23);
    return(0);
} /* setvesa */


int screencapture(char *filename, char inverseit)
{
    fprintf(stderr, "screencapture() is a stub in the AMIGA driver.\n");
    return(0);
} /* screencapture */


void setvmode(int mode)
{
    int w = -1;
    int h = -1;

    if (mode == 0x3)  /* text mode. */
    {
	AMIGA_CloseDisplay();
	return;
    } /* if */

    if (mode == 0x13)
    {
	w = 800;
	h = 600;
    } /* if */

    else
    {
	fprintf(stderr, "setvmode(0x%x) is unsupported in AMIGA driver.\n", mode);
	exit(13);
    } /* if */

    assert(w > 0);
    assert(h > 0);

    go_to_new_vid_mode(-1, w, h);
} /* setvmode */


int _setgamemode(char davidoption, long daxdim, long daydim)
{
    if (daxdim > MAXXDIM || daydim > MAXYDIM)
    {
	    daxdim = 1600;
	    daydim = 1200;
    }

    if (in_egapalette)
	restore256_palette();       

    go_to_new_vid_mode((int) davidoption, daxdim, daydim);

    qsetmode = 200;
    last_render_ticks = AMIGA_GetTicks();
    return(0);
} /* setgamemode */


void qsetmode640350(void)
{
    assert(0);

    go_to_new_vid_mode(-1, 640, 350);
} /* qsetmode640350 */


void qsetmode640480(void)
{
    if (!in_egapalette)
	set16color_palette();

    go_to_new_vid_mode(-1, 640, 480);
    pageoffset = 0;     /* Make sure it goes to the right place - DDOI */
    fillscreen16(0L,8L,640L*144L);
} /* qsetmode640480 */



static void add_vesa_mode(const char *typestr, int w, int h)
{
//    printf("Adding %s resolution (%dx%d).\n", typestr, w, h);
    validmode[validmodecnt] = validmodecnt;
    validmodexdim[validmodecnt] = w;
    validmodeydim[validmodecnt] = h;
    validmodecnt++;
} /* add_vesa_mode */


static void remove_vesa_mode(int index, const char *reason)
{
    int i;

    assert(index < validmodecnt);
//    printf("Removing resolution #%d, %dx%d [%s].\n",
//              index, validmodexdim[index], validmodeydim[index], reason);

    for (i = index; i < validmodecnt - 1; i++)
    {
	validmode[i] = validmode[i + 1];
	validmodexdim[i] = validmodexdim[i + 1];
	validmodeydim[i] = validmodeydim[i + 1];
    } /* for */

    validmodecnt--;
} /* remove_vesa_mode */


static __inline void cull_large_vesa_modes(void)
{
    long max_w = DEFAULT_MAXRESWIDTH;
    long max_h = DEFAULT_MAXRESHEIGHT;
    int i;

    for (i = 0; i < validmodecnt; i++)
    {
	if ((validmodexdim[i] > max_w) || (validmodeydim[i] > max_h))
	{
	    remove_vesa_mode(i, "above resolution ceiling");
	    i--;  /* list shrinks. */
	} /* if */
    } /* for */
} /* cull_large_vesa_modes */


static __inline void cull_duplicate_vesa_modes(void)
{
    int i;
    int j;

    for (i = 0; i < validmodecnt; i++)
    {
	for (j = i + 1; j < validmodecnt; j++)
	{
	    if ( (validmodexdim[i] == validmodexdim[j]) &&
		 (validmodeydim[i] == validmodeydim[j]) )
	    {
		remove_vesa_mode(j, "duplicate");
		j--;  /* list shrinks. */
	    } /* if */
	} /* for */
    } /* for */
} /* cull_duplicate_vesa_modes */


#define swap_macro(tmp, x, y) { tmp = x; x = y; y = tmp; }

/* be sure to call cull_duplicate_vesa_modes() before calling this. */
static __inline void sort_vesa_modelist(void)
{
    int i;
    int sorted;
    long tmp;

    do
    {
	sorted = 1;
	for (i = 0; i < validmodecnt - 1; i++)
	{
	    if ( (validmodexdim[i] >= validmodexdim[i+1]) &&
		 (validmodeydim[i] >= validmodeydim[i+1]) )
	    {
		sorted = 0;
		swap_macro(tmp, validmode[i], validmode[i+1]);
		swap_macro(tmp, validmodexdim[i], validmodexdim[i+1]);
		swap_macro(tmp, validmodeydim[i], validmodeydim[i+1]);
	    } /* if */
	} /* for */
    } while (!sorted);
} /* sort_vesa_modelist */


static __inline void cleanup_vesa_modelist(void)
{
    cull_large_vesa_modes();
    cull_duplicate_vesa_modes();
    sort_vesa_modelist();
} /* cleanup_vesa_modelist */


static __inline void output_vesa_modelist(void)
{
    char buffer[256];
    char numbuf[20];
    int i;

    buffer[0] = '\0';

    for (i = 0; i < validmodecnt; i++)
    {
	sprintf(numbuf, " (%ldx%ld)",
		  (long) validmodexdim[i], (long) validmodeydim[i]);

	if ( (strlen(buffer) + strlen(numbuf)) >= (sizeof (buffer) - 1) )
	    strcpy(buffer + (sizeof (buffer) - 5), " ...");
	else
	    strcat(buffer, numbuf);
    } /* for */

//    printf("Final sorted modelist:%s\n", buffer);
} /* output_vesa_modelist */


void getvalidvesamodes(void)
{
    static int already_checked = 0;
    int i, nummodes;
    AMIGA_ScreenRes *modes = NULL;
    int stdres[][2] = {
			{320, 200}, {320, 240}, {640, 480}
		      };

    if (already_checked)
	return;

    already_checked = 1;
    validmodecnt = 0;
    vidoption = 1;  /* !!! tmp */

	/* fill in the standard resolutions... */
    for (i = 0; i < sizeof (stdres) / sizeof (stdres[0]); i++)
	add_vesa_mode("standard", stdres[i][0], stdres[i][1]);

	 /* Anything the hardware can specifically do is added now... */
    modes = AMIGA_GetResolutions(&nummodes);
    for (i = 0; i<nummodes; i++)
	add_vesa_mode("physical", modes[i].w, modes[i].h);

	/* get rid of dupes and bogus resolutions... */
    cleanup_vesa_modelist();

	/* print it out for debugging purposes... */
    output_vesa_modelist();
} /* getvalidvesamodes */


int VBE_setPalette(long start, long num, char *palettebuffer)
/*
 * (From Ken's docs:)
 *   Set (num) palette palette entries starting at (start)
 *   palette entries are in a 4-byte format in this order:
 *       0: Blue (0-63)
 *       1: Green (0-63)
 *       2: Red (0-63)
 *       3: Reserved
 *
 * Naturally, the bytes are in the reverse order that SDL wants them...
 *  More importantly, SDL wants the color elements in a range from 0-255,
 *  so we do a conversion.
 */
{
    AMIGA_Color fmt_swap[256];
    AMIGA_Color *sdlp = &fmt_swap[start];
    char *p = palettebuffer;
    int i;

    //assert( (start + num) <= (sizeof (fmt_swap) / sizeof (SDL_Color)) );

    for (i = 0; i < num; i++)
    {
	sdlp->b = (Uint8) ((((float) *p++) / 63.0) * 255.0);
	sdlp->g = (Uint8) ((((float) *p++) / 63.0) * 255.0);
	sdlp->r = (Uint8) ((((float) *p++) / 63.0) * 255.0);
	p++;

	sdlp++;
    } /* for */

    return(AMIGA_SetColors(fmt_swap, start, num));
} /* VBE_setPalette */


int VBE_getPalette(long start, long num, char *palettebuffer)
{
    AMIGA_Color sdlp[256];
    char *p = palettebuffer + (start * 4);
    int i;

    AMIGA_GetColors(sdlp);

    for (i = 0; i < num; i++)
    {
	*p++ = (Uint8) ((((float) sdlp[start+i].b) / 255.0) * 63.0);
	*p++ = (Uint8) ((((float) sdlp[start+i].g) / 255.0) * 63.0);
	*p++ = (Uint8) ((((float) sdlp[start+i].r) / 255.0) * 63.0);
	*p++ = 0;
    } /* for */

    return(1);
} /* VBE_getPalette */


void _uninitengine(void)
{
   AMIGA_CloseDisplay();
} /* _uninitengine */


void uninitvesa(void)
{
   AMIGA_CloseDisplay();
} /* uninitvesa */


int setupmouse(void)
{
    if (surface == NULL)
	return(0);

    mouse_x = surface->w / 2;
    mouse_y = surface->h / 2;
    mouse_relative_x = mouse_relative_y = 0;

	/*
	 * this global usually gets set by BUILD, but it's a one-shot
	 *  deal, and we may not have an SDL surface at that point. --ryan.
	 */
    moustat = 1;

    return(1);
} /* setupmouse */


void readmousexy(short *x, short *y)
{
    if (x) *x = mouse_relative_x << 2;
    if (y) *y = mouse_relative_y << 2;

    mouse_relative_x = mouse_relative_y = 0;
} /* readmousexy */


void readmousebstatus(short *bstatus)
{
    if (bstatus)
	*bstatus = mouse_buttons;

    // special wheel treatment
    if(mouse_buttons&8) mouse_buttons ^= 8;
    if(mouse_buttons&16) mouse_buttons ^= 16;

} /* readmousebstatus */


void _updateScreenRect(long x, long y, long w, long h)
{
    AMIGA_UpdateRect(screen,x, y, w, h);
} /* _updatescreenrect */


void _nextpage(void)
{
    Uint32 ticks;

    handle_events();

    
	AMIGA_PutPixels((unsigned char*)frameplace,surface->w,surface->h);

   // if(directaccess)
//	AMIGA_UnlockBackbuffer();

    AMIGA_Flip();

   // if(directaccess)
//	frameplace=AMIGA_LockBackbuffer(&bytesperline);

    ticks = AMIGA_GetTicks();
    total_render_time = (ticks - last_render_ticks);
    if (total_render_time > 1000)
    {
	total_rendered_frames = 0;
	total_render_time = 1;
	last_render_ticks = ticks;
    } /* if */
    total_rendered_frames++;
} /* _nextpage */


unsigned char readpixel(long offset)
{
    return( *((unsigned char *) offset) );
} /* readpixel */

void drawpixel(long offset, unsigned char pixel)
{
    *((unsigned char *) offset) = pixel;
} /* drawpixel */


/* !!! These are incorrect. */
void drawpixels(long offset, unsigned short pixels)
{
/*    Uint8 *surface_end;
    Uint16 *pos;

		printf("Blargh!\n");
		exit(91);

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);

    surface_end = (((Uint8 *) surface->pixels) + (surface->w * surface->h)) - 2;
    pos = (Uint16 *) (((Uint8 *) surface->pixels) + offset);
    if ((pos >= (Uint16 *) surface->pixels) && (pos < (Uint16 *) surface_end))
	*pos = pixels;

    if (SDL_MUSTLOCK(surface))
	SDL_UnlockSurface(surface);
*/
} /* drawpixels */


void drawpixelses(long offset, unsigned int pixelses)
{
/*    Uint8 *surface_end;
    Uint32 *pos;

		printf("Blargh!\n");
		exit(91);

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);

    surface_end = (((Uint8 *)surface->pixels) + (surface->w * surface->h)) - 2;
    pos = (Uint32 *) (((Uint8 *) surface->pixels) + offset);
    if ((pos >= (Uint32 *) surface->pixels) && (pos < (Uint32 *) surface_end))
	*pos = pixelses;

    if (SDL_MUSTLOCK(surface))
	SDL_UnlockSurface(surface);*/
} /* drawpixelses */


/* Fix this up The Right Way (TM) - DDOI */
void setcolor16(int col)
{
	drawpixel_color = col;
}

void drawpixel16(long offset)
{
    //drawpixel(((long) surface->pixels + offset), drawpixel_color);
} /* drawpixel16 */


void fillscreen16(long offset, long color, long blocksize)
{
/*    Uint8 *surface_end;
    Uint8 *wanted_end;
    Uint8 *pixels;

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);

    pixels = get_framebuffer();

    if (!pageoffset) { 
	    offset = offset << 3;
	    offset += 640*336;
    }

    surface_end = (pixels + (surface->w * surface->h)) - 1;
    wanted_end = (pixels + offset) + blocksize;

    if (offset < 0)
	offset = 0;

    if (wanted_end > surface_end)
	blocksize = ((unsigned long) surface_end) - ((unsigned long) pixels + offset);

    memset(pixels + offset, (int) color, blocksize);

    if (SDL_MUSTLOCK(surface))
	SDL_UnlockSurface(surface);

    _nextpage();
*/
} /* fillscreen16 */


/* Most of this line code is taken from Abrash's "Graphics Programming Blackbook".
Remember, sharing code is A Good Thing. AH */
/*static __inline void DrawHorizontalRun (char **ScreenPtr, int XAdvance, int RunLength, char Color)
{
    int i;
    char *WorkingScreenPtr = *ScreenPtr;

    for (i=0; i<RunLength; i++)
    {
	*WorkingScreenPtr = Color;
	WorkingScreenPtr += XAdvance;
    }
    WorkingScreenPtr += surface->w;
    *ScreenPtr = WorkingScreenPtr;
}

static __inline void DrawVerticalRun (char **ScreenPtr, int XAdvance, int RunLength, char Color)
{
    int i;
    char *WorkingScreenPtr = *ScreenPtr;

    for (i=0; i<RunLength; i++)
    {
	*WorkingScreenPtr = Color;
	WorkingScreenPtr += surface->w;
    }
    WorkingScreenPtr += XAdvance;
    *ScreenPtr = WorkingScreenPtr;
}
*/
void drawline16(long XStart, long YStart, long XEnd, long YEnd, char Color)
{
/*    int Temp, AdjUp, AdjDown, ErrorTerm, XAdvance, XDelta, YDelta;
    int WholeStep, InitialPixelCount, FinalPixelCount, i, RunLength;
    char *ScreenPtr;
    long dx, dy;

    if (SDL_MUSTLOCK(surface))
	SDL_LockSurface(surface);

	dx = XEnd-XStart; dy = YEnd-YStart;
	if (dx >= 0)
	{
		if ((XStart > 639) || (XEnd < 0)) return;
		if (XStart < 0) { if (dy) YStart += scale(0-XStart,dy,dx); XStart = 0; }
		if (XEnd > 639) { if (dy) YEnd += scale(639-XEnd,dy,dx); XEnd = 639; }
	}
	else
	{
		if ((XEnd > 639) || (XStart < 0)) return;
		if (XEnd < 0) { if (dy) YEnd += scale(0-XEnd,dy,dx); XEnd = 0; }
		if (XStart > 639) { if (dy) YStart += scale(639-XStart,dy,dx); XStart = 639; }
	}
	if (dy >= 0)
	{
		if ((YStart >= ydim16) || (YEnd < 0)) return;
		if (YStart < 0) { if (dx) XStart += scale(0-YStart,dx,dy); YStart = 0; }
		if (YEnd >= ydim16) { if (dx) XEnd += scale(ydim16-1-YEnd,dx,dy); YEnd = ydim16-1; }
	}
	else
	{
		if ((YEnd >= ydim16) || (YStart < 0)) return;
		if (YEnd < 0) { if (dx) XEnd += scale(0-YEnd,dx,dy); YEnd = 0; }
		if (YStart >= ydim16) { if (dx) XStart += scale(ydim16-1-YStart,dx,dy); YStart = ydim16-1; }
	}

	if (!pageoffset) { YStart += 336; YEnd += 336; }

    if (YStart > YEnd) {
	Temp = YStart;
	YStart = YEnd;
	YEnd = Temp;
	Temp = XStart;
	XStart = XEnd;
	XEnd = Temp;
    }

    ScreenPtr = (char *) (get_framebuffer()) + XStart + (surface->w * YStart);

    if ((XDelta = XEnd - XStart) < 0)
    {
	XAdvance = (-1);
	XDelta = -XDelta;
    } else {
	XAdvance = 1;
    }

    YDelta = YEnd - YStart;

    if (XDelta == 0)
    {
	for (i=0; i <= YDelta; i++)
	{
	    *ScreenPtr = Color;
	    ScreenPtr += surface->w;
	}

	UNLOCK_SURFACE_AND_RETURN;
    }
    if (YDelta == 0)
    {
	for (i=0; i <= XDelta; i++)
	{
	    *ScreenPtr = Color;
	    ScreenPtr += XAdvance;
	}
	UNLOCK_SURFACE_AND_RETURN;
    }
    if (XDelta == YDelta)
    {
	for (i=0; i <= XDelta; i++)
	{
	    *ScreenPtr = Color;
	    ScreenPtr += XAdvance + surface->w;
	}
	UNLOCK_SURFACE_AND_RETURN;
    }

    if (XDelta >= YDelta)
    {
	WholeStep = XDelta / YDelta;
	AdjUp = (XDelta % YDelta) * 2;
	AdjDown = YDelta * 2;
	ErrorTerm = (XDelta % YDelta) - (YDelta * 2);

	InitialPixelCount = (WholeStep / 2) + 1;
	FinalPixelCount = InitialPixelCount;

	if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) InitialPixelCount--;
	if ((WholeStep & 0x01) != 0) ErrorTerm += YDelta;

	DrawHorizontalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

	for (i=0; i<(YDelta-1); i++)
	{
	    RunLength = WholeStep;
	    if ((ErrorTerm += AdjUp) > 0)
	    {
		RunLength ++;
		ErrorTerm -= AdjDown;
	    }

	    DrawHorizontalRun(&ScreenPtr, XAdvance, RunLength, Color);
	 }

	 DrawHorizontalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);

	 UNLOCK_SURFACE_AND_RETURN;
    } else {
	WholeStep = YDelta / XDelta;
	AdjUp = (YDelta % XDelta) * 2;
	AdjDown = XDelta * 2;
	ErrorTerm = (YDelta % XDelta) - (XDelta * 2);
	InitialPixelCount = (WholeStep / 2) + 1;
	FinalPixelCount = InitialPixelCount;

	if ((AdjUp == 0) && ((WholeStep & 0x01) == 0)) InitialPixelCount --;
	if ((WholeStep & 0x01) != 0) ErrorTerm += XDelta;

	DrawVerticalRun(&ScreenPtr, XAdvance, InitialPixelCount, Color);

	for (i=0; i<(XDelta-1); i++)
	{
	    RunLength = WholeStep;
	    if ((ErrorTerm += AdjUp) > 0)
	    {
		RunLength ++;
		ErrorTerm -= AdjDown;
	    }

	    DrawVerticalRun(&ScreenPtr, XAdvance, RunLength, Color);
	}

	DrawVerticalRun(&ScreenPtr, XAdvance, FinalPixelCount, Color);
	UNLOCK_SURFACE_AND_RETURN;
     }*/
} /* drawline16 */


void clear2dscreen(void)
{
    AMIGA_Rect rect;

    rect.x = rect.y = 0;
    rect.w = surface->w;

	if (qsetmode == 350)
	rect.h = 350;
	else if (qsetmode == 480)
	{
		if (ydim16 <= 336)
	    rect.h = 336;
	else
	    rect.h = 480;
	} /* else if */

    AMIGA_FillRect(&rect, 0);
} /* clear2dscreen */


void _idle(void)
{
    if (surface != NULL)
	handle_events();
   // AMIGA_Delay(1);
} /* _idle */

void *_getVideoBase(void)
{
    if(directaccess)
	return screen;
    else
	return((void *) surface->pixels);
}

void setactivepage(long dapagenum)
{
	/* !!! Is this really still needed? - DDOI */
    /*fprintf(stderr, "%s, line %d; setactivepage(): STUB.\n", __FILE__, __LINE__);*/
} /* setactivepage */

void limitrate(void)
{
    /* this is a no-op in SDL. It was for buggy VGA cards in DOS. */
} /* limitrate */

static unsigned int __interrupt _timer_catcher(void *bleh, void *bluh)
{
    timerhandler();
    return 0;
} /* _timer_catcher */

void inittimer(void)
{
    AMIGA_StartTimerInt(1000 / PLATFORM_TIMER_HZ, (APTR)_timer_catcher);
}

void uninittimer(void)
{
    AMIGA_EndTimerInt();
}

void initkeys(void)
{
    /* does nothing in SDL. Key input handling is set up elsewhere. */
    /* !!! why not here? */
}

void uninitkeys(void)
{
    /* does nothing in SDL. Key input handling is set up elsewhere. */
}

void set16color_palette(void)
{
	/* Backup old palette */
	/*VBE_getPalette (0, 16, (char *)&backup_palette);*/
	memcpy (&backup_palette, &palette, 16*3);

	/* Set new palette */
	/*VBE_setPalette (0, 16, (char *)&egapalette);*/
	memcpy (&palette, &egapalette, 16*3);
	in_egapalette = 1;
} /* set16color_palette */

void restore256_palette(void)
{
	/*VBE_setPalette (0, 16, (char *)&backup_palette);*/
	memcpy (&palette, &backup_palette, 16*3);
	in_egapalette = 0;
} /* restore256_palette */

unsigned long getticks(void)
{
    return (AMIGA_GetTicks());
} /* getticks */

/* end of amiga_driver.c ... */

