/**
 * Copyright (c) 2012 Jean-Christophe Dubois.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * @file arm_board.c
 * @author Jean-Christophe Dubois (jcd@tribudubois.net)
 * @brief various platform specific functions
 */
#define u32  unsigned int 
#define SIZE 20

int main(int argc, char** argv) {
    u32 tabl[SIZE];
    u32 index;
    for(u32 i=0; i<SIZE; i++) {
	tabl[i]=1;
    }
    while(1) {
	index = 0;
	tabl[index]++;
	while(tabl[index]==0 && index<SIZE) {
	    index ++;
	    tabl[index]++;
	}
    }
}
