/******************************************************************************/
/*                                                                            */
/*                        JALECO MEGA SYSTEM-1/1B/1C                          */
/*                        ------------------------                          */
/*   CPU: 68000                                                               */
/* SOUND: 68000 YM2151 M6295x2                                                */
/* VIDEO: 256x224 JALECO CUSTOM <3xBG0 1xSPR+CHAIN>                           */
/*                                                                            */
/*                            LEGEND OF MAKAI                                 */
/*                            ---------------                                 */
/*   CPU: 68000                                                               */
/* SOUND: Z80 YM2203                                                          */
/* VIDEO: 256x224 JALECO CUSTOM <2xBG0 1xSPR>                                 */
/*                                                                            */
/******************************************************************************/
#include "gameinc.h"
#include "megasys1.h"
#include "taitosnd.h"
#include "decode.h"
#include "sasound.h"
#include "2151intf.h"
#include "2203intf.h"
#include "adpcm.h"
#include "blit.h" // clear_game_screen

#define ROM_COUNT       23
static void (*ExecuteSoundFrame)();	// Pointer to ExecuteSoundFrame rountine (sound cpu work for 1 frame), used for pausegame + playsound

/*

 Supported romsets:

  0 - Rodland Japanese     - 1990 - MS1
  1 - Saint Dragon         - 1989 - MS1
  2 - P47 Japanese         - 1988 - MS1
  3 - Phantasm             - 1991 - MS1
  4 - Kick Off             - 1988 - MS1
  5 - Hachoo               - 1989 - MS1
  6 - Plus Alpha           - 1989 - MS1
  7 - Avenging Spirit      - 1991 - MS1B
  8 - Cybattler            - 1993 - MS1C
  9 - 64th Street American - 1991 - MS1C
 10 - Earth Defence Force  - 1991 - MS1B
 11 - Shingen              - 1988 - MS1
 12 - Legend of Makai      - 1988 - Pre MS1
 13 - Astyanax             - 1989 - MS1 [The Lord of King]
 14 - P47 American         - 1988 - MS1
 15 - Rodland American     - 1990 - MS1
 16 - Peek a Boo           - 1988 - MSx
 17 - 64th Street Japanese - 1991 - MS1C
 18 - Chimera Beast        - 1993 - MS1
 19 - The Lord of King     - 1988 - MS1
 20 - Iga Ninjyutsuden     - 1988 - MS1
 21 - Soldam               - 1990 - MS1

 Todo:

 - Sprite Mosaic <only used in Jaleco logo in P47>.
 - What is YM3014 (M6295 compatible? Rodland uses one).
 - I think there might be volume control for the M6295, instruments
   are ok volume in Rodland, but explosions are too quiet. Or maybe
   the M6295 need different volume settings?

 Changes:

 08-02-99:

 - Fixed bug in Cybattler and 64th Street sub cpu memory map.

 24-01-99:

 - Added Rodland English Version
 - Changed AddMS1SoundCPU(), so it supports games with no M6295 samples (check
   PCMROM==NULL) and also support Cybattler and 64th Street (soundread from diff
   address).

 01-2002 : lots of cleanups, automatic rom loading (at least for the code),
 added rodlandj and tshingen.
 nlx_doom fixed loads of bugs and improved dsw.

About shingen (nlx_doom) :
tshingna randomly locks up with no
apparent reason. It seems to be more frequent when we
get all the letters "sa-mu-ra-i", though.
   By the way, it will happen in both raine and mame
(bad dump??). tshingen has no problems.

*/

// By luck input port are all at the same place
#define player1_r(a) ReadWord(&RAM[0x14000])
#define player2_r(a) ReadWord(&RAM[0x14002])

static struct DIR_INFO _64th_street_dirs[] =
{
   { "64th_street", },
   { "64street", },
   { NULL, },
};

