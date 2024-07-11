/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <nds.h>
#include <nds/fifocommon.h>
#include <nds/fifomessages.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include "args.h"
#include "file_browse.h"
#include "font.h"
#include "hbmenu_consolebg.h"
#include "iconTitle.h"
#include "skin.h"
#include "nds_loader_arm9.h"

#define BG_256_COLOR (BIT(7))
#define FlashBase_S98	0x09000000

using namespace std;

volatile bool guiEnabled = false;

void InitGUI(void) {
	if (guiEnabled)return;
	guiEnabled = true;
	iconTitleInit();
	videoSetModeSub(MODE_4_2D);
	vramSetBankC(VRAM_C_SUB_BG);
	int bgSub = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);
	PrintConsole *console = consoleInit(0, 0, BgType_Text4bpp, BgSize_T_256x256, 4, 6, false, false);
	dmaCopy(hbmenu_consolebgBitmap, bgGetGfxPtr(bgSub), 256*256);
	ConsoleFont font;
	font.gfx = (u16*)fontTiles;
	font.pal = (u16*)fontPal;
	font.numChars = 95;
	font.numColors = (fontPalLen / 2);
	font.bpp = 4;
	font.asciiOffset = 32;
	font.convertSingleColor = true;
	consoleSetFont(console, &font);
	dmaCopy(hbmenu_consolebgPal, BG_PALETTE_SUB, 256*2);
	BG_PALETTE_SUB[255] = RGB15(31,31,31);
	keysSetRepeat(25,5);
	consoleSetWindow(console, 1, 1, 30, 22);
}


void Pixel_SUB(u16* screen, s16 x, s16 y, uint16 palet) {
	uint16	bgsubAddress, h;

	if (x < 0 || x > 256-1 || y < 0 || y > 192-1)return;

	bgsubAddress = (x>>3)*32 + ((x&7)>>1) + (y>>3)*1024 + ((y&7)<<2);
	h = screen[bgsubAddress];

	if ((x & 1 ) == 0) {
		h = palet | (h&0xff00);
	} else {
		h = (palet * 0x100) | ( h & 0xff);
	}
	screen[bgsubAddress] = h;
}

void ClearBG(uint16* screen,uint16 color) {
	int x = 0, y = 0;
	for(y = 0; y < 192; y++) {
		for(x = 0; x < 256; x++)screen[(y)*256 + (x)] = color;
	}
}

void ClearBG_SUB(uint16* screen, uint16 palet) {
	int x = 0, y = 0;

	for (y = 0; y < 192; y++) {
		for(x = 0; x < 256; x++)Pixel_SUB(screen, x, y, palet);
	}
}

void InitGUIForGBA() {
	
	defaultExceptionHandler();
	
	u16* MainScreen = VRAM_A;
	u16* SubScreen = (u16*)BG_TILE_RAM_SUB(1);

	int	i;

	vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_SUB_BG, VRAM_D_MAIN_BG);
	powerOn(POWER_ALL);

	videoSetMode(MODE_FB0 | DISPLAY_BG2_ACTIVE);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE );
	REG_BG0CNT_SUB = BG_256_COLOR | BG_MAP_BASE(0) | BG_TILE_BASE(1);
	uint16* map1 = (uint16*)BG_MAP_RAM_SUB(0);
	for(i=0;i<(256*192/8/8);i++)map1[i]=i;
	lcdMainOnTop();
	ClearBG(MainScreen, RGB15(31,31,31));
	BG_PALETTE_SUB[0] = RGB15(31,31,31);
	BG_PALETTE_SUB[1] = RGB15(0,0,0);
	BG_PALETTE_SUB[2] = RGB15(29,0,0);
	BG_PALETTE_SUB[3] = RGB15(0,20,0);
	BG_PALETTE_SUB[4] = RGB15(0,31,31);
	BG_PALETTE_SUB[5] = RGB15(0,0,31);
	BG_PALETTE_SUB[6] = RGB15(31,31,0);
	ClearBG_SUB( SubScreen, 0 );
	swiWaitForVBlank();
}

u16 Read_S98NOR_ID() {
	*((vu16*)(FlashBase_S98)) = 0xF0;	
	*((vu16*)(FlashBase_S98+0x555*2)) = 0xAA;
	*((vu16*)(FlashBase_S98+0x2AA*2)) = 0x55;
	*((vu16*)(FlashBase_S98+0x555*2)) = 0x90;
	return *((vu16*)(FlashBase_S98+0xE*2));
}

