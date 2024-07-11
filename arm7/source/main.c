#include <nds.h>

volatile bool switchedMode = false;
volatile bool exitflag = false;

// libnds messed up the original SWI bios call ASM.
// They used r0 instead of r2. This reimplementation fixes that issue for now.
extern void swiSwitchToGBAModeFixed();

void powerButtonCB() { exitflag = true; }

void gbaMode() {
	vu32	vr;

	REG_IME = IME_DISABLE;
	for(vr = 0; vr < 0x1000; vr++);	// Wait ARM9

	if (PersonalData->gbaScreen) {
		writePowerManagement(PM_CONTROL_REG, PM_BACKLIGHT_BOTTOM | PM_SOUND_AMP);
	} else {
		writePowerManagement(PM_CONTROL_REG, PM_BACKLIGHT_TOP | PM_SOUND_AMP);
	}
	swiSwitchToGBAModeFixed();
	while(1);
}

void fifoCheckHandler() {
	if (!switchedMode) {
		if (fifoCheckValue32(FIFO_USER_01)) {
			switchedMode = true;
			gbaMode();
		}
	}
}

void VblankHandler() { fifoCheckHandler(); }
void VcountHandler() { inputGetAndSend(); }

int main(void) {
	readUserSettings();
	ledBlink(0);
	
	irqInit();
	
	initClockIRQ();
	fifoInit();
	touchInit();
	
	SetYtrigger(80);
	
	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);
	
	irqEnable(IRQ_VBLANK | IRQ_VCOUNT);
	
	setPowerButtonCB(powerButtonCB);
	
	if (REG_SNDEXTCNT != 0) {
		i2cWriteRegister(0x4A, 0x12, 0x00);	// Press power-button for auto-reset
		i2cWriteRegister(0x4A, 0x70, 0x01);	// Bootflag = Warmboot/SkipHealthSafety
	}
	while(1)swiWaitForVBlank();
	return 0;
}

