
//
//   fixed_window_bib.c is part of SIC.
//
//   Copyright (C) 2018  your name
//
//   SIC is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   SIC is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
//

#include <stdio.h>
#include <stdlib.h>

typedef struct  
{ 
	int size; 
	int index; 
	double *w;
} fixed_window;

double fixed_window_read(fixed_window *fw, int index)
{
	double data;
	int i;
	
	i= index % fw->size;
	data=fw->w[i];

	return data;
}

int fixed_window_Hread(fixed_window *fw, int h, double *data)
{
	int i, start,end;
	
	if( h<0 || h>1 )
		return -1; //No valid parameter h 
	else 
	{
		start = fw->index-1 - (fw->size/2)+ 1 +( (h+2)*fw->size/2 );
		end   = fw->index-1 + ( (h+2)*fw->size/2 );
		for(i=start; i <= end; i++)
			data[i-start]=fw->w[i % fw->size];
	}

	return 0;
}

void fixed_window_insert(fixed_window *fw, double data)
{
	int i;
	i = fw->index;
	fw->w[i]=data;
	i= (fw->index +1) % fw->size;
	fw->index=i;
	
	return;
}

fixed_window* fixed_window_init(int size)
{
	fixed_window *w_ = malloc(sizeof *w_ );
	double *aux = calloc(size, sizeof (double));
	w_->w = aux;
	w_->size = size;
	w_->index = 0;
		
	return w_;
}
