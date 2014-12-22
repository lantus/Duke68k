typedef struct
{
    WORD primary_kbassigns[52];
    WORD secondary_kbassigns[52];
}KeyboardData;

typedef struct
{
    WORD    sc_l, sc_m, sc_r;
    WORD    dc_l, dc_m, dc_r;
    WORD    sensi;
    BOOL    flipped;
    BYTE    aimtype;
    BYTE    pad[3];
}MouseData;

typedef struct
{
    char    mode_name[64];
    ULONG   modeid,width,height;
    BOOL    dblbuffer,direct,cgfx;
    BYTE    pad[2];
}ScreenData;

typedef struct
{
    char    ahi_name[128];
    ULONG   ahi_id, ahi_bits, freq;
    LONG    sfxvolume, musicvolume;
    BOOL    music,stereo,rvstereo;
    BYTE    bits,numvoices;
}SoundData;

typedef struct
{
    SoundData sounddata;
    ScreenData screendata;
    MouseData mousedata;
    KeyboardData kbdata;
}SetupData;
