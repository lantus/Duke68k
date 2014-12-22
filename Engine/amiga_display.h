#define AMIGA_KEYUP             0x00000001
#define AMIGA_KEYDOWN           0x00000002
#define AMIGA_MOUSEMOTION       0x00000004
#define AMIGA_MOUSEBUTTONUP     0x00000008
#define AMIGA_MOUSEBUTTONDOWN   0x00000010

#define AMIGA_QUIT              0x80000000


/* keycodes */
#define AMIKB_1            0x01
#define AMIKB_2            0x02
#define AMIKB_3            0x03
#define AMIKB_4            0x04
#define AMIKB_5            0x05
#define AMIKB_6            0x06
#define AMIKB_7            0x07
#define AMIKB_8            0x08
#define AMIKB_9            0x09
#define AMIKB_0            0x0A
#define AMIKB_A            0x20
#define AMIKB_B            0x35
#define AMIKB_C            0x33
#define AMIKB_D            0x22
#define AMIKB_E            0x12
#define AMIKB_F            0x23
#define AMIKB_G            0x24
#define AMIKB_H            0x25
#define AMIKB_I            0x17
#define AMIKB_J            0x26
#define AMIKB_K            0x27
#define AMIKB_L            0x28
#define AMIKB_M            0x37
#define AMIKB_N            0x36
#define AMIKB_O            0x18
#define AMIKB_P            0x19
#define AMIKB_Q            0x10
#define AMIKB_R            0x13
#define AMIKB_S            0x21
#define AMIKB_T            0x14
#define AMIKB_U            0x16
#define AMIKB_V            0x34
#define AMIKB_W            0x11
#define AMIKB_X            0x32
#define AMIKB_Y            0x31
#define AMIKB_Z            0x15
#define AMIKB_F1           0x50
#define AMIKB_F2           0x51
#define AMIKB_F3           0x52
#define AMIKB_F4           0x53
#define AMIKB_F5           0x54
#define AMIKB_F6           0x55
#define AMIKB_F7           0x56
#define AMIKB_F8           0x57
#define AMIKB_F9           0x58
#define AMIKB_F10          0x59
#define AMIKB_ESCAPE       0x45
#define AMIKB_BACKSPACE    0x41
#define AMIKB_TAB          0x42
#define AMIKB_RETURN       0x44
#define AMIKB_LCTRL         0x63
#define AMIKB_RCTRL         0x67
#define AMIKB_LSHIFT        0x60
#define AMIKB_RSHIFT        0x61
#define AMIKB_LALT          0x64
#define AMIKB_RALT          0x65
#define AMIKB_SPACE        0x40
#define AMIKB_CAPSLOCK      0x62
#define AMIKB_HOME         0x3D
#define AMIKB_END          0x1D
#define AMIKB_UP           0x4C
#define AMIKB_DOWN         0x4D
#define AMIKB_RIGHT        0x4E
#define AMIKB_LEFT         0x4F
#define AMIKB_INSERT       0x0F
#define AMIKB_DEL          0x46
#define AMIKB_DELETE       0x3c
#define AMIKB_MINUS        0x0b
#define AMIKB_EQUALS       0x0C
#define AMIKB_LEFTBRACKET   0x1a
#define AMIKB_RIGHTBRACKET  0x1b
#define AMIKB_SEMICOLON     0x29
#define AMIKB_QUOTE         0x2a
#define AMIKB_BACKQUOTE     0x0
#define AMIKB_BACKSLASH     0x0d
#define AMIKB_COMMA          0x38
#define AMIKB_PERIOD         0x39
#define AMIKB_SLASH          0x3a
#define AMIKB_KP_MINUS      0x4A
#define AMIKB_KP_PLUS       0x5E
#define AMIKB_KP_ENTER      0x43
#define AMIKB_KP8           0x3e
#define AMIKB_KP4           0x2d
#define AMIKB_KP5           0x2e
#define AMIKB_KP6           0x2f
#define AMIKB_KP2           0x1e
#define AMIKB_KP_DIVIDE     0x5c
#define AMIKB_PRINT         0x5d
#define AMIKB_PAGEUP        0x3f
#define AMIKB_PAGEDOWN      0x1f
#define AMIKB_SCROLLOCK     0x5b
#define AMIKB_NUMLOCK      0x5a

/* end keycodes */




typedef struct AMIGA_Surface
{
	int w, h, d;
	unsigned long pitch;
	void *pixels;
} AMIGA_Surface;

typedef struct AMIGA_Event
{
    unsigned int type;

    unsigned char keycode;
    BOOL keyup,keydown;

    int mousex,mousey;
    BOOL lbutton,rbutton,mbutton;

} AMIGA_Event;

typedef struct AMIGA_ScreenRes
{
    int w,h;

} AMIGA_ScreenRes;

typedef struct AMIGA_Color
{
   unsigned char r,g,b;

} AMIGA_Color;

typedef struct AMIGA_Rect
{
   int x,y,w,h;

} AMIGA_Rect;


BOOL AMIGA_Init(void);
void AMIGA_Quit(void);
AMIGA_Surface* AMIGA_SetVideoMode(int w, int h, BOOL f, char *title);
void AMIGA_CloseDisplay(void);
void AMIGA_SetCaption(STRPTR title);
BOOL AMIGA_PollEvent(AMIGA_Event *event);
AMIGA_ScreenRes* AMIGA_GetResolutions(int *nummodes);
BOOL AMIGA_SetColors(AMIGA_Color *cols, int start, int num);
void AMIGA_GetColors(AMIGA_Color *cols);
void AMIGA_UpdateRect(unsigned char *src,int x, int y, int w, int h);
void AMIGA_PutPixels(unsigned char *src, int w, int h);
void AMIGA_Flip(void);
unsigned int AMIGA_GetTicks(void);
void AMIGA_FillRect(AMIGA_Rect *rect,unsigned char color);
void* AMIGA_LockBackbuffer(long *bpl);
void AMIGA_UnlockBackbuffer(void);
void AMIGA_StartTimerInt(unsigned int intervall, APTR timercallback);
void AMIGA_EndTimerInt(void);
void AMIGA_Delay( int ticks );


