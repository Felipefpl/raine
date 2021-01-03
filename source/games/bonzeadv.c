#define DRV_DEF_SOUND sound_mcatadv
/******************************************************************************/
/*									      */
/*	       BONZE ADVENTURE/JIGOKU (C) 1988 TAITO CORPORATION	      */
/*									      */
/******************************************************************************/

#include "gameinc.h"
#include "tc100scn.h"
#include "tc110pcr.h"
#include "tc002obj.h"
#include "sasound.h"            // sample support routines
#include "taitosnd.h"
#include "asuka.h"
#include "def_dsw.h"

// taito_ym2610_sound uses 2 sound regions, we have only 1 here, like mcatadv !
extern struct SOUND_INFO sound_mcatadv[];

/*******************
   BONZE ADVENTURE
 *******************/
/* Stephh's notes (based on the game M68000 code and some tests) :

1) 'bonzeadv', 'jigkmgri' and 'bonzeadvu'

  - Region stored at 0x03fffe.w
  - Sets :
      * 'bonzeadv' : region = 0x0002
      * 'jigkmgri' : region = 0x0000
      * 'bonzeadvu': region = 0x0001
  - These 3 games are 100% the same, only region differs !
  - Coinage relies on the region (code at 0x02d344) :
      * 0x0000 (Japan) and 0x0001 (US) use TAITO_COINAGE_JAPAN_OLD_LOC()
      * 0x0002 (World) uses TAITO_COINAGE_WORLD_LOC()
  - Notice screen only if region = 0x0000
  - Texts and game name rely on the region :
      * 0x0000 : most texts in Japanese - game name is "Jigoku Meguri"
      * other : all texts in English - game name is "Bonze Adventure"
  - Bonus lives aren't awarded correctly due to bogus code at 0x00961e :

      00961E: 302D 0B7E                  move.w  ($b7e,A5), D0
      009622: 0240 0018                  andi.w  #$18, D0
      009626: E648                       lsr.w   #3, D0

    Here is what the correct code should be :

      00961E: 302D 0B7E                  move.w  ($b7e,A5), D0
      009622: 0240 0030                  andi.w  #$30, D0
      009626: E848                       lsr.w   #4, D0

  - DSWB bit 7 was previously used to allow map viewing (C-Chip test ?),
    but it is now unused due to "bra" instruction at 0x007572 */

static struct ROM_INFO rom_bonzeadv[] =
{
  LOAD8_16( CPU1, "b41-09-1.17", 0x00000, 0x10000, 0xaf821fbc),
  LOAD8_16( CPU1, "b41-11-1.26", 0x00000+1, 0x10000, 0x823fff00),
  LOAD8_16( CPU1, "b41-10.16", 0x20000, 0x10000, 0x4ca94d77),
  LOAD8_16( CPU1, "b41-15.25", 0x20000+1, 0x10000, 0xaed7a0d0 ),
  // Level data :
  LOAD( CPU1, "b41-01.15", 0x40000, 0x80000, 0x5d072fa4),

  LOAD( GFX1, "b41-03.1", 0x00000, 0x80000, 0x736d35d0),
  LOAD( GFX2, "b41-02.7", 0x00000, 0x80000, 0x29f205d9),
  LOAD( ROM2, "b41-13.20", 0, 0x10000, 0x9e464254),
  LOAD( SMP1, "b41-04.48", 0x00000, 0x80000, 0xc668638f),
   {           NULL,          0,          0, 0, 0, 0, },
};

static struct INPUT_INFO input_bonzeadv[] =
{
   INP1( COIN1, 0x040009, 0x01 ),
   INP1( COIN2, 0x040009, 0x02 ),
   INP0( TILT, 0x04000B, 0x01 ),
   INP0( SERVICE, 0x040007, 0x80 ),

   INP0( P1_START, 0x040007, 0x40 ),
   INP0( P1_UP, 0x04000B, 0x04 ),
   INP0( P1_DOWN, 0x04000B, 0x08 ),
   INP0( P1_LEFT, 0x04000B, 0x10 ),
   INP0( P1_RIGHT, 0x04000B, 0x20 ),
   INP0( P1_B1, 0x04000B, 0x40 ),
   INP0( P1_B2, 0x04000B, 0x80 ),

   INP0( P2_START, 0x040007, 0x20 ),
   INP0( P2_UP, 0x04000D, 0x02 ),
   INP0( P2_DOWN, 0x04000D, 0x04 ),
   INP0( P2_LEFT, 0x04000D, 0x08 ),
   INP0( P2_RIGHT, 0x04000D, 0x10 ),
   INP0( P1_B1, 0x04000D, 0x20 ),
   INP0( P2_B2, 0x04000D, 0x40 ),