static struct ROM_INFO _64th_street_roms[] =
{
  { "64th_03.rom", 0x040000, 0xed6c6942 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "64th_02.rom", 0x040000, 0x0621ed1d , REGION_ROM1, 0x000001, LOAD_8_16 },
  // CPU 2
  { "64th_08.rom", 0x010000, 0x632be0c1 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "64th_07.rom", 0x010000, 0x13595d01 , REGION_ROM1, 0x080001, LOAD_8_16 },
   {  "64th_01.rom", 0x00080000, 0x06222f90, 0, 0, 0, },
   {  "64th_04.rom", 0x00080000, 0x98f83ef6, 0, 0, 0, },
   {  "64th_05.rom", 0x00080000, 0xa89a7020, 0, 0, 0, },
   {  "64th_06.rom", 0x00080000, 0x2bfcdc75, 0, 0, 0, },
   {  "64th_09.rom", 0x00020000, 0xa4a97db4, 0, 0, 0, },
   {  "64th_10.rom", 0x00040000, 0xa3390561, 0, 0, 0, },
   {  "64th_11.rom", 0x00020000, 0xb0b8a65c, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct INPUT_INFO megasys_1_inputs[] =
{
   { KB_DEF_COIN1,        MSG_COIN1,               0x010000, 0x40, BIT_ACTIVE_0 },
   { KB_DEF_COIN2,        MSG_COIN2,               0x010000, 0x80, BIT_ACTIVE_0 },
   { KB_DEF_SERVICE,      MSG_SERVICE,             0x010000, 0x3C, BIT_ACTIVE_0 },

   { KB_DEF_P1_START,     MSG_P1_START,            0x010000, 0x01, BIT_ACTIVE_0 },
   { KB_DEF_P1_UP,        MSG_P1_UP,               0x010002, 0x08, BIT_ACTIVE_0 },
   { KB_DEF_P1_DOWN,      MSG_P1_DOWN,             0x010002, 0x04, BIT_ACTIVE_0 },
   { KB_DEF_P1_LEFT,      MSG_P1_LEFT,             0x010002, 0x02, BIT_ACTIVE_0 },
   { KB_DEF_P1_RIGHT,     MSG_P1_RIGHT,            0x010002, 0x01, BIT_ACTIVE_0 },
   { KB_DEF_P1_B1,        MSG_P1_B1,               0x010002, 0x10, BIT_ACTIVE_0 },
   { KB_DEF_P1_B2,        MSG_P1_B2,               0x010002, 0x20, BIT_ACTIVE_0 },
   { KB_DEF_P1_B3,        MSG_P1_B3,               0x010002, 0x40, BIT_ACTIVE_0 },

   { KB_DEF_P2_START,     MSG_P2_START,            0x010000, 0x02, BIT_ACTIVE_0 },
   { KB_DEF_P2_UP,        MSG_P2_UP,               0x010004, 0x08, BIT_ACTIVE_0 },
   { KB_DEF_P2_DOWN,      MSG_P2_DOWN,             0x010004, 0x04, BIT_ACTIVE_0 },
   { KB_DEF_P2_LEFT,      MSG_P2_LEFT,             0x010004, 0x02, BIT_ACTIVE_0 },
   { KB_DEF_P2_RIGHT,     MSG_P2_RIGHT,            0x010004, 0x01, BIT_ACTIVE_0 },
   { KB_DEF_P2_B1,        MSG_P2_B1,               0x010004, 0x10, BIT_ACTIVE_0 },
   { KB_DEF_P2_B2,        MSG_P2_B2,               0x010004, 0x20, BIT_ACTIVE_0 },
   { KB_DEF_P2_B3,        MSG_P2_B3,               0x010004, 0x40, BIT_ACTIVE_0 },

   { 0,                   NULL,                    0,        0,    0            },
};

static struct DSW_DATA dsw_data_64street_0[] =
{
   COINAGE_8BITS
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_64street_1[] =
{
   { MSG_SCREEN,		      0x01, 0x02 },
   { MSG_NORMAL,		      0x01 },
   { MSG_INVERT,		      0x00 },
   { MSG_DEMO_SOUND,          0x02, 0x02 },
   { MSG_OFF,                 0x02 },
   { MSG_ON,                  0x00 },
   { MSG_CONTINUE_PLAY,       0x04, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x04 },
   { MSG_DIFFICULTY,          0x18, 0x04 },
   { MSG_EASY,                0x10 },
   { MSG_NORMAL,              0x18 },
   { MSG_HARD,                0x08 },
   { MSG_HARDEST,             0x00 },
   { MSG_LIVES,               0x60, 0x04 },
   { "1",                     0x40 },
   { "2",                     0x60 },
   { "3",                     0x20 },
   { "5",                     0x00 },
   { MSG_SERVICE,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

struct DSW_INFO _64th_street_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_64street_0 },
   { 0x010006, 0xBD, dsw_data_64street_1 },
   { 0,        0,    NULL,      },
};

static struct VIDEO_INFO megasys2_video =
{
   DrawMegaSystem2,
   256,
   224,
   32,
   VIDEO_ROTATE_NORMAL |
   VIDEO_ROTATABLE,
};

static struct VIDEO_INFO megasys2_r90_video =
{
   DrawMegaSystem2,
   256,
   224,
   32,
   VIDEO_ROTATE_90 |
   VIDEO_ROTATABLE,
};

static struct YM2151interface ym2151_interface =
{
  1,                    // 1 chip
  3500000,              // 3.5 MHz
  { YM3012_VOL(200,OSD_PAN_LEFT,200,OSD_PAN_RIGHT) },
  { NULL },             // sorry, but the adpcm seemed too loud at 96 - Antiriad
  { NULL }              // maybe i tried the wrong games? :).
};


static struct OKIM6295interface m6295_interface =
{
   2,					// 1 chip
   { 30000,
     30000 },				// rate
   { 0,
     0 },		// rom list
   { 100, 100 }, // volumes
};

static struct SOUND_INFO jaleco_ym2151_m6295x2_sound[] =
{
   { SOUND_YM2151J, &ym2151_interface,  },
   { SOUND_M6295,   &m6295_interface,   },
   { 0,             NULL,               },
};

static struct DIR_INFO _64th_street_japanese_dirs[] =
{
   { "64th_street_japanese", },
   { "64streej", },
   { ROMOF("64street"), },
   { CLONEOF("64street"), },
   { NULL, },
};

static struct ROM_INFO _64th_street_japanese_roms[] =
{
  { "91105-3.bin", 0x040000, 0xa211a83b , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "91105-2.bin", 0x040000, 0x27c1f436 , REGION_ROM1, 0x000001, LOAD_8_16 },
  // cpu 2
  { "64th_08.rom", 0x010000, 0x632be0c1 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "64th_07.rom", 0x010000, 0x13595d01 , REGION_ROM1, 0x080001, LOAD_8_16 },
   {  "64th_01.rom", 0x00080000, 0x06222f90, 0, 0, 0, },
   {  "64th_04.rom", 0x00080000, 0x98f83ef6, 0, 0, 0, },
   {  "64th_05.rom", 0x00080000, 0xa89a7020, 0, 0, 0, },
   {  "64th_06.rom", 0x00080000, 0x2bfcdc75, 0, 0, 0, },
   {  "64th_09.rom", 0x00020000, 0xa4a97db4, 0, 0, 0, },
   {  "64th_10.rom", 0x00040000, 0xa3390561, 0, 0, 0, },
   {  "64th_11.rom", 0x00020000, 0xb0b8a65c, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DIR_INFO astyanax_dirs[] =
{
   { "astyanax", },
   { NULL, },
};

static struct ROM_INFO astyanax_roms[] =
{
  { "astyan2.bin", 0x20000, 0x1b598dcc , REGION_ROM1, 0x00000, LOAD_8_16 },
  { "astyan1.bin", 0x20000, 0x1a1ad3cf , REGION_ROM1, 0x00001, LOAD_8_16 },
  { "astyan3.bin", 0x10000, 0x097b53a6 , REGION_ROM1, 0x40000, LOAD_8_16 },

  { "astyan4.bin", 0x10000, 0x1e1cbdb2 , REGION_ROM1, 0x40001, LOAD_8_16 },
  // cpu 2
  { "astyan5.bin", 0x010000, 0x11c74045 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "astyan6.bin", 0x010000, 0xeecd4b16 , REGION_ROM1, 0x080001, LOAD_8_16 },
   {  "astyan7.bin", 0x00020000, 0x319418cc, 0, 0, 0, },
   {  "astyan8.bin", 0x00020000, 0x5e5d2a22, 0, 0, 0, },
   {  "astyan9.bin", 0x00020000, 0xa10b3f17, 0, 0, 0, },
   { "astyan10.bin", 0x00020000, 0x4f704e7a, 0, 0, 0, },
   { "astyan11.bin", 0x00020000, 0x5593fec9, 0, 0, 0, },
   { "astyan12.bin", 0x00020000, 0xe8b313ec, 0, 0, 0, },
   { "astyan13.bin", 0x00020000, 0x5f3496c6, 0, 0, 0, },
   { "astyan14.bin", 0x00020000, 0x29a09ec2, 0, 0, 0, },
   { "astyan15.bin", 0x00020000, 0x0d316615, 0, 0, 0, },
   { "astyan16.bin", 0x00020000, 0xba96e8d9, 0, 0, 0, },
   { "astyan17.bin", 0x00020000, 0xbe60ba06, 0, 0, 0, },
   { "astyan18.bin", 0x00020000, 0x3668da3d, 0, 0, 0, },
   { "astyan19.bin", 0x00020000, 0x98158623, 0, 0, 0, },
   { "astyan20.bin", 0x00020000, 0xc1ad9aa0, 0, 0, 0, },
   { "astyan21.bin", 0x00020000, 0x0bf498ee, 0, 0, 0, },
   { "astyan22.bin", 0x00020000, 0x5f04d9b1, 0, 0, 0, },
   { "astyan23.bin", 0x00020000, 0x7bd4d1e7, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_astyanax_0[] =
{
   { MSG_COIN1,               0x07, 0x04 },
   { MSG_4COIN_1PLAY,         0x00, 0x00 },
   { MSG_3COIN_1PLAY,         0x04, 0x00 },
   { MSG_2COIN_1PLAY,         0x02, 0x00 },
   { MSG_1COIN_1PLAY,         0x07, 0x00 },
   { MSG_COIN2,               0x38, 0x04 },
   { MSG_4COIN_1PLAY,         0x00, 0x00 },
   { MSG_3COIN_1PLAY,         0x20, 0x00 },
   { MSG_2COIN_1PLAY,         0x10, 0x00 },
   { MSG_1COIN_1PLAY,         0x38, 0x00 },
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x40, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_TEST_MODE,           0x80, 0x02 },	// p1_start + p2_start to pause
   { MSG_OFF,                 0x80, 0x00 },	// p2_start to advance one frame
   { MSG_ON,                  0x00, 0x00 },	// up or down to open menu
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_astyanax_1[] =
{
   { MSG_UNKNOWN,             0x01, 0x02 },
   { MSG_OFF,                 0x01, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_UNKNOWN,             0x02, 0x02 },
   { MSG_OFF,                 0x02, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_EXTRA_LIFE,          0x04, 0x02 },
   { "30k 70k..",             0x04, 0x00 },
   { "50k 100k..",            0x00, 0x00 },
   { MSG_LIVES,               0x18, 0x04 },
   { "2",                     0x08, 0x00 },
   { "3",                     0x18, 0x00 },
   { "4",                     0x10, 0x00 },
   { "5",                     0x00, 0x00 },
   { MSG_DIFFICULTY,          0x20, 0x02 },
   { MSG_NORMAL,              0x20, 0x00 },
   { MSG_HARD,                0x00, 0x00 },
   { "1P/2P Control Flip",    0x40, 0x02 },
   { MSG_OFF,                 0x40, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80, 0x00 },
   { MSG_INVERT,              0x00, 0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO astyanax_dsw[] =
{
   { 0x010007, 0xBF, dsw_data_astyanax_0 },
   { 0x010006, 0xFF, dsw_data_astyanax_1 },
   { 0,        0,    NULL,      },
};

static struct VIDEO_INFO megasys1_video =
{
   DrawMegaSystem1,
   256,
   224,
   32,
   VIDEO_ROTATE_NORMAL |
   VIDEO_ROTATABLE,
};

static struct VIDEO_INFO megasys1_r270_video =
{
   DrawMegaSystem1,
   256,
   224,
   32,
   VIDEO_ROTATE_270 |
   VIDEO_ROTATABLE,
};

static struct DIR_INFO the_lord_of_king_dirs[] =
{
   { "the_lord_of_king", },
   { "lord_of_king", },
   { "lordofk", },
   { ROMOF("astyanax"), },
   { CLONEOF("astyanax"), },
   { NULL, },
};

static struct ROM_INFO the_lord_of_king_roms[] =
{
  { "lokj02.bin", 0x20000, 0x0d7f9b4a , REGION_ROM1, 0x00000, LOAD_8_16 },
  { "lokj01.bin", 0x20000, 0xbed3cb93 , REGION_ROM1, 0x00001, LOAD_8_16 },
  { "lokj03.bin", 0x20000, 0xd8702c91 , REGION_ROM1, 0x40000, LOAD_8_16 },

  { "lokj04.bin", 0x20000, 0xeccbf8c9 , REGION_ROM1, 0x40001, LOAD_8_16 },
  // cpu 2
  { "astyan5.bin", 0x010000, 0x11c74045 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "astyan6.bin", 0x010000, 0xeecd4b16 , REGION_ROM1, 0x080001, LOAD_8_16 },
   {  "astyan7.bin", 0x00020000, 0x319418cc, 0, 0, 0, },
   {  "astyan8.bin", 0x00020000, 0x5e5d2a22, 0, 0, 0, },
   {  "astyan9.bin", 0x00020000, 0xa10b3f17, 0, 0, 0, },
   { "astyan10.bin", 0x00020000, 0x4f704e7a, 0, 0, 0, },
   { "astyan11.bin", 0x00020000, 0x5593fec9, 0, 0, 0, },
   { "astyan12.bin", 0x00020000, 0xe8b313ec, 0, 0, 0, },
   { "astyan13.bin", 0x00020000, 0x5f3496c6, 0, 0, 0, },
   { "astyan14.bin", 0x00020000, 0x29a09ec2, 0, 0, 0, },
   { "astyan15.bin", 0x00020000, 0x0d316615, 0, 0, 0, },
   { "astyan16.bin", 0x00020000, 0xba96e8d9, 0, 0, 0, },
   { "astyan17.bin", 0x00020000, 0xbe60ba06, 0, 0, 0, },
   { "astyan18.bin", 0x00020000, 0x3668da3d, 0, 0, 0, },
   { "astyan19.bin", 0x00020000, 0x98158623, 0, 0, 0, },
   { "astyan20.bin", 0x00020000, 0xc1ad9aa0, 0, 0, 0, },
   { "astyan21.bin", 0x00020000, 0x0bf498ee, 0, 0, 0, },
   { "astyan22.bin", 0x00020000, 0x5f04d9b1, 0, 0, 0, },
   { "astyan23.bin", 0x00020000, 0x7bd4d1e7, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DIR_INFO avenging_spirit_dirs[] =
{
   { "avenging_spirit", },
   { "avspirit", },
   { NULL, },
};

static struct ROM_INFO avenging_spirit_roms[] =
{
  { "spirit05.rom", 0x40000, 0xb26a341a , REGION_ROM1, 0, LOAD_8_16 },

  { "spirit06.rom", 0x40000, 0x609f71fe , REGION_ROM1, 1, LOAD_8_16 },
  // cpu 2
  { "spirit01.rom", 0x020000, 0xd02ec045 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "spirit02.rom", 0x020000, 0x30213390 , REGION_ROM1, 0x080001, LOAD_8_16 },
   { "spirit09.rom", 0x00020000, 0x0c37edf7, 0, 0, 0, },
   { "spirit10.rom", 0x00080000, 0x2b1180b3, 0, 0, 0, },
   { "spirit11.rom", 0x00080000, 0x7896f6b0, 0, 0, 0, },
   { "spirit12.rom", 0x00080000, 0x728335d4, 0, 0, 0, },
   { "spirit13.rom", 0x00040000, 0x05bc04d9, 0, 0, 0, },
   { "spirit14.rom", 0x00040000, 0x13be9979, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_avspirit_0[] =
{
   COINAGE_8BITS
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_avspirit_1[] =
{
   { MSG_SCREEN,		      0x01, 0x02 },
   { MSG_NORMAL,		      0x01, 0x00 },
   { MSG_INVERT,		      0x00, 0x00 },
   { MSG_DEMO_SOUND,          0x02, 0x02 },
   { MSG_OFF,                 0x02, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_CONTINUE_PLAY,       0x04, 0x02 },
   { MSG_OFF,                 0x00, 0x00 },
   { MSG_ON,                  0x04, 0x00 },
   { MSG_DIFFICULTY,          0x18, 0x04 },
   { MSG_EASY,                0x08, 0x00 },
   { MSG_NORMAL,              0x18, 0x00 },
   { MSG_HARD,                0x10, 0x00 },
   { MSG_HARDEST,             0x00, 0x00 },
   { MSG_CABINET,             0x20, 0x02 },
   { MSG_UPRIGHT,             0x20, 0x00 },
   { MSG_TABLE,               0x00, 0x00 },
   { MSG_TEST_MODE,           0x40, 0x02 },	// p1_start + p2_start to pause
   { MSG_OFF,                 0x40, 0x00 },	// p2_start to advance one frame
   { MSG_ON,                  0x00, 0x00 },
   { MSG_SERVICE,             0x80, 0x02 },
   { MSG_OFF,                 0x80, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO avenging_spirit_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_avspirit_0 },
   { 0x010006, 0xFD, dsw_data_avspirit_1 },
   { 0,        0,    NULL,      },
};

static struct DIR_INFO chimera_beast_dirs[] =
{
   { "chimera_beast", },
   { "chimerab", },
   { NULL, },
};

static struct ROM_INFO chimera_beast_roms[] =
{
  { "prg3.bin", 0x040000, 0x70f1448f , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "prg2.bin", 0x040000, 0x821dbb85 , REGION_ROM1, 0x000001, LOAD_8_16 },
  // cpu 2
  { "prg8.bin", 0x010000, 0xa682b1ca , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "prg7.bin", 0x010000, 0x83b9982d , REGION_ROM1, 0x080001, LOAD_8_16 },
   {       "b1.bin", 0x00080000, 0x29c0385e, 0, 0, 0, },
   {       "b2.bin", 0x00080000, 0x6e7f1778, 0, 0, 0, },
   {       "s1.bin", 0x00080000, 0xe4c2ac77, 0, 0, 0, },
   {       "s2.bin", 0x00080000, 0xfafb37a5, 0, 0, 0, },
   {     "scr3.bin", 0x00020000, 0x5fe38a83, 0, 0, 0, },
   {    "voi10.bin", 0x00040000, 0x67498914, 0, 0, 0, },
   {    "voi11.bin", 0x00040000, 0x14b3afe6, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_chimerab_0[] =
{
   { MSG_SCREEN,		      0x01, 0x02 },
   { MSG_NORMAL,		      0x01 },
   { MSG_INVERT,		      0x00 },
   { MSG_DEMO_SOUND,          0x02, 0x02 },
   { MSG_OFF,                 0x02 },
   { MSG_ON,                  0x00 },
   { MSG_CONTINUE_PLAY,       0x04, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x04 },
   { MSG_DIFFICULTY,          0x18, 0x04 },
   { MSG_EASY,                0x10 },
   { MSG_NORMAL,              0x18 },
   { MSG_HARD,                0x08 },
   { MSG_HARDEST,             0x00 },
   { MSG_LIVES,               0x60, 0x04 },
   { "1",                     0x40 },
   { "2",                     0x60 },
   { "3",                     0x20 },
   { "4",                     0x00 },
   { MSG_SERVICE,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_chimerab_1[] =
{
   COINAGE_8BITS
   { NULL,                    0,    0,   },
};

static struct DSW_INFO chimera_beast_dsw[] =
{
   { 0x010007, 0xBD, dsw_data_chimerab_0 },
   { 0x010006, 0xFF, dsw_data_chimerab_1 },
   { 0,        0,    NULL,      },
};

GAME( chimera_beast ,
   chimera_beast_dirs,
   chimera_beast_roms,
   megasys_1_inputs,
   chimera_beast_dsw,
   NULL,

   load_chimera_beast,
   NULL,
   &megasys2_video,
   ExecuteMegaSystem2Frame,
   "chimerab",
   "Chimera Beast",
   "キメラビースト",
   COMPANY_ID_JALECO,
   NULL,
   1993,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

static struct DIR_INFO cybattler_dirs[] =
{
   { "cybattler", },
   { "cybattlr", },
   { NULL, },
};

static struct ROM_INFO cybattler_roms[] =
{
  { "cb_03.rom", 0x040000, 0xbee20587 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "cb_02.rom", 0x040000, 0x2ed14c50 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "cb_08.rom", 0x010000, 0xbf7b3558 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "cb_07.rom", 0x010000, 0x85d219d7 , REGION_ROM1, 0x080001, LOAD_8_16 },
   {   "cb_m01.rom", 0x00080000, 0x1109337f, 0, 0, 0, },
   {   "cb_m04.rom", 0x00080000, 0x0c91798e, 0, 0, 0, },
   {    "cb_02.rom", 0x00040000, 0x2ed14c50, 0, 0, 0, },
   {    "cb_03.rom", 0x00040000, 0xbee20587, 0, 0, 0, },
   {    "cb_09.rom", 0x00020000, 0x37b1f195, 0, 0, 0, },
   {    "cb_10.rom", 0x00040000, 0x8af95eed, 0, 0, 0, },
   {    "cb_11.rom", 0x00040000, 0x59d62d1f, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_cybattler_0[] =
{
   { MSG_COIN1,               0x07, 0x08 },
   { MSG_4COIN_1PLAY,         0x00 },
   { MSG_3COIN_1PLAY,         0x01 },
   { MSG_2COIN_1PLAY,         0x03 },
   { MSG_1COIN_1PLAY,         0x07 },
   { MSG_2COIN_3PLAY,         0x02 },
   { MSG_1COIN_2PLAY,         0x06 },
   { MSG_1COIN_3PLAY,         0x05 },
   { MSG_1COIN_4PLAY,         0x04 },
   { MSG_COIN2,               0x38, 0x08 },
   { MSG_4COIN_1PLAY,         0x00 },
   { MSG_3COIN_1PLAY,         0x08 },
   { MSG_2COIN_1PLAY,         0x18 },
   { MSG_1COIN_1PLAY,         0x38 },
   { MSG_2COIN_3PLAY,         0x10 },
   { MSG_1COIN_2PLAY,         0x30 },
   { MSG_1COIN_3PLAY,         0x28 },
   { MSG_1COIN_4PLAY,         0x20 },
   { MSG_FREE_PLAY,           0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { MSG_SERVICE,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_cybattler_1[] =
{
   { MSG_DIFFICULTY,          0x03, 0x04 },
   { MSG_EASY,                0x02 },
   { MSG_NORMAL,              0x03 },
   { MSG_HARD,                0x01 },
   { MSG_HARDEST,             0x00 },
   { "Instructions",          0x04, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x04 },
   { MSG_CONTINUE_PLAY,       0x18, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x18 },
   { MSG_DEMO_SOUND,          0x60, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x20 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO cybattler_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_cybattler_0 },
   { 0x010006, 0xBF, dsw_data_cybattler_1 },
   { 0,        0,    NULL,      },
};

GAME( cybattler ,
   cybattler_dirs,
   cybattler_roms,
   megasys_1_inputs,
   cybattler_dsw,
   NULL,

   LoadCybattler,
   NULL,
   &megasys2_r90_video,
   ExecuteMegaSystem2Frame,
   "cybattlr",
   "Cybattler",
   "サイバトラー",
   COMPANY_ID_JALECO,
   NULL,
   1993,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

static struct DIR_INFO earth_defence_force_dirs[] =
{
   { "earth_defense_force", },
   { "earth_defence_force", },
   { "edf", },
   { NULL, },
};

static struct ROM_INFO earth_defence_force_roms[] =
{
  { "edf_05.rom", 0x40000, 0x105094d1 , REGION_ROM1, 0, LOAD_8_16 },

  { "edf_06.rom", 0x40000, 0x94da2f0c , REGION_ROM1, 1, LOAD_8_16 },
  // cpu 2
  { "edf_01.rom", 0x020000, 0x2290ea19 , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "edf_02.rom", 0x020000, 0xce93643e , REGION_ROM1, 0x080001, LOAD_8_16 },
   {   "edf_09.rom", 0x00020000, 0x96e38983, 0, 0, 0, },
   {  "edf_m01.rom", 0x00040000, 0x9149286b, 0, 0, 0, },
   {  "edf_m02.rom", 0x00040000, 0xfc4281d2, 0, 0, 0, },
   {  "edf_m03.rom", 0x00080000, 0xef469449, 0, 0, 0, },
   {  "edf_m04.rom", 0x00080000, 0x6744f406, 0, 0, 0, },
   {  "edf_m05.rom", 0x00080000, 0x6f47e456, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_edf_0[] =
{
   COINAGE_6BITS
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_SERVICE,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_edf_1[] =
{
   { MSG_UNKNOWN,             0x01, 0x02 },
   { MSG_OFF,                 0x01 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x02, 0x02 },
   { MSG_OFF,                 0x02 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x04, 0x02 },
   { MSG_OFF,                 0x04 },
   { MSG_ON,                  0x00 },
   { MSG_LIVES,               0x08, 0x02 },
   { "3",                     0x08 },
   { "4",                     0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x00 },
   { MSG_NORMAL,              0x30 },
   { MSG_HARD,                0x10 },
   { MSG_HARDEST,             0x20 },
   { MSG_UNKNOWN,             0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO earth_defence_force_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_edf_0 },
   { 0x010006, 0xFF, dsw_data_edf_1 },
   { 0,        0,    NULL,      },
};

GAME( earth_defence_force ,
   earth_defence_force_dirs,
   earth_defence_force_roms,
   megasys_1_inputs,
   earth_defence_force_dsw,
   NULL,

   LoadEarthDefForce,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "edf",
   "Earth Defence Force",
   "Ｅ．Ｄ．Ｆ",
   COMPANY_ID_JALECO,
   NULL,
   1991,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

static struct DIR_INFO hachoo_dirs[] =
{
   { "hachoo", },
   { NULL, },
};

static struct ROM_INFO hachoo_roms[] =
{
  { "hacho02.rom", 0x020000, 0x49489c27 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "hacho01.rom", 0x020000, 0x97fc9515 , REGION_ROM1, 0x000001, LOAD_8_16 },
  // cpu 2
  { "hacho05.rom", 0x010000, 0x6271f74f , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "hacho06.rom", 0x010000, 0xdb9e743c , REGION_ROM1, 0x060001, LOAD_8_16 },
   {  "hacho08.rom", 0x00020000, 0x888a6df1, 0, 0, 0, },
   {  "hacho07.rom", 0x00020000, 0x06e6ca7f, 0, 0, 0, },
   {  "hacho09.rom", 0x00020000, 0xe9f35c90, 0, 0, 0, },
   {  "hacho10.rom", 0x00020000, 0x1aeaa188, 0, 0, 0, },
   {  "hacho14.rom", 0x00080000, 0x10188483, 0, 0, 0, },
   {  "hacho15.rom", 0x00020000, 0xe559347e, 0, 0, 0, },
   {  "hacho16.rom", 0x00020000, 0x105fd8b5, 0, 0, 0, },
   {  "hacho17.rom", 0x00020000, 0x77f46174, 0, 0, 0, },
   {  "hacho18.rom", 0x00020000, 0x0be21111, 0, 0, 0, },
   {  "hacho19.rom", 0x00020000, 0x33bc9de3, 0, 0, 0, },
   {  "hacho20.rom", 0x00020000, 0x2ae2011e, 0, 0, 0, },
   {  "hacho21.rom", 0x00020000, 0x6dcfb8d5, 0, 0, 0, },
   {  "hacho22.rom", 0x00020000, 0xccabf0e0, 0, 0, 0, },
   {  "hacho23.rom", 0x00020000, 0xff5f77aa, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_hachoo_0[] =
{
   COINAGE_6BITS
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_UNKNOWN,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_hachoo_1[] =
{
   { MSG_UNKNOWN,             0x01, 0x02 },
   { MSG_OFF,                 0x01 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x02, 0x02 },
   { MSG_OFF,                 0x02 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x04, 0x02 },
   { MSG_OFF,                 0x04 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x08, 0x02 },
   { MSG_OFF,                 0x08 },
   { MSG_ON,                  0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },	// does it change something??
   { MSG_EASY,                0x20 },
   { MSG_NORMAL,              0x30 },
   { MSG_HARD,                0x10 },
   { MSG_HARDEST,             0x00 },
   { MSG_UNKNOWN,             0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO hachoo_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_hachoo_0 },
   { 0x010006, 0xFF, dsw_data_hachoo_1 },
   { 0,        0,    NULL,      },
};

GAME( hachoo ,
   hachoo_dirs,
   hachoo_roms,
   megasys_1_inputs,
   hachoo_dsw,
   NULL,

   LoadHachoo,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "hachoo",
   "Hachoo",
   "�j汀",
   COMPANY_ID_JALECO,
   NULL,
   1989,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

static struct DIR_INFO kick_off_dirs[] =
{
   { "kick_off", },
   { "kickoff", },
   { NULL, },
};

static struct ROM_INFO kick_off_roms[] =
{
  { "kioff03.rom", 0x010000, 0x3b01be65 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "kioff01.rom", 0x010000, 0xae6e68a1 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "kioff09.rom", 0x010000, 0x1770e980 , REGION_ROM1, 0x020000, LOAD_8_16 },

  { "kioff19.rom", 0x010000, 0x1b03bbe4 , REGION_ROM1, 0x020001, LOAD_8_16 },
   {  "kioff07.rom", 0x00020000, 0xed649919, 0, 0, 0, },
   {  "kioff05.rom", 0x00020000, 0xe7232103, 0, 0, 0, },
   {  "kioff06.rom", 0x00020000, 0xa0b3cb75, 0, 0, 0, },
   {  "kioff10.rom", 0x00020000, 0xfd739fec, 0, 0, 0, },
   {  "kioff16.rom", 0x00020000, 0x22c46314, 0, 0, 0, },
   {  "kioff17.rom", 0x00020000, 0xf171559e, 0, 0, 0, },
   {  "kioff18.rom", 0x00020000, 0xd7909ada, 0, 0, 0, },
   {  "kioff20.rom", 0x00020000, 0x5c28bd2d, 0, 0, 0, },
   {  "kioff21.rom", 0x00020000, 0x195940cf, 0, 0, 0, },
   {  "kioff26.rom", 0x00020000, 0x2a90df1b, 0, 0, 0, },
   {  "kioff27.rom", 0x00020000, 0xca221ae2, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_kick_off_0[] =
{
   { MSG_COIN1,               0x07, 0x08 },
   { MSG_4COIN_1PLAY,         0x01 },
   { MSG_3COIN_1PLAY,         0x02 },
   { MSG_2COIN_1PLAY,         0x03 },
   { MSG_1COIN_1PLAY,         0x07 },
   { MSG_1COIN_2PLAY,         0x06 },
   { MSG_1COIN_3PLAY,         0x05 },
   { MSG_1COIN_4PLAY,         0x04 },
   { MSG_FREE_PLAY,           0x00 },
   { MSG_UNKNOWN,             0x08, 0x02 },
   { MSG_OFF,                 0x08 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x10, 0x02 },
   { MSG_OFF,                 0x10 },
   { MSG_ON,                  0x00 },
   { "Freeze",                0x20, 0x02 },
   { MSG_OFF,                 0x20 },
   { MSG_ON,                  0x00 },
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { "Text",                  0x80, 0x02 },
   { "Japanese",              0x80 },
   { "English",               0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_kick_off_1[] =
{
   { "Time",                  0x03, 0x04 },
   { "3'",                    0x03, 0x00 },
   { "4'",                    0x02, 0x00 },
   { "5'",                    0x01, 0x00 },
   { "6'",                    0x00, 0x00 },
   { MSG_UNKNOWN,             0x04, 0x02 },
   { MSG_OFF,                 0x04, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_UNKNOWN,             0x08, 0x02 },
   { MSG_OFF,                 0x08, 0x00 },
   { MSG_ON,                  0x00, 0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x30, 0x00 },
   { MSG_NORMAL,              0x20, 0x00 },
   { MSG_HARD,                0x10, 0x00 },
   { MSG_HARDEST,             0x00, 0x00 },
   { "Controls",              0x40, 0x02 },
   { MSG_UNKNOWN,             0x40, 0x00 },
   { "Joystick",              0x00, 0x00 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80, 0x00 },
   { MSG_INVERT,              0x00, 0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO kick_off_dsw[] =
{
   { 0x010007, 0x3F, dsw_data_kick_off_0 },
   { 0x010006, 0xAF, dsw_data_kick_off_1 },
   { 0,        0,    NULL,      },
};

GAME( kick_off ,
   kick_off_dirs,
   kick_off_roms,
   megasys_1_inputs,
   kick_off_dsw,
   NULL,

   LoadKickOff,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "kickoff",
   "Kick Off",
   NULL,
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_SPORTS
);

static struct DIR_INFO legend_of_makai_dirs[] =
{
   { "legend_of_makai", },
   { "legend_of_makaj", },
   { "lomakai", },
   { "lomakaj", },
   { NULL, },
};

static struct ROM_INFO legend_of_makai_roms[] =
{
  { "lom_01.rom", 0x10000, 0x46e85e90 , REGION_ROM2, 0x0000, LOAD_NORMAL },
   {   "lom_05.rom", 0x00020000, 0xd04fc713, 0, 0, 0, },
   {   "lom_06.rom", 0x00020000, 0xf33b6eed, 0, 0, 0, },
   {   "lom_08.rom", 0x00010000, 0xbdb15e67, 0, 0, 0, },
  { "lom_30.rom", 0x020000, 0xba6d65b8 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "lom_20.rom", 0x020000, 0x56a00dc2 , REGION_ROM1, 0x000001, LOAD_8_16 },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_legend_of_makai_0[] =
{
   COINAGE_6BITS_ALT
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { "Invulnerability",       0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_legend_of_makai_1[] =
{
   { MSG_LIVES,               0x03, 0x04 },
   { "2",                     0x00 },
   { "3",                     0x03 },
   { "4",                     0x02 },
   { "5",                     0x01 },
   { MSG_UNKNOWN,             0x04, 0x02 },
   { MSG_OFF,                 0x04 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x08, 0x02 },
   { MSG_OFF,                 0x08 },
   { MSG_ON,                  0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x30 },
   { MSG_NORMAL,              0x20 },
   { MSG_HARD,                0x10 },
   { MSG_HARDEST,             0x00 },
   { "Controls",              0x40, 0x02 },
   { "1",                     0x00 },
   { "2",                     0x40 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO legend_of_makai_dsw[] =
{
   { 0x010007, 0xBF, dsw_data_legend_of_makai_0 },
   { 0x010006, 0xEF, dsw_data_legend_of_makai_1 },
   { 0,        0,    NULL,      },
};

static struct VIDEO_INFO legend_of_makai_video =
{
   DrawLegendOfMakaj,
   256,
   224,
   32,
   VIDEO_ROTATE_NORMAL |
   VIDEO_ROTATABLE,
};

static struct YM2203interface ym2203_interface =
{
   1,
   4000000,
   {YM2203_VOL(200,200)},
   //{ 0x00ff20c0 },
   {0},
   {0},
   {0},
   {0},
   {NULL}
};

static struct SOUND_INFO jaleco_ym2203_sound[] =
{
   { SOUND_YM2203,  &ym2203_interface,  },
   { 0,             NULL,               },
};

GAME( legend_of_makai ,
   legend_of_makai_dirs,
   legend_of_makai_roms,
   megasys_1_inputs,
   legend_of_makai_dsw,
   NULL,

   LoadLegendOfMakaj,
   NULL,
   &legend_of_makai_video,
   ExecuteMegaSystem1Frame,
   "lomakai",
   "Legend of Makai",
   "魔魁伝説",
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2203_sound,
   GAME_PLATFORM
);

static struct DIR_INFO makai_densetsu_dirs[] =
{
   { "makai_densetsu", },
   { "makaiden", },
   { ROMOF("lomakai"), },
   { CLONEOF("lomakai"), },
   { NULL, },
};

static struct ROM_INFO makai_densetsu_roms[] =
{
  { "lom_01.rom", 0x10000, 0x46e85e90 , REGION_ROM2, 0x0000, LOAD_NORMAL },
   {   "lom_05.rom", 0x00020000, 0xd04fc713, 0, 0, 0, },
   {   "lom_06.rom", 0x00020000, 0xf33b6eed, 0, 0, 0, },
   {   "makaiden.8", 0x00010000, 0xa7f623f9, 0, 0, 0, },
  { "makaiden.3a", 0x020000, 0x87cf81d1 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "makaiden.2a", 0x020000, 0xd40e0fea , REGION_ROM1, 0x000001, LOAD_8_16 },
   {           NULL,          0,          0, 0, 0, 0, },
};

GAME( makai_densetsu ,
   makai_densetsu_dirs,
   makai_densetsu_roms,
   megasys_1_inputs,
   legend_of_makai_dsw,
   NULL,

   LoadLegendOfMakaj,
   NULL,
   &legend_of_makai_video,
   ExecuteMegaSystem1Frame,
   "makaiden",
   "Makai Densetsu",
   "魔魁伝説",
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2203_sound,
   GAME_PLATFORM
);

static struct DIR_INFO p47_american_dirs[] =
{
   { "p47", },
   { "p47_american", },
   { "p47_usa", },
   { NULL, },
};

static struct ROM_INFO p47_american_roms[] =
{
  { "p47us3.bin", 0x020000, 0x022e58b8 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "p47us1.bin", 0x020000, 0xed926bd8 , REGION_ROM1, 0x000001, LOAD_8_16 },
  // cpu 2
  { "p47j_9.bin", 0x010000, 0xffcf318e , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "p47j_19.bin", 0x010000, 0xadb8c12e , REGION_ROM1, 0x060001, LOAD_8_16 },
   {  "p47j_12.bin", 0x00020000, 0x5268395f, 0, 0, 0, },
  { "p47us16.bin", 0x010000, 0x5a682c8f , REGION_GFX3, 0x000000, LOAD_NORMAL },
   {  "p47j_26.bin", 0x00020000, 0x4d07581a, 0, 0, 0, },
   {   "p47j_7.bin", 0x00020000, 0xf77723b7, 0, 0, 0, },
   {  "p47j_27.bin", 0x00020000, 0x9e2bde8e, 0, 0, 0, },
   {  "p47j_18.bin", 0x00020000, 0x29d8f676, 0, 0, 0, },
   {  "p47j_23.bin", 0x00020000, 0x6e9bc864, 0, 0, 0, },
   {   "p47j_5.bin", 0x00020000, 0xfe65b65c, 0, 0, 0, },
   {   "p47j_6.bin", 0x00020000, 0xe191d2d2, 0, 0, 0, },
   {  "p47j_10.bin", 0x00020000, 0xb9d79c1e, 0, 0, 0, },
   {  "p47j_11.bin", 0x00020000, 0xfa0d1887, 0, 0, 0, },
   {  "p47j_20.bin", 0x00020000, 0x2ed53624, 0, 0, 0, },
   {  "p47j_21.bin", 0x00020000, 0x6f56b56d, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_p47_0[] =
{
   COINAGE_6BITS_ALT
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { "Invulnerability",       0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_p47_1[] =
{
   { MSG_LIVES,               0x03, 0x04 },
   { "2",                     0x02 },
   { "3",                     0x03 },
   { "4",                     0x01 },
   { "5",                     0x00 },
   { MSG_UNKNOWN,             0x04, 0x02 },
   { MSG_OFF,                 0x04 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x08, 0x02 },
   { MSG_OFF,                 0x08 },
   { MSG_ON,                  0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x00 },
   { MSG_NORMAL,              0x30 },
   { MSG_HARD,                0x20 },
   { MSG_HARDEST,             0x10 },
   { MSG_UNKNOWN,             0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO p47_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_p47_0 },
   { 0x010006, 0xFF, dsw_data_p47_1 },
   { 0,        0,    NULL,      },
};

static struct DIR_INFO p47_japanese_dirs[] =
{
   { "p47_japanese", },
   { "p47j", },
   { ROMOF("p47"), },
   { CLONEOF("p47"), },
   { NULL, },
};

static struct ROM_INFO p47_japanese_roms[] =
{
  { "p47j_3.bin", 0x020000, 0x11c655e5 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "p47j_1.bin", 0x020000, 0x0a5998de , REGION_ROM1, 0x000001, LOAD_8_16 },
  // cpu 2
  { "p47j_9.bin", 0x010000, 0xffcf318e , REGION_ROM1, 0x060000, LOAD_8_16 },
  { "p47j_19.bin", 0x010000, 0xadb8c12e , REGION_ROM1, 0x060001, LOAD_8_16 },
   {  "p47j_10.bin", 0x00020000, 0xb9d79c1e, 0, 0, 0, },
   {  "p47j_11.bin", 0x00020000, 0xfa0d1887, 0, 0, 0, },
   {  "p47j_12.bin", 0x00020000, 0x5268395f, 0, 0, 0, },
  { "p47j_16.bin", 0x010000, 0x30e44375 , REGION_GFX3, 0x000000, LOAD_NORMAL },
   {  "p47j_18.bin", 0x00020000, 0x29d8f676, 0, 0, 0, },
   {  "p47j_20.bin", 0x00020000, 0x2ed53624, 0, 0, 0, },
   {  "p47j_21.bin", 0x00020000, 0x6f56b56d, 0, 0, 0, },
   {  "p47j_23.bin", 0x00020000, 0x6e9bc864, 0, 0, 0, },
   {  "p47j_26.bin", 0x00020000, 0x4d07581a, 0, 0, 0, },
   {  "p47j_27.bin", 0x00020000, 0x9e2bde8e, 0, 0, 0, },
   {   "p47j_5.bin", 0x00020000, 0xfe65b65c, 0, 0, 0, },
   {   "p47j_6.bin", 0x00020000, 0xe191d2d2, 0, 0, 0, },
   {   "p47j_7.bin", 0x00020000, 0xf77723b7, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DIR_INFO peek_a_boo_dirs[] =
{
   { "peek_a_boo", },
   { "peekaboo", },
   { NULL, },
};

static struct ROM_INFO peek_a_boo_roms[] =
{
  { "j3", 0x020000, 0xf5f4cf33 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "j2", 0x020000, 0x7b3d430d , REGION_ROM1, 0x000001, LOAD_8_16 },
   {            "1", 0x00080000, 0x5a444ecf, 0, 0, 0, },
   {            "5", 0x00080000, 0x34fa07bb, 0, 0, 0, },
   {            "4", 0x00020000, 0xf037794b, 0, 0, 0, },
   { "peeksamp.124", 0x100000, 0xe1206fa8 , REGION_SMP1, 0, LOAD_NORMAL },
   { "peeksamp.124", 0x100000, 0xe1206fa8 , REGION_SMP1,0x20000, LOAD_NORMAL },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct INPUT_INFO peek_a_boo_inputs[] =
{
   { KB_DEF_COIN1,        MSG_COIN1,               0x040000, 0x04, BIT_ACTIVE_0 },
   { KB_DEF_COIN2,        MSG_COIN2,               0x040000, 0x08, BIT_ACTIVE_0 },
   { KB_DEF_TILT,         MSG_TILT,                0x040000, 0x01, BIT_ACTIVE_0 },
   { KB_DEF_SERVICE,      MSG_SERVICE,             0x040000, 0x02, BIT_ACTIVE_0 },

   { KB_DEF_P1_START,     MSG_P1_START,            0x040000, 0x10, BIT_ACTIVE_0 },
   { KB_DEF_P1_LEFT,      MSG_P1_LEFT,             0x014010, 0xFF, BIT_ACTIVE_1 },
   { KB_DEF_P1_RIGHT,     MSG_P1_RIGHT,            0x014011, 0xFF, BIT_ACTIVE_1 },
   { KB_DEF_P1_B1,        MSG_P1_B1,               0x040001, 0x01, BIT_ACTIVE_0 },
   { KB_DEF_P1_B2,        MSG_P1_B2,               0x040001, 0x02, BIT_ACTIVE_0 },
   { KB_DEF_P1_B3,        MSG_P1_B3,               0x040001, 0x10, BIT_ACTIVE_0 },
   { KB_DEF_P1_B4,        MSG_P1_B4,               0x040001, 0x20, BIT_ACTIVE_0 },

   { KB_DEF_P2_START,     MSG_P2_START,            0x040000, 0x20, BIT_ACTIVE_0 },
   { KB_DEF_P2_LEFT,      MSG_P2_LEFT,             0x014020, 0xFF, BIT_ACTIVE_1 },
   { KB_DEF_P2_RIGHT,     MSG_P2_RIGHT,            0x014021, 0xFF, BIT_ACTIVE_1 },
   { KB_DEF_P2_B1,        MSG_P2_B1,               0x040001, 0x04, BIT_ACTIVE_0 },
   { KB_DEF_P2_B2,        MSG_P2_B2,               0x040001, 0x08, BIT_ACTIVE_0 },

   { 0,                   NULL,                    0,        0,    0            },
};

static struct DSW_DATA dsw_data_peekaboo_0[] =
{
   COINAGE_6BITS_ALT
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_SCREEN,         	0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_peekaboo_1[] =
{
   { MSG_DIFFICULTY,          0x03, 0x04 },
   { MSG_EASY,                0x00 },
   { MSG_NORMAL,              0x03 },
   { MSG_HARD,                0x02 },
   { MSG_HARDEST,             0x01 },
   { MSG_SERVICE,             0x04, 0x02 },
   { MSG_OFF,                 0x04 },
   { MSG_ON,                  0x00 },
   { "Movement",              0x08, 0x02 },
   { "Paddles",               0x08 },
   { "Buttons",               0x00 },
   { "Nudity",                0x30, 0x04 },
   { "Female & Male (Full)",  0x30 },
   { "Female & Male",         0x10 },
   { "Female (Full)",         0x20 },
   { "None",                  0x00 },
   { MSG_CABINET,             0x40, 0x02 },
   { MSG_UPRIGHT,             0x40 },
   { MSG_TABLE,               0x00 },
   { "Controls",              0x80, 0x02 },
   { "1",                     0x80 },
   { "2",                     0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO peek_a_boo_dsw[] =
{
   { 0x030001, 0xFF, dsw_data_peekaboo_0 },
   { 0x030000, 0x6F, dsw_data_peekaboo_1 },
   { 0,        0,    NULL,      },
};

static struct VIDEO_INFO peek_a_boo_video =
{
   DrawPeekABoo,
   256,
   224,
   32,
   VIDEO_ROTATE_NORMAL |
   VIDEO_ROTATABLE,
};

static struct OKIM6295interface m6295_interface_D =
{
	1,
	{ 12000 },
	{ REGION_SMP1 },
	{ 255 }
};

static struct SOUND_INFO m6295_sound_D[] =
{
   { SOUND_M6295,   &m6295_interface_D,   },
   { 0,             NULL,               },
};

static struct DIR_INFO phantasm_dirs[] =
{
   { "phantasm", },
   { ROMOF("avspirit"), },
   { CLONEOF("avspirit"), },
   { NULL, },
};

static struct ROM_INFO phantasm_roms[] =
{
  { "phntsm02.bin", 0x020000, 0xd96a3584 , REGION_ROM1, 0x000000, LOAD_8_16 },
  { "phntsm01.bin", 0x020000, 0xa54b4b87 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "phntsm03.bin", 0x010000, 0x1d96ce20 , REGION_ROM1, 0x040000, LOAD_8_16 },

  { "phntsm04.bin", 0x010000, 0xdc0c4994 , REGION_ROM1, 0x040001, LOAD_8_16 },
  // cpu 2
  { "phntsm05.bin", 0x010000, 0x3b169b4a , REGION_ROM1, 0x080000, LOAD_8_16 },

  { "phntsm06.bin", 0x010000, 0xdf2dfb2e , REGION_ROM1, 0x080001, LOAD_8_16 },
   { "spirit13.rom", 0x00040000, 0x05bc04d9, 0, 0, 0, },
   { "spirit14.rom", 0x00040000, 0x13be9979, 0, 0, 0, },
   { "spirit12.rom", 0x00080000, 0x728335d4, 0, 0, 0, },
   { "spirit11.rom", 0x00080000, 0x7896f6b0, 0, 0, 0, },
   { "spirit09.rom", 0x00020000, 0x0c37edf7, 0, 0, 0, },
   { "spirit10.rom", 0x00080000, 0x2b1180b3, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};


static struct DIR_INFO plus_alpha_dirs[] =
{
   { "plus_alpha", },
   { "plusalph", },
   { NULL, },
};

static struct ROM_INFO plus_alpha_roms[] =
{
   {  "pa-rom1.bin", 0x00020000, 0xa32fdcae, 0, 0, 0, },
   {  "pa-rom2.bin", 0x00020000, 0x33244799, 0, 0, 0, },
   {  "pa-rom3.bin", 0x00010000, 0x1b739835, 0, 0, 0, },
   {  "pa-rom4.bin", 0x00010000, 0xff760e80, 0, 0, 0, },
   {  "pa-rom5.bin", 0x00010000, 0xddc2739b, 0, 0, 0, },
   {  "pa-rom6.bin", 0x00010000, 0xf6f8a167, 0, 0, 0, },
   {  "pa-rom7.bin", 0x00020000, 0x9f5d800e, 0, 0, 0, },
   {  "pa-rom8.bin", 0x00020000, 0xae007750, 0, 0, 0, },
   {  "pa-rom9.bin", 0x00020000, 0x065364bd, 0, 0, 0, },
   { "pa-rom10.bin", 0x00020000, 0x395df3b2, 0, 0, 0, },
   { "pa-rom11.bin", 0x00020000, 0xeb709ae7, 0, 0, 0, },
   { "pa-rom12.bin", 0x00020000, 0xcacbc350, 0, 0, 0, },
   { "pa-rom13.bin", 0x00020000, 0xfad093dd, 0, 0, 0, },
   { "pa-rom14.bin", 0x00020000, 0xd3676cd1, 0, 0, 0, },
   { "pa-rom15.bin", 0x00020000, 0x8787735b, 0, 0, 0, },
   { "pa-rom16.bin", 0x00020000, 0xa06b813b, 0, 0, 0, },
   { "pa-rom17.bin", 0x00020000, 0xc6b38a4b, 0, 0, 0, },
   { "pa-rom19.bin", 0x00010000, 0x39ef193c, 0, 0, 0, },
   { "pa-rom20.bin", 0x00020000, 0x86c557a8, 0, 0, 0, },
   { "pa-rom21.bin", 0x00020000, 0x81140a88, 0, 0, 0, },
   { "pa-rom22.bin", 0x00020000, 0x97e39886, 0, 0, 0, },
   { "pa-rom23.bin", 0x00020000, 0x0383fb65, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_plusalph_0[] =
{
   COINAGE_6BITS
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { "Freeze",                0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_plusalph_1[] =
{
   { MSG_LIVES,               0x03, 0x04 },
   { "3",                     0x03 },
   { "4",                     0x02 },
   { "5",                     0x01 },
   { "Infinite",              0x00 },
   { "Bombs",                 0x04, 0x02 },
   { "2",                     0x00 },
   { "3",                     0x04 },
   { MSG_EXTRA_LIFE,          0x08, 0x02 },
   { "70k and every 130k",    0x08 },
   { "100k and every 200k",   0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x00 },
   { MSG_NORMAL,              0x30 },
   { MSG_HARD,                0x10 },
   { MSG_HARDEST,             0x20 },
   { MSG_CABINET,             0x40, 0x02 },
   { MSG_UPRIGHT,             0x00 },
   { MSG_TABLE,               0x40 },
   { MSG_SCREEN,         	0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO plus_alpha_dsw[] =
{
  { 0x10007, 0xff, dsw_data_plusalph_0 },
  { 0x10006, 0xbf, dsw_data_plusalph_1 },
  { 0, 0, NULL }
};

GAME( plus_alpha ,
   plus_alpha_dirs,
   plus_alpha_roms,
   megasys_1_inputs,
   plus_alpha_dsw,
   NULL,

   LoadPlusAlpha,
   NULL,
   &megasys1_r270_video,
   ExecuteMegaSystem1Frame,
   "plusalph",
   "Plus Alpha",
   "プラスアルファ",
   COMPANY_ID_JALECO,
   NULL,
   1989,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

static struct DIR_INFO rodland_dirs[] =
{
   { "rodland", },
   { "rodland_english", },
   { "rodlande", },
   { NULL, },
};

static struct ROM_INFO rodland_roms[] =
{
  { "rl_02.rom", 0x020000, 0xc7e00593 , REGION_ROM1, 0x000000, LOAD_8_16 },
  { "rl_01.rom", 0x020000, 0x2e748ca1 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "rl_03.rom", 0x010000, 0x62fdf6d7 , REGION_ROM1, 0x040000, LOAD_8_16 },

  { "rl_04.rom", 0x010000, 0x44163c86 , REGION_ROM1, 0x040001, LOAD_8_16 },
  // cou 2
  { "rl_05.rom", 0x010000, 0xc1617c28 , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "rl_06.rom", 0x010000, 0x663392b2 , REGION_ROM1, 0x060001, LOAD_8_16 },
  { "rl_23.rom", 0x80000, 0xac60e771 , REGION_GFX1, 0, LOAD_NORMAL },

  { "rl_18.rom", 0x080000, 0xf3b30ca6 , REGION_GFX2, 0x000000, LOAD_NORMAL },

  { "rl_19.bin", 0x020000, 0x124d7e8f , REGION_GFX3, 0x000000, LOAD_NORMAL },

  { "rl_14.rom", 0x080000, 0x08d01bf4 , REGION_GFX4, 0x000000, LOAD_NORMAL },
   {    "rl_08.rom", 0x00040000, 0x8a49d3a7, 0, 0, 0, },
   {    "rl_10.rom", 0x00040000, 0xe1d1cd99, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_rodland_0[] =
{
   COINAGE_6BITS
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { MSG_SERVICE,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_rodland_1[] =
{
   { MSG_LIVES,               0x0f, 0x04 },
   { "2",                     0x06 },
   { "3",                     0x0f },
   { "4",                     0x0c },
   { "Infinite",              0x03 },
   { "Default Episode",       0x10, 0x02 },
   { "1",                     0x10 },
   { "2",                     0x00 },
   { MSG_DIFFICULTY,          0x60, 0x04 },	// does it change something??
   { MSG_EASY,                0x20 },
   { MSG_NORMAL,              0x60 },
   { MSG_HARD,                0x40 },
   { MSG_HARDEST,             0x00 },
   { MSG_SCREEN,         	0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO rodland_dsw[] =
{
   { 0x010007, 0xBF, dsw_data_rodland_0 },
   { 0x010006, 0xFF, dsw_data_rodland_1 },
   { 0,        0,    NULL,      },
};

static struct DIR_INFO rodlandjb_dirs[] =
{
   { "rodland_japaneseb", },
   { "rodlandjb", },
   { "rodlndjb", },
   { ROMOF("rodland"), },
   { CLONEOF("rodland"), },
   { NULL, },
};

static struct DIR_INFO rodlandj_dirs[] =
{
   { "rodland_japanese", },
   { "rodlandj", },
   { ROMOF("rodland"), },
   { CLONEOF("rodland"), },
   { NULL, },
};

static struct ROM_INFO rodlandjb_roms[] =
{
  { "rl19.bin", 0x010000, 0x028de21f , REGION_ROM1, 0x000000, LOAD_8_16 },
  { "rl17.bin", 0x010000, 0x9c720046 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "rl20.bin", 0x010000, 0x3f536d07 , REGION_ROM1, 0x020000, LOAD_8_16 },
  { "rl18.bin", 0x010000, 0x5aa61717 , REGION_ROM1, 0x020001, LOAD_8_16 },
  { "rl_3.bin", 0x010000, 0xc5b1075f , REGION_ROM1, 0x040000, LOAD_8_16 },

  { "rl_4.bin", 0x010000, 0x9ec61048 , REGION_ROM1, 0x040001, LOAD_8_16 },
  { "rl02.bin", 0x010000, 0xd26eae8f , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "rl01.bin", 0x010000, 0x04cf24bc , REGION_ROM1, 0x060001, LOAD_8_16 },
  { "rl_23.rom", 0x80000, 0xac60e771 , REGION_GFX1, 0, LOAD_NORMAL },

  { "rl_18.rom", 0x080000, 0xf3b30ca6 , REGION_GFX2, 0x000000, LOAD_NORMAL },

  { "rl_19.bin", 0x020000, 0x124d7e8f , REGION_GFX3, 0x000000, LOAD_NORMAL },

  { "rl_14.rom", 0x080000, 0x08d01bf4 , REGION_GFX4, 0x000000, LOAD_NORMAL },

   {    "rl_08.rom", 0x00040000, 0x8a49d3a7, 0, 0, 0, },
   {    "rl_10.rom", 0x00040000, 0xe1d1cd99, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct ROM_INFO rodlandj_roms[] =
{
  { "rl_2.bin", 0x020000, 0xb1d2047e , REGION_ROM1, 0x000000, LOAD_8_16 },
  { "rl_1.bin", 0x020000, 0x3c47c2a3 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "rl_3.bin", 0x010000, 0xc5b1075f , REGION_ROM1, 0x040000, LOAD_8_16 },

  { "rl_4.bin", 0x010000, 0x9ec61048 , REGION_ROM1, 0x040001, LOAD_8_16 },
  { "rl_05.rom", 0x010000, 0xc1617c28 , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "rl_06.rom", 0x010000, 0x663392b2 , REGION_ROM1, 0x060001, LOAD_8_16 },
  { "rl_14.bin", 0x080000, 0x8201e1bb , REGION_GFX1, 0x000000, LOAD_NORMAL },

  { "rl_18.rom", 0x080000, 0xf3b30ca6 , REGION_GFX2, 0x000000, LOAD_NORMAL },

  { "rl_19.bin", 0x020000, 0x124d7e8f , REGION_GFX3, 0x000000, LOAD_NORMAL },


  { "rl_23.bin", 0x080000, 0x936db174 , REGION_GFX4, 0x000000, LOAD_NORMAL },
   {    "rl_08.rom", 0x00040000, 0x8a49d3a7, 0, 0, 0, },
   {    "rl_10.rom", 0x00040000, 0xe1d1cd99, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DIR_INFO saint_dragon_dirs[] =
{
   { "saint_dragon", },
   { "stdragon", },
   { NULL, },
};

static struct ROM_INFO saint_dragon_roms[] =
{
   {   "jsd-01.bin", 0x00020000, 0x67429a57, 0, 0, 0, },
   {   "jsd-02.bin", 0x00020000, 0xcc29ab19, 0, 0, 0, },
   {   "jsd-05.bin", 0x00010000, 0x8c04feaa, 0, 0, 0, },
   {   "jsd-06.bin", 0x00010000, 0x0bb62f3a, 0, 0, 0, },
   {   "jsd-07.bin", 0x00020000, 0x6a48e979, 0, 0, 0, },
   {   "jsd-08.bin", 0x00020000, 0x40704962, 0, 0, 0, },
   {   "jsd-09.bin", 0x00020000, 0xe366bc5a, 0, 0, 0, },
   {   "jsd-10.bin", 0x00020000, 0x4a8f4fe6, 0, 0, 0, },
   {   "jsd-11.bin", 0x00020000, 0x2783b7b1, 0, 0, 0, },
   {   "jsd-12.bin", 0x00020000, 0x89466ab7, 0, 0, 0, },
   {   "jsd-13.bin", 0x00020000, 0x9896ae82, 0, 0, 0, },
   {   "jsd-14.bin", 0x00020000, 0x7e8da371, 0, 0, 0, },
   {   "jsd-15.bin", 0x00020000, 0xe296bf59, 0, 0, 0, },
   {   "jsd-16.bin", 0x00020000, 0xd8919c06, 0, 0, 0, },
   {   "jsd-17.bin", 0x00020000, 0x4f7ad563, 0, 0, 0, },
   {   "jsd-18.bin", 0x00020000, 0x1f4da822, 0, 0, 0, },
   {   "jsd-19.bin", 0x00010000, 0x25ce807d, 0, 0, 0, },
   {   "jsd-20.bin", 0x00020000, 0x2c6e93bb, 0, 0, 0, },
   {   "jsd-21.bin", 0x00020000, 0x864bcc61, 0, 0, 0, },
   {   "jsd-22.bin", 0x00020000, 0x44fe2547, 0, 0, 0, },
   {   "jsd-23.bin", 0x00020000, 0x6b010e1a, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_saint_dragon_0[] =
{
   COINAGE_6BITS_ALT
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_UNKNOWN,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_saint_dragon_1[] =
{
   { MSG_LIVES,               0x03, 0x04 },
   { "2",                     0x02 },
   { "3",                     0x03 },
   { "4",                     0x01 },
   { "5",                     0x00 },
   { MSG_UNKNOWN,             0x04, 0x02 },
   { MSG_OFF,                 0x04 },
   { MSG_ON,                  0x00 },
   { MSG_UNKNOWN,             0x08, 0x02 },
   { MSG_OFF,                 0x08 },
   { MSG_ON,                  0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x30 },
   { MSG_NORMAL,              0x20 },
   { MSG_HARD,                0x10 },
   { MSG_HARDEST,             0x00 },
   { MSG_CABINET,             0x40, 0x02 },
   { MSG_UPRIGHT,             0x00 },
   { MSG_TABLE,               0x40 },
   { MSG_SCREEN,         	0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO saint_dragon_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_saint_dragon_0 },
   { 0x010006, 0xAF, dsw_data_saint_dragon_1 },
   { 0,        0,    NULL,      },
};

GAME( saint_dragon ,
   saint_dragon_dirs,
   saint_dragon_roms,
   megasys_1_inputs,
   saint_dragon_dsw,
   NULL,

   LoadSaintDragon,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "stdragon",
   "Saint Dragon",
   "天聖龍",
   COMPANY_ID_JALECO,
   NULL,
   1989,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

static struct DIR_INFO soldam_dirs[] =
{
   { "soldam", },
   { "soldamj", },
   { NULL, },
};

static struct ROM_INFO soldam_roms[] =
{
   {  "soldam1.bin", 0x00020000, 0xe7cb0c20, 0, 0, 0, },
   {  "soldam2.bin", 0x00020000, 0xc73d29e4, 0, 0, 0, },
   {  "soldam3.bin", 0x00010000, 0xc5382a07, 0, 0, 0, },
   {  "soldam4.bin", 0x00010000, 0x1df7816f, 0, 0, 0, },
   {  "soldam5.bin", 0x00010000, 0xd1019a67, 0, 0, 0, },
   {  "soldam6.bin", 0x00010000, 0x3ed219b4, 0, 0, 0, },
   {  "soldam8.bin", 0x00040000, 0xfcd36019, 0, 0, 0, },
   { "soldam10.bin", 0x00040000, 0x8d5613bf, 0, 0, 0, },
   { "soldam14.bin", 0x00080000, 0x26cea54a, 0, 0, 0, },
   { "soldam18.bin", 0x00080000, 0x7d8e4712, 0, 0, 0, },
   { "soldam19.bin", 0x00020000, 0x38465da1, 0, 0, 0, },
   { "soldam23.bin", 0x00080000, 0x0ca09432, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_soldam_0[] =
{
   COINAGE_6BITS_ALT
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_SERVICE,         	0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_soldam_1[] =
{
   { MSG_DIFFICULTY,          0x03, 0x04 },
   { MSG_EASY,                0x00 },
   { MSG_NORMAL,              0x03 },
   { MSG_HARD,                0x02 },
   { MSG_HARDEST,             0x01 },
   { "Games to Play (Vs)",    0x0c, 0x04 },
   { "1",                     0x00 },
   { "2",                     0x0c },
   { "3",                     0x08 },
   { "4",                     0x04 },
   { MSG_CONTINUE_PLAY,       0x10, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x10 },
   { "Cred. to Start (Vs)",   0x20, 0x02 },
   { "1",                     0x20 },
   { "2",                     0x00 },
   { "Cred. Continue (Vs)",   0x40, 0x02 },
   { "1",                     0x40 },
   { "2",                     0x00 },
   { MSG_SCREEN,              0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO soldam_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_soldam_0 },
   { 0x010006, 0xFB, dsw_data_soldam_1 },
   { 0,        0,    NULL,      },
};

static struct ROMSW_DATA romsw_data_soldam_0[] =
{
   { "Soldam - Japan",           0x00 },
   { "Soldam - America",         0x01 },
   { NULL,              0    },
};

static struct ROMSW_INFO soldam_romsw[] =
{
   { 0x3a9d, 0x02, romsw_data_soldam_0 },
   { 0,        0,    NULL },
};

GAME( soldam ,
   soldam_dirs,
   soldam_roms,
   megasys_1_inputs,
   soldam_dsw,
   soldam_romsw,

   load_soldam,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "soldamj",
   "Soldam",
   NULL,
   COMPANY_ID_JALECO,
   NULL,
   1992,
   jaleco_ym2151_m6295x2_sound,
   GAME_PUZZLE
);

static struct DIR_INFO iga_ninjyutsuden_dirs[] =
{
   { "iga_ninjyutsuden", },
   { "iganinju", },
   { "kazan", },
   { NULL, },
};

static struct ROM_INFO iga_ninjyutsuden_roms[] =
{
   {   "iga_01.bin", 0x00020000, 0xfa416a9e, 0, 0, 0, },
   {   "iga_02.bin", 0x00020000, 0xbd00c280, 0, 0, 0, },
   {   "iga_03.bin", 0x00010000, 0xde5937ad, 0, 0, 0, },
   {   "iga_04.bin", 0x00010000, 0xafaf0480, 0, 0, 0, },
   {   "iga_05.bin", 0x00010000, 0x13580868, 0, 0, 0, },
   {   "iga_06.bin", 0x00010000, 0x7904d5dd, 0, 0, 0, },
   {   "iga_08.bin", 0x00040000, 0x857dbf60, 0, 0, 0, },
   {   "iga_10.bin", 0x00040000, 0x67a89e0d, 0, 0, 0, },
   {   "iga_14.bin", 0x00040000, 0xc707d513, 0, 0, 0, },
   {   "iga_18.bin", 0x00080000, 0x6c727519, 0, 0, 0, },
   {   "iga_19.bin", 0x00020000, 0x98a7e998, 0, 0, 0, },
   {   "iga_23.bin", 0x00080000, 0xfb58c5f4, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_iganinju_0[] =
{
   COINAGE_6BITS
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x40 },
   { MSG_ON,                  0x00 },
   { "Freeze",                0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_iganinju_1[] =
{
   { MSG_LIVES,               0x03, 0x04 },
   { "2",                     0x03 },
   { "3",                     0x01 },
   { "4",                     0x02 },
   { "Infinite",              0x00 },
   { MSG_EXTRA_LIFE,          0x04, 0x02 },
   { "50k",                   0x04 },
   { "200k",                  0x00 },
   { MSG_CONTINUE_PLAY,       0x08, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x08 },
   { MSG_DIFFICULTY,          0x30, 0x04 },	// does it change something??
   { MSG_EASY,                0x20 },
   { MSG_NORMAL,              0x30 },
   { MSG_HARD,                0x10 },
   { MSG_HARDEST,             0x00 },
   { MSG_CABINET,             0x40, 0x02 },
   { MSG_UPRIGHT,             0x00 },
   { MSG_TABLE,               0x40 },
   { MSG_SCREEN,         	0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO iga_ninjyutsuden_dsw[] =
{
   { 0x010007, 0xBF, dsw_data_iganinju_0 },
   { 0x010006, 0xBD, dsw_data_iganinju_1 },
   { 0,        0,    NULL,      },
};

GAME( iga_ninjyutsuden ,
   iga_ninjyutsuden_dirs,
   iga_ninjyutsuden_roms,
   megasys_1_inputs,
   iga_ninjyutsuden_dsw,
   NULL,

   load_iga_ninjyutsuden,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "iganinju",
   "Iga Ninjyutsuden",
   "伊賀�E術伝",
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

static struct DIR_INFO shingen_dirs[] =
{
   { "shingen", },
   { "tshingen", },
   { NULL, },
};

static struct ROM_INFO shingen_roms[] =
{
  { "takeda2.bin", 0x020000, 0x6ddfc9f3 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "takeda1.bin", 0x020000, 0x1afc6b7d , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "takeda5.bin", 0x010000, 0xfbdc51c0 , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "takeda6.bin", 0x010000, 0x8fa65b69 , REGION_ROM1, 0x060001, LOAD_8_16 },
   { "shing_07.rom", 0x00020000, 0xc37ecbdc, 0, 0, 0, },
   { "shing_08.rom", 0x00020000, 0x36d56c8c, 0, 0, 0, },
   { "takeda9.bin", 0x00020000, 0xdb7f3f4f, 0, 0, 0, },
   { "takeda10.bin", 0x00020000, 0xc9959d71, 0, 0, 0, },
  { "takeda11.bin", 0x020000, 0xbf0b40a6 , REGION_GFX1, 0x000000, LOAD_NORMAL },

  { "takeda12.bin", 0x020000, 0x07987d89 , REGION_GFX1, 0x020000, LOAD_NORMAL },
  { "takeda15.bin", 0x020000, 0x4c316b79 , REGION_GFX2, 0x000000, LOAD_NORMAL },
  { "takeda16.bin", 0x020000, 0xceda9dd6 , REGION_GFX2, 0x020000, LOAD_NORMAL },

  { "takeda17.bin", 0x020000, 0x3d4371dc , REGION_GFX2, 0x040000, LOAD_NORMAL },

  { "takeda19.bin", 0x010000, 0x2ca2420d , REGION_GFX3, 0x000000, LOAD_NORMAL },
  { "takeda20.bin", 0x020000, 0x1bfd636f , REGION_GFX4, 0x000000, LOAD_NORMAL },
  { "takeda21.bin", 0x020000, 0x12fb006b , REGION_GFX4, 0x020000, LOAD_NORMAL },
  { "takeda22.bin", 0x020000, 0xb165b6ae , REGION_GFX4, 0x040000, LOAD_NORMAL },

  { "takeda23.bin", 0x020000, 0x37cb9214 , REGION_GFX4, 0x060000, LOAD_NORMAL },
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct DSW_DATA dsw_data_shingen_0[] =
{
   COINAGE_6BITS
   { MSG_DEMO_SOUND,          0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_UNKNOWN,             0x80, 0x02 },
   { MSG_OFF,                 0x80 },
   { MSG_ON,                  0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_DATA dsw_data_shingen_1[] =
{
   { MSG_LIVES,               0x03, 0x04 },
   { "2",                     0x03 },
   { "3",                     0x01 },
   { "4",                     0x02 },
   { "Infinite",              0x00 },
   { MSG_EXTRA_LIFE,          0x0c, 0x04 },
   { "20k",                   0x0c },
   { "30k",                   0x04 },
   { "40k",                   0x08 },
   { "50k",                   0x00 },
   { MSG_DIFFICULTY,          0x30, 0x04 },
   { MSG_EASY,                0x30 },
   { MSG_NORMAL,              0x10 },
   { MSG_HARD,                0x20 },
   { MSG_HARDEST,             0x00 },
   { MSG_CONTINUE_PLAY,       0x40, 0x02 },
   { MSG_OFF,                 0x00 },
   { MSG_ON,                  0x40 },
   { MSG_SCREEN,         	0x80, 0x02 },
   { MSG_NORMAL,              0x80 },
   { MSG_INVERT,              0x00 },
   { NULL,                    0,    0,   },
};

static struct DSW_INFO shingen_dsw[] =
{
   { 0x010007, 0xFF, dsw_data_shingen_0 },
   { 0x010006, 0xDD, dsw_data_shingen_1 },
   { 0,        0,    NULL,      },
};

static struct DIR_INFO tshingna_dirs[] =
{
   { "tshingna", },
   { "tshingna", },
   { ROMOF("tshingen"), },
   { CLONEOF("tshingen"), },
   { NULL, },
};

static struct ROM_INFO tshingna_roms[] =
{
  { "shing_02.rom", 0x020000, 0xd9ab5b78 , REGION_ROM1, 0x000000, LOAD_8_16 },

  { "shing_01.rom", 0x020000, 0xa9d2de20 , REGION_ROM1, 0x000001, LOAD_8_16 },
  { "takeda5.bin", 0x010000, 0xfbdc51c0 , REGION_ROM1, 0x060000, LOAD_8_16 },

  { "takeda6.bin", 0x010000, 0x8fa65b69 , REGION_ROM1, 0x060001, LOAD_8_16 },
  { "takeda11.bin", 0x020000, 0xbf0b40a6 , REGION_GFX1, 0x000000, LOAD_NORMAL },

  { "shing_12.rom", 0x020000, 0x5e4adedb , REGION_GFX1, 0x020000, LOAD_NORMAL },
  { "shing_15.rom", 0x020000, 0x9db18233 , REGION_GFX2, 0x000000, LOAD_NORMAL },
  { "takeda16.bin", 0x020000, 0xceda9dd6 , REGION_GFX2, 0x020000, LOAD_NORMAL },

  { "takeda17.bin", 0x020000, 0x3d4371dc , REGION_GFX2, 0x040000, LOAD_NORMAL },

  { "shing_19.rom", 0x010000, 0x97282d9d , REGION_GFX3, 0x000000, LOAD_NORMAL },
  { "shing_20.rom", 0x020000, 0x7f6f8384 , REGION_GFX4, 0x000000, LOAD_NORMAL },
  { "takeda21.bin", 0x020000, 0x12fb006b , REGION_GFX4, 0x020000, LOAD_NORMAL },
  { "takeda22.bin", 0x020000, 0xb165b6ae , REGION_GFX4, 0x040000, LOAD_NORMAL },

  { "takeda23.bin", 0x020000, 0x37cb9214 , REGION_GFX4, 0x060000, LOAD_NORMAL },
   { "shing_07.rom", 0x00020000, 0xc37ecbdc, 0, 0, 0, },
   { "shing_08.rom", 0x00020000, 0x36d56c8c, 0, 0, 0, },
   { "takeda9.bin", 0x00020000, 0xdb7f3f4f, 0, 0, 0, },
   { "takeda10.bin", 0x00020000, 0xc9959d71, 0, 0, 0, },
   {           NULL,          0,          0, 0, 0, 0, },
};

static UINT8 *RAM_COL;

static UINT8 *GFX_FG0;
static UINT8 *FG0_Mask;

static UINT8 *GFX_SPR;
static UINT8 *SPR_Mask;

#define MSK_SPR         0x0FFF

static UINT8 *GFX_BG1;
static UINT8 *BG1_Mask;

static UINT8 *GFX_BG0;
static UINT8 *BG0_Mask;

static UINT8 RenderSpr;

/**********************************************************/

static int romset, spr_pri_needed;
int render_hachoo_spr = 1, x_start;

/**********************************************************/

/**** add & modified by hiro-shi!! ****/
static UINT16 SoundByte[32];
static int SoundW, SoundR;
static int NowReadSound;

void SoundWorkInit( void )
{
  SoundW = SoundR = 0;
  NowReadSound = 0;
};

static void MS1VideoWrite(UINT32 addr, UINT16 data)
{
   addr&=0xFFFF;

   WriteWord(&RAM[0x10000+addr],data);

   // Sprite Chain RAM

   if(addr>=0xE000){
      RenderSpr=0;                      // Force Recalculate Chain Pointers
      return;
   }

   // Sound COMM

   if(addr==0x4308){
      SoundByte[SoundW]=data;
      SoundW = (SoundW+1) & 31;
      //#ifdef RAINE_DEBUG
      //print_debug("Main 68000 Sends:%04x\n",data);
      //#endif
      return;
   }
}

static void MS2SoundWrite(UINT32 addr, UINT16 data)
{
   WriteWord(&RAM[0x18000],data);

   SoundByte[SoundW]=data;
   SoundW = (SoundW+1) & 31;
   //#ifdef RAINE_DEBUG
   //print_debug("Main 68000 Sends:%04x\n",SoundByte);
   //#endif
}

/*-------[Sub 68000 Sound Port]-------*/
static int SubSoundRead(UINT32 address)
{
   //#ifdef RAINE_DEBUG
   //print_debug("Sub 68000 Reads:%04x\n",SoundByte);
   //#endif
   if( SoundW != SoundR ){
      NowReadSound = SoundByte[SoundR];
      SoundR = (SoundR+1) & 31;
   }
   return NowReadSound;
}

static UINT16 SubSoundReadZ80(UINT16 address)
{
   //#ifdef RAINE_DEBUG
   //print_debug("Sub Z80 Reads:%02x\n",SoundByte[SoundR]&0xFF);
   //#endif
   if( SoundW != SoundR ){
      NowReadSound = SoundByte[SoundR] & 0xFF;
      SoundR = (SoundR+1) & 31;
   }
   return NowReadSound;
}

static void SubSoundWrite(UINT32 address, UINT16 data)
{
   WriteWord(&RAM[0x10008],NowReadSound);       // Write to 68000 readback port
}

/*-------[YM2151 PORT]-------*/


static UINT8 ym2151_rb(UINT32 address)
{
  static UINT8 ta=0;
  int res;
  if (ta++>251) ta =0;
  if (ta > 249) res= 1;
  else
    res= 0;
#if 0
  if (res)
  fprintf(stderr,"*%d*\n",res);
  else
  fprintf(stderr,"%d ",res);
#endif
  return res;
     //}
}

static UINT16 ym2151_rw(UINT32 address)
{
  return ym2151_rb(address)<<8 | ym2151_rb(address+1);
}

static void ym2151_wb(UINT32 address, UINT8 data)
{
   address&=3;

   if(address<2)
      YM2151_register_port_0_w(address, data);
   else
      YM2151_data_port_0_w(address, data);
}

static void ym2151_ww(UINT32 address, UINT16 data)
{
  ym2151_wb(address, (UINT8) (data&0xff)); //>>8) );
  //   ym2151_wb(address+1, (UINT8) (data&0xFF) );
}

int MS1DecodeFG0(UINT8 *src, UINT32 size)
{
   UINT32 ta,tb;

   if(!(GFX_FG0=AllocateMem(0x40000))) return(0);
   memset(GFX_FG0,0x00,0x40000);

   tb=0;
   for(ta=0;ta<size;ta++,tb+=2){
      GFX_FG0[tb+0]=(src[ta]>>4)^15;
      GFX_FG0[tb+1]=(src[ta]&15)^15;
   }

   FG0_Mask = make_solid_mask_8x8(GFX_FG0, 0x1000);

   return 1;
}

int MS1DecodeSPR(UINT8 *src, UINT32 size)
{
   UINT32 ta,tb;

   if(!(GFX_SPR=AllocateMem(size<<1))) return(0);

   tb=0;
   for(ta=0;ta<size;ta+=4){
      GFX_SPR[tb+0]=(src[ta+0]>>4)^15;
      GFX_SPR[tb+1]=(src[ta+0]&15)^15;
      GFX_SPR[tb+2]=(src[ta+1]>>4)^15;
      GFX_SPR[tb+3]=(src[ta+1]&15)^15;
      GFX_SPR[tb+4]=(src[ta+2]>>4)^15;
      GFX_SPR[tb+5]=(src[ta+2]&15)^15;
      GFX_SPR[tb+6]=(src[ta+3]>>4)^15;
      GFX_SPR[tb+7]=(src[ta+3]&15)^15;
      tb+=16;
      if((tb&0xFF)==0){tb-=0xF8;}
      else{if((tb&0xFF)==8){tb-=8;}}
   }

   SPR_Mask = make_solid_mask_16x16(GFX_SPR, size/0x80);

   RenderSpr=0;

   return 1;
}

int MS1DecodeBG1(UINT8 *src, UINT32 size)
{
   UINT32 ta,tb;

   if(!(GFX_BG1=AllocateMem(0x100000))) return(0);
   memset(GFX_BG1,0x00,0x100000);

   tb=0;
   for(ta=0;ta<size;ta+=4){
      GFX_BG1[tb+0]=(src[ta+0]>>4)^15;
      GFX_BG1[tb+1]=(src[ta+0]&15)^15;
      GFX_BG1[tb+2]=(src[ta+1]>>4)^15;
      GFX_BG1[tb+3]=(src[ta+1]&15)^15;
      GFX_BG1[tb+4]=(src[ta+2]>>4)^15;
      GFX_BG1[tb+5]=(src[ta+2]&15)^15;
      GFX_BG1[tb+6]=(src[ta+3]>>4)^15;
      GFX_BG1[tb+7]=(src[ta+3]&15)^15;
      tb+=16;
      if((tb&0xFF)==0){tb-=0xF8;}
      else{if((tb&0xFF)==8){tb-=8;}}
   }

   BG1_Mask = make_solid_mask_16x16(GFX_BG1, 0x1000);

   return 1;
}

int MS1DecodeBG0(UINT8 *src, UINT32 size)
{
   UINT32 ta,tb;

   if(!(GFX_BG0=AllocateMem(0x100000))) return(0);
   memset(GFX_BG0,0x00,0x100000);

   tb=0;
   for(ta=0;ta<size;ta+=4){
      GFX_BG0[tb+0]=(src[ta+0]>>4)^15;
      GFX_BG0[tb+1]=(src[ta+0]&15)^15;
      GFX_BG0[tb+2]=(src[ta+1]>>4)^15;
      GFX_BG0[tb+3]=(src[ta+1]&15)^15;
      GFX_BG0[tb+4]=(src[ta+2]>>4)^15;
      GFX_BG0[tb+5]=(src[ta+2]&15)^15;
      GFX_BG0[tb+6]=(src[ta+3]>>4)^15;
      GFX_BG0[tb+7]=(src[ta+3]&15)^15;
      tb+=16;
      if((tb&0xFF)==0){tb-=0xF8;}
      else{if((tb&0xFF)==8){tb-=8;}}
   }

   BG0_Mask = make_solid_mask_16x16(GFX_BG0, 0x1000);

   return 1;
}


#define DEF_MS1_SOUNDCLOCK  (CPU_FRAME_MHz(12,60))
static int MS1SoundLoop = 8;
static int MS1SoundClock = DEF_MS1_SOUNDCLOCK;

static void MS1SoundFrame(void)
{
  int ta;
  for( ta = MS1SoundLoop; ta > 0; ta-- ){
    cpu_execute_cycles(CPU_68K_1, MS1SoundClock);
#ifdef RAINE_DEBUG
       if(ta==1) print_debug("PC1:%06x SR:%04x\n",s68000context.pc,s68000context.sr);
#endif
    cpu_interrupt(CPU_68K_1, 4);
  }
}

static void MS2SoundFrame(void)
{
   int ta;

   for(ta=0;ta<3;ta++){
   cpu_execute_cycles(CPU_68K_1, CPU_FRAME_MHz(12,60)/3);          // 2 Ints/Frame (correct music speed?)
#ifdef RAINE_DEBUG
   if(ta==1) print_debug("PC1:%06x SR:%04x\n",s68000context.pc,s68000context.sr);
#endif
   cpu_interrupt(CPU_68K_1, 2);
   }
}

static void LegendOfMakajSoundFrame(void)
{
   cpu_execute_cycles(CPU_Z80_0, 4000000/60);                        // Z80 4MHz
   /*#ifdef RAINE_DEBUG
      print_debug("Z80PC0:%04x\n",z80pc);
#endif*/
   cpu_interrupt(CPU_Z80_0, 0x38);                               // 4 Ints/Frame (correct speed?)
   cpu_interrupt(CPU_Z80_0, 0x38);
   cpu_interrupt(CPU_Z80_0, 0x38);
   cpu_interrupt(CPU_Z80_0, 0x38);
}

static void PeekABooSoundFrame(void)
{
//   cpu_execute_cycles(CPU_68K_0, CPU_FRAME_MHz(10,60));
//   cpu_interrupt(CPU_68K_0, 2);
}

static int layer_id_data[4];

static char *layer_id_name[4] =
{
   "BG0",   "BG1",   "BG2",   "OBJ",
};

static void AddMS1Controls(void)
{
   ExecuteSoundFrame=&MS1SoundFrame;

   if(romset==12){
      ExecuteSoundFrame=&LegendOfMakajSoundFrame;
   }

   if(romset!=12){
      memset(RAM+0x00000,0x00,0x40000);
   }
   memset(RAM+0x10000,0xFF,0x00008);

   RAM_COL=RAM+0x18000;
   InitPaletteMap(RAM_COL, 0x40, 0x10, 0x8000);

   set_colour_mapper(&col_map_rrrr_gggg_bbbb_rgbx_rev);

   layer_id_data[0] = add_layer_info(layer_id_name[0]);
   layer_id_data[1] = add_layer_info(layer_id_name[1]);
   layer_id_data[2] = add_layer_info(layer_id_name[2]);
   layer_id_data[3] = add_layer_info(layer_id_name[3]);
}

static void AddMS2Controls(void)
{
   if(romset!=8){
      ExecuteSoundFrame=&MS1SoundFrame;
   }
   else{
      ExecuteSoundFrame=&MS2SoundFrame;
   }

   memset(&RAM[0x00000],0x00,0x60000);
   memset(&RAM[0x10000],0xFF,0x00008);

   RAM_COL=RAM+0x48000;
   InitPaletteMap(RAM_COL, 0x40, 0x10, 0x8000);

   set_colour_mapper(&col_map_rrrr_gggg_bbbb_rgbx_rev);

   layer_id_data[0] = add_layer_info(layer_id_name[0]);
   layer_id_data[1] = add_layer_info(layer_id_name[1]);
   layer_id_data[2] = add_layer_info(layer_id_name[2]);
   layer_id_data[3] = add_layer_info(layer_id_name[3]);
}

void AddMS1MainCPU(UINT32 ram_addr)
{
   AddMemFetch(0x000000, 0x05FFFF, ROM+0x000000-0x000000);      // 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x05FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadByte(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x000000);          // 68000 RAM
   AddReadByte(0x080000, 0x09FFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x05FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadWord(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x000000);          // 68000 RAM
   AddReadWord(0x080000, 0x09FFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x000000);         // 68000 RAM
   AddWriteByte(0x080000, 0x09FFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x000000);         // 68000 RAM
   AddWriteWord(0x090000, 0x09FFFF, NULL, RAM+0x020000);                // SCREEN RAM
   AddWriteWord(0x080000, 0x08FFFF, MS1VideoWrite, NULL);               // MISC SCREEN RAM
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...
}

void AddMS1SoundCPU(UINT32 rom_offset, UINT32 ram_offset, UINT32 ram_addr)
{
   AddMemFetchMC68000B(0x000000, 0x01FFFF, ROM+rom_offset-0x000000);    // SUB 68000 ROM
   AddMemFetchMC68000B(-1, -1, NULL);

   AddReadByteMC68000B(0x000000, 0x01FFFF, NULL, ROM+rom_offset);               // SUB 68000 ROM
   AddReadByteMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+ram_offset);        // SUB 68000 RAM
   AddReadByteMC68000B(0x080002, 0x080003, ym2151_rb, NULL);                   // YM2151
   if(PCMROM){
     AddReadByteMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Read_68k, NULL);           // OKI M6295 A
     AddReadByteMC68000B(0x0C0000, 0x0CFFFF, M6295_B_Read_68k, NULL);           // OKI M6295 B
   }
   AddReadByteMC68000B(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByteMC68000B(-1, -1, NULL, NULL);

   AddReadWordMC68000B(0x000000, 0x01FFFF, NULL, ROM+rom_offset);               // SUB 68000 ROM
   AddReadWordMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+ram_offset);        // SUB 68000 RAM
   AddReadWordMC68000B(0x080002, 0x080003, ym2151_rw, NULL);                    // YM2151
   if(PCMROM){
     AddReadWordMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Read_68k, NULL);           // OKI M6295 A
     AddReadWordMC68000B(0x0C0000, 0x0CFFFF, M6295_B_Read_68k, NULL);           // OKI M6295 B
   }
   if((romset==8)||(romset==9)||(romset==17)||(romset==18)){                // Cybattler + 64th Street
   AddReadWordMC68000B(0x060000, 0x060001, SubSoundRead, NULL);                 // SOUND COMM
   }
   else{
   AddReadWordMC68000B(0x040000, 0x040001, SubSoundRead, NULL);                 // SOUND COMM
   }
   AddReadWordMC68000B(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWordMC68000B(-1, -1, NULL, NULL);

   AddWriteByteMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+ram_offset);       // SUB 68000 RAM
   AddWriteByteMC68000B(0x080000, 0x080003, ym2151_wb, NULL);                   // YM2151
   if(PCMROM){
     AddWriteByteMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Write_68k, NULL);           // OKI M6295 A
     AddWriteByteMC68000B(0x0C0000, 0x0CFFFF, M6295_B_Write_68k, NULL);           // OKI M6295 B
   }
   AddWriteByteMC68000B(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByteMC68000B(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByteMC68000B(-1, -1, NULL, NULL);

   AddWriteWordMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+ram_offset);       // SUB 68000 RAM
   AddWriteWordMC68000B(0x080000, 0x080003, ym2151_ww, NULL);                   // YM2151
   if(PCMROM){
   AddWriteWordMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Write_68k, NULL);           // OKI M6295 A
   AddWriteWordMC68000B(0x0C0000, 0x0CFFFF, M6295_B_Write_68k, NULL);           // OKI M6295 B
   }
   AddWriteWordMC68000B(0x040000, 0x040001, SubSoundWrite, NULL);               // SOUND COMM
   AddWriteWordMC68000B(0x060000, 0x060001, SubSoundWrite, NULL);               // SOUND COMM
   AddWriteWordMC68000B(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWordMC68000B(-1, -1, NULL, NULL);

   AddInitMemoryMC68000B();     // Set Starscream mem pointers...
}

void AddMS1SoundCPUHachoo(UINT32 ram_addr)
{
   AddMemFetchMC68000B(0x000000, 0x01FFFF, ROM+0x060000-0x000000);      // SUB 68000 ROM
   AddMemFetchMC68000B(-1, -1, NULL);
//   fprintf(stderr,"hachoo init\n");
   AddReadByteMC68000B(0x000000, 0x01FFFF, NULL, ROM+0x060000);                 // SUB 68000 ROM
   AddReadByteMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x030000);          // SUB 68000 RAM
   AddReadByteMC68000B(0x080002, 0x080003, ym2151_rb, NULL);                    // YM2151
   if(PCMROM){
   AddReadByteMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Read_68k, NULL);             // OKI M6295 A
   AddReadByteMC68000B(0x0C0000, 0x0CFFFF, OKIM6295_status_1_r, NULL);          // OKI M6295 B
   }
   AddReadByteMC68000B(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByteMC68000B(-1, -1, NULL, NULL);

   AddReadWordMC68000B(0x000000, 0x01FFFF, NULL, ROM+0x060000);                 // SUB 68000 ROM
   AddReadWordMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x030000);          // SUB 68000 RAM
   AddReadWordMC68000B(0x080002, 0x080003, ym2151_rw, NULL);                    // YM2151
   if(PCMROM){
   AddReadWordMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Read_68k, NULL);             // OKI M6295 A
   AddReadWordMC68000B(0x0C0000, 0x0CFFFF, OKIM6295_status_1_r, NULL);          // OKI M6295 B
   }
   AddReadWordMC68000B(0x040000, 0x040001, SubSoundRead, NULL);                 // SOUND COMM
   AddReadWordMC68000B(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWordMC68000B(-1, -1, NULL, NULL);

   AddWriteByteMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x030000);         // SUB 68000 RAM
   AddWriteByteMC68000B(0x080000, 0x080003, ym2151_wb, NULL);                   // YM2151
   if(PCMROM){
   AddWriteByteMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Write_68k, NULL);           // OKI M6295 A
   AddWriteByteMC68000B(0x0C0000, 0x0CFFFF, M6295_B_Write_68k, NULL);           // OKI M6295 B
   }
   AddWriteByteMC68000B(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByteMC68000B(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByteMC68000B(-1, -1, NULL, NULL);

   AddWriteWordMC68000B(ram_addr, ram_addr+0xFFFF, NULL, RAM+0x030000);         // SUB 68000 RAM
   AddWriteWordMC68000B(0x080000, 0x080003, ym2151_ww, NULL);                   // YM2151
   if(PCMROM){
   AddWriteWordMC68000B(0x0A0000, 0x0AFFFF, M6295_A_Write_68k, NULL);           // OKI M6295 A
   AddWriteWordMC68000B(0x0C0000, 0x0CFFFF, M6295_B_Write_68k, NULL);           // OKI M6295 B
   }
   AddWriteWordMC68000B(0x060000, 0x060001, SubSoundWrite, NULL);               // SOUND COMM
   AddWriteWordMC68000B(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWordMC68000B(-1, -1, NULL, NULL);

   AddInitMemoryMC68000B();     // Set Starscream mem pointers...
}

static void rodlandj_gfx_unmangle(int region)
{
  UINT8 *rom = load_region[REGION_GFX1+region];
  int size = get_region_size(REGION_GFX1+region);
  UINT8 *buffer;
  int i;

  /* data lines swap: 76543210 -> 64537210 */
  for (i = 0;i < size;i++)
    rom[i] =   (rom[i] & 0x27)
      | ((rom[i] & 0x80) >> 4)
      | ((rom[i] & 0x48) << 1)
      | ((rom[i] & 0x10) << 2);

  buffer = malloc(size);
  if (!buffer) return;

  memcpy(buffer,rom,size);

  /* address lines swap: ..dcba9876543210 -> ..acb8937654d210 */
  for (i = 0;i < size;i++)
    {
      int a =    (i &~0x2508)
	| ((i & 0x2000) >> 10)
	| ((i & 0x0400) << 3)
	| ((i & 0x0100) << 2)
	| ((i & 0x0008) << 5);
      rom[i] = buffer[a];
    }

  free(buffer);
}

static void load_rodland(void)
{
   int ta,tb,tc,td,te;

   romset=0; spr_pri_needed=0;

   RAM = load_region[REGION_GFX3];
   if (!strcmp(current_game->main_name,"rodlandj")) {
     // Thanks to mame for this.
     // I don't think this clone is really usefull by the way, but anyway...
     rodlandj_gfx_unmangle(0);
     rodlandj_gfx_unmangle(3);
   }

   if(!MS1DecodeFG0(RAM,0x10000))return;
   FreeMem(RAM);
   if(!(RAM=AllocateMem(0x60000))) return;

   ROM = load_region[REGION_GFX4];
   if(!MS1DecodeSPR(ROM,0x80000))return;
   FreeMem(ROM);

   ROM = load_region[REGION_GFX2];
   if(!MS1DecodeBG1(ROM,0x80000))return;
   FreeMem(ROM);

   ROM = load_region[REGION_GFX1];
   for(ta=0;ta<0x10000;ta++){
   tb=ROM[ta+0x20000];
   tc=ROM[ta+0x30000];
   td=ROM[ta+0x40000];
   te=ROM[ta+0x50000];
   ROM[ta+0x20000]=td;
   ROM[ta+0x30000]=tb;
   ROM[ta+0x40000]=te;
   ROM[ta+0x50000]=tc;
   }
   if(!MS1DecodeBG0(ROM,0x80000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_CPU1];

   if (!strcmp(current_game->main_name,"rodland")) // only this one is decoded
     DecodeRodlandE(ROM);       // Deprotection
   else if (!strcmp(current_game->main_name,"rodlandj")) // special
     DecodePlusAlpha(ROM);      // Deprotection
   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("rl_10.rom",PCMROM+0x00000,0x40000)) return;
   if(!load_rom("rl_08.rom",PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 11;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x072E],0x13FC0000);       // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x0732],0x00AA0000);       //

   // Sub 68000
   // ---------

   WriteWord68k(&ROM[0x62058],0x4E75);          // rts

   WriteLong68k(&ROM[0x604AA],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x604AE],0x00AA0000);      //
   WriteLong68k(&ROM[0x604B2],0x4E714E71);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x60000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

void LoadSaintDragon(void)
{
   int ta;

   romset=1; spr_pri_needed=1;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("jsd-19.bin", RAM, 0x10000)) return;          // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x10000))return;

   if(!load_rom("jsd-20.bin", ROM+0x00000, 0x20000)) return;  // 16x16 SPRITES
   if(!load_rom("jsd-21.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("jsd-22.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("jsd-23.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("jsd-15.bin", ROM+0x00000, 0x20000)) return;  // 16x16 TILES
   if(!load_rom("jsd-16.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("jsd-17.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("jsd-18.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("jsd-11.bin", ROM+0x00000, 0x20000)) return;  // 16x16 TILES
   if(!load_rom("jsd-12.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("jsd-13.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("jsd-14.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeBG0(ROM,0x80000))return;

   if(!load_rom("jsd-02.bin", RAM, 0x20000)) return;          // MAIN 68000
   for(ta=0;ta<0x20000;ta++){
      ROM[ta+ta]=RAM[ta];
   }
   if(!load_rom("jsd-01.bin", RAM, 0x20000)) return;
   for(ta=0;ta<0x20000;ta++){
      ROM[ta+ta+1]=RAM[ta];
   }
   DecodeStDragon(ROM);                                                 // Deprotection

   if(!load_rom("jsd-05.bin", RAM, 0x10000)) return;          // SUB 68000
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60000]=RAM[ta];
   }
   if(!load_rom("jsd-06.bin", RAM, 0x10000)) return;
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60001]=RAM[ta];
   }

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("jsd-09.bin",PCMROM+0x00000,0x20000)) return;
   if(!load_rom("jsd-10.bin",PCMROM+0x20000,0x20000)) return;
   if(!load_rom("jsd-07.bin",PCMROM+0x40000,0x20000)) return;
   if(!load_rom("jsd-08.bin",PCMROM+0x60000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 8;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   // Checksum Fix
   // ------------

   ROM[0x00482]=0x60;
   WriteLong68k(&ROM[0x0045C],0x4E714E71);

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x007FA],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x000C0],0x51CF0002+12);   //
   WriteLong68k(&ROM[0x000C4],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x000C8],0x00AA0000);      //
   WriteLong68k(&ROM[0x000CC],0x4EF807FE);      //      jmp     $7FE.w
   WriteLong68k(&ROM[0x000D0],0x4EF807EC);      //      jmp     $7EC.w

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x60464],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x600C0],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x600C4],0x00AA0000);      //
   WriteWord68k(&ROM[0x600C8],0x6100-10);       //      bra.s   <loop>

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x60000, 0x30000, 0x0F0000);

   AddMS1Controls();
}

void load_iga_ninjyutsuden(void)
{
   int ta;

   romset=20; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("iga_19.bin", RAM, 0x10000)) return;          // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x10000))return;

   if(!load_rom("iga_23.bin", ROM+0x00000, 0x80000)) return;  // 16x16 SPRITES
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("iga_18.bin", ROM+0x00000, 0x80000)) return;  // 16x16 TILES
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("iga_14.bin", ROM+0x00000, 0x40000)) return;  // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x80000))return;

   if(!load_rom("iga_02.bin", RAM, 0x20000)) return;          // MAIN 68000
   for(ta=0;ta<0x20000;ta++){
      ROM[ta+ta]=RAM[ta];
   }
   if(!load_rom("iga_01.bin", RAM, 0x20000)) return;
   for(ta=0;ta<0x20000;ta++){
      ROM[ta+ta+1]=RAM[ta];
   }
   DecodeStDragon(ROM);                                       // Deprotection

   if(!load_rom("iga_03.bin", RAM, 0x10000)) return;          // MAIN 68000
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x40000]=RAM[ta];
   }
   if(!load_rom("iga_04.bin", RAM, 0x10000)) return;
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x40001]=RAM[ta];
   }

   if(!load_rom("iga_05.bin", RAM, 0x10000)) return;          // SUB 68000
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60000]=RAM[ta];
   }
   if(!load_rom("iga_06.bin", RAM, 0x10000)) return;
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60001]=RAM[ta];
   }

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("iga_10.bin",PCMROM+0x00000,0x40000)) return;
   if(!load_rom("iga_08.bin",PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 24;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;

   // Protection Fix

   ROM[0x00888]=0x60;
   WriteLong68k(&ROM[0x11786],0x4E714E71);

   // 68000 Speed hack

   WriteLong68k(&ROM[0x10046],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x1004A],0x00AA0000);      //

   // Sub 68000

   ROM[0x60000]=0x00;

   WriteLong68k(&ROM[0x60494],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x60498],0x00AA0000);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x60000, 0x30000, 0x0F0000);

   AddMS1Controls();
}

static void LoadP47J(void)
{
   romset=2; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!MS1DecodeFG0(load_region[REGION_GFX3],0x10000))return;

   if(!load_rom("p47j_27.bin", ROM+0x00000, 0x20000)) return;             // 16x16 SPRITES
   if(!load_rom("p47j_18.bin", ROM+0x20000, 0x20000)) return;             // *
   if(!load_rom("p47j_26.bin", ROM+0x40000, 0x20000)) return;             // <Blank>
   if(!load_rom("p47j_26.bin", ROM+0x60000, 0x20000)) return;             // *
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("p47j_23.bin", ROM+0x00000, 0x20000)) return;             // 16x16 TILES
   if(!load_rom("p47j_23.bin", ROM+0x20000, 0x20000)) return;             // <Blank>
   if(!load_rom("p47j_12.bin", ROM+0x40000, 0x20000)) return;             // *
   if(!load_rom("p47j_12.bin", ROM+0x60000, 0x20000)) return;             // <Blank>
   if(!MS1DecodeBG1(ROM,0x60000))return;

   if(!load_rom("p47j_5.bin", ROM+0x00000, 0x20000)) return;              // 16x16 TILES
   if(!load_rom("p47j_6.bin", ROM+0x20000, 0x20000)) return;              // *
   if(!load_rom("p47j_7.bin", ROM+0x40000, 0x20000)) return;              // *
   if(!MS1DecodeBG0(ROM,0x60000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_ROM1];

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("p47j_20.bin",PCMROM+0x00000,0x20000)) return;
   if(!load_rom("p47j_21.bin",PCMROM+0x20000,0x20000)) return;
   if(!load_rom("p47j_10.bin",PCMROM+0x40000,0x20000)) return;
   if(!load_rom("p47j_11.bin",PCMROM+0x60000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 24;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   // Checksum Fix
   // ------------

   ROM[0x011AE]=0x60;
   ROM[0x074D2]=0x60;

   WriteLong68k(&ROM[0x004CE],0x33FC0424);
   WriteLong68k(&ROM[0x004D2],0x000F01E6);

   WriteWord68k(&ROM[0x0103C],1);
   WriteWord68k(&ROM[0x01134],1);
   WriteWord68k(&ROM[0x01164],1);

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x006DE],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x000C0],0x51CF0002+12);   //
   WriteLong68k(&ROM[0x000C4],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x000C8],0x00AA0000);      //
   WriteLong68k(&ROM[0x000CC],0x4EF806E2);      //      jmp     $6E2.w
   WriteLong68k(&ROM[0x000D0],0x4EF806D0);      //      jmp     $6D0.w

   // Sub 68000
   // ---------

   ROM[0x60000]=0x00;

   WriteLong68k(&ROM[0x60494],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x60498],0x00AA0000);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x60000, 0x30000, 0x0F0000);

   AddMS1Controls();
}

static void LoadPhantasm(void)
{
   romset=3; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("spirit09.rom", RAM, 0x20000)) return;                // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("spirit10.rom", ROM, 0x80000)) return;                // 16x16 SPRITES
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("spirit11.rom", ROM, 0x80000)) return;                // 16x16 TILES
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("spirit12.rom", ROM, 0x80000)) return;                // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x80000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_ROM1];

   DecodeStDragon(ROM);                                               // Deprotection

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("spirit14.rom",PCMROM+0x00000,0x40000)) return;
   if(!load_rom("spirit13.rom",PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 10;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   // Checksum Fix
   // ------------

   ROM[0x0EDA]=0x60;

   //WriteWord68k(&ROM[0x03CCE],0x4E71);

   // 68000 Speed hack
   // ----------------
/*
   WriteLong68k(&ROM[0x0080C],0x4EF800D0);      //      jmp     $00D0.w
   WriteLong68k(&ROM[0x000D0],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x000D4],0x00AA0000);      //
   WriteLong68k(&ROM[0x000D8],0x4EF80810);      //      jmp     $0810.w

   WriteLong68k(&ROM[0x03CCA],0x4EF800C0);      //      jmp     $00C0.w
   WriteLong68k(&ROM[0x000C0],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x000C4],0x00AA0000);      //
   WriteLong68k(&ROM[0x000C8],0x4EF83CD0);      //      jmp     $3CD0.w
*/
   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x804AA],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x804AE],0x00AA0000);      //
   WriteLong68k(&ROM[0x804B2],0x4E714E71);      //


/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xa0000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0xFF0000);

   AddMS1SoundCPU(0x80000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

void LoadKickOff(void)
{
   romset=4; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("kioff16.rom", RAM, 0x20000)) return;          // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("kioff27.rom", ROM+0x00000, 0x20000)) return;  // 16x16 SPRITES
   if(!load_rom("kioff18.rom", ROM+0x20000, 0x20000)) return;
   if(!load_rom("kioff17.rom", ROM+0x40000, 0x20000)) return;
   if(!load_rom("kioff26.rom", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("kioff05.rom", ROM+0x00000, 0x20000)) return;  // 16x16 TILES
   if(!load_rom("kioff06.rom", ROM+0x20000, 0x20000)) return;
   if(!load_rom("kioff07.rom", ROM+0x40000, 0x20000)) return;
   if(!MS1DecodeBG0(ROM,0x60000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_ROM1];

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x60000))) return;
   if(!load_rom("kioff20.rom",PCMROM+0x00000,0x20000)) return;
   if(!load_rom("kioff21.rom",PCMROM+0x20000,0x20000)) return;
   if(!load_rom("kioff10.rom",PCMROM+0x40000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 26;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   // Int Fix
   // -------

   WriteWord68k(&ROM[0x00336],0x4E71);
   WriteWord68k(&ROM[0x003A0],0x2000);
   //WriteWord68k(&ROM[0x00338],0x5479);

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x003FE],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x00402],0x00AA0000);      //

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x20AA0],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x200C0],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x200C4],0x00AA0000);      //
   WriteLong68k(&ROM[0x200C8],0x4EF805B4);      //      jmp     $5B4.w

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x40000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x20000, 0x30000, 0x0F0000);

   AddMS1Controls();
}

void LoadHachoo(void)
{
   romset=5; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("hacho19.rom", RAM, 0x20000)) return;           // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("hacho20.rom", ROM+0x00000, 0x20000)) return;   // 16x16 SPRITES
   if(!load_rom("hacho21.rom", ROM+0x20000, 0x20000)) return;
   if(!load_rom("hacho22.rom", ROM+0x40000, 0x20000)) return;
   if(!load_rom("hacho23.rom", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("hacho15.rom", ROM+0x00000, 0x20000)) return;   // 16x16 TILES
   if(!load_rom("hacho16.rom", ROM+0x20000, 0x20000)) return;
   if(!load_rom("hacho17.rom", ROM+0x40000, 0x20000)) return;
   if(!load_rom("hacho18.rom", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("hacho14.rom", ROM+0x00000, 0x80000)) return;   // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x80000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_ROM1];

   DecodePlusAlpha(ROM);                                                // Deprotection

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("hacho09.rom",PCMROM+0x00000,0x20000)) return;
   if(!load_rom("hacho10.rom",PCMROM+0x20000,0x20000)) return;
   if(!load_rom("hacho07.rom",PCMROM+0x40000,0x20000)) return;
   if(!load_rom("hacho08.rom",PCMROM+0x60000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 13;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   // Protection Fix
   // --------------

   ROM[0x006DA]=0x60;

   WriteLong68k(&ROM[0x04446],0x4E714E71);
   WriteLong68k(&ROM[0x0446E],0x4E714E71);
   WriteWord68k(&ROM[0x043FE],0x4E71);

   // 68000 Speed hack
   // ----------------

   //WriteLong68k(&ROM[0x008A6],0x13FC0000);    //      move.b  #$00,$AA0000
   //WriteLong68k(&ROM[0x008AA],0x00AA0000);    //
   WriteWord68k(&ROM[0x008B0],0x4E71);          //

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x604B0],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x600C0],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x600C4],0x00AA0000);      //
   WriteLong68k(&ROM[0x600C8],0x30390004);      //
   WriteWord68k(&ROM[0x600CC],0x0000);          //
   WriteLong68k(&ROM[0x600CE],0x4EF804B6);      //      jmp     $4B6.w

   /*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPUHachoo(0x0F0000);

   AddMS1Controls();
}

void LoadPlusAlpha(void)
{
   int ta;

   romset=6; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("pa-rom19.bin", RAM, 0x10000)) return;                // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x10000))return;

   if(!load_rom("pa-rom20.bin", ROM+0x00000, 0x20000)) return;        // 16x16 SPRITES
   if(!load_rom("pa-rom21.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("pa-rom22.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("pa-rom23.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("pa-rom15.bin", ROM+0x00000, 0x20000)) return;        // 16x16 TILES
   if(!load_rom("pa-rom16.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("pa-rom17.bin", ROM+0x40000, 0x20000)) return;
   if(!MS1DecodeBG1(ROM,0x60000))return;

   if(!load_rom("pa-rom11.bin", ROM+0x00000, 0x20000)) return;        // 16x16 TILES
   if(!load_rom("pa-rom12.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("pa-rom13.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("pa-rom14.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeBG0(ROM,0x80000))return;

   if(!load_rom("pa-rom2.bin", RAM, 0x20000)) return;         // MAIN 68000
   for(ta=0;ta<0x20000;ta++){
      ROM[ta+ta]=RAM[ta];
   }
   if(!load_rom("pa-rom1.bin", RAM, 0x20000)) return;
   for(ta=0;ta<0x20000;ta++){
      ROM[ta+ta+1]=RAM[ta];
   }
   DecodePlusAlpha(ROM);                                                        // Deprotection

   if(!load_rom("pa-rom3.bin", RAM, 0x10000)) return;         // MAIN 68000
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x40000]=RAM[ta];
   }
   if(!load_rom("pa-rom4.bin", RAM, 0x10000)) return;
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x40001]=RAM[ta];
   }

   if(!load_rom("pa-rom5.bin", RAM, 0x10000)) return;         // SUB 68000
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60000]=RAM[ta];
   }
   if(!load_rom("pa-rom6.bin", RAM, 0x10000)) return;
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60001]=RAM[ta];
   }

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "pa-rom9.bin", PCMROM+0x00000,0x20000)) return;
   if(!load_rom( "pa-rom10.bin",PCMROM+0x20000,0x20000)) return;
   if(!load_rom( "pa-rom7.bin", PCMROM+0x40000,0x20000)) return;
   if(!load_rom( "pa-rom8.bin", PCMROM+0x60000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 7;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   // Checksum Fix
   // ------------

   WriteLong68k(&ROM[0x012A4],0x4E714E71);

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x008BE],0x4EF80100);      //      jmp     $100.w

   WriteWord68k(&ROM[0x00100],0x4279);          //      clr     EXT_0375
   WriteLong68k(&ROM[0x00102],0x000F0008);      //
   WriteLong68k(&ROM[0x00106],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x0010A],0x00AA0000);      //
   WriteWord68k(&ROM[0x0010E],0x4A79);          //      tst     EXT_0375
   WriteLong68k(&ROM[0x00110],0x000F0008);      //
   WriteWord68k(&ROM[0x00114],0x67F8-8);        //      beq.S   LAB_001A
   WriteWord68k(&ROM[0x00116],0x4E75);          //      rts

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x604AA],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x604AE],0x00AA0000);      //
   WriteLong68k(&ROM[0x604B2],0x4E714E71);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x60000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

static void LoadAvengingSpirit(void)
{
   romset=7; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0xA0000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("spirit09.rom", RAM, 0x20000)) return;                // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("spirit10.rom", ROM, 0x80000)) return;                // 16x16 SPRITES
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("spirit11.rom", ROM, 0x80000)) return;                // 16x16 TILES
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("spirit12.rom", ROM, 0x80000)) return;                // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x80000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_ROM1];

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "spirit14.rom", PCMROM+0x00000,0x40000)) return;
   if(!load_rom( "spirit13.rom", PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 10;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   ROM[0x0105A]=0x60;                           // Watchdog fix
   WriteWord68k(&ROM[0x0006A],0x1052);          // Use Int#2 instead of 4

   ROM[0x0742]=0x60;
   WriteWord68k(&ROM[0x00FD6],0x4E75);
   WriteWord68k(&ROM[0x00FFA],0x4E75);

   WriteLong68k(&ROM[0x010E8],0x30390004);      //      move    $xxxx,d0
   WriteLong68k(&ROM[0x010EC],0x00024E71);      //

   WriteLong68k(&ROM[0x010FC],0x30390004);      //      move    $xxxx,d0
   WriteLong68k(&ROM[0x01100],0x00044E71);      //

   WriteLong68k(&ROM[0x01122],0x30390004);      //      move    $xxxx,d0
   WriteLong68k(&ROM[0x01126],0x00064E71);      //

   WriteLong68k(&ROM[0x01134],0x30390004);      //      move    $xxxx,d0
   WriteLong68k(&ROM[0x01138],0x00004E71);      //

   WriteLong68k(&ROM[0x01152],0x30390004);      //      move    $xxxx,d0
   WriteLong68k(&ROM[0x01156],0x00004E71);      //
/*
   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x007FA],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x000C0],0x51CF0002+12);   //
   WriteLong68k(&ROM[0x000C4],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x000C8],0x00AA0000);      //
   WriteLong68k(&ROM[0x000CC],0x4EF807FE);      //      jmp     $7FE.w
   WriteLong68k(&ROM[0x000D0],0x4EF807EC);      //      jmp     $7EC.w
*/
   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x804AA],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x804AE],0x00AA0000);      //
   WriteLong68k(&ROM[0x804B2],0x4E714E71);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xA0000);
   ByteSwap(RAM,0x40000);

   AddMemFetch(0x000000, 0x03FFFF, ROM+0x000000-0x000000);      // 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x03FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadByte(0x070000, 0x07FFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadByte(0x080000, 0x0BFFFF, NULL, ROM+0x040000);                 // DATA ROM
   AddReadByte(0x040000, 0x05FFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x03FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadWord(0x070000, 0x07FFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadWord(0x080000, 0x0BFFFF, NULL, ROM+0x040000);                 // DATA ROM
   AddReadWord(0x040000, 0x05FFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(0x070000, 0x07FFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteByte(0x040000, 0x05FFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0x070000, 0x07FFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteWord(0x050000, 0x05FFFF, NULL, RAM+0x020000);                // SCREEN RAM
   AddWriteWord(0x040000, 0x04FFFF, MS1VideoWrite, NULL);               // MISC SCREEN RAM
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...

   AddMS1SoundCPU(0x80000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

void LoadCybattler(void)
{
   UINT8 *TMP;

   romset=8; spr_pri_needed=0;

   if(!(TMP=AllocateMem(0x100000))) return;

   if(!load_rom("cb_09.rom", TMP, 0x20000)) return;           // 8x8 FG0 TILES
   if(!MS1DecodeFG0(TMP,0x020000)) return;

   if(!load_rom("cb_m01.rom", TMP, 0x80000)) return;          // 16x16 TILES
   if(!MS1DecodeBG0(TMP,0x080000)) return;

   if(!load_rom("cb_m03.rom", TMP+0x00000, 0x80000)) return;  // 16x16 SPRITES
   if(!load_rom("cb_m02.rom", TMP+0x80000, 0x80000)) return;  // 16x16 SPRITES
   if(!MS1DecodeSPR(TMP,0x100000)) return;

   if(!load_rom("cb_m04.rom", TMP, 0x80000)) return;          // 16x16 TILES
   if(!MS1DecodeBG1(TMP,0x080000)) return;

   FreeMem(TMP);

   if(!(RAM=AllocateMem(0x80000))) return;
   ROM = load_region[REGION_CPU1];

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "cb_11.rom", PCMROM+0x00000,0x40000)) return;
   if(!load_rom( "cb_10.rom", PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 8;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x60000;


   ROM[0x01AF4]=0x60;                           // Checksum fix

   WriteWord68k(&ROM[0x00498],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0049A],0x000C0000);
   WriteLong68k(&ROM[0x0049E],0x001FD2D0);

   WriteWord68k(&ROM[0x004A2],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x004A4],0x000C0002);
   WriteLong68k(&ROM[0x004A8],0x001FD2D2);

   WriteWord68k(&ROM[0x004AC],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x004AE],0x000C0004);
   WriteLong68k(&ROM[0x004B2],0x001FD2D4);

   WriteWord68k(&ROM[0x004B6],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x004B8],0x000C0006);
   WriteLong68k(&ROM[0x004BC],0x001FD2D6);

   WriteWord68k(&ROM[0x004C0],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x004C2],0x000C0007);
   WriteLong68k(&ROM[0x004C6],0x001FD2D8);

   WriteLong68k(&ROM[0x004CA],0x33FC0008);
   WriteLong68k(&ROM[0x004CE],0x001FD2C0);

   WriteWord68k(&ROM[0x004D2],0x4E73);

   WriteLong68k(&ROM[0x01568],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x0156C],0x00AA0000);      //

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x804FE],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x80502],0x00AA0000);      //
   WriteWord68k(&ROM[0x80506],0x4E71);          //

   WriteWord68k(&ROM[0x80DC4],0x4E71);          //

   //WriteWord68k(&ROM[0x80072],0x040C);                //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xA0000);
   ByteSwap(RAM,0x60000);

   AddMemFetch(0x000000, 0x07FFFF, ROM+0x000000-0x000000);      // 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x07FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadByte(0x1F0000, 0x1FFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadByte(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x07FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadWord(0x1F0000, 0x1FFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadWord(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(0x1F0000, 0x1FFFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteByte(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0x1F0000, 0x1FFFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteWord(0x0C8000, 0x0C8001, MS2SoundWrite, NULL);               // SOUND
   AddWriteWord(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...

   AddMS1SoundCPU(0x80000, 0x50000, 0x0E0000);

   AddMS2Controls();
}

void unprotect_64thstreet() {
  /* This function hacks the rom to ignore the protection */
  /* Note that the work of ip_select_r (in mame) is to decode this protection !
     But I could not find how to use this function yet !!! */

  // This one makes the game to go a little faster (why ??!)
   WriteWord68k(&ROM[0x055F6],0x4E71);
   // These two prevent black screen on startup
   WriteWord68k(&ROM[0x10F5A],0x4E73);
   WriteWord68k(&ROM[0x10FB6],0x4E75);

   WriteLong68k(&ROM[0x804B2],0x4E714E71);      //
   WriteWord68k(&ROM[0x81912],0x4E71);          //


   ROM[0x10EDE]=0x60;                           //      Watch Dog Timer


   WriteWord68k(&ROM[0x10F28],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x10F2A],0x000C0000);
   WriteLong68k(&ROM[0x10F2E],0x00FFB6C8);

   WriteWord68k(&ROM[0x10F32],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x10F34],0x000C0002);
   WriteLong68k(&ROM[0x10F38],0x00FFB6CA);

   WriteWord68k(&ROM[0x10F3C],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x10F3E],0x000C0004);
   WriteLong68k(&ROM[0x10F42],0x00FFB6CC);

   WriteWord68k(&ROM[0x10F46],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x10F48],0x000C0007);
   WriteLong68k(&ROM[0x10F4C],0x00FFB6CE);

   WriteWord68k(&ROM[0x10F50],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x10F52],0x000C0006);
   WriteLong68k(&ROM[0x10F56],0x00FFB6D0);

   // Sub 68000
   // ---------

   // Without this move, music is TOOOOO SLOW !!!
   WriteLong68k(&ROM[0x804AA],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x804AE],0x00AA0000);      //
}

static void Load64thStreet(void)
{
   UINT8 *TMP;

   romset=9; spr_pri_needed=0;

   if(!(TMP=AllocateMem(0x100000))) return;

   if(!load_rom("64th_09.rom", TMP, 0x20000)) return;         // 8x8 FG0 TILES
   if(!MS1DecodeFG0(TMP,0x020000))return;

   if(!load_rom("64th_05.rom",&TMP[0x00000],0x80000)) return; // 16x16 SPRITES
   if(!load_rom("64th_04.rom",&TMP[0x80000],0x80000)) return; // 16x16 SPRITES
   if(!MS1DecodeSPR(TMP,0x100000))return;

   if(!load_rom("64th_06.rom", TMP, 0x80000)) return;         // 16x16 TILES
   if(!MS1DecodeBG1(TMP,0x080000))return;

   if(!load_rom("64th_01.rom", TMP, 0x80000)) return;         // 16x16 TILES
   if(!MS1DecodeBG0(TMP,0x080000))return;

   FreeMem(TMP);

   if(!(RAM=AllocateMem(0x80000))) return;

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "64th_11.rom", PCMROM+0x00000,0x20000)) return;
   if(!load_rom( "64th_10.rom", PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 10;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x60000;
   unprotect_64thstreet();

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xA0000);
   ByteSwap(RAM,0x60000);

   AddMemFetch(0x000000, 0x07FFFF, ROM+0x000000-0x000000);      // 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x07FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadByte(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadByte(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x07FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadWord(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadWord(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteByte(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteWord(0x0C8000, 0x0C8001, MS2SoundWrite, NULL);               // SOUND
   AddWriteWord(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...

   AddMS1SoundCPU(0x80000, 0x50000, 0x0E0000);

   AddMS2Controls();
}

void load_chimera_beast(void)
{
   UINT8 *TMP;

   romset=18; spr_pri_needed=0;

   if(!(TMP=AllocateMem(0x100000))) return;

   if(!load_rom("scr3.bin", TMP, 0x20000)) return;         // 8x8 FG0 TILES
   if(!MS1DecodeFG0(TMP,0x020000))return;

   if(!load_rom("b2.bin", TMP+0x00000, 0x80000)) return;   // 16x16 SPRITES
   if(!load_rom("b1.bin", TMP+0x80000, 0x80000)) return;   // 16x16 SPRITES
   if(!MS1DecodeSPR(TMP,0x100000))return;

   if(!load_rom("s2.bin", TMP, 0x80000)) return;           // 16x16 TILES
   if(!MS1DecodeBG1(TMP,0x080000))return;

   if(!load_rom("s1.bin", TMP, 0x80000)) return;           // 16x16 TILES
   if(!MS1DecodeBG0(TMP,0x080000))return;

   FreeMem(TMP);

   if(!(RAM=AllocateMem(0x80000))) return;
   ROM = load_region[REGION_CPU1];

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "voi11.bin", PCMROM+0x00000,0x40000)) return;
   if(!load_rom( "voi10.bin", PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 8;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x60000;

   WriteWord68k(&ROM[0x0EB06],0x4E75);

   WriteWord68k(&ROM[0x0CF9C],0x4E71);
/*
	dc.w	$FA56			;0EAC2
	dc.w	$FA58			;0EAC4
	dc.w	$FA5A			;0EAC6
	dc.w	$FA5C			;0EAC8
	dc.w	$FA5E			;0EACA
*/
   WriteWord68k(&ROM[0x0EA8C],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0EA8E],0x000C0000);
   WriteLong68k(&ROM[0x0EA92],0x00FFFA56);

   WriteWord68k(&ROM[0x0EA96],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0EA98],0x000C0002);
   WriteLong68k(&ROM[0x0EA9C],0x00FFFA58);

   WriteWord68k(&ROM[0x0EAA0],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0EAA2],0x000C0004);
   WriteLong68k(&ROM[0x0EAA6],0x00FFFA5A);

   WriteWord68k(&ROM[0x0EAAA],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0EAAC],0x000C0006);
   WriteLong68k(&ROM[0x0EAB0],0x00FFFA5C);

   WriteWord68k(&ROM[0x0EAB4],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0EAB6],0x000C0007);
   WriteLong68k(&ROM[0x0EABA],0x00FFFA5E);

   WriteWord68k(&ROM[0x0EABE],0x4E73);

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x804B0],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x804B4],0x00AA0000);      //
   WriteLong68k(&ROM[0x804B8],0x4E714E71);      //

   WriteWord68k(&ROM[0x81918],0x4E71);          //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xA0000);
   ByteSwap(RAM,0x60000);

   AddMemFetch(0x000000, 0x07FFFF, ROM+0x000000-0x000000);      	// 68000 ROM
   AddMemFetch(0xFF0000, 0xFFFFFF, RAM+0x000000-0xFF0000);              // 68000 RAM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x07FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadByte(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadByte(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x07FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadWord(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadWord(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteByte(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0xFF0000, 0xFFFFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteWord(0x0C8000, 0x0C8001, MS2SoundWrite, NULL);               // SOUND
   AddWriteWord(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...

   AddMS1SoundCPU(0x80000, 0x50000, 0x0E0000);

   AddMS2Controls();
}

void LoadEarthDefForce(void)
{

   romset=10; spr_pri_needed=1;

   if(!(ROM=AllocateMem(0xA0000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("edf_09.rom", RAM, 0x20000)) return;               // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("edf_m03.rom", ROM, 0x80000)) return;              // 16x16 SPRITES
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("edf_m04.rom", ROM, 0x80000)) return;              // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x80000))return;

   if(!load_rom("edf_m05.rom", ROM, 0x80000)) return;              // 16x16 TILES
   if(!MS1DecodeBG1(ROM,0x80000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_CPU1];

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "edf_m02.rom", PCMROM+0x00000,0x40000)) return;
   if(!load_rom( "edf_m01.rom", PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 10;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;


   WriteLong68k(&ROM[0x00000],0x0006FFFE);      // Stack

   WriteWord68k(&ROM[0x00066],0x0428);          // Use Int#1 instead of 4

   ROM[0x05478]=0x60;

   WriteWord68k(&ROM[0x05456],0x4E75);

   WriteLong68k(&ROM[0x05488],0x4EF8554C);      //      jmp     $C0.w

   WriteLong68k(&ROM[0x0573E],0x4EF8554C);      //      jmp     $C0.w

   WriteLong68k(&ROM[0x058AE],0x4EF8554C);      //      jmp     $C0.w

   WriteWord68k(&ROM[0x0554C],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0554E],0x00040000);
   WriteLong68k(&ROM[0x05552],0x00066000);

   WriteWord68k(&ROM[0x05556],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x05558],0x00040002);
   WriteLong68k(&ROM[0x0555C],0x00066002);

   WriteWord68k(&ROM[0x05560],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x05562],0x00040004);
   WriteLong68k(&ROM[0x05566],0x00066004);

   WriteWord68k(&ROM[0x0556A],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x0556C],0x00040006);
   WriteLong68k(&ROM[0x05570],0x00066006);

   WriteWord68k(&ROM[0x05574],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x05576],0x00040008);
   WriteLong68k(&ROM[0x0557A],0x00066008);

   WriteWord68k(&ROM[0x0557E],0x4E75);

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x0720],0x13FC0000);       //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x0724],0x00AA0000);       //

   WriteLong68k(&ROM[0x1B0A],0x13FC0000);       //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x1B0E],0x00AA0000);       //

   WriteLong68k(&ROM[0x2138],0x13FC0000);       //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x213C],0x00AA0000);       //

   WriteLong68k(&ROM[0x6EFC],0x13FC0000);       //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x6F00],0x00AA0000);       //

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x804AA],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x804AE],0x00AA0000);      //
   WriteLong68k(&ROM[0x804B2],0x4E714E71);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xA0000);
   ByteSwap(RAM,0x40000);

   AddMemFetch(0x000000, 0x03FFFF, ROM+0x000000-0x000000);      // 68000 ROM
   AddMemFetch(0x080000, 0x0BFFFF, ROM+0x040000-0x080000);      // 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x03FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadByte(0x060000, 0x06FFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadByte(0x080000, 0x0BFFFF, NULL, ROM+0x040000);                 // DATA ROM
   AddReadByte(0x040000, 0x05FFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x03FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddReadWord(0x060000, 0x06FFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadWord(0x080000, 0x0BFFFF, NULL, ROM+0x040000);                 // DATA ROM
   AddReadWord(0x040000, 0x05FFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(0x060000, 0x06FFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteByte(0x040000, 0x05FFFF, NULL, RAM+0x010000);                // SCREEN RAM
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0x060000, 0x06FFFF, NULL, RAM+0x000000);                // 68000 RAM
   AddWriteWord(0x050000, 0x05FFFF, NULL, RAM+0x020000);                // SCREEN RAM
   AddWriteWord(0x040000, 0x04FFFF, MS1VideoWrite, NULL);               // MISC SCREEN RAM
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...

   AddMS1SoundCPU(0x80000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

void LoadShingen(void)
{
   romset=11; spr_pri_needed=1;


   RAM = load_region[REGION_GFX3];         // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x10000))return;
   FreeMem(RAM);
   if(!(RAM=AllocateMem(0x60000))) return;

   ROM = load_region[REGION_GFX4];
   if(!MS1DecodeSPR(ROM,0x80000))return;
   FreeMem(ROM);

   ROM = load_region[REGION_GFX2];
   if(!MS1DecodeBG1(ROM,0x60000))return;
   FreeMem(ROM);

   ROM = load_region[REGION_GFX1];
   if(!MS1DecodeBG0(ROM,0x40000))return;
   FreeMem(ROM);

   ROM = load_region[REGION_CPU1];
   DecodeStDragon(ROM);                                                 // Deprotection

   ROM[0]=0;    // Garbage

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom( "takeda9.bin", PCMROM+0x00000,0x20000)) return;
   if(!load_rom( "takeda10.bin", PCMROM+0x20000,0x20000)) return;
   if(!load_rom( "shing_07.rom", PCMROM+0x40000,0x20000)) return;
   if(!load_rom( "shing_08.rom", PCMROM+0x60000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 24;                   // Antiriad: Change from 8
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;

   // 68000 Speed hack
   // ----------------

   if (!strcmp(current_game->main_name,"tshingna")) {
     WriteLong68k(&ROM[0x00CB4],0x4EF800C0);      //      jmp     $C0.w
     WriteLong68k(&ROM[0x000C0],0x13FC0000);      //      move.b  #$00,$AA0000
     WriteLong68k(&ROM[0x000C4],0x00AA0000);      //
     WriteLong68k(&ROM[0x000C8],0x4EF80A7E);      //      jmp     $A7E.w
   } else {
     // tshingen
     WriteLong68k(&ROM[0x00CFA],0x4EF800C0);      //      jmp     $C0.w
     WriteLong68k(&ROM[0x000C0],0x13FC0000);      //      move.b  #$00,$AA0000
     WriteLong68k(&ROM[0x000C4],0x00AA0000);      //
     WriteLong68k(&ROM[0x000C8],0x4EF80AC4);      //      jmp     $A7E.w
   }
   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x60486],0x4EF800C0);      //      jmp     $00C0.w
   WriteLong68k(&ROM[0x600C0],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x600C4],0x00AA0000);      //
   WriteLong68k(&ROM[0x600C8],0x4EF8048A);      //      jmp     $048A.w

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddInitMemory();     // Set Starscream mem pointers...

   AddMS1SoundCPU(0x60000, 0x30000, 0x0F0000);

   AddMS1Controls();
}

void LoadLegendOfMakaj(void)
{
   romset=12; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom_index(3, RAM, 0x10000)) return;           // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x10000))return;

   if(!load_rom_index(2, ROM, 0x20000)) return;           // 16x16 SPRITES
   if(!MS1DecodeSPR(ROM,0x20000))return;

   if(!load_rom_index(1, ROM, 0x20000)) return;           // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x20000))return;

   /*-------[SOUND SYSTEM INIT]-------*/

   // Apply Speed Patch
   // -----------------

   Z80ROM[0x0111]=0xD3; // OUTA (AAh)
   Z80ROM[0x0112]=0xAA; //

   SetStopZ80Mode2(0x0111);

   Z80ROM[0x004A]=0x00;
   Z80ROM[0x004B]=0x00;

   // Setup Z80 memory map
   // --------------------

   AddZ80AROMBase(Z80ROM, 0x0038, 0x0066);

   AddZ80AReadByte(0x0000, 0xC7FF, NULL,                        Z80ROM+0x0000); // Z80 ROM+RAM
   AddZ80AReadByte(0xE000, 0xE000, SubSoundReadZ80,             NULL);          // 68000
   AddZ80AReadByte(0x0000, 0xFFFF, DefBadReadZ80,               NULL);          // <bad reads>
   AddZ80AReadByte(-1, -1, NULL, NULL);

   AddZ80AWriteByte(0xC000, 0xC7FF, NULL,                       Z80ROM+0xC000); // Z80 RAM
   AddZ80AWriteByte(0x0000, 0xFFFF, DefBadWriteZ80,             NULL);          // <bad writes>
   AddZ80AWriteByte(-1, -1, NULL, NULL);

   AddZ80AReadPort(0x00, 0x01, YM2203AReadZ80,          NULL);
   AddZ80AReadPort(0x00, 0xFF, DefBadReadZ80,           NULL);
   AddZ80AReadPort(  -1,   -1, NULL,                    NULL);

   AddZ80AWritePort(0x00, 0x01, YM2203AWriteZ80,        NULL);
   AddZ80AWritePort(0xAA, 0xAA, StopZ80Mode2,           NULL);
   AddZ80AWritePort(0x00, 0xFF, DefBadWriteZ80,         NULL);
   AddZ80AWritePort(  -1,   -1, NULL,                   NULL);

   AddZ80AInit();

   /*---------------------------------*/

   RAMSize=0x40000;

   FreeMem(ROM);
   ROM = load_region[REGION_CPU1];

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x00C24],0x4EF800C0);      //      jmp     $C0.w
   WriteLong68k(&ROM[0x000C0],0x51CF0002+12);   //
   WriteLong68k(&ROM[0x000C4],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x000C8],0x00AA0000);      //
   WriteLong68k(&ROM[0x000CC],0x4EF80C28);      //      jmp     $C28.w
   WriteLong68k(&ROM[0x000D0],0x4EF80C16);      //      jmp     $C16.w

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,get_region_size(REGION_ROM1));
   ByteSwap(RAM,0x30000);

   AddMS1MainCPU(0x0F0000);

   AddMS1Controls();
}

static void load_astyanax(void)
{
   romset=13; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("astyan19.bin", RAM, 0x20000)) return;                // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("astyan20.bin", ROM+0x00000, 0x20000)) return;        // 16x16 SPRITES
   if(!load_rom("astyan21.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("astyan22.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("astyan23.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("astyan15.bin", ROM+0x00000, 0x20000)) return;        // 16x16 TILES
   if(!load_rom("astyan16.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("astyan17.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("astyan18.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("astyan11.bin", ROM+0x00000, 0x20000)) return;        // 16x16 TILES
   if(!load_rom("astyan12.bin", ROM+0x20000, 0x20000)) return;
   if(!load_rom("astyan13.bin", ROM+0x40000, 0x20000)) return;
   if(!load_rom("astyan14.bin", ROM+0x60000, 0x20000)) return;
   if(!MS1DecodeBG0(ROM,0x80000))return;

   FreeMem(ROM);
   ROM = load_region[REGION_CPU1];
   DecodePlusAlpha(ROM);                                                // Deprotection

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("astyan9.bin", PCMROM+0x00000,0x20000)) return;
   if(!load_rom("astyan10.bin",PCMROM+0x20000,0x20000)) return;
   if(!load_rom("astyan7.bin", PCMROM+0x40000,0x20000)) return;
   if(!load_rom("astyan8.bin", PCMROM+0x60000,0x20000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 10;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;



   // Checksum Fix
   // ------------

   ROM[0x11E26]=0x60;
   ROM[0x004E6]=0x60;

   WriteLong68k(&ROM[0x11DDC],0x00FF0000);      //       20/24-bit address fix

   // Sub 68000
   // ---------

   WriteLong68k(&ROM[0x804AA],0x13FC0000);      //      move.b  #$00,$AA0000
   WriteLong68k(&ROM[0x804AE],0x00AA0000);      //
   WriteLong68k(&ROM[0x804B2],0x4E714E71);      //      nop

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0xa0000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0xFF0000);

   AddMS1SoundCPU(0x80000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

void load_soldam(void)
{
   int ta;

   romset=21; spr_pri_needed=0;

   if(!(ROM=AllocateMem(0x80000))) return;
   if(!(RAM=AllocateMem(0x60000))) return;

   if(!load_rom("soldam19.bin", RAM, 0x20000)) return;           // 8x8 FG0 TILES
   if(!MS1DecodeFG0(RAM,0x20000))return;

   if(!load_rom("soldam23.bin", ROM+0x00000, 0x80000)) return;   // 16x16 SPRITES
   if(!MS1DecodeSPR(ROM,0x80000))return;

   if(!load_rom("soldam18.bin", ROM+0x00000, 0x80000)) return;   // 16x16 TILES
   if(!MS1DecodeBG1(ROM,0x80000))return;

   if(!load_rom("soldam14.bin", ROM+0x00000, 0x80000)) return;   // 16x16 TILES
   if(!MS1DecodeBG0(ROM,0x80000))return;

   if(!load_rom("soldam2.bin", RAM+0x00000, 0x20000)) return;   // MAIN 68000
   if(!load_rom("soldam3.bin", RAM+0x20000, 0x10000)) return;
   for(ta=0;ta<0x30000;ta++){
      ROM[ta+ta]=RAM[ta];
   }
   if(!load_rom("soldam1.bin", RAM+0x00000, 0x20000)) return;
   if(!load_rom("soldam4.bin", RAM+0x20000, 0x10000)) return;
   for(ta=0;ta<0x30000;ta++){
      ROM[ta+ta+1]=RAM[ta];
   }
   DecodePlusAlpha(ROM);

   if(!load_rom("soldam5.bin", RAM, 0x10000)) return;           // SUB 68000
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60000]=RAM[ta];
   }
   if(!load_rom("soldam6.bin", RAM, 0x10000)) return;
   for(ta=0;ta<0x10000;ta++){
      ROM[ta+ta+0x60001]=RAM[ta];
   }

   /*-----[Sound Setup]-----*/

   SoundWorkInit();             /* sound call work init */

   if(!(PCMROM = AllocateMem(0x80000))) return;
   if(!load_rom("soldam10.bin",PCMROM+0x00000,0x40000)) return;
   if(!load_rom("soldam8.bin",PCMROM+0x40000,0x40000)) return;
   ADPCMSetBuffers(((struct ADPCMinterface*)&m6295_interface),PCMROM,0x40000);

   MS1SoundLoop = 7;
   MS1SoundClock = DEF_MS1_SOUNDCLOCK / MS1SoundLoop; /* hiro-shi!! */

   /*-----------------------*/

   RAMSize=0x40000;

   // 68000 Speed hack
   // ----------------

   WriteLong68k(&ROM[0x009B0],0x13FC0000);       // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x009B4],0x00AA0000);       //

   // Sub 68000
   // ---------

   WriteWord68k(&ROM[0x6195A],0x4E75);          // rts

   WriteLong68k(&ROM[0x604BA],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x604BE],0x00AA0000);      //
   WriteLong68k(&ROM[0x604C2],0x4E714E71);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x80000);
   ByteSwap(RAM,0x40000);

   AddMS1MainCPU(0x0F0000);

   AddMS1SoundCPU(0x60000, 0x30000, 0x0E0000);

   AddMS1Controls();
}

/* These are the protection handlers for peekaboo.
   Don't see how mame uses them though !!! */

static int protection_val;

/* Read the input ports, through a protection device */
static UINT16 protection_peekaboo_r(UINT32 offset)
{
  //fprintf(stderr,"protection_r %x p1_r %x\n",protection_val,player1_r(0));
  switch (protection_val)
	{
		case 0x02:	return 0x03;
		case 0x51:	return player1_r(0);
		case 0x52:	return player2_r(0);
		default:	return protection_val;
	}
}

void protection_peekaboo_w(UINT32 offset, UINT16 data)
{
	static int bank;

	protection_val = data;

	if ((protection_val & 0x90) == 0x90)
	{
	  unsigned char *RAM = PCMROM;
	  if (RAM) {
	    int new_bank = (protection_val & 0x7) % 7;

	    if (bank != new_bank)
	      {
		memcpy(&RAM[0x20000],&RAM[0x40000 + 0x20000*new_bank],0x20000);
		bank = new_bank;
	      }
	  }
	}

	cpu_interrupt(CPU_68K_0, 4);
}

void LoadPeekABoo(void)
{
   UINT8 *TMP;

   romset=16; spr_pri_needed=0;

   if(!(TMP=AllocateMem(0x080000))) return;

   if(!load_rom("4", TMP, 0x20000)) return;         // 8x8 FG0 TILES
   if(!MS1DecodeFG0(TMP,0x020000))return;

   if(!load_rom("1", TMP, 0x80000)) return;	    // 16x16 SPRITES
   if(!MS1DecodeSPR(TMP,0x080000))return;

   if(!load_rom("5", TMP, 0x80000)) return;         // 16x16 TILES
   if(!MS1DecodeBG0(TMP,0x080000))return;

   FreeMem(TMP);

   if(!(RAM=AllocateMem(0x80000))) return;
   ROM = load_region[REGION_CPU1];
   PCMROM = load_region[REGION_SMP1];

   RAMSize=0x60000;


   // Protection/Trackballs
#if 0
   WriteWord68k(&ROM[0x00DCA],0x4E71);

   WriteWord68k(&ROM[0x04B5C],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x04B5E],0x000C4000);
   WriteLong68k(&ROM[0x04B62],0x001F003C);

   WriteWord68k(&ROM[0x04B66],0x33F9);          //      move.w src,dest
   WriteLong68k(&ROM[0x04B68],0x000C4002);
   WriteLong68k(&ROM[0x04B6C],0x001F0040);

   WriteWord68k(&ROM[0x04B70],0x4E75);

   // Checksum hack

   WriteWord68k(&ROM[0x02F1C],0x6008);
#endif
   // Speed hack

   WriteLong68k(&ROM[0x00834],0x13FC0000);      // move.b #$00,$AA0000
   WriteLong68k(&ROM[0x00838],0x00AA0000);      //

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x40000);
   ByteSwap(RAM,0x60000);

   AddMemFetch(0x000000, 0x03FFFF, ROM+0x000000-0x000000);      // 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadBW(0x000000, 0x03FFFF, NULL, ROM+0x000000);                 // 68000 ROM
   AddRWBW(0x1F0000, 0x1FFFFF, NULL, RAM+0x000000);                 // 68000 RAM
   AddReadWord(0x0f8000, 0x0f8001, OKIM6295_status_0_r, NULL);
   AddWriteWord(0x0f8000, 0x0f8001, OKIM6295_data_0_w, NULL);
   AddRWBW(0x0C0000, 0x0FFFFF, NULL, RAM+0x010000);                 // SCREEN RAM
   AddReadByte(0x100000, 0x100001, protection_peekaboo_r, NULL);
   AddReadByte(0x000000, 0xFFFFFF, DefBadReadByte, NULL);               // <Bad Reads>
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x100000, 0x100001, protection_peekaboo_r, NULL);
   AddReadWord(0x000000, 0xFFFFFF, DefBadReadWord, NULL);               // <Bad Reads>
   AddReadWord(-1, -1,NULL, NULL);

   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);                   // Trap Idle 68000
   AddWriteByte(0x100000, 0x100001, protection_peekaboo_w, NULL);
   AddWriteByte(0x000000, 0xFFFFFF, DefBadWriteByte, NULL);             // <Bad Writes>
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0x100000, 0x100001, protection_peekaboo_w, NULL);
   AddWriteWord(0x000000, 0xFFFFFF, DefBadWriteWord, NULL);             // <Bad Writes>
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();     // Set Starscream mem pointers...

   ExecuteSoundFrame=&PeekABooSoundFrame;

   memset(RAM+0x00000,0x00,0x40000);

   RAM_COL=RAM+0x2B000;
   InitPaletteMap(RAM_COL, 0x40, 0x10, 0x8000);

   set_colour_mapper(&col_map_rrrr_rggg_ggbb_bbbx_rev);

   GameMouse=1;

   layer_id_data[0] = add_layer_info(layer_id_name[0]);
   layer_id_data[1] = add_layer_info(layer_id_name[1]);
   layer_id_data[2] = add_layer_info(layer_id_name[2]);
   layer_id_data[3] = add_layer_info(layer_id_name[3]);
}

void ExecuteMegaSystem1Frame(void)
{
   //print_ingame(60,"%04x.%04x/%04x/%04x.%04x.%04x",ReadWord(&RAM[0x14000]),ReadWord(&RAM[0x14204]),ReadWord(&RAM[0x1420C]),ReadWord(&RAM[0x1400C]),ReadWord(&RAM[0x14100]),ReadWord(&RAM[0x14300]));
  cpu_execute_cycles(CPU_68K_0, CPU_FRAME_MHz(12,60));
  cpu_interrupt(CPU_68K_0, 1);
  cpu_interrupt(CPU_68K_0, 2);
  ExecuteSoundFrame();                         // 68000 *or* Z80

}

void ExecuteMegaSystem1FrameKO(void)
{
   int ta;
   //print_ingame(60,"%04x.%04x/%04x/%04x.%04x.%04x",ReadWord(&RAM[0x14000]),ReadWord(&RAM[0x14204]),ReadWord(&RAM[0x1420C]),ReadWord(&RAM[0x1400C]),ReadWord(&RAM[0x14100]),ReadWord(&RAM[0x14300]));

   for(ta=0;ta<4;ta++){
   cpu_execute_cycles(CPU_68K_0, CPU_FRAME_MHz(12,60)/4);
   cpu_interrupt(CPU_68K_0, 1);
   cpu_interrupt(CPU_68K_0, 2);
   }

   ExecuteSoundFrame();                         // 68000 *or* Z80
}

void ExecuteMegaSystem2Frame(void)
{
   cpu_execute_cycles(CPU_68K_0, CPU_FRAME_MHz(16,60));
   cpu_interrupt(CPU_68K_0, 4);
   cpu_interrupt(CPU_68K_0, 3);
   cpu_interrupt(CPU_68K_0, 2);
   cpu_interrupt(CPU_68K_0, 1);

   ExecuteSoundFrame();
}

void execute_systemd(void)
{
   static int px,px2;
   int mx,my;

   GetMouseMickeys(&mx,&my);

   px += mx/2;

   if(RAM[0x14010]) px-=6;
   if(RAM[0x14011]) px+=6;

   if(RAM[0x14020]) px2-=6;
   if(RAM[0x14021]) px2+=6;

   if(px<0x1F) px=0x1F;
   if(px>0xE0) px=0xE0;

   if(px2<0x1F) px2=0x1F;
   if(px2>0xE0) px2=0xE0;

   //print_ingame(60,"%02x %02x",px,px2);

   WriteWord(&RAM[0x014000],px);
   WriteWord(&RAM[0x014002],px2);

   cpu_execute_cycles(CPU_68K_0, CPU_FRAME_MHz(16,60));
   cpu_interrupt(CPU_68K_0, 2);
}

static UINT8 *SPRC[128];

static void MS1ChainRecalc(UINT8 *src)
{
   int ta,tb;

   for(ta=0;ta<128;ta++){
      SPRC[ta]=&src[0];
   }

   for(ta=0x7F8;ta>=0;ta-=8){
      tb=ReadWord(&src[ta])&0x7F;
      SPRC[tb]=&src[ta];
   }
}

   static UINT8 spr_pri[10][16]={
      {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},                // Render All Sprites
      {0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1},                // Render 'Low Pri' Sprites
      {1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0},                // Render 'High Pri' Sprites
      {0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0},                // Render 'High/Low Pri' Sprites
      {1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0},                // Render 'High/High Pri' Sprites
      {0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1},                // ** Shingen, EDF, Saint Dragon **
      {1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0},                // ** Shingen, EDF, Saint Dragon **
      {0,0,1,1,0,0,1,1,0,0,0,0,0,0,0,0},                // ** Hachoo **
      {1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0},                // ** Hachoo **
      {0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},                // ** Peek a Boo **
   };

   static UINT8 spr_pri_mask[10]={
      0x0F,                                             // 16 Colour Banks
      0x07,                                             //  8 Colour Banks
      0x07,                                             //  8 Colour Banks
      0x03,                                             //  4 Colour Banks
      0x03,                                             //  4 Colour Banks
      0x0F,
      0x0F,
      0x07,
      0x07,
      0x01,
   };

   static UINT8 spr_order[ROM_COUNT]={
      0,                                //  0 - Rodland
      1,                                //  1 - Saint Dragon
      0,                                //  2 - P47 Japanese
      0,                                //  3 - Phantasm
      1,                                //  4 - Kick Off
      1,                                //  5 - Hachoo
      1,                                //  6 - Plus Alpha
      0,                                //  7 - Avenging Spirit
      1,                                //  8 - Cybattler
      0,                                //  9 - 64th Street
      0,                                // 10 - Earth Defence Force
      1,                                // 11 - Shingen English
      0,                                // 12 - Legend of Makaj
      1,                                // 13 - Astyanax
      0,                                // 14 - P47 American
      0,                                // 15 - Rodland English
      1,                                // 16 - Peek a Boo
      0,                                // 17 - 64th Street Japanese
      0,                                // 18 - Chimera Beast
      1,                                // 19 - The Lord of King
      1,                                // 20 - Iga Ninjyutsuden
      0,                                // 21 - Soldam
   };

static UINT8 *SPRITE_GFX;
static UINT8 *SPRITE_MSK;

static int JalecoLayerCount;

static void RenderMS1Sprites(int pri)
{
   int x,y,r1,zz,rx,ry,ta;
   int pri_mask;
   UINT8 *pri_list;
   UINT8 *SPR1;
   UINT8 *MAP;

   if(!check_layer_enabled(layer_id_data[3]))
       return;

   if(!JalecoLayerCount){
      JalecoLayerCount = 1;
      clear_game_screen(0);
   }

   pri_mask=spr_pri_mask[pri];
   pri_list=spr_pri[pri];

   if (romset==5) {		// Hachoo
     if (pri==1)
       spr_order[romset]=0;
     else spr_order[romset]=1;
   }
   if (romset==6) {		// Plus Alpha
     if (pri==2)
       spr_order[romset]=0;
     else spr_order[romset]=1;
   }

   if(spr_order[romset]==0){

   zz=0x87F0;
   for(r1=127;r1>=0;r1--,zz-=16){               // Default Order

      if((pri_list[RAM[zz+8]&0x0F])!=0){

      x=(ReadWord(&RAM[zz+10])+32)&0x1FF;
      y=(ReadWord(&RAM[zz+12])+16)&0x1FF;
      ta=ReadWord(&RAM[zz+14])&MSK_SPR;

      if((ta!=0)||((x>32)&&(y>16)&&(x<256+32)&&(x<224+32))){    // Any sprite inside screen, only sprites!=0 outside screen

            MAP_PALETTE_MAPPED_NEW(
               (RAM[zz+8]&pri_mask)|0x30,
               16,
               MAP
            );

            SPR1=SPRC[r1];

		if (romset==5) {		// Hachoo - Portal and cave at the end of stage 1 and 3.
		  if (RAM[zz+8]==0x08) {
		    render_hachoo_spr = 0;
		    x_start = rx+2;
		    if (x_start==0xd2)
			x_start-=8;
		  }
		  else render_hachoo_spr = 1;
		}

            switch(RAM[zz+8]&0xC0){
            case 0x00:
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            case 0x40:
            SPR1+=0x0800;
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_FlipY_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            case 0x80:
            SPR1+=0x1000;
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_FlipX_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            case 0xC0:
            SPR1+=0x1800;
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_FlipXY_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            }
         }
      }
   }

   }
   else{                        // Second Order

   zz=0x8000;
   for(r1=0;r1<128;r1++,zz+=16){

      if((pri_list[RAM[zz+8]&0x0F])!=0){

      x=(ReadWord(&RAM[zz+10])+32)&0x1FF;
      y=(ReadWord(&RAM[zz+12])+16)&0x1FF;
      ta=ReadWord(&RAM[zz+14])&MSK_SPR;

      if((ta!=0)||((x>32)&&(y>16)&&(x<256+32)&&(x<224+32))){    // Any sprite inside screen, only sprites!=0 outside screen

            MAP_PALETTE_MAPPED_NEW(
               (RAM[zz+8]&pri_mask)|0x30,
               16,
               MAP
            );

            SPR1=SPRC[r1];

		if (romset==5)
		  if ((!render_hachoo_spr) && (pri==0) && (RAM[zz+8]==0x40))
		    render_hachoo_spr = 1;

            switch(RAM[zz+8]&0xC0){
            case 0x00:
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            case 0x40:
            SPR1+=0x0800;
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
		   if (!((romset==5) && (!render_hachoo_spr) &&		// when to not draw
			   (rx>=x_start) && (rx<=x_start+0x40)))
               Draw16x16_Trans_Mapped_FlipY_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            case 0x80:
            SPR1+=0x1000;
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_FlipX_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            case 0xC0:
            SPR1+=0x1800;
            while(SPR1[0]==r1){
               rx=(x+ReadWord(&SPR1[2]))&0x1FF;
               ry=(y+ReadWord(&SPR1[4]))&0x1FF;
               if((rx>16)&&(ry>16)&&(rx<256+32)&&(ry<224+32)){
               Draw16x16_Trans_Mapped_FlipXY_Rot(&SPRITE_GFX[((ta+ReadWord(&SPR1[6]))&MSK_SPR)<<8],rx,ry,MAP);
               }
               SPR1+=8;
            }
            break;
            }
         }
      }
   }

   }
}

// RenderLOMSprites():
// Hardware has no sprite chaining, colour banks 0x10-0x1F.
// No Multiple Priorities? Pre MegaSystem-1.

static void RenderLOMSprites(int pri)
{
   int x,y,r1,zz,ta;
   UINT8 *MAP;

   if(!check_layer_enabled(layer_id_data[3]))
       return;

   if(!JalecoLayerCount){
      JalecoLayerCount = 1;
      clear_game_screen(0);
   }

   zz=0x8000;
   r1=127;

   do {
      x=(ReadWord(&RAM[zz+10])+32)&0x1FF;
      y=(ReadWord(&RAM[zz+12])+16)&0x1FF;

      if((x>16)&&(y>16)&&(x<256+32)&&(y<224+32)){

            ta=ReadWord(&RAM[zz+14])&0xFFF;

            MAP_PALETTE_MAPPED_NEW(
               (RAM[zz+8]&0x0F)|0x10,
               16,
               MAP
            );

            switch(RAM[zz+8]&0xC0){
            case 0x00: Draw16x16_Trans_Mapped_Rot(&GFX_SPR[ta<<8],x,y,MAP);        break;
            case 0x40: Draw16x16_Trans_Mapped_FlipY_Rot(&GFX_SPR[ta<<8],x,y,MAP);  break;
            case 0x80: Draw16x16_Trans_Mapped_FlipX_Rot(&GFX_SPR[ta<<8],x,y,MAP);  break;
            case 0xC0: Draw16x16_Trans_Mapped_FlipXY_Rot(&GFX_SPR[ta<<8],x,y,MAP); break;
            }
      }
      zz+=16;
   } while(--r1);
}

/*

priority prom data - thanks to mamedev

*/

static UINT32 pri_data[ROM_COUNT][16]=
{
   /*    0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F       */
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  0 - Rodland
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  1 - Saint Dragon
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  2 - P47 Japanese
   { 0x14032,0x04132,0x13042,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x13042,0xfffff,0x14032,0xfffff, }, //  3 - Phantasm
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  4 - Kick Off
   {
0x24130,0x04123,0xfffff,0x02413,0x04132,0xfffff,0x24130,0x13240,0x24103,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  5 - Hachoo
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  6 - Plus Alpha
   { 0x14032,0x04132,0x13042,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x13042,0xfffff,0x14032,0xfffff, }, //  7 - Avenging Spirit
   { 0x04132,0xfffff,0xfffff,0xfffff,0x14032,0x14023,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x04132, }, //  8 - Cybattler
   { 0xfffff,0x03142,0x14032,0x04132,0xfffff,0x04132,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, //  9 - 64th Street
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 10 - Earth Defence Force
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 11 - Shingen English
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 12 - Legend of Makaj
   { 0x04132,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 13 - Astyanax
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 14 - P47 American
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 15 - Rodland English
   { 0x0134f,0x034ff,0x0341f,0x3401f,0x1340f,0x3410f,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 16 - Peek a Boo
   { 0xfffff,0x03142,0x14032,0x04132,0xfffff,0x04132,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 17 - 64th Street Japanese
   { 0x14302,0x04132,0x14032,0x04312,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0x01324,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 18 - Chimera Beast
   { 0x04132,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 19 - The Lord of King
   { 0x04132,0x02413,0x03142,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 20 - Iga Ninjyutsuden
   { 0x04132,0x02413,0x03142,0x01423,0xfffff,0xfffff,0xfffff,0x24103,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff,0xfffff, }, // 21 - Soldam
};

typedef struct MS1LAYER
{
   UINT8 *RAM;
   UINT8 *GFX16;
   UINT8 *GFX8;
   UINT8 *MSK16;
   UINT8 *MSK8;
   UINT8 *SCR;
   UINT8 PAL;
} MS1LAYER;

static struct MS1LAYER JalecoLayers[3];

void RenderJalecoLayer(int layer)
{
   UINT8 *RAM_BG,*SCR_BG,*GFX_BG16,*MSK_BG16;
   UINT8 *GFX_BG8,*MSK_BG8,PAL_BG;
   UINT8 *MAP;
   int x,y,x16,y16,zz,zzz,zzzz,ta;

   if(!check_layer_enabled(layer_id_data[layer]))
       return;

   RAM_BG       =JalecoLayers[layer].RAM;
   SCR_BG       =JalecoLayers[layer].SCR;
   GFX_BG16     =JalecoLayers[layer].GFX16;
   MSK_BG16     =JalecoLayers[layer].MSK16;
   GFX_BG8      =JalecoLayers[layer].GFX8;
   MSK_BG8      =JalecoLayers[layer].MSK8;
   PAL_BG       =JalecoLayers[layer].PAL;

   if((ReadWord(&SCR_BG[4])&0x0010)==0){      // 16x16

   if(GFX_BG16){                              // HAVE GFX

   if(!JalecoLayerCount){                     // **** SOLID ****

   switch(ReadWord(&SCR_BG[4])&0x0003){

   case 0x00:                                 // <<<<$1000x$200>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0100)>>4)<<9;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x0FF0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz&=0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){

      MAP_PALETTE_MAPPED_NEW(
         (RAM_BG[1+zz]>>4)|PAL_BG,
         16,
         MAP
      );

      Draw16x16_Mapped_Rot(&GFX_BG16[(ReadWord(&RAM_BG[zz])&0xFFF)<<8],x,y,MAP);

   zz+=2;
   if((zz&0x1F)==0){zz+=0x1FE0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0x1FE0)==0){zzzz-=0x2000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x01:                                   // <<<<$800x$400>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0300)>>4)<<8;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x07F0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){

      MAP_PALETTE_MAPPED_NEW(
         (RAM_BG[1+zz]>>4)|PAL_BG,
         16,
         MAP
      );

      Draw16x16_Mapped_Rot(&GFX_BG16[(ReadWord(&RAM_BG[zz])&0xFFF)<<8],x,y,MAP);

   zz+=2;
   if((zz&0x1F)==0){zz+=0xFE0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0xFE0)==0){zzzz-=0x1000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x02:                                   // <<<<$400x$800>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0700)>>4)<<7;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x03F0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){

      MAP_PALETTE_MAPPED_NEW(
         (RAM_BG[1+zz]>>4)|PAL_BG,
         16,
         MAP
      );

      Draw16x16_Mapped_Rot(&GFX_BG16[(ReadWord(&RAM_BG[zz])&0xFFF)<<8],x,y,MAP);

   zz+=2;
   if((zz&0x1F)==0){zz+=0x7E0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0x7E0)==0){zzzz-=0x800;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x03:                                   // <<<<$200x$1000>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0F00)>>4)<<6;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x01F0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){

      MAP_PALETTE_MAPPED_NEW(
         (RAM_BG[1+zz]>>4)|PAL_BG,
         16,
         MAP
      );

      Draw16x16_Mapped_Rot(&GFX_BG16[(ReadWord(&RAM_BG[zz])&0xFFF)<<8],x,y,MAP);

   zz+=2;
   if((zz&0x1F)==0){zz+=0x3E0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0x3E0)==0){zzzz-=0x400;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   }
   JalecoLayerCount++;
   }                                            // END SOLID
   else{                                        // **** TRANSPARENT ****

   switch(ReadWord(&SCR_BG[4])&3){

   case 0x00:                                   // <<<<$1000x$200>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0100)>>4)<<9;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x0FF0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG16[ta]!=0){                      // No pixels; skip

         MAP_PALETTE_MAPPED_NEW(
            (RAM_BG[1+zz]>>4)|PAL_BG,
            16,
            MAP
         );

         if(MSK_BG16[ta]==1){                   // Some pixels; trans
            Draw16x16_Trans_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw16x16_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x1F)==0){zz+=0x1FE0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0x1FE0)==0){zzzz-=0x2000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x01:                                   // <<<<$800x$400>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0300)>>4)<<8;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x07F0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG16[ta]!=0){                      // No pixels; skip

         MAP_PALETTE_MAPPED_NEW(
            (RAM_BG[1+zz]>>4)|PAL_BG,
            16,
            MAP
         );

         if(MSK_BG16[ta]==1){                   // Some pixels; trans
            Draw16x16_Trans_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw16x16_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x1F)==0){zz+=0xFE0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0xFE0)==0){zzzz-=0x1000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x02:                                   // <<<<$400x$800>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0700)>>4)<<7;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x03F0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG16[ta]!=0){                      // No pixels; skip

         MAP_PALETTE_MAPPED_NEW(
            (RAM_BG[1+zz]>>4)|PAL_BG,
            16,
            MAP
         );

         if(MSK_BG16[ta]==1){                   // Some pixels; trans
            Draw16x16_Trans_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw16x16_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x1F)==0){zz+=0x7E0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0x7E0)==0){zzzz-=0x800;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x03:                                   // <<<<$200x$1000>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F0)>>4)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0F00)>>4)<<6;                  // Y Offset (256-nn)
   y16=zzz&15;                                  // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x01F0)>>4)<<5;                  // X Offset (16-nn)
   x16=zzz&15;                                  // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=16){
   for(y=(32-y16);y<(224+32);y+=16){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG16[ta]!=0){                      // No pixels; skip

         MAP_PALETTE_MAPPED_NEW(
            (RAM_BG[1+zz]>>4)|PAL_BG,
            16,
            MAP
         );

         if(MSK_BG16[ta]==1){                   // Some pixels; trans
            Draw16x16_Trans_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw16x16_Mapped_Rot(&GFX_BG16[ta<<8],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x1F)==0){zz+=0x3E0;zz&=0x3FFF;}
   }
   zzzz+=0x20;
   if((zzzz&0x3E0)==0){zzzz-=0x400;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   }
   JalecoLayerCount++;
   }                                            // END TRANSPARENT
   }                                            // END HAVE GFX
   }                                            // END 16x16
   else{                                        // 8x8

   if(GFX_BG8){                          	// HAVE GFX

   if(!JalecoLayerCount){                       // **** SOLID ****

   if(ReadLong(&SCR_BG[0])==0){

   zz=4;
   for(x=32;x<256+32;x+=8,zz+=8){
   for(y=32;y<224+32;y+=8,zz+=2){

      MAP_PALETTE_MAPPED_NEW(
         (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
         16,
         MAP
      );

      Draw8x8_Mapped_Rot(&GFX_BG8[(ReadWord(&RAM_BG[zz])&0xFFF)<<6],x,y,MAP);
   }
   }

   }
   else{

   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F8)>>3)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0000)>>3)<<9;                  // Y Offset (256-nn)
   y16=zzz&7;                                   // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x07F8)>>3)<<6;                  // X Offset (16-nn)
   x16=zzz&7;                                   // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=8){
   for(y=(32-y16);y<(224+32);y+=8){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;

         MAP_PALETTE_MAPPED_NEW(
            (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
            16,
            MAP
         );

         Draw8x8_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);

   zz+=2;
   if((zz&0x3F)==0){zz+=0x3FC0;zz&=0x3FFF;}
   }
   zzzz+=0x40;
   if((zzzz&0x3FC0)==0){zzzz-=0x4000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }

   }

   JalecoLayerCount++;

   }                                            // END SOLID
   else{                                        // **** TRANSPARENT ****

   if(ReadLong(&SCR_BG[0])==0){

   zz=4;
   for(x=32;x<256+32;x+=8,zz+=8){
   for(y=32;y<224+32;y+=8,zz+=2){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG8[ta]!=0){

         MAP_PALETTE_MAPPED_NEW(
            (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
            16,
            MAP
         );

         Draw8x8_Trans_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
      }
   }
   }

   }
   else{

   switch(ReadWord(&SCR_BG[4])&3){

   case 0x00:                                   // <<<<$1000x$80>>>> [unlikely]
   case 0x01:                                   // <<<<$800x$100>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F8)>>3)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0000)>>3)<<9;                  // Y Offset (256-nn)
   y16=zzz&7;                                   // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x07F8)>>3)<<6;                  // X Offset (16-nn)
   x16=zzz&7;                                   // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=8){
   for(y=(32-y16);y<(224+32);y+=8){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG8[ta]!=0){                       // No pixels; skip

            MAP_PALETTE_MAPPED_NEW(
               (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
               16,
               MAP
            );

         if(MSK_BG8[ta]==1){                    // Some pixels; trans
            Draw8x8_Trans_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw8x8_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x3F)==0){zz+=0x3FC0;zz&=0x3FFF;}
   }
   zzzz+=0x40;
   if((zzzz&0x3FC0)==0){zzzz-=0x4000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x02:                                   // <<<<$400x$200>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F8)>>3)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0100)>>3)<<8;                  // Y Offset (256-nn)
   y16=zzz&7;                                   // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x03F8)>>3)<<6;                  // X Offset (16-nn)
   x16=zzz&7;                                   // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=8){
   for(y=(32-y16);y<(224+32);y+=8){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG8[ta]!=0){                       // No pixels; skip

            MAP_PALETTE_MAPPED_NEW(
               (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
               16,
               MAP
            );

         if(MSK_BG8[ta]==1){                    // Some pixels; trans
            Draw8x8_Trans_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw8x8_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x3F)==0){zz+=0x1FC0;zz&=0x3FFF;}
   }
   zzzz+=0x40;
   if((zzzz&0x1FC0)==0){zzzz-=0x2000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
   case 0x03:                                   // <<<<$200x$400>>>>
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F8)>>3)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0300)>>3)<<7;                  // Y Offset (256-nn)
   y16=zzz&7;                                   // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x01F8)>>3)<<6;                  // X Offset (16-nn)
   x16=zzz&7;                                   // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=8){
   for(y=(32-y16);y<(224+32);y+=8){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG8[ta]!=0){                       // No pixels; skip

            MAP_PALETTE_MAPPED_NEW(
               (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
               16,
               MAP
            );

         if(MSK_BG8[ta]==1){                    // Some pixels; trans
            Draw8x8_Trans_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw8x8_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x3F)==0){zz+=0xFC0;zz&=0x3FFF;}
   }
   zzzz+=0x40;
   if((zzzz&0xFC0)==0){zzzz-=0x1000;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
/*
   case 0x00:                                   // <<<<$100x$800>>>> [hmm]
   zzz=(ReadWord(&SCR_BG[2])+16);
   zzzz =((zzz&0x00F8)>>3)<<1;                  // Y Offset (16-255)
   zzzz+=((zzz&0x0700)>>3)<<6;                  // Y Offset (256-nn)
   y16=zzz&7;                                   // Y Offset (0-15)
   zzz=ReadWord(&SCR_BG[0]);
   zzzz+=((zzz&0x00F8)>>3)<<6;                  // X Offset (16-nn)
   x16=zzz&7;                                   // X Offset (0-15)

   zzzz=zzzz&0x3FFF;
   zz=zzzz;
   for(x=(32-x16);x<(256+32);x+=8){
   for(y=(32-y16);y<(224+32);y+=8){
      ta=ReadWord(&RAM_BG[zz])&0xFFF;
      if(MSK_BG8[ta]!=0){                       // No pixels; skip

      MAP_PALETTE_MAPPED_NEW(
               (ReadWord(&RAM_BG[zz])>>12)|PAL_BG,
               16, MAP
            );

         if(MSK_BG8[ta]==1){                    // Some pixels; trans
            Draw8x8_Trans_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
         else{                                  // all pixels; solid
            Draw8x8_Mapped_Rot(&GFX_BG8[ta<<6],x,y,MAP);
         }
      }
   zz+=2;
   if((zz&0x3F)==0){zz+=0x7C0;zz&=0x3FFF;}
   }
   zzzz+=0x40;
   if((zzzz&0x7C0)==0){zzzz-=0x800;}
   zzzz&=0x3FFF;
   zz=zzzz;
   }
   break;
*/
   }

   }

   JalecoLayerCount++;
   }                                            // END TRANSPARENT
   }                                            // END HAVE GFX

   }                                            // END 8x8

}

void BG1ColPatch(void)
{
   UINT32 ta;
   UINT16 tb;

   if(romset==10){	// EDF

   tb = ReadWord(&RAM_COL[0x01E]);

   for(ta=0;ta<16;ta++){
      WriteWord(&RAM_COL[0x21E + (ta<<5)],tb);
   }

   return;
   }

   if((romset==2)||(romset==14)){	// P47-J/USA

   tb = 0x0000;

   for(ta=0;ta<16;ta++){
      WriteWord(&RAM_COL[0x21E + (ta<<5)],tb);
   }

   return;
   }

}

void DrawMegaSystem1(void)
{
   UINT16 ctrl, ctrl2;

   ClearPaletteMap();

   ctrl  = ReadWord(&RAM[0x14000]);
   ctrl2 = ReadWord(&RAM[0x14100]);

   JalecoLayerCount=0;

   if(RefreshBuffers){
   JalecoLayers[0].RAM          =RAM+0x20000;
   JalecoLayers[0].GFX16        =GFX_BG0;
   JalecoLayers[0].GFX8         =NULL;
   JalecoLayers[0].MSK16        =BG0_Mask;
   JalecoLayers[0].MSK8         =NULL;
   JalecoLayers[0].SCR          =RAM+0x14200;
   JalecoLayers[0].PAL          =0x00;

   JalecoLayers[1].RAM          =RAM+0x24000;
   JalecoLayers[1].GFX16        =GFX_BG1;
   JalecoLayers[1].GFX8         =NULL;
   JalecoLayers[1].MSK16        =BG1_Mask;
   JalecoLayers[1].MSK8         =NULL;
   JalecoLayers[1].SCR          =RAM+0x14208;
   JalecoLayers[1].PAL          =0x10;

   JalecoLayers[2].RAM          =RAM+0x28000;
   JalecoLayers[2].GFX16        =NULL;
   JalecoLayers[2].GFX8         =GFX_FG0;
   JalecoLayers[2].MSK16        =NULL;
   JalecoLayers[2].MSK8         =FG0_Mask;
   JalecoLayers[2].SCR          =RAM+0x14008;
   JalecoLayers[2].PAL          =0x20;

   SPRITE_GFX=GFX_SPR;
   SPRITE_MSK=SPR_Mask;

   RenderSpr=0;
   }

   {

   int i, pri;

   pri = pri_data[romset][(ctrl & 0x0f00) >> 8];

#ifdef RAINE_DEBUG
   clear_ingame_message_list();
   print_ingame(60, "ctrl:%04x pri:%04x", ctrl, pri);
   print_ingame(60, "%04x", ReadWord(JalecoLayers[2].SCR + 4));
#endif

   if (pri == 0xfffff) pri = 0x04132;

   for (i = 0; i < 5; i++){

      int layer = (pri & 0xf0000) >> 16;
      pri <<= 4;

      switch (layer){

         case 0:
            if(ctrl & 0x0001)
	      RenderJalecoLayer(0);
	    else if ((romset==20) || (romset==6) || (romset==1))
	      RenderJalecoLayer(0);	// Iga Ninjyu, Plus Alpha, Saint Dragon - render anyway!
         break;
         case 1:
            if(ctrl & 0x0002)
            RenderJalecoLayer(1);
         break;
         case 2:
            if(ctrl & 0x0004)
            RenderJalecoLayer(2);
         break;
         case 3:
         case 4:
            if(ctrl & 0x0008){

               if(!RenderSpr){

                  MS1ChainRecalc(RAM+0x1E000);
                  RenderSpr = 1;

               }

               if(ctrl2 & 0x0100){
                  if(layer == 3){
                     BG1ColPatch();
			   if (romset!=5)
                       RenderMS1Sprites(2);
			   else {				// Hachoo
                       RenderMS1Sprites(7);	// objects (clouds, stones, etc.)
                       RenderMS1Sprites(8);	// characters
			   }
                  }
                  else
                     RenderMS1Sprites(1);
               }
               else{
                  if(layer == 3){
			   if (!spr_pri_needed)
                       RenderMS1Sprites(0);
			   else {
			     if (romset==11)	// Shingen
                         RenderMS1Sprites(6);
                       RenderMS1Sprites(5);
			     if ((romset==10) || (romset==1))	// EDF, Saint Dragon
                         RenderMS1Sprites(6);
			   }
			}
               }

            }

         break;
      }

   }

   if(!JalecoLayerCount){
      JalecoLayerCount = 1;
      clear_game_screen(0);
   }

   }

}

void DrawMegaSystem2(void)
{
   UINT16 ctrl, ctrl2;

   ClearPaletteMap();

   ctrl  = ReadWord(&RAM[0x12208]);
   ctrl2 = ReadWord(&RAM[0x12200]);

   JalecoLayerCount=0;

   if(RefreshBuffers){
   JalecoLayers[0].RAM          =RAM+0x30000;
   JalecoLayers[0].GFX16        =GFX_BG0;
   JalecoLayers[0].GFX8         =NULL;
   JalecoLayers[0].MSK16        =BG0_Mask;
   JalecoLayers[0].MSK8         =NULL;
   JalecoLayers[0].SCR          =RAM+0x12000;
   JalecoLayers[0].PAL          =0x00;

   JalecoLayers[1].RAM          =RAM+0x38000;
   JalecoLayers[1].GFX16        =GFX_BG1;
   JalecoLayers[1].GFX8         =NULL;
   JalecoLayers[1].MSK16        =BG1_Mask;
   JalecoLayers[1].MSK8         =NULL;
   JalecoLayers[1].SCR          =RAM+0x12008;
   JalecoLayers[1].PAL          =0x10;

   JalecoLayers[2].RAM          =RAM+0x40000;
   JalecoLayers[2].GFX16        =NULL;
   JalecoLayers[2].GFX8         =GFX_FG0;
   JalecoLayers[2].MSK16        =NULL;
   JalecoLayers[2].MSK8         =FG0_Mask;
   JalecoLayers[2].SCR          =RAM+0x12100;
   JalecoLayers[2].PAL          =0x20;

   RenderSpr=0;
   }

   if((ReadWord(&RAM[0x12108]) & 0x0001) == 0){
   SPRITE_GFX=GFX_SPR;
   SPRITE_MSK=SPR_Mask;
   }
   else{
   SPRITE_GFX=GFX_SPR+0x100000;
   SPRITE_MSK=SPR_Mask+0x1000;
   }

   {

   int i, pri;

   pri = pri_data[romset][(ctrl & 0x0f00) >> 8];

#ifdef RAINE_DEBUG
   clear_ingame_message_list();
   print_ingame(60, "ctrl:%04x pri:%04x", ctrl, pri);
   print_ingame(60, "%04x", ReadWord(JalecoLayers[2].SCR + 4));
#endif

   if (pri == 0xfffff) pri = 0x04132;

   for (i = 0; i < 5; i++){

      int layer = (pri & 0xf0000) >> 16;
      pri <<= 4;

      switch (layer){

         case 0:
            if(ctrl & 0x0001)
            RenderJalecoLayer(0);
         break;
         case 1:
            if(ctrl & 0x0002)
            RenderJalecoLayer(1);
         break;
         case 2:
            if(ctrl & 0x0004)
            RenderJalecoLayer(2);
         break;
         case 3:
         case 4:
            if(ctrl & 0x0008){

               if(!RenderSpr){

                  MS1ChainRecalc(RAM+0x22000);
                  RenderSpr = 1;

               }

               if(ctrl2 & 0x0100){
                  if(layer == 3){
                     BG1ColPatch();
                     RenderMS1Sprites(2);
                  }
                  else
                     RenderMS1Sprites(1);
               }
               else{
                  if(layer == 3)
                     RenderMS1Sprites(0);
               }

            }

         break;
      }

   }

   if(!JalecoLayerCount){
      JalecoLayerCount = 1;
      clear_game_screen(0);
   }

   }

}

void DrawLegendOfMakaj(void)
{
   ClearPaletteMap();

   JalecoLayerCount=0;

   if(RefreshBuffers){
   JalecoLayers[0].RAM          =RAM+0x20000;
   JalecoLayers[0].GFX16        =GFX_BG0;
   JalecoLayers[0].GFX8         =NULL;
   JalecoLayers[0].MSK16        =BG0_Mask;
   JalecoLayers[0].MSK8         =NULL;
   JalecoLayers[0].SCR          =RAM+0x14200;
   JalecoLayers[0].PAL          =0x00;

   JalecoLayers[1].RAM          =NULL;
   JalecoLayers[1].GFX16        =NULL;
   JalecoLayers[1].GFX8         =NULL;
   JalecoLayers[1].MSK16        =NULL;
   JalecoLayers[1].MSK8         =NULL;
   JalecoLayers[1].SCR          =NULL;
   JalecoLayers[1].PAL          =0x10;

   JalecoLayers[2].RAM          =RAM+0x24000;
   JalecoLayers[2].GFX16        =NULL;
   JalecoLayers[2].GFX8         =GFX_FG0;
   JalecoLayers[2].MSK16        =NULL;
   JalecoLayers[2].MSK8         =FG0_Mask;
   JalecoLayers[2].SCR          =RAM+0x14208;
   JalecoLayers[2].PAL          =0x20;
   }

   RenderJalecoLayer(0);

   RenderLOMSprites(0);

   RenderJalecoLayer(2);

   if(!JalecoLayerCount){
      JalecoLayerCount = 1;
      clear_game_screen(0);
   }
}

void DrawPeekABoo(void)
{
   UINT16 ctrl, ctrl2;

   ClearPaletteMap();

   ctrl  = ReadWord(&RAM[0x12208]);
   ctrl2 = ReadWord(&RAM[0x12200]);

   JalecoLayerCount=0;

   if(RefreshBuffers){
   JalecoLayers[0].RAM          =RAM+0x38000;
   JalecoLayers[0].GFX16        =GFX_BG0;
   JalecoLayers[0].GFX8         =NULL;
   JalecoLayers[0].MSK16        =BG0_Mask;
   JalecoLayers[0].MSK8         =NULL;
   JalecoLayers[0].SCR          =RAM+0x12000;
   JalecoLayers[0].PAL          =0x00;

   JalecoLayers[1].RAM          =RAM+0x20000;
   JalecoLayers[1].GFX16        =NULL;
   JalecoLayers[1].GFX8         =GFX_FG0;
   JalecoLayers[1].MSK16        =NULL;
   JalecoLayers[1].MSK8         =FG0_Mask;
   JalecoLayers[1].SCR          =RAM+0x12008;
   JalecoLayers[1].PAL          =0x10;

   JalecoLayers[2].RAM          =NULL;
   JalecoLayers[2].GFX16        =NULL;
   JalecoLayers[2].GFX8         =NULL;
   JalecoLayers[2].MSK16        =NULL;
   JalecoLayers[2].MSK8         =NULL;
   JalecoLayers[2].SCR          =NULL;
   JalecoLayers[2].PAL          =0x20;

   }

   SPRITE_GFX=GFX_SPR;
   SPRITE_MSK=SPR_Mask;

   {

   int i, pri;

   pri = pri_data[romset][(ctrl & 0x0f00) >> 8];

#ifdef RAINE_DEBUG
   clear_ingame_message_list();
   print_ingame(60, "ctrl:%04x pri:%04x", ctrl, pri);
#endif

   if (pri == 0xfffff) pri = 0x04132;

   for (i = 0; i < 5; i++){

      int layer = (pri & 0xf0000) >> 16;
      pri <<= 4;

      switch (layer){

         case 0:
            if(ctrl & 0x0001)
            RenderJalecoLayer(0);
         break;
         case 1:
            if(ctrl & 0x0002)
            RenderJalecoLayer(1);
         break;
         case 2:
            if(ctrl & 0x0004)
            RenderJalecoLayer(2);
         break;
         case 3:
         case 4:
            if(ctrl & 0x0008){

               //if(!RenderSpr){

                  MS1ChainRecalc(RAM+0x1A000);
                  //RenderSpr = 1;

               //}

               if(ctrl2 & 0x0100){
                  if(layer == 3){
                     BG1ColPatch();
                     RenderMS1Sprites(2);
                  }
                  else
                     RenderMS1Sprites(1);
               }
               else{
                  if(layer == 3){
                     RenderMS1Sprites(0);
                     RenderMS1Sprites(9);
			}
               }

            }

         break;
      }

   }

   if(!JalecoLayerCount){
      JalecoLayerCount = 1;
      clear_game_screen(0);
   }

   }
}

/*

JALECO MEGA SYSTEM-1
--------------------

Main CPU....68000
Sound CPUs..68000; YM2151; M6295x2
Monitor.....256x224

BG0: 512x4096 / 1024x2048 / 2048x1024 / 4096x512
     16x16 / 8x8 16 colour tiles
BG1: 512x4096 / 1024x2048 / 2048x1024 / 4096x512
     16x16 / 8x8 16 colour tiles
BG2: 512x4096 / 1024x2048 / 2048x1024 / 4096x512
     16x16 / 8x8 16 colour tiles

--------------+-------------------------
Address Range | Description
--------------+-------------------------
000000-05FFFF | 68000 ROM
080000-08000F | Input RAM
084000-08400F | Screen Control RAM
084100-08410F | Sprite Control RAM
084200-08420F | Scroll RAM
084300-08430F | Sound Write RAM
088000-0887FF | Color RAM
08E000-08E7FF | Chain Sprite RAM
08E800-08EFFF | Chain Sprite Flip X RAM
08F000-08F7FF | Chain Sprite Flip Y RAM
08F800-08FFFF | Chain Sprite Flip XY RAM
090000-093FFF | BG0 RAM
094000-097FFF | BG1 RAM
098000-098FFF | BG2 RAM
0F0000-0F7FFF | 68000 RAM
0F8000-0F87FF | Main Sprite RAM
0F8800-0FFFFF | 68000 RAM
--------------+-------------------------

68000 ROM: $000000-$05FFFF
--------------------------

- Sometimes protected. Not much else to say.

INPUT RAM: $080000-$08000F
--------------------------

- Buttons 3 and 4 usually not used.

Byte | Bit(s) | Description
-----+76543210+----------------------
  1  |.......x| Player 1 Start
  1  |......x.| Player 2 Start
  1  |..x.....| Service
  1  |.x......| Coin A
  1  |x.......| Coin B
  3  |.......x| Player 1 Right
  3  |......x.| Player 1 Left
  3  |.....x..| Player 1 Down
  3  |....x...| Player 1 Up
  3  |...x....| Player 1 Button 1
  3  |..x.....| Player 1 Button 2
  3  |.x......| Player 1 Button 3
  3  |x.......| Player 1 Button 4
  5  |.......x| Player 2 Right
  5  |......x.| Player 2 Left
  5  |.....x..| Player 2 Down
  5  |....x...| Player 2 Up
  5  |...x....| Player 2 Button 1
  5  |..x.....| Player 2 Button 2
  5  |.x......| Player 2 Button 3
  5  |x.......| Player 2 Button 4
  6  |xxxxxxxx| Dipswitch Bank A
  7  |xxxxxxxx| Dipswitch Bank B
  8  |xxxxxxxx| Sound Port Readback (high)
  9  |xxxxxxxx| Sound Port Readback (low)

SCREEN CONTROL RAM: $084000-$08400F
-----------------------------------

Byte | Bit(s) | Description
-----+76543210+----------------------
  0  |....xxxx| Priority mode (uses prom)
  1  |...x....| Sprite Reverse Render Order? <shingen>
  1  |....x...| Sprite Enable
  1  |.....x..| BG2 Enable
  1  |......x.| BG1 Enable
  1  |.......x| BG0 Enable
  8  |....xxxx| BG2 Scroll X (high)
  9  |xxxxxxxx| BG2 Scroll X (low)
  A  |....xxxx| BG2 Scroll Y (high)
  B  |xxxxxxxx| BG2 Scroll Y (low)
  D  |...x....| BG2 Tile Size
  D  |......xx| BG2 Size

SPRITE CONTROL RAM: $084100-$08410F
-----------------------------------

Byte | Bit(s) | Description
-----+76543210+----------------------------
  0  |.......x| Sprite Dual Priority Enable
  1  |...xxxxx| Mosaic <P47 Jaleco Logo>

SCROLL RAM: $084200-$08420F
---------------------------

Byte | Bit(s) | Description
-----+76543210+----------------------
  0  |....xxxx| BG0 Scroll X (high)
  1  |xxxxxxxx| BG0 Scroll X (low)
  2  |....xxxx| BG0 Scroll Y (high)
  3  |xxxxxxxx| BG0 Scroll Y (low)
  5  |...x....| BG0 Tile Size
  5  |......xx| BG0 Size
  8  |....xxxx| BG1 Scroll X (high)
  9  |xxxxxxxx| BG1 Scroll X (low)
  A  |....xxxx| BG1 Scroll Y (high)
  B  |xxxxxxxx| BG1 Scroll Y (low)
  D  |...x....| BG1 something
  D  |......xx| BG1 Tile Size

SOUND WRITE RAM: $084300-$08430F
--------------------------------

Byte | Bit(s) | Description
-----+76543210+----------------------
  0  |xxxxxxxx| Reset Port?
  1  |xxxxxxxx| Reset Port?
  8  |xxxxxxxx| Send data to sub 68000 (high)
  9  |xxxxxxxx| Send data to sub 68000 (low)

COLOR RAM: $088000-$0887FF
--------------------------

- 64 banks of 16 colours (1024 colours onscreen max).
- Format is RRRR.GGGG.BBBB.RGBx

Bank  0-15: BG0
Bank 16-31: BG1
Bank 32-47: BG2
Bank 48-63: SPR

CHAIN SPRITE RAM: $08E000-$08FFFF
---------------------------------

Byte | Bit(s) | Description
-----+76543210+----------------------
  0  |........| Main Sprite Link
  1  |.xxxxxxx| Main Sprite Link
  2  |.......x| Offset to Base X
  3  |xxxxxxxx| Offset to Base X
  4  |.......x| Offset to Base Y
  5  |xxxxxxxx| Offset to Base Y
  6  |....xxxx| Offset to Base Tile
  7  |xxxxxxxx| Offset to Base Tile

BG0/1/BG2 RAM: $090000-$098FFF
------------------------------

Byte | Bit(s) | Description
-----+76543210+----------------------
  0  |xxxx....| Colour Bank
  0  |....xxxx| Tile number (high)
  1  |xxxxxxxx| Tile number (low)

68000 RAM: $0F0000-$0FFFFF
--------------------------

- Not much to say, except the sprite ram is
  half way through it.

MAIN SPRITE RAM: $0F8000-$0F87FF
--------------------------------

- 16 bytes/sprite entry
- 128 sprite entires
- Priorities in inverse (render from bottom of list to top)

Byte | Bit(s) | Description
-----+76543210+---------------------------------------------
  8  |....xxxx| Mosaic Number
  8  |...x....| Mosaic Enable
  9  |.....xxx| Colour Bank
  9  |....x...| Sprite-BG1 Priority/Colour Bank high bit
  9  |.x......| Flip Y Axis
  9  |x.......| Flip X Axis
  A  |.......x| Sprite X (high)
  B  |xxxxxxxx| Sprite X (low)
  C  |.......x| Sprite Y (high)
  D  |xxxxxxxx| Sprite Y (low)
  E  |....xxxx| Sprite Number (high)
  F  |xxxxxxxx| Sprite Number (low)

Main 68000 Interrupts
---------------------

Saint Dragon:
1 60fps
2 60fps
3 <rte>

QUARTEX - THE WOLF KEEPS ON CLIMBING...

1 - Rodland: 0000 0180 0180 0000
             0000 0000 0000 0000

*/

GAME( 64th_street ,
   _64th_street_dirs,
   _64th_street_roms,
   megasys_1_inputs,
   _64th_street_dsw,
   NULL,

   Load64thStreet,
   NULL,
   &megasys2_video,
   ExecuteMegaSystem2Frame,
   "64street",
   "64th Street",
   "６４�ﾔ街 American",
   COMPANY_ID_JALECO,
   NULL,
   1991,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

GAME( 64th_street_japanese ,
   _64th_street_japanese_dirs,
   _64th_street_japanese_roms,
   megasys_1_inputs,
   _64th_street_dsw,
   NULL,

   Load64thStreet,
   NULL,
   &megasys2_video,
   ExecuteMegaSystem2Frame,
   "64streej",
   "64th Street Japanese",
   "６４�ﾔ街",
   COMPANY_ID_JALECO,
   NULL,
   1991,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

GAME( astyanax ,
   astyanax_dirs,
   astyanax_roms,
   megasys_1_inputs,
   astyanax_dsw,
   NULL,

   load_astyanax,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "astyanax",
   "Astyanax",
   "ザ・ロード・オブ・キング American",
   COMPANY_ID_JALECO,
   NULL,
   1989,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

GAME( the_lord_of_king ,
   the_lord_of_king_dirs,
   the_lord_of_king_roms,
   megasys_1_inputs,
   astyanax_dsw,
   NULL,

   load_astyanax,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "lordofk",
   "The Lord of King",
   "ザ・ロード・オブ・キング",
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

GAME( p47_american ,
   p47_american_dirs,
   p47_american_roms,
   megasys_1_inputs,
   p47_dsw,
   NULL,

   LoadP47J,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "p47",
   "P47",
   "Ｐ４７ American",
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

GAME( p47_japanese ,
   p47_japanese_dirs,
   p47_japanese_roms,
   megasys_1_inputs,
   p47_dsw,
   NULL,

   LoadP47J,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "p47j",
   "P47 Japanese",
   "Ｐ４７",
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_SHOOT
);

// avenging spirit has a different memory map (the first part of the rom
// is not encrypted, but there is a hole in the middle).
// The easiest solution is to keep 2 separate loading functions...
GAME( avenging_spirit ,
   avenging_spirit_dirs,
   avenging_spirit_roms,
   megasys_1_inputs,
   avenging_spirit_dsw,
   NULL,

   LoadAvengingSpirit, //LoadPhantasm,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "avspirit",
   "Avenging Spirit",
   "ファンタズム American",
   COMPANY_ID_JALECO,
   NULL,
   1991,
   jaleco_ym2151_m6295x2_sound,
   GAME_PLATFORM
);

GAME( phantasm ,
   phantasm_dirs,
   phantasm_roms,
   megasys_1_inputs,
   avenging_spirit_dsw,
   NULL,

   LoadPhantasm,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "phantasm",
   "Phantasm",
   "ファンタズム",
   COMPANY_ID_JALECO,
   NULL,
   1990,
   jaleco_ym2151_m6295x2_sound,
   GAME_PLATFORM
);

GAME( rodland ,
   rodland_dirs,
   rodland_roms,
   megasys_1_inputs,
   rodland_dsw,
   NULL,

   load_rodland,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "rodland",
   "Rodland",
   "ロッドランド American",
   COMPANY_ID_JALECO,
   NULL,
   1990,
   jaleco_ym2151_m6295x2_sound,
   GAME_PLATFORM
);

GAME( rodlandjb ,
   rodlandjb_dirs,
   rodlandjb_roms,
   megasys_1_inputs,
   rodland_dsw,
   NULL,

   load_rodland,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "rodlndjb",
   "Rodland Japanese bootleg",
   "ロッドランド",
   COMPANY_ID_JALECO,
   NULL,
   1990,
   jaleco_ym2151_m6295x2_sound,
   GAME_PLATFORM
);

GAME( rodlandj ,
   rodlandj_dirs,
   rodlandj_roms,
   megasys_1_inputs,
   rodland_dsw,
   NULL,

   load_rodland,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "rodlandj",
   "Rodland Japanese",
   "ロッドランド",
   COMPANY_ID_JALECO,
   NULL,
   1990,
   jaleco_ym2151_m6295x2_sound,
   GAME_PLATFORM
);

GAME( shingen ,
   shingen_dirs,
   shingen_roms,
   megasys_1_inputs,
   shingen_dsw,
   NULL,

   LoadShingen,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "tshingen",
   "Takeda Shingen",
   NULL,
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

GAME( tshingna ,
   tshingna_dirs,
   tshingna_roms,
   megasys_1_inputs,
   shingen_dsw,
   NULL,

   LoadShingen,
   NULL,
   &megasys1_video,
   ExecuteMegaSystem1Frame,
   "tshingna",
   "Takeda Shingen (English)",
   NULL,
   COMPANY_ID_JALECO,
   NULL,
   1988,
   jaleco_ym2151_m6295x2_sound,
   GAME_BEAT
);

GAME( peek_a_boo ,
   peek_a_boo_dirs,
   peek_a_boo_roms,
   peek_a_boo_inputs,
   peek_a_boo_dsw,
   NULL,

   LoadPeekABoo,
   NULL,
   &peek_a_boo_video,
   execute_systemd,
   "peekaboo",
   "Peek A Boo",
   NULL,
   COMPANY_ID_JALECO,
   NULL,
   1993,
   m6295_sound_D,
   GAME_BREAKOUT
);
