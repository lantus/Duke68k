#define SETUPVERSION    2

#include <stdio.h>
#include <stdlib.h>
#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <intuition/pointerclass.h>

#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/lowlevel.h>
#include <proto/dos.h>
#include <proto/timer.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <devices/timer.h>

#include "amiga_display.h"
#include "amigasetupstructs.h"

#include "amiga_mmu.h"

#define REG(xn, parm) parm __asm(#xn)
#define REGARGS __regargs
#define STDARGS __stdargs
#define SAVEDS __saveds
#define ALIGNED __attribute__ ((aligned(4))
#define FAR
#define CHIP
#define INLINE __inline__

extern void REGARGS c2p1x1_8_c5_bm_040(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

extern void REGARGS c2p1x1_6_c5_bm_040(
REG(d0, UWORD chunky_x),
REG(d1, UWORD chunky_y),
REG(d2, UWORD offset_x),
REG(d3, UWORD offset_y),
REG(a0, UBYTE *chunky_buffer),
REG(a1, struct BitMap *bitmap));

extern void mmu_stuff2(void);
extern void mmu_stuff2_cleanup(void);
extern char *screen;

int mmu_chunky = 0;
int mmu_active = 0;

struct ExecBase *SysBase;
struct Library *CyberGfxBase=0;
struct GfxBase *GfxBase=0;
struct DosBase *DosBase=0;
struct IntuitionBase *IntuitionBase=0;
struct Library *LowLevelBase=0;


AMIGA_ScreenRes *ScreenRes;
AMIGA_Surface theSurface;

SetupData setupdata;
BOOL amigasetup;

unsigned int displayID,bmattr,realdepth;
BOOL dblbuf,directaccess,cgfx;

struct Screen *theScreen = NULL;
struct RastPort *theRastPort = NULL;
struct RastPort *theTmpRastPort = NULL;
struct BitMap *theBitMap = NULL;
struct BitMap *theTmpBitMap = NULL;
struct Window *theWindow = NULL;
struct BitMap pointer_bitmap;
void *MousePointer = NULL;
struct ScreenBuffer *scbuf[2];
struct RastPort rport[2];
int workbuf;

APTR lockhandle;
BOOL fullscreen;

static struct EClockVal timeval;

static int scr_w, scr_h;
static int timecount;
static int *timerinthandle;


/*****************************************************************************
    opens needed libraries
 ****************************************************************************/
BOOL AMIGA_Init(void)
{
    FILE *fp;
    LONG setupversion;

    if(!CyberGfxBase) CyberGfxBase = (struct Library *) OpenLibrary((UBYTE *)"cybergraphics.library",41L);
    if(!GfxBase) GfxBase = (struct GfxBase *) OpenLibrary((UBYTE *)"graphics.library", 39L);
    if(!DosBase) DosBase = (struct DosBase *) OpenLibrary((UBYTE *)"dos.library", 39L);
    if(!IntuitionBase) IntuitionBase = (struct IntuitionBase *) OpenLibrary((UBYTE *)"intuition.library", 39L);
    if(!LowLevelBase) LowLevelBase = (struct Library *) OpenLibrary((UBYTE *)"lowlevel.library", 40L);

    if( !GfxBase || !IntuitionBase || !DosBase )
    {
	printf("amiga_display: Couldn't open all needed libraries !!!\n");
	return FALSE;
    }

    amigasetup=FALSE;
    if((fp=fopen("amigaduke.prefs","rb")))
    {
	fread(&setupversion,4,1,fp);
	if(setupversion==SETUPVERSION)
	{
	    fread(&setupdata,sizeof(SetupData),1,fp);
	    fclose(fp);
	    amigasetup=TRUE;

	    displayID=setupdata.screendata.modeid;
	    dblbuf=setupdata.screendata.dblbuffer;
	    directaccess=setupdata.screendata.direct;
	    cgfx=setupdata.screendata.cgfx;
	}
    }

    if(!amigasetup)
    {
	displayID=INVALID_ID;
	if(CyberGfxBase)
	    cgfx=TRUE;
	else
	    cgfx=FALSE;

	dblbuf=TRUE;
	directaccess=FALSE;
    }

    ScreenRes=NULL;
    timeval.ev_hi=0;
    timeval.ev_lo=0;
    timecount=0;
    timerinthandle=NULL;
    lockhandle=NULL;

    return TRUE;
}

/*****************************************************************************
    closes all libraries
 ****************************************************************************/
void AMIGA_Quit(void)
{
    AMIGA_CloseDisplay();

    if (LowLevelBase) CloseLibrary( (struct Library *)LowLevelBase ), LowLevelBase=NULL;
    if (IntuitionBase) CloseLibrary( (struct Library *)IntuitionBase ), IntuitionBase=NULL;
    if (DosBase) CloseLibrary(( struct Library *)DosBase), DosBase=NULL;
    if (GfxBase) CloseLibrary(( struct Library *)GfxBase), GfxBase=NULL;
    if (CyberGfxBase) CloseLibrary(( struct Library *)CyberGfxBase),CyberGfxBase=0;
}

/*****************************************************************************
    set video mode
 ****************************************************************************/
AMIGA_Surface* AMIGA_SetVideoMode(int w, int h, BOOL f, char *title)
{
    if(!amigasetup)
    {
	if(CyberGfxBase)
	{
	    displayID=BestCModeIDTags(CYBRBIDTG_NominalWidth, w,
				      CYBRBIDTG_NominalHeight, h,
				      CYBRBIDTG_Depth,8,
				      TAG_DONE );

	    cgfx=TRUE;
	    dblbuf=TRUE;
	    directaccess=FALSE;

	}
	else
	{
	    displayID=BestModeID( BIDTAG_NominalWidth,w,
				  BIDTAG_NominalHeight,h,
				  BIDTAG_DesiredWidth,w,
				  BIDTAG_DesiredHeight,h,
				  BIDTAG_Depth, 8,
				  BIDTAG_DIPFMustHave,DIPF_IS_SPRITES | DIPF_IS_GENLOCK,
				  TAG_DONE);

	    cgfx=FALSE;
	    dblbuf=TRUE;
	    directaccess=FALSE;
	}
    }

    if(cgfx)
	realdepth=GetCyberIDAttr(CYBRIDATTR_DEPTH, displayID);
    else
	realdepth=8;

    if(displayID==INVALID_ID)
    {
	printf("AMIGA Display: invalid modeid found, abort!\n");
	return NULL;
    }

    scr_w=w;
    if(cgfx && dblbuf)
	scr_h=h*2;
    else
	scr_h=h;

    if (!(theScreen = OpenScreenTags( NULL,
				     SA_DisplayID, displayID,
				     SA_Width, 320,
				     SA_Height,200,
				     SA_Depth, 8,
				     SA_Top,0,
				     SA_Left,0,
				     SA_Title,(int)title,
				     SA_DetailPen,0,
				     SA_BlockPen,0,
				     SA_Font,0,
				     SA_ShowTitle, 0,
				     SA_Behind,0,
				     SA_Quiet,0,
				     SA_Type,CUSTOMSCREEN,
				     SA_AutoScroll,0,
				     SA_SysFont,0,
				     SA_Draggable,0,
				     SA_Interleaved,0,
				     TAG_DONE )))
    {
	printf("AMIGA Display: couldn't open screen, abort!\n");
	return NULL;
    }


  //  mmu_chunky = mmu_mark(screen,(320 * 200 + 4095) & (~0xFFF),CM_WRITETHROUGH,SysBase);
  //  mmu_active = 1;
 
    InitBitMap(&pointer_bitmap,0,0,0);
    MousePointer=(void*)NewObject(NULL,"pointerclass",POINTERA_BitMap,(int)&pointer_bitmap,TAG_DONE);

    if (!( theWindow = OpenWindowTags(NULL,
				     WA_Width, scr_w,
				     WA_Height, scr_h,
				     WA_Borderless, TRUE,
				     WA_Backdrop, TRUE,
				     WA_Activate, TRUE,
				     WA_RMBTrap, TRUE,
				     WA_ReportMouse, TRUE,
				     WA_Pointer,(int)MousePointer,
				     WA_IDCMP, RAWKEY | MOUSEMOVE | DELTAMOVE | MOUSEBUTTONS,
				     WA_CustomScreen, (int)theScreen,
				     TAG_DONE ) ))
    {
	printf("AMIGA Display: couldn't open window, abort!\n");
	return NULL;
    }
    theRastPort=theWindow->RPort;
    theBitMap=theRastPort->BitMap;



    		

    if(!cgfx)
    {
	scbuf[0]=AllocScreenBuffer( theScreen, NULL, SB_SCREEN_BITMAP );
	InitRastPort( &rport[ 0 ] );
	rport[ 0 ].BitMap = scbuf[ 0 ]->sb_BitMap;
	workbuf=0;

	if(dblbuf)
	{
	    scbuf[1]=AllocScreenBuffer( theScreen, NULL, SB_COPY_BITMAP );
	    InitRastPort( &rport[ 1 ] );
	    rport[ 1 ].BitMap = scbuf[ 1 ]->sb_BitMap;
	    workbuf=1;
	}
    }

    fullscreen=TRUE;

    /* alloc tmp rastport for wpa8 */
    if(!directaccess)
    {
	theTmpRastPort=(struct RastPort *)AllocVec(sizeof(struct RastPort),MEMF_CLEAR);
	CopyMem(theRastPort,theTmpRastPort,sizeof(struct RastPort));
	theTmpRastPort->Layer=NULL;
	theTmpBitMap=AllocBitMap(scr_w,1,8,NULL,theBitMap);
	theTmpRastPort->BitMap=theTmpBitMap;
    }

    theSurface.w=w;
    theSurface.h=h;
    theSurface.d=8;
    theSurface.pixels=0;
    theSurface.pitch=w;

    if(directaccess)
    {
	theSurface.pixels=AMIGA_LockBackbuffer(&theSurface.pitch);
	AMIGA_UnlockBackbuffer();
    }

    
    ElapsedTime(&timeval);

    return &theSurface;
}

/*****************************************************************************
    close display
 ****************************************************************************/
void AMIGA_CloseDisplay(void)
{
    if( lockhandle )
	AMIGA_UnlockBackbuffer();

    if(!directaccess)
    {
	if(theTmpBitMap)
	{
	    FreeBitMap(theTmpBitMap);
	    theTmpBitMap=NULL;
	}

	if(theTmpRastPort)
	{
	    FreeVec(theTmpRastPort);
	    theTmpRastPort=NULL;
	}
    }

    if(theWindow)
    {
	CloseWindow( theWindow );
	theWindow=NULL;
    }

    if(MousePointer)
    {
	DisposeObject(MousePointer);
	MousePointer=NULL;
    }

    if(!cgfx)
    {
	if(dblbuf)
	{
	    if( scbuf[1] )
	    {
		FreeScreenBuffer( theScreen, scbuf[1] );
		scbuf[1]=NULL;
	    }
	}

	if( scbuf[0] )
	{
	    FreeScreenBuffer( theScreen, scbuf[0] );
	    scbuf[0]=NULL;
	}
    }

    if(fullscreen && theScreen)
    {
	CloseScreen( theScreen );
	theScreen=NULL;
	fullscreen=FALSE;
    }

    if(ScreenRes)
    {
	free(ScreenRes);
	ScreenRes=NULL;
    }
}

/*****************************************************************************
    poll events
 ****************************************************************************/
BOOL AMIGA_PollEvent(AMIGA_Event *event)
{
    struct IntuiMessage *msg;
    unsigned int iclass;
    unsigned short code;

    memset(event,0,sizeof(AMIGA_Event));
    while((msg = (struct IntuiMessage *)GetMsg(theWindow->UserPort)) != NULL)
    {
	// Extract data and reply message
	iclass = msg->Class;
	code = msg->Code;
	ReplyMsg((struct Message *)msg);

	// Action depends on message class
	switch (iclass)
	{
	    case IDCMP_RAWKEY:
		if( code & IECODE_UP_PREFIX )
		{
		    code &= 0x7f;
		    event->keycode=code;
		    event->keyup=TRUE;
		    event->type=AMIGA_KEYUP;
		    return TRUE;
		}
		else
		{
		    event->keycode=code;
		    event->keydown=TRUE;
		    event->type=AMIGA_KEYDOWN;
		    return TRUE;
		}
	    break;

	    case IDCMP_MOUSEBUTTONS:
		switch( code )
		{
		    case SELECTDOWN:
			event->lbutton=TRUE;
			event->type=AMIGA_MOUSEBUTTONDOWN;
			return TRUE;
		    break;

		    case MIDDLEDOWN:
			event->mbutton=TRUE;
			event->type=AMIGA_MOUSEBUTTONDOWN;
			return TRUE;
		    break;

		    case MENUDOWN:
			event->rbutton=TRUE;
			event->type=AMIGA_MOUSEBUTTONDOWN;
			return TRUE;
		    break;

		    case SELECTUP:
			event->lbutton=TRUE;
			event->type=AMIGA_MOUSEBUTTONUP;
			return TRUE;
		    break;

		    case MIDDLEUP:
			event->mbutton=TRUE;
			event->type=AMIGA_MOUSEBUTTONUP;
			return TRUE;
		    break;

		    case MENUUP:
			event->rbutton=TRUE;
			event->type=AMIGA_MOUSEBUTTONUP;
			return TRUE;
		    break;

		}
	    break;

	    case IDCMP_MOUSEMOVE:
		event->mousex=msg->MouseX;
		event->mousey=msg->MouseY;
		event->type=AMIGA_MOUSEMOTION;
		return TRUE;
	    break;

	    case IDCMP_CLOSEWINDOW:
		event->type=AMIGA_QUIT;
		return TRUE;
	    break;
	}
    }
    return FALSE;
}

/*****************************************************************************
    get resolutions
 ****************************************************************************/
AMIGA_ScreenRes* AMIGA_GetResolutions(int *nummodes)
{
    ULONG id=NextDisplayInfo(INVALID_ID);
    int numm=0;
    int w,h;

    if(ScreenRes)
    {
	free(ScreenRes);
	ScreenRes=NULL;
    }

    do
    {
	if(CyberGfxBase)
	{
	    if(IsCyberModeID(id))
	    {
		if(GetCyberIDAttr(CYBRIDATTR_DEPTH,id)==8)
		{
		    w=GetCyberIDAttr(CYBRIDATTR_WIDTH,id);
		    h=GetCyberIDAttr(CYBRIDATTR_HEIGHT,id);
		    if(w>=320 && h>=200)
		    {
			ScreenRes=(AMIGA_ScreenRes*)realloc(ScreenRes,(numm+1)*sizeof(AMIGA_ScreenRes));
			ScreenRes[numm].w=w;
			ScreenRes[numm].h=h;
			numm++;
		    }
		}
	    }
	}
	else
	{
	    unsigned char buf[128];
	    unsigned char buf2[128];
	    struct DimensionInfo *di=(struct DimensionInfo *)buf;
	    struct DisplayInfo *disi=(struct DisplayInfo *)buf2;

	    if( id>=NTSC_MONITOR_ID && id<0x000b0000 )
	    {
		GetDisplayInfoData(NULL,buf2,128,DTAG_DISP,id);
		if( !(disi->PropertyFlags & DIPF_IS_DUALPF) &&
		    !(disi->PropertyFlags & DIPF_IS_HAM))
		{
		    GetDisplayInfoData(NULL,buf,128,DTAG_DIMS,id);
		    if(di->MaxDepth==8)
		    {
			w=di->Nominal.MaxX - di->Nominal.MinX + 1;
			h=di->Nominal.MaxY - di->Nominal.MinY + 1;
			if(w>=320 && h>=200)
			{
			    ScreenRes=(AMIGA_ScreenRes*)realloc(ScreenRes,(numm+1)*sizeof(AMIGA_ScreenRes));
			    ScreenRes[numm].w=w;
			    ScreenRes[numm].h=h;
			    numm++;
			}
		     }
		 }
	    }
	}
	id=NextDisplayInfo(id);
    }while(id!=INVALID_ID);

    *nummodes = numm;
    return ScreenRes;
}

/*****************************************************************************
    set colors
 ****************************************************************************/
BOOL AMIGA_SetColors(AMIGA_Color *cols, int start, int num)
{
    int i;
    unsigned int colortable[780];

    colortable[0] = (num<<16)+start;
    for( i=0; i<num; i++ )
    {
	colortable[i*3+1]=cols[i].r<<24;
	colortable[i*3+2]=cols[i].g<<24;
	colortable[i*3+3]=cols[i].b<<24;
    }
    colortable[num*3+1]=0;

    LoadRGB32( &theScreen->ViewPort, (ULONG*)colortable );

    return TRUE;
}

/*****************************************************************************
    get colors
 ****************************************************************************/
void AMIGA_GetColors(AMIGA_Color *cols)
{
    int i;
    unsigned int colortable[768];

    GetRGB32( theScreen->ViewPort.ColorMap,0,256,(ULONG*)colortable);

    for( i=0; i<256; i++ )
    {
	cols[i].r = colortable[i*3]>>24;
	cols[i].g = colortable[i*3+1]>>24;
	cols[i].b = colortable[i*3+2]>>24;
    }
}

/*****************************************************************************
    update rect
 ****************************************************************************/
void AMIGA_UpdateRect(unsigned char *src, int x, int y, int w, int h)
{
/*    if(cgfx)
    {
	if(directaccess)
	{
	   BltBitMapRastPort( theBitMap, x,scr_h/2+y, theRastPort,
			      x,y, w-1, h-1,
			      ABNC |ABC );
	}
	else
	{
	    long yoff=0;

	    if(dblbuf)
		yoff=scr_h/2;

	    WritePixelArray8(theRastPort,x,yoff+y,w-1,yoff+h-1,src,theTmpRastPort);
	}
    }
    else
	WritePixelArray8(&rport[workbuf],x,y,w-1,h-1,src,theTmpRastPort);
*/
}

/*****************************************************************************
    put pixels
 ****************************************************************************/
void AMIGA_PutPixels(unsigned char *src, int w, int h)
{
    if(!directaccess)
    {
	if(cgfx)
	{
	    long yoff=0;

	    if(dblbuf)
		yoff=scr_h/2;

	    WritePixelArray8(theRastPort,0,yoff,w-1,yoff+h-1,src,theTmpRastPort);
	}
	else
	    //WritePixelArray8(&rport[workbuf],0,0,w-1,h-1,src,theTmpRastPort);
	    
	    c2p1x1_8_c5_bm_040(320,200,0,0,src,scbuf[workbuf]->sb_BitMap);     
    }
}

/*****************************************************************************
    Flip Buffers
 ****************************************************************************/
void AMIGA_Flip(void)
{
    if(cgfx)
    {
	if(directaccess)
	{
	    BltBitMapRastPort( theBitMap, 0,scr_h/2, theRastPort,
			       0,0, scr_w, scr_h/2-1,
			       ABNC |ABC );
	}
	else if(dblbuf)
	{
	    ScrollRaster(theRastPort,0,scr_h/2,0,0,scr_w,scr_h);
	}
    }
    else
    {
	if(dblbuf)
	{
	    ChangeScreenBuffer( theScreen, scbuf[workbuf]);
	    workbuf^=1;
	}
    }
}

/*****************************************************************************
    get ticks
 ****************************************************************************/
unsigned int AMIGA_GetTicks(void)
{
    unsigned int res;
    float dummy;

    dummy=(float)ElapsedTime(&timeval);
    res=(unsigned int)(dummy*1000.0f/65536.0f);
    timecount+=res;
    return timecount;
}

/*****************************************************************************
    fill rect
 ****************************************************************************/
void AMIGA_FillRect(AMIGA_Rect *rect,unsigned char color)
{
    //SetAPen(theRastPort,color);
    //RectFill(theRastPort,rect->x,rect->y,rect->x+rect->w-1,rect->y+rect->h-1);
}

/*****************************************************************************
    lock backbuffer
 ****************************************************************************/
void* AMIGA_LockBackbuffer(long *bpl)
{
    if(cgfx)
    {
	unsigned char *ptr;
	int bpr,height;

	lockhandle=LockBitMapTags( theBitMap,
				   LBMI_BYTESPERROW,(int)bpl,
				   LBMI_HEIGHT,(int)&height,
				   LBMI_BASEADDRESS,(int)&ptr,
				   TAG_DONE);


	bpr=*bpl;
	ptr += (height/2)*bpr;

	return (void*)ptr;
    }
    else
	return NULL;
}

/*****************************************************************************
    unlock backbuffer
 ****************************************************************************/
void AMIGA_UnlockBackbuffer(void)
{
    if(cgfx)
	UnLockBitMap(lockhandle);

    lockhandle=NULL;
}

/*****************************************************************************
    start timer interrupt
 ****************************************************************************/

#ifdef __MORPHOS__

#include <emul/emulinterface.h>

static int timerint_stub(void)
{
	int (*func)(void) = (int (*)(void)) REG_A1;
	return (*func)();
}

struct EmulLibEntry timerint_GATE = { TRAP_LIBNR, 0, (void (*)(void)) timerint_stub };

void AMIGA_StartTimerInt(unsigned int intervall, APTR timercallback)
{
    timerinthandle=AddTimerInt(&timerint_GATE, (APTR) timercallback);
    if(timerinthandle)
    {
		StartTimerInt(timerinthandle,intervall*1000,TRUE);
    }
}

#else

void AMIGA_StartTimerInt(unsigned int intervall, APTR timercallback)
{
    timerinthandle=AddTimerInt((APTR)timercallback, NULL);
    if(timerinthandle)
    {
	StartTimerInt(timerinthandle,intervall*1000,TRUE);
    }
}

#endif

/*****************************************************************************
    end timer interrupt
 ****************************************************************************/
void AMIGA_EndTimerInt(void)
{
    if(timerinthandle)
    {
	StopTimerInt(timerinthandle);
	RemTimerInt(timerinthandle);
	timerinthandle=NULL;
    }
}

/*****************************************************************************
    Delay
 ****************************************************************************/
void AMIGA_Delay( int ticks )
{
    Delay(ticks);
}