   END_INPUT
};

static struct DSW_DATA dsw_data_bonze_adventure_0[] =
{
    DSW_TAITO_SCREEN_TEST_DEMO,
    DSW_REGION(2), // World region
      DSW_TAITO_COINAGE_WORLD,
    DSW_DEFAULT_REGION,                    \
      DSW_TAITO_COINAGE_OLD_JAPAN,
    { NULL,		      0,	 },
};

static struct DSW_DATA dsw_data_bonze_adventure_1[] =
{
   { MSG_DIFFICULTY,	      0x03, 0x04 },
   { MSG_NORMAL,	      0x03},
   { MSG_EASY,		      0x02},
   { MSG_HARD,		      0x01},
   { MSG_HARDEST,	      0x00},
   { MSG_EXTRA_LIFE,	      0x0C, 0x04 },
   { _("50k and 150k"),          0x0C},
   { _("40k and 100k"),          0x08},
   { _("60k and 200k"),          0x04},
   { _("80k and 250k"),          0x00},
   { MSG_LIVES, 	      0x30, 0x04 },
   { "3",                     0x30},
   { "2",                     0x20},
   { "4",                     0x10},
   { "5",                     0x00},
   DSW_CONTINUE_PLAY( 0x00, 0x40),
   { _("Level test"),            0x80, 0x02 },
   { MSG_OFF,		      0x80},
   { MSG_ON,		      0x00},
   { NULL,		      0,	 },
};

static struct DSW_INFO dsw_bonzeadv[] =
{
   { 0x020000, 0xFF, dsw_data_bonze_adventure_0 },
   { 0x020020, 0xBF, dsw_data_bonze_adventure_1 },
   { 0,        0,    NULL,	},
};

static struct ROMSW_DATA romsw_data_bonze_adventure_0[] =
{
   { "Taito Japan (Jigoku Meguri)", 0x00 },
   { "Taito America",          0x01 },
   { "Taito Japan (World)",            0x02 },
   { NULL,		       0    },
};

static struct ROMSW_INFO romsw_bonzeadv[] =
{
   { 0x03FFFF, 0x02, romsw_data_bonze_adventure_0 },
   { 0,        0,    NULL },
};

static UINT8 *RAM_VIDEO;
static UINT8 *RAM_SCROLL;

static UINT8 *RAM_OBJECT;

static UINT8 *CBANK[8];
static int CChip_Bank=0;

static int CChip_ID=0x01;

static int CChipReadB(UINT32 address)
{
   int i;

   i=address&0x0FFF;

   switch(i){
      case 0x803:
	 return(CChip_ID);
      break;
      case 0xC01:
	 return(CChip_Bank);
      break;
      default:
	 /*#ifdef RAINE_DEBUG
	    if(i>0x20){
	    print_debug("CCRB[%02x][%03x](%02x)\n",CChip_Bank,i,CBANK[CChip_Bank][i]);
	    print_ingame(60,gettext("CCRB[%02x][%03x](%02x)\n"),CChip_Bank,i,CBANK[CChip_Bank][i]);
	    }
#endif*/
	 return(CBANK[CChip_Bank][i]);
      break;
   }
}

static int CChipReadW(UINT32 address)
{
   return(CChipReadB(address+1));
}

#define cval(n) CBANK[0][2*(n)+0x23]
#define current_round CBANK[0][0x21]

// Improvements with a lot of help from Ruben Panossian.
// Additional thanks to Robert Gallagher and Stephane Humbert.
//  - CLEV is correct except for last two lines
//  - CPOS is correct but probably incomplete
//  - CMAP is freely made up
// (from mame 0.57)

struct cchip_mapping
{
	UINT16 xmin;
	UINT16 xmax;
	UINT16 ymin;
	UINT16 ymax;
	unsigned char index;
};

