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
 * @file injection_test.c
 * @author Guillaume Milan (guillaume.milan@grenoble-inp.org)
 * @brief various platform specific functions
 */
#define SIZE 20

int main(int argc, char** argv) {
int i=0x444444;
int j=0x444444;
int k=0x444444;
int l=0x444444;
int m=0x444444;
int n=0x444444;
int o=0x444444;
int p=0x444444;
int q=0x444444;
int r=0x444444;
int s=0x444444;
return 0;
/*
    u32 tabl[SIZE];
    u32 index;
    u32* value = (u32*)0x300;

    for(u32 i=0; i<SIZE; i++) {
	tabl[i]=1;
    }
    while(1) {
	*(value ++);
	index = 0;
	tabl[index]++;
	while(tabl[index]==0 && index<SIZE) {
	    index ++;
	    tabl[index]++;
	}
    }
*/
}
