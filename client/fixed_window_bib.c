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
	// h = 0 last, h = 1 first
	// Correcion h = 0 first, h = 1 last
	int i, start,end;
	
	if( h<0 || h>1 )
		return -1; //No valid parameter h 
	else 
	{
		//start=indice-1-(size/2)+(h+2)*size/2
		//end=indice-2+(h+2)*size/2+1
		start = fw->index-1 - (fw->size/2)+ 1 +( (h+2)*fw->size/2 );
		end   = fw->index-1 + ( (h+2)*fw->size/2 );
		//printf("Start %d End %d ----index %d\n", start, end,fw->index);
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