static UINT16 CLEV[][13] =
{
/*	  map start	  player start	  player y-range  player x-range  map y-range	  map x-range	  time	 */
	{ 0x0000, 0x0018, 0x0020, 0x0030, 0x0028, 0x00D0, 0x0050, 0x0090, 0x0000, 0x0118, 0x0000, 0x0C90, 0x3800 },
	{ 0x0000, 0x0100, 0x0048, 0x0028, 0x0028, 0x0090, 0x0070, 0x00B0, 0x0000, 0x02C0, 0x0000, 0x0CA0, 0x3000 },
	{ 0x0000, 0x0518, 0x0068, 0x00B8, 0x0028, 0x00D0, 0x0068, 0x00B0, 0x02F8, 0x0518, 0x0000, 0x0EF8, 0x3000 },
	{ 0x0978, 0x0608, 0x00C8, 0x00B0, 0x0028, 0x00D0, 0x0098, 0x00C8, 0x0608, 0x06E8, 0x0000, 0x0A48, 0x2000 },
	{ 0x0410, 0x0708, 0x0070, 0x0030, 0x0028, 0x00D0, 0x0060, 0x0090, 0x0708, 0x0708, 0x0410, 0x1070, 0x3800 },
	{ 0x1288, 0x0808, 0x0099, 0x00CE, 0x0028, 0x00D0, 0x0060, 0x00C0, 0x0000, 0x0808, 0x1288, 0x1770, 0x3000 },
	{ 0x11B0, 0x0908, 0x0118, 0x0040, 0x0028, 0x00D0, 0x00B0, 0x00C0, 0x0900, 0x0910, 0x0050, 0x11B0, 0x3800 },
	{ 0x0000, 0x0808, 0x0028, 0x00B8, 0x0028, 0x00D0, 0x0070, 0x00B0, 0x0808, 0x0808, 0x0000, 0x0398, 0x1000 },
	{ 0x06F8, 0x0808, 0x0028, 0x00B8, 0x0028, 0x00D0, 0x0070, 0x00B0, 0x0808, 0x0808, 0x06F8, 0x06F8, 0x8800 },
	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 },
	{ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x6000 }
};

static struct cchip_mapping CMAP[] =
{
	{ 0x0000, 0x0250, 0x0000, 0x0100, 0x00 },
	{ 0x0250, 0x04A8, 0x0000, 0x0100, 0x01 },
	{ 0x04A8, 0x0730, 0x0000, 0x0100, 0x02 },
	{ 0x0300, 0x0800, 0x0100, 0x0200, 0x03 },
	{ 0x0730, 0x0938, 0x0000, 0x0100, 0x04 },
	{ 0x0938, 0x0A30, 0x0000, 0x0100, 0x05 },
	{ 0x0A30, 0x0AD8, 0x0000, 0x0100, 0x06 },
	{ 0x0AD8, 0x0CB0, 0x0000, 0x0100, 0x07 },
	{ 0x0C40, 0x0E80, 0x0000, 0x0100, 0x08 },
	{ 0x0000, 0x0130, 0x0100, 0x0300, 0x09 },
	{ 0x0130, 0x04F0, 0x0200, 0x0300, 0x0A },
	{ 0x04F0, 0x0808, 0x0200, 0x0280, 0x0B },
	{ 0x04F0, 0x0808, 0x0280, 0x0300, 0x0C },
	{ 0x0808, 0x08C8, 0x0200, 0x0300, 0x0D },
	{ 0x08C8, 0x09F8, 0x0200, 0x0300, 0x0E },
	{ 0x09F8, 0x0B20, 0x0200, 0x0300, 0x0F },
	{ 0x0B20, 0x0CC0, 0x0200, 0x0300, 0x10 },
	{ 0x0CC0, 0x0E00, 0x0200, 0x0300, 0x11 },
	{ 0x0000, 0x0148, 0x0500, 0x0600, 0x12 },
	{ 0x0148, 0x0300, 0x0500, 0x0600, 0x13 },
	{ 0x0300, 0x06E8, 0x0500, 0x0600, 0x14 },
	{ 0x06E8, 0x0848, 0x0500, 0x0600, 0x15 },
	{ 0x0848, 0x0A68, 0x0500, 0x0600, 0x16 },
	{ 0x0A68, 0x0C00, 0x0500, 0x0600, 0x17 },
	{ 0x0200, 0x0480, 0x0400, 0x0500, 0x18 },
	{ 0x0200, 0x0400, 0x0300, 0x0400, 0x19 },
	{ 0x0400, 0x0688, 0x0300, 0x0400, 0x1A },
	{ 0x0688, 0x08F0, 0x0300, 0x0400, 0x1B },
	{ 0x08F0, 0x0A80, 0x0300, 0x0400, 0x1C },
	{ 0x0800, 0x0B90, 0x0400, 0x0500, 0x1D },
	{ 0x0B90, 0x1100, 0x0400, 0x0500, 0x1E },
	{ 0x09E8, 0x0C00, 0x0600, 0x0700, 0x1F },
	{ 0x08A0, 0x09E8, 0x0600, 0x0700, 0x20 },
	{ 0x06F0, 0x08A0, 0x0600, 0x0700, 0x21 },
	{ 0x0388, 0x06F0, 0x0600, 0x0700, 0x22 },
	{ 0x0368, 0x0388, 0x0600, 0x0700, 0x23 },
	{ 0x0190, 0x0368, 0x0600, 0x0700, 0x24 },
	{ 0x0000, 0x0190, 0x0600, 0x0700, 0x25 },
	{ 0x0000, 0x0280, 0x0700, 0x0800, 0x26 },
	{ 0x0380, 0x08C0, 0x0700, 0x0800, 0x27 },
	{ 0x08C0, 0x0A28, 0x0700, 0x0800, 0x28 },
	{ 0x0A20, 0x0CC0, 0x0700, 0x0800, 0x29 },
	{ 0x0CC0, 0x0DF0, 0x0700, 0x0800, 0x2A },
	{ 0x0DF0, 0x0F18, 0x0700, 0x0800, 0x2B },
	{ 0x0F18, 0x1040, 0x0700, 0x0800, 0x2C },
	{ 0x1040, 0x1280, 0x0700, 0x0800, 0x2D },
	{ 0x1200, 0x1368, 0x0800, 0x0900, 0x2E },
	{ 0x1368, 0x1508, 0x0800, 0x0900, 0x2F },
	{ 0x1508, 0x1658, 0x0800, 0x0900, 0x30 },
	{ 0x1658, 0x1900, 0x0800, 0x0900, 0x31 },
	{ 0x1480, 0x1620, 0x0700, 0x0800, 0x32 },
	{ 0x1620, 0x1900, 0x0700, 0x0800, 0x33 },

