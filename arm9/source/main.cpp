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
#include "tonccpy.h"
#include "nds_loader_arm9.h"

#define BG_256_COLOR (BIT(7))
#define FlashBase_S98	0x09000000

using namespace std;

volatile bool guiEnabled = false;
volatile bool gbaGuiEnabled = false;

const bool BlankScreenOnBoot = true;

void InitGUI(void) {
	if (guiEnabled)return;
	guiEnabled = true;
	gbaGuiEnabled = false;
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

void InitGUIForGBA() {
	if (gbaGuiEnabled)return;
	gbaGuiEnabled = true;
	guiEnabled = false;
	videoSetMode(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);
	vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
	vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	vramSetBankD(VRAM_D_LCD);
	// for the main screen
	REG_BG3CNT = BG_BMP16_256x256 | BG_BMP_BASE(0) | BG_WRAP_OFF;
	REG_BG3PA = 1 << 8; //scale x
	REG_BG3PB = 0; //rotation x
	REG_BG3PC = 0; //rotation y
	REG_BG3PD = 1 << 8; //scale y
	REG_BG3X = 0; //translation x
	REG_BG3Y = 0; //translation y*/
	toncset((void*)BG_BMP_RAM(0),0,0x18000);
	toncset((void*)BG_BMP_RAM(8),0,0x18000);
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

void LoadGBAFrame() {
	InitGUIForGBA();
	if ((access("/gbaframe.bmp", F_OK) == 0) && LoadSkin(3, "/gbaframe.bmp"))return;
	if ((access("/GBA_SIGN/gbaframe.bmp", F_OK) == 0) && LoadSkin(3, "/GBA_SIGN/gbaframe.bmp"))return;
	if ((access("/_system_/gbaframe.bmp", F_OK) == 0) && LoadSkin(3, "/_system_/gbaframe.bmp"))return;
	if ((access("/ttmenu/gbaframe.bmp", F_OK) == 0) && LoadSkin(3, "/ttmenu/gbaframe.bmp"))return;
}

void gbaMode() {
	sysSetCartOwner(true);
	
	swiWaitForVBlank();
	
	if (Read_S98NOR_ID() == 0x223D)SetKernelRomPage();
	
	LoadGBAFrame();
	
	if(PersonalData->gbaScreen) { lcdMainOnBottom(); } else { lcdMainOnTop(); }
	
	sysSetCartOwner(false);
	fifoSendValue32(FIFO_USER_01, 1);
	REG_IME = 0;
	irqDisable(IRQ_VBLANK);
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
	while(1) {
		swiWaitForVBlank();
		scanKeys();
		if (!keysHeld())break;
	}
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
	if (BlankScreenOnBoot)InitGUIForGBA();
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
		case KEY_B: {
			if (!isDSiMode()) { gbaMode(); } else { FileBrowser(); }
		} break;
		case KEY_X: {
			if((access("/Misc.nds", F_OK) == 0)) {
				runNdsFile("/Misc.nds", 0, NULL);
			} else {
				FileBrowser();
			}
		} break;
		case KEY_Y: {
			if((access("/Misc2.nds", F_OK) == 0)) {
				runNdsFile("/Misc2.nds", 0, NULL);
			} else {
				FileBrowser();
			}
		} break;
		case 0: {
			if((access("/r4tf.nds", F_OK) == 0)) {
				runNdsFile("/r4tf.nds", 0, NULL);
			} else if((access("/boot.nds", F_OK) == 0)) {
				runNdsFile("/boot.nds", 0, NULL);
			} else {
				FileBrowser();
			}
		} break;
		default: FileBrowser(); break;
	}
	return stop();
}