void SetKernelRomPage() {
	*(vu16*)0x09FE0000 = 0xD200;
	*(vu16*)0x08000000 = 0x1500;
	*(vu16*)0x08020000 = 0xD200;
	*(vu16*)0x08040000 = 0x1500;
	*(vu16*)0x09880000 = 0x8002; // Kernel section of NorFlash
	*(vu16*)0x09FC0000 = 0x1500;
}


void gba_frame() {
	int	ret;
	int	x = 0, y = 0;
	u16	*pDstBuf1;
	u16	*pDstBuf2;
	
	if (access("/gbaframe.bmp", F_OK) == 0) {
		ret = LoadSkin(2, "/gbaframe.bmp");
		if(ret)return;
	}
	
	if (access("/GBA_SIGN/gbaframe.bmp", F_OK) == 0) {
		ret = LoadSkin(2, "/GBA_SIGN/gbaframe.bmp");
		if(ret)return;
	}

	if (access("/_system_/gbaframe.bmp", F_OK) == 0) {
		ret = LoadSkin(2, "/_system_/gbaframe.bmp");
		if(ret)return;
	}
	
	if (access("/ttmenu/gbaframe.bmp", F_OK) == 0) {
		ret = LoadSkin(2, "/ttmenu/gbaframe.bmp");
		if(ret)return;
	}

	pDstBuf1 = (u16*)0x06000000;
	pDstBuf2 = (u16*)0x06020000;
	for(y = 0; y < 192; y++) {
		for(x = 0; x < 256; x++) {
			pDstBuf1[x] = 0x0000;
			pDstBuf2[x] = 0x0000;
		}
		pDstBuf1 += 256;
		pDstBuf2 += 256;
	}
}

void gbaMode() {
	InitGUIForGBA();
	
	sysSetCartOwner(true);
	
	swiWaitForVBlank();
	
	if (Read_S98NOR_ID() == 0x223D)SetKernelRomPage();
	
	swiWaitForVBlank();
	
	videoSetMode(0);
	videoSetModeSub(0);

	vramSetPrimaryBanks(VRAM_A_MAIN_BG, VRAM_B_MAIN_BG, VRAM_C_MAIN_BG, VRAM_D_MAIN_BG);

	if(PersonalData->gbaScreen) { lcdMainOnBottom(); } else { lcdMainOnTop(); }

	gba_frame();
	
	sysSetCartOwner(false);
	fifoSendValue32(FIFO_USER_01, 1);
	REG_IME = 0;
	while(1)swiWaitForVBlank();
} 

int stop(void) {
	while (1) {
		swiWaitForVBlank();
		scanKeys();
		if (keysDown() != 0)break;
	}
	return 0;
}

int FileBrowser() {
	InitGUI();
	consoleClear();
	vector<string> extensionList = argsGetExtensionList();
	chdir("/nds");
	while(1) {
		string filename = browseForFile(extensionList);
		// Construct a command line
		vector<string> argarray;
		if (!argsFillArray(filename, argarray)) {
			printf("Invalid NDS or arg file selected\n");
		} else {
			iprintf("Running %s with %d parameters\n", argarray[0].c_str(), argarray.size());
			// Make a copy of argarray using C strings, for the sake of runNdsFile
			vector<const char*> c_args;
			for (const auto& arg: argarray) { c_args.push_back(arg.c_str()); }
			// Try to run the NDS file with the given arguments
			int err = runNdsFile(c_args[0], c_args.size(), &c_args[0]);
			iprintf("Start failed. Error %i\n", err);
		}
		argarray.clear();
	}
	return 0;
}

int main(int argc, char **argv) {
	// overwrite reboot stub identifier
	// so tapping power on DSi returns to DSi menu
	extern u64 *fake_heap_end;
	*fake_heap_end = 0;
	if (!fatInitDefault()) {
		InitGUI();
		consoleClear();
		printf ("\n\n\n\n\n\n\n\n\n\n       FAT init failed!       \n");
		return stop();
	}
	swiWaitForVBlank();
	scanKeys();
	switch (keysDown()) {
		case KEY_A: {
			if (!isDSiMode()) { gbaMode(); } else { FileBrowser(); }
		} break;
		case KEY_B: FileBrowser(); break;
		default: {
			if((access("/r4tf.nds", F_OK) == 0)) {
				return runNdsFile("/r4tf.nds", 0, NULL);
			} else if((access("/boot.nds", F_OK) == 0)) {
				return runNdsFile("/boot.nds", 0, NULL);
			} else {
				FileBrowser();
			}
		} break;
	}
	return stop();
}