	     /* round 6 not fully mapped */

	{ 0x1200, 0x1580, 0x0000, 0x0100, 0x4C },
	{ 0x10A0, 0x1380, 0x0900, 0x0A00, 0x4D },
	{ 0x0EC0, 0x10A0, 0x0900, 0x0A00, 0x4E },
	{ 0x0D80, 0x0EC0, 0x0900, 0x0A00, 0x4F },
	{ 0x09A8, 0x0D80, 0x0900, 0x0A00, 0x50 },
	{ 0x08A0, 0x09A8, 0x0900, 0x0A00, 0x51 },
	{ 0x0710, 0x08A0, 0x0900, 0x0A00, 0x52 },
	{ 0x0480, 0x0710, 0x0900, 0x0A00, 0x53 },
	{ 0x0180, 0x0480, 0x0900, 0x0A00, 0x54 },
	{ 0x0000, 0x0180, 0x0900, 0x0A00, 0x55 },
	{ 0x0000, 0x0580, 0x0800, 0x0900, 0x56 },
	{ 0x0580, 0x0900, 0x0800, 0x0900, 0x57 }
};

static UINT16 CPOS[][4] =
{
/*    map start       player start   */
	{ 0x0000, 0x0018, 0x0020, 0x00A8 },
	{ 0x01E0, 0x0018, 0x0070, 0x0098 },
	{ 0x0438, 0x0018, 0x0070, 0x00A8 },
	{ 0x04A0, 0x0018, 0x0070, 0x0080 },
	{ 0x06B8, 0x0018, 0x0078, 0x0078 },
	{ 0x08C8, 0x0018, 0x0070, 0x0028 },
	{ 0x09C0, 0x0018, 0x0070, 0x00A8 },
	{ 0x0A68, 0x0018, 0x0070, 0x0058 },
	{ 0x0C40, 0x0018, 0x0070, 0x0040 },
	{ 0x0000, 0x0208, 0x0040, 0x0070 },
	{ 0x0080, 0x0218, 0x00B0, 0x0080 },
	{ 0x0450, 0x01F8, 0x0090, 0x0030 },
	{ 0x0450, 0x0218, 0x00A0, 0x00A0 },
	{ 0x07A8, 0x0218, 0x0090, 0x0060 },
	{ 0x0840, 0x0218, 0x0088, 0x0060 },
	{ 0x0958, 0x0218, 0x00A0, 0x0070 },
	{ 0x0A98, 0x0218, 0x0088, 0x0050 },
	{ 0x0C20, 0x0200, 0x00A0, 0x0028 },
	{ 0x0000, 0x0518, 0x0068, 0x00B0 },
	{ 0x00D0, 0x0518, 0x0078, 0x0060 },
	{ 0x02F0, 0x0518, 0x0078, 0x0048 },
	{ 0x0670, 0x0518, 0x0078, 0x0048 },
	{ 0x07D8, 0x0518, 0x0070, 0x0060 },
	{ 0x09E8, 0x0500, 0x0080, 0x0080 },
	{ 0x02E8, 0x04B0, 0x0080, 0x0090 },
	{ 0x0278, 0x0318, 0x0078, 0x00A8 },
	{ 0x0390, 0x0318, 0x0070, 0x00B8 },
	{ 0x0608, 0x0318, 0x0080, 0x0058 },
	{ 0x0878, 0x0318, 0x0078, 0x0098 },
	{ 0x0908, 0x0418, 0x0078, 0x0030 },
	{ 0x0B20, 0x0418, 0x0070, 0x0080 },
	{ 0x0A48, 0x0608, 0x00C0, 0x0068 },
	{ 0x0930, 0x0608, 0x00B8, 0x0080 },
	{ 0x07E8, 0x0608, 0x00B8, 0x0078 },
	{ 0x0630, 0x0608, 0x00C0, 0x0028 },
	{ 0x02C8, 0x0608, 0x00C0, 0x0090 },
	{ 0x02B0, 0x0608, 0x00B8, 0x0090 },
	{ 0x00D8, 0x0608, 0x00B8, 0x00A0 },
	{ 0x0020, 0x0610, 0x00B8, 0x00A8 },
	{ 0x0560, 0x0708, 0x0068, 0x0090 },
	{ 0x0860, 0x0708, 0x0060, 0x0090 },
	{ 0x09C0, 0x0708, 0x0068, 0x0080 },
	{ 0x0C58, 0x0708, 0x0068, 0x0070 },
	{ 0x0D80, 0x0708, 0x0070, 0x00B0 },
	{ 0x0EA8, 0x0708, 0x0070, 0x00B0 },
	{ 0x0FC0, 0x0708, 0x0080, 0x0030 },
	{ 0x1288, 0x0808, 0x0099, 0x00CE },
	{ 0x12F0, 0x0808, 0x0078, 0x0028 },
	{ 0x1488, 0x0808, 0x0080, 0x0070 },
	{ 0x1600, 0x0808, 0x0080, 0x0090 },
	{ 0x1508, 0x0708, 0x0080, 0x0090 },
	{ 0x1770, 0x0728, 0x0080, 0x0080 },
	{ 0x1770, 0x0650, 0x00D8, 0x0070 },
	{ 0x1578, 0x05D0, 0x0098, 0x0060 },
	{ 0x1508, 0x05E8, 0x0040, 0x0088 },
	{ 0x1658, 0x0528, 0x0088, 0x0088 },
	{ 0x1508, 0x04F8, 0x0080, 0x0060 },
	{ 0x1500, 0x03E8, 0x0090, 0x0058 },
	{ 0x1518, 0x0368, 0x0090, 0x0058 },
	{ 0x15A8, 0x0268, 0x0080, 0x0058 },
	{ 0x1650, 0x0250, 0x0080, 0x0058 },
	{ 0x1630, 0x02B0, 0x0088, 0x0060 },
	{ 0x16E0, 0x0398, 0x0068, 0x0068 },
	{ 0x1680, 0x0528, 0x00A8, 0x0070 },
	{ 0x1658, 0x0528, 0x0088, 0x0088 },
	{ 0x1640, 0x05E8, 0x0088, 0x0088 },
	{ 0x1770, 0x0588, 0x0090, 0x0088 },
	{ 0x1770, 0x0508, 0x0098, 0x0088 },
	{ 0x1770, 0x0450, 0x00D8, 0x0060 },
	{ 0x1770, 0x0330, 0x00C0, 0x0080 },
	{ 0x1740, 0x02A8, 0x0090, 0x0058 },
	{ 0x16C8, 0x0178, 0x0080, 0x0060 },
	{ 0x1508, 0x0208, 0x0028, 0x0060 },
	{ 0x1618, 0x0110, 0x0078, 0x0068 },
	{ 0x1770, 0x0118, 0x00C0, 0x0060 },
	{ 0x1698, 0x0000, 0x0080, 0x0050 },
	{ 0x1500, 0x0000, 0x0080, 0x0048 },
	{ 0x1140, 0x0908, 0x00B8, 0x0068 },
	{ 0x0FE8, 0x0908, 0x00B8, 0x0098 },
	{ 0x0E08, 0x0908, 0x00B8, 0x0070 },
	{ 0x0CD0, 0x0908, 0x00B8, 0x00B0 },
	{ 0x08F8, 0x0908, 0x00B0, 0x0080 },
	{ 0x07E8, 0x0908, 0x00B8, 0x00B8 },
	{ 0x0660, 0x0908, 0x00B0, 0x0098 },
	{ 0x03D0, 0x0908, 0x00B0, 0x0038 },
	{ 0x00D0, 0x0908, 0x00B0, 0x0090 },
	{ 0x0000, 0x0808, 0x0028, 0x00B8 },
	{ 0x06F8, 0x0808, 0x0028, 0x00B8 }
};

