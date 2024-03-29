/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================
  definicion de funciones del manejador de memoria
*/

#include "mmu.h"
#include "defines.h"
#include "screen.h"
#include "colors.h"
#include "i386.h"

void* libre = (void*) 0x30000;
void* libreMar = (void*) 0x100000;



void* directorios_tareas[8];
void* direcciones_fisicas_tarea_pagina1[8];
void* direcciones_fisicas_tarea_pagina2[8];
void* direcciones_fisicas_tarea_ancla[8];

void mnu_inicializar_memoria_tareas()
{
	int i;
	for (i = 0; i < 8; i++)
	{
		mmu_inicializar_dir_tarea(i);
	}


}

void* mmu_inicializar_dir_tarea(int t)
{


	void* directorio = proximaPaginaLibre();
	directorios_tareas[t] = directorio;

	void* tabla1 = proximaPaginaLibre();
	void* tabla2 = proximaPaginaLibre();

	identity_mapping((int) directorio, (int) tabla1, (int) tabla2);

	void* p1_mar = proximaPaginaLibreDelMar();
	void* p2_mar = proximaPaginaLibreDelMar();
	void* p3_tierra = (void*) (0x00);

	direcciones_fisicas_tarea_pagina1[t] = (void*) p1_mar;
	direcciones_fisicas_tarea_pagina2[t] = (void*) p2_mar;
	direcciones_fisicas_tarea_ancla[t] = p3_tierra;

	mmu_mapear_pagina((int*) directorio, PAGINA1_VIRTUAL_TAREA, (int) p1_mar);
	mmu_mapear_pagina((int*) directorio, PAGINA2_VIRTUAL_TAREA, (int) p2_mar);
	mmu_mapear_pagina((int*) directorio, PAGINA_ANCLA_VIRTUAL_TAREA, (int) p3_tierra);

	// cr3 = pagina_libre
	// inicializar directorios para la tarea t.
		// con cr3
	// mapear(40.000.000, z1=alguna del mar,cr3)
	// mapear(40.001.000, z2=otra del mar,cr3)
	// mapear(40.002.000, 0,cr3)
	// copiar(0x10000,z1)
	// copiar(0x11000,z2)

	copiar_int((int*) p1_mar, (int*) (0x10000 + (t * 0x2000)), 0x1000);
	copiar_int((int*) p2_mar, (int*) (0x11000 + (t * 0x2000)), 0x1000);

	return directorio;
}

void mmu_inicializar_dir_kernel()
{
	// Mapeamos con identity mapping desde 0x00000000 hasta la 0x0077ffff
	identity_mapping(DIRECTORIO_PAGINAS_KERNEL_POS, TABLA_PAGINAS_1_KERNEL_POS, TABLA_PAGINAS_2_KERNEL_POS);

	// Mapeamos desde las fisicas 0x20000 a 0x21fff hacia las virtuales 0x4000000 a 0x4001fff (para la tarea idle)
	mmu_mapear_pagina((int*) DIRECTORIO_PAGINAS_KERNEL_POS, PAGINA1_VIRTUAL_TAREA, 0x20000);
	mmu_mapear_pagina((int*) DIRECTORIO_PAGINAS_KERNEL_POS, PAGINA2_VIRTUAL_TAREA, 0x21000);

}


void identity_mapping(int directorio_pos, int tabla1_pos, int tabla2_pos)
{
	// Seteamos las Page Directory Entries (son dos)
	int* dir = (int*) directorio_pos + 0x00;

	// Seteamos la dirección de la primer tabla en la primer entrada del directorio y también los flags
	*dir = (tabla1_pos | 0x7);

	dir = (int*) (directorio_pos + 0x04);

	// Seteamos la dirección de la segunda tabla en la segunda entrada del directorio y también los flags
	*dir = (tabla2_pos | 0x7);

	int i;


	// limpiamos el resto del directorio
	for (i = 2; i < 1024; i++)
	{
		dir = (int*) (directorio_pos +  i*4);

		*dir = 0x00;
	}


	// Seteamos cada entrada de la tabla 1 (son 1024 paginas, de la 0 a la 1023)
	for (i = 0; i < 1024; i++)
	{
		dir = (int*) (tabla1_pos +  i*4);

		*dir = (i << 12) | 0x7;
	}



	// Seteamos cada entrada de la tabla 2 (son 895, de la 1024 a 1918)
	for (i = 1024; i < 1920; i++)
	{
		dir = (int*) (tabla2_pos +  ((i-1024)*4) );

		*dir = (i << 12) | 0x7;
	}

	// el resto de la tabla en 0
	for (i = 1920; i < 2048; i++)
	{
		dir = (int*) (tabla2_pos +  ((i-1024)*4) );

		*dir = 0;
	}
}




void mmu_mapear_pagina(int* dp, int virtual, int fisica)
{

	int IPD = virtual;
	IPD = IPD >> 22;


	int IPT = virtual;
	IPT = IPT >> 12;
	IPT = IPT & 0x3FF;

	//int off = virtual & 0xFFF;

	//dp[IPD]
	int PDE = *(dp + IPD);

	int PT;
	if (!(PDE & 1))
	{
		// No presente,
		PT = (int) proximaPaginaLibre();

		*(dp + IPD) = PT | 0x7;

		//limpiamos toda la tabla
		int* dir;
		int i;
		for (i = 0; i < 1024; i++)
		{
			dir = (int*) (PT +  i*4);

			*dir = 0x00;
		}

	}
	else
	{
		PT = (PDE & 0xFFFFF000);
	}


	int* PTE = (int*) (PT + IPT*4);
	PTE[0] = fisica | 0x7; // asumimos fisica es multiplo de 4kb



	tlbflush();
}
void mmu_unmapear_pagina(int* dp, int virtual)
{

	int IPD = virtual;
	IPD = IPD >> 22;


	int IPT = virtual;
	IPT = IPT >> 12;
	IPT = IPT & 0x3FF;

	//int off = virtual & 0xFFF;

	//dp[IPD]
	int PDE = *(dp + IPD);

	int PT;
	if (!(PDE & 1))
	{
		// ya está, no está presente, no hay nada que hacer
		return;

	}
	else
	{
		// está presente
		PT = (PDE & 0xFFFFF000);
	}


	int* PTE = (int*) (PT + IPT*4);
	PTE[0] = 0x00; // limpiamos la entrada de la tabla

	tlbflush();
}



void* proximaPaginaLibre()
{
  void* m = libre;
  libre+= 0x1000;
  return m;
}



void* proximaPaginaLibreDelMar()
{
  void* m = libreMar;
  libreMar+= 0x1000;
  return m;
}



void copiar_int(int* dest, int* source, int size)
{
	int i;
	size = size >> 2;
	for (i = 0; i < size; i++)
	{
		(dest)[i] = (source)[i];
	}

}



void copiar_bytes(char* dest, char* source, int size)
{
	int i;
	for (i = 0; i < size; i++)
	{
		(dest)[i] = (source)[i];
	}
}
