/* ** por compatibilidad se omiten tildes **
================================================================================
 TRABAJO PRACTICO 3 - System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================
*/

#include "colors.h"
#include "defines.h"
#include "game.h"
#include "syscall.h"

#define BANDERA_BUFFER  0x40001000

/*
 * Estructura de la tarea :
 *
 *  0x40000000 - 0x40001000 : codigo tarea          (   5 KB)
 *  0x40001000 - 0x40001400 : area de banderas      (   1 KB)
 *  0x40001400 - 0x40001c00 : pila tarea            (   2 KB)
 *  0x40001c00 - 0x40001ffc : pila bandera          (1020 B )
 *  0x40001ffc - 0x40002000 : dir. funcion bandera  (   4 B )
 */

/* Estructura de para acceder a memoria de video */
typedef struct {
    unsigned char c;
    unsigned char a;
} ca;

unsigned char *bandera();

// global var
#define GLOBAL_START 0x40000000
#define var_B GLOBAL_START+0x2000-0x200

void task()
{
    /* Tarea 2 */
    unsigned int s = 0;

    while (1) {
        if (s == 0) {
            syscall_navegar(0x00350000, 0x00351000);
            s = 1;
        } else {
            syscall_navegar(0x00680000, 0x00681000);
            s = 0;
        }
    };
}

unsigned char *bandera()
{
    ca(*buffer)[10] = (ca(*)[10]) (BANDERA_BUFFER);
    int *b = (int *) (var_B);
    unsigned int fil;
    unsigned int col;

    (*b)++;

    if (*b == 11)
        *b = 0;

    for (fil = 0; fil < 5; fil++) {
        for (col = 0; col < 10; col++) {
            buffer[fil][col].c = ' ';

            if (col < *b)
                buffer[fil][col].a = (0x2 << 4) | C_FG_GREEN;
            else
                buffer[fil][col].a = (0x1 << 4) | C_FG_BLUE;
        }
    }

    syscall_bandera_fin((unsigned int) buffer);

    /* Para que el compilador no tire warning... */
    return 0;
}
