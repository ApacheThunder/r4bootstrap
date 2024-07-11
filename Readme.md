# R4 Auto Boot Bootstrap

This is a modified build of HBMenu with UI enhancements but setup to act as a kernel/firmware replacement to original R4.
(though can use this with other carts if you provide your own DLDI file).

* Holding no button on boot cause it to auto boot R4TF.nds if present or Boot.nds if present.
* Holding A button will cause quick boot into GBA Mode. If a gbaframe.bmp file is present it will be loaded to. Curently setup to use gbaframe bmps made to work with GBA-Exploader.
* Holding B button will skip auto boot and bring up file browser so you can select a NDS file of your choice.

Currently the gbaframe loader is setup to load GBA-Exploader compatible BMP files. The directories it will check are as follows in the indicated order:

1. fat:/gbaframe.bmp
2. fat:/GBA_SIGN/gbaframe.bmp
3. fat:/_system_/gbaframe.bmp
4. fat:/ttmenu/gbaframe.bmp




# License
Note: While the GPL license allows you to distribute modified versions of this program it would be appreciated if any improvements are contributed to devkitPro. Ultimately the community as a whole is better served by having a single official source for tools, applications and libraries.

The latest sources may be obtained from devkitPro git using the command: `git clone git@github.com:devkitPro/nds-hb-menu.git`

```
 Copyright (C) 2005 - 2017
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy

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
 ```
