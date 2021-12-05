/*
* Copyright (C) 2017 Amlogic, Inc. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Description:
*/
#ifndef _AML_DMX_EXT_H_
#define _AML_DMX_EXT_H_

struct dmx_ext_asyncfifo_param{
	int size; 	/*secure buf size */
	unsigned long addr;   /*secure buf addr */
};

#define DMX_EXT_SET_ASYNCFIFO_PARAMS	_IOW('o', 64, struct dmx_ext_asyncfifo_param)

#define PVR_PID_NUM			32

struct dmx_ext_param {
	int cmd;
	int pvr_count;
	int pvr_pid[PVR_PID_NUM];
};

int dmx_ext_init(int id);
void dmx_ext_exit(int id);
int dmx_ext_add_pvr_pid(int id, int pid);
int dmx_ext_remove_pvr_pid(int id, int pid);

#endif