static void WriteRestartPos(void)
{
	int n;

	int x = cval(0) + 256 * cval(1) + cval(4) + 256 * cval(5);
	int y = cval(2) + 256 * cval(3) + cval(6) + 256 * cval(7);

	for (n = 0; n < sizeof CMAP / sizeof CMAP[0]; n++)
	{
		if (x >= CMAP[n].xmin && x < CMAP[n].xmax &&
		    y >= CMAP[n].ymin && y < CMAP[n].ymax)
		{
			int i;

			for (i = 0; i < 4; i++)
			{
				UINT16 v = CPOS[CMAP[n].index][i];

				cval(2 * i + 0) = v & 0xff;
				cval(2 * i + 1) = v >> 8;
			}

			return;
		}
	}
}

static void WriteLevelData(void)
{
  int i;

  for (i = 0; i < 13; i++)
    {
      UINT16 v = CLEV[current_round][i];

      cval(2 * i + 0) = v & 0xff;
      cval(2 * i + 1) = v >> 8;
    }
}

static void CChipWriteB(UINT32 address, int data)
{
   int i;
   // int ta;

   // ta=CBANK[0][0x21];
   i=address&0x0FFF;
   data&=0xFF;

   switch(i){
      case 0x11:				// cchip[0][0x011]: COIN LEDS
	 CBANK[CChip_Bank][i]=data;
	 switch_led(0,(data>>4)&1);		// Coin A [Coin Inserted]
	 switch_led(1,(data>>5)&1);		// Coin B [Coin Inserted]
	 switch_led(2,((data>>6)&1)^1); 	// Coin A [Ready for coins]
	 //switch_led(3,((data>>7)&1)^1);	// Coin B [Ready for coins]
      break;
      case 0x1D:				// cchip[0][0x01D]: GENERATE LEVEL RESTART POS
	 WriteRestartPos();
      break;
      case 0x1F:				// cchip[0][0x01F]: GENERATE LEVEL START POS

	WriteLevelData();

/*	     if(data==0x55){ */
/*	     #ifdef RAINE_DEBUG */
/*		print_debug("LEVELSTART(%02x)\n",ta); */
/*		print_ingame(60,gettext("LEVELSTART(%02x)"),ta); */
/*	     #endif */
/*	     CBANK[0][0x01F]=0x00; */
/*	   if(ta<32){ */
/*	   CBANK[0][0x023]=CLEV[ta].MapXStart&0xFF; */
/*	   CBANK[0][0x025]=(CLEV[ta].MapXStart>>8)&0xFF; */
/*	   CBANK[0][0x027]=CLEV[ta].MapYStart&0xFF; */
/*	   CBANK[0][0x029]=(CLEV[ta].MapYStart>>8)&0xFF; */

/*	   CBANK[0][0x02B]=CLEV[ta].ScrollXStart&0xFF; */
/*	   CBANK[0][0x02D]=(CLEV[ta].ScrollXStart>>8)&0xFF; */

/*	   CBANK[0][0x02F]=CLEV[ta].ScrollYStart&0xFF; */
/*	   CBANK[0][0x031]=(CLEV[ta].ScrollYStart>>8)&0xFF; */

/*	   CBANK[0][0x033]=CLEV[ta].ScrollYMin&0xFF; */
/*	   CBANK[0][0x035]=(CLEV[ta].ScrollYMin>>8)&0xFF; */
/*	   CBANK[0][0x037]=CLEV[ta].ScrollYMax&0xFF; */
/*	   CBANK[0][0x039]=(CLEV[ta].ScrollYMax>>8)&0xFF; */

/*	   CBANK[0][0x03B]=CLEV[ta].ScrollXMin&0xFF; */
/*	   CBANK[0][0x03D]=(CLEV[ta].ScrollXMin>>8)&0xFF; */
/*	   CBANK[0][0x03F]=CLEV[ta].ScrollXMax&0xFF; */
/*	   CBANK[0][0x041]=(CLEV[ta].ScrollXMax>>8)&0xFF; */

/*	   CBANK[0][0x043]=CLEV[ta].MapYMin&0xFF; */
/*	   CBANK[0][0x045]=(CLEV[ta].MapYMin>>8)&0xFF; */
/*	   CBANK[0][0x047]=CLEV[ta].MapYMax&0xFF; */
/*	   CBANK[0][0x049]=(CLEV[ta].MapYMax>>8)&0xFF; */

/*	   CBANK[0][0x04B]=CLEV[ta].MapXMin&0xFF; */
/*	   CBANK[0][0x04D]=(CLEV[ta].MapXMin>>8)&0xFF; */
/*	   CBANK[0][0x04F]=CLEV[ta].MapXMax&0xFF; */
/*	   CBANK[0][0x051]=(CLEV[ta].MapXMax>>8)&0xFF; */

/*	   CBANK[0][0x053]=CLEV[ta].LevelTime&0xFF; */
/*	   CBANK[0][0x055]=(CLEV[ta].LevelTime>>8)&0xFF; */
/*	   } */
/*	     } */
      break;
      case 0x21:	// LEVEL NUMBER
	 CBANK[CChip_Bank][i]=data;
      break;
      case 0x803:	// C-CHIP ID
      break;
      case 0xC01:	// C-CHIP BANK SELECT
	 CChip_Bank=data&7;
      break;
      default:
	 CBANK[CChip_Bank][i]=data;
	 if (!CChip_Bank && i <= 0xd) {
	   init_inputs();
	 }
      break;
   }
}

