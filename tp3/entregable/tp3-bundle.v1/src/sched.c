/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================
  definicion de funciones del scheduler
*/

#include "sched.h"
#include "defines.h"
#include "i386.h"
#include "gdt.h"
#include "screen.h"
#include "colors.h"


char corriendoBanderas;
unsigned int tareaActual;
unsigned int banderaActual;
char contadorTareas;
char contadorBanderas;
char cantTareasVivas;

int indicesTareas[8];
int indicesBanderas[8];

void sched_inicializar() {

	int i;
	for(i=0;i<8;i++)
	{
		indicesTareas[i] = (GDT_IDX_T1_DESC+i)<<3;

	}
	for(i=0;i<8;i++)
	{
		indicesBanderas[i]= (GDT_IDX_T1_FLAG_DESC+i)<<3;
	}

	tareaActual = -1;
	banderaActual = -1;
	corriendoBanderas=0;
	contadorTareas=-1;
	contadorBanderas=-1;
	cantTareasVivas=8;
}

unsigned short sched_proximo_indice() {

	if (cantTareasVivas != 0)
	{


		if(!corriendoBanderas)
		{
			// Me fijo si ya ejecuté 3 tareas
			if(++contadorTareas==3)
			{
				// Ya corrí 3 tareas
				// Empiezo a correr banderas
				corriendoBanderas=1;
				contadorTareas = 0;
				return indicesBanderas[buscarIndiceSiguienteViva(1)]; // El 1 en indicesBanderas indica que busque en banderas
			}
			else
			{
				// Busco la siguiente tarea a ejecutar
				return indicesTareas[buscarIndiceSiguienteViva(0)];
			}

		}
		else
		{

			if(++contadorBanderas == cantTareasVivas)
			{
				//Termine de correr banderas
				// Busco la siguiente tarea a ejecutar
				corriendoBanderas = 0;
				contadorBanderas=0;

				return indicesTareas[buscarIndiceSiguienteViva(0)];
			}
			else
			{
				// Busco la siguiente bandera a ejecutar
				return indicesBanderas[buscarIndiceSiguienteViva(1)]; // El 1 en indicesBanderas indica que busque en banderas
			}
		}
	}
	else
	{
		return 0;
	}
}


// Devuelve el índice en el arreglo de la siguiente tarea/bandera viva
unsigned char buscarIndiceSiguienteViva(char esBandera)
{

	if (esBandera)
	{
		banderaActual = (banderaActual + 1)%8;
		while (indicesBanderas[banderaActual] == 0)
		{
			banderaActual = (banderaActual  + 1)%8;
		}

		return banderaActual;
	}
	else
	{
		tareaActual = (tareaActual + 1)%8;
		while (indicesTareas[tareaActual] == 0)
		{
			tareaActual = (tareaActual + 1)%8;
		}
		return tareaActual;
	}
}



void desalojarTareaActual()
{
	int laQueMurio;

	if (corriendoBanderas)
	{
		indicesTareas[banderaActual] = 0;
		indicesBanderas[banderaActual] = 0;

		laQueMurio = banderaActual;

		if (contadorBanderas+1 == cantTareasVivas-1)
		{
			contadorBanderas--;
		}

	}
	else
	{
		indicesTareas[tareaActual] = 0;
		indicesBanderas[tareaActual] = 0;

		laQueMurio = tareaActual;
	}

	screen_colorear(2 + (12*(laQueMurio%4)), 3 + (7 * (laQueMurio/4)), 2 + (12*(laQueMurio%4)) + 9, 3 + (7 * (laQueMurio/4)) + 4, C_BG_RED, 0);

	char charTarea[2];
	charTarea[0] = laQueMurio + 0x30 + 1;
	charTarea[1] = 0x00;

	screen_imprimir((char*) charTarea, C_FG_WHITE, C_BG_RED, 0, 4 + (3 * laQueMurio), 24, 0, 0);
	screen_imprimir((char*) charTarea, C_FG_WHITE, C_BG_RED, 0, 32 + (3 * laQueMurio), 24, 0, 0);
	screen_imprimir(" ", C_FG_WHITE, C_BG_RED, 0, 4 + (3 * laQueMurio) + 1, 24, 0, 0);
	screen_imprimir(" ", C_FG_WHITE, C_BG_RED, 0, 32 + (3 * laQueMurio) + 1, 24, 0, 0);

	screen_imprimir((char*) charTarea, C_FG_WHITE, C_BG_RED, 0, 4 + (3 * laQueMurio), 24, 0, 1);
	screen_imprimir((char*) charTarea, C_FG_WHITE, C_BG_RED, 0, 32 + (3 * laQueMurio), 24, 0, 1);
	screen_imprimir(" ", C_FG_WHITE, C_BG_RED, 0, 4 + (3 * laQueMurio) + 1, 24, 0, 1);
	screen_imprimir(" ", C_FG_WHITE, C_BG_RED, 0, 32 + (3 * laQueMurio) + 1, 24, 0, 1);

	cantTareasVivas--;
	actualizar_pantalla();
}


char check_soy_tarea(unsigned short tr)
{
	tr = tr>>3;
	if ((tr >= GDT_IDX_T1_FLAG_DESC && tr <= GDT_IDX_T8_FLAG_DESC) && tr != GDT_IDX_T_IDLE_DESC)
	{
		desalojarTareaActual();
		return 0;
	}

	return 1;

}


char check_soy_bandera(unsigned short tr)
{
	tr = tr>>3;
	if (tr >= GDT_IDX_T1_DESC && tr <= GDT_IDX_T8_DESC)
	{
		desalojarTareaActual();
		return 0;
	}

	return 1;

}






void quitarBitBusy(unsigned short tr)
{
	tr = tr>>3;
	gdt[tr].type = 0x9;
}