static void CChipWriteW(UINT32 address, int data)
{
   CChipWriteB(address+1,data&0xFF);
}

static void load_bonzeadv()
{
   RAMSize=0x54000;

   if(!(RAM=AllocateMem(RAMSize))) return;
   if(!(GFX_FG0=AllocateMem(0x4000))) return;

   CBANK[0]=RAM+0x40000;	// C-CHIP BANKS ($800000-$800FFF)
   CBANK[1]=RAM+0x40800;
   CBANK[2]=RAM+0x41000;
   CBANK[3]=RAM+0x41800;
   CBANK[4]=RAM+0x42000;
   CBANK[5]=RAM+0x42800;
   CBANK[6]=RAM+0x43000;
   CBANK[7]=RAM+0x43800;

   /*-----[Sound Setup]-----*/

   Z80ROM=RAM+0x44000;
   memcpy(Z80ROM,load_region[REGION_CPU2],0x10000);

   AddTaitoYM2610(0x02EE, 0x028D, 0x10000);

   /*-----------------------*/

   RAM_VIDEO  = RAM+0x04000;
   RAM_SCROLL = RAM+0x20060;
   RAM_OBJECT = RAM+0x14000;

   // Fix SOUND ERROR

   ROM[0x26568]=0x60;

   WriteWord68k(&ROM[0x26558],0x4E71);

   WriteWord68k(&ROM[0x26540],0x4E71);

   // Fix BAD HARDWARE

   ROM[0x07526]=0x60;

   // Enable last dsw

   WriteWord68k(&ROM[0x7572],0x4E71);

   // 68000 Speed Hack

   WriteLong68k(&ROM[0x013DE],0x13FC0000);	//	move.b	#$00,$AA0000
   WriteLong68k(&ROM[0x013E2],0x00AA0000);	//
   WriteWord68k(&ROM[0x013D6],0x60E6-8);	//	bra.s	loop

   memset(RAM+0x00000,0x00,0x44000);

   tc0110pcr_init(RAM+0x16000, 1);

   set_colour_mapper(&col_map_xbbb_bbgg_gggr_rrrr);
   InitPaletteMap(RAM+0x16000, 0x100, 0x10, 0x8000);


   // Init tc0002obj emulation
   // ------------------------

   tc0002obj.RAM	= RAM_OBJECT;
   tc0002obj.ofs_x	= 0;
   tc0002obj.ofs_y	= -16;
   tc0002obj.MASK = NULL;

   setup_asuka_layers(RAM_VIDEO,RAM_SCROLL,GFX_FG0,16,16,&RAM[0x20010]);
   tc0100scn_0_copy_gfx_fg0(ROM+0x011A92, 0x1000);

/*
 *  StarScream Stuff follows
 */

   ByteSwap(ROM,0x40000);
   ByteSwap(RAM,0x40000);

   AddMemFetch(0x000000, 0x03FFFF, ROM+0x000000-0x000000);		// 68000 ROM
   AddMemFetch(-1, -1, NULL);

   AddReadByte(0x000000, 0x03FFFF, NULL, ROM+0x000000); 		// 68000 ROM
   AddReadByte(0x10C000, 0x10FFFF, NULL, RAM+0x000000); 		// MAIN RAM
   AddReadByte(0x080000, 0x0FFFFF, NULL, ROM+0x040000); 		// DATA ROM
   AddReadByte(0xC00000, 0xC0FFFF, NULL, RAM+0x004000); 		// SCREEN RAM
   AddReadByte(0xD00000, 0xD01FFF, NULL, RAM_OBJECT);			// OBJECT RAM
   AddReadByte(0x390000, 0x39000F, NULL, RAM+0x020000); 		// DSWA
   AddReadByte(0x3B0000, 0x3B000F, NULL, RAM+0x020020); 		// DSWB
   AddReadByte(0x3E0000, 0x3E0003, tc0140syt_read_main_68k, NULL);	// SOUND
   AddReadByte(0x800000, 0x800FFF, CChipReadB, NULL);			// C-CHIP
   AddReadByte(-1, -1, NULL, NULL);

   AddReadWord(0x000000, 0x03FFFF, NULL, ROM+0x000000); 		// 68000 ROM
   AddReadWord(0x10C000, 0x10FFFF, NULL, RAM+0x000000); 		// MAIN RAM
   AddReadWord(0x080000, 0x0FFFFF, NULL, ROM+0x040000); 		// DATA ROM
   AddReadWord(0xC00000, 0xC0FFFF, NULL, RAM+0x004000); 		// SCREEN RAM
   AddReadWord(0xD00000, 0xD01FFF, NULL, RAM_OBJECT);			// OBJECT RAM
   AddReadWord(0x200000, 0x200007, tc0110pcr_rw, NULL); 		// COLOR RAM
   AddReadWord(0x390000, 0x39000F, NULL, RAM+0x020000); 		// DSWA
   AddReadWord(0x3B0000, 0x3B000F, NULL, RAM+0x020020); 		// DSWB
   AddReadWord(0x3D0000, 0x3D000F, NULL, RAM+0x020040); 		// ???
   AddReadWord(0x800000, 0x800FFF, CChipReadW, NULL);			// C-CHIP
   AddReadWord(-1, -1, NULL, NULL);

   AddWriteByte(0x10C000, 0x10FFFF, NULL, RAM+0x000000);		// MAIN RAM
   AddWriteByte(0xC00000, 0xC0FFFF, NULL, RAM+0x004000);		// SCREEN RAM
   AddWriteByte(0xD00000, 0xD01FFF, NULL, RAM_OBJECT);			// OBJECT RAM
   AddWriteBW(0x3A0000, 0x3A0001, NULL, RAM+0x020010);		// ???
   AddWriteByte(0x3E0000, 0x3E0003, tc0140syt_write_main_68k, NULL);	// SOUND
   AddWriteByte(0x800000, 0x800FFF, CChipWriteB, NULL); 		// C-CHIP
   AddWriteByte(0xAA0000, 0xAA0001, Stop68000, NULL);			// Trap Idle 68000
   AddWriteByte(-1, -1, NULL, NULL);

   AddWriteWord(0x10C000, 0x10FFFF, NULL, RAM+0x000000);		// MAIN RAM
   AddWriteWord(0xC00000, 0xC0FFFF, NULL, RAM+0x004000);		// SCREEN RAM
   AddWriteWord(0xD00000, 0xD01FFF, NULL, RAM_OBJECT);			// OBJECT RAM
   AddWriteWord(0x200000, 0x200007, tc0110pcr_ww, NULL);		// COLOR RAM
   AddWriteWord(0xC20000, 0xC2000F, NULL, RAM+0x020060);		// SCROLL RAM
   AddWriteWord(0x3C0000, 0x3C000F, NULL, RAM+0x020030);		// ???
   AddWriteWord(0x800000, 0x800FFF, CChipWriteW, NULL); 		// C-CHIP
   AddWriteWord(-1, -1, NULL, NULL);

   AddInitMemory();	// Set Starscream mem pointers...
}

static void execute_bonzeadv(void)
{
   cpu_execute_cycles(CPU_68K_0, CPU_FRAME_MHz(12,60)); // M68000 12MHz (60fps)
   cpu_interrupt(CPU_68K_0, 4);

   Taito2610_Frame();			// Z80 and YM2610
}

static struct VIDEO_INFO video_bonzeadv =
{
   DrawAsuka,
   320,
   224,
   32,
   VIDEO_ROTATE_NORMAL|VIDEO_ROTATABLE,
   asuka_gfx
};
static struct DIR_INFO dir_bonzeadv[] =
{
   { "bonze_adventure", },
   { "bonzeadv", },
   { NULL, },
};
GME( bonzeadv, "Bonze's Adventure", TAITO, 1988, GAME_PLATFORM,
	.long_name_jpn = "�n�������� American",
	.romsw = romsw_bonzeadv,
	.board = "B41",
);
