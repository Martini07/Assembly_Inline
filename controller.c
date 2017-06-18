#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

/* Inserite eventuali extern modules qui */

/* ************************************* */

enum { MAXLINES = 400 };

long long current_timestamp() {
    struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);
	/* te.tv_nsec nanoseconds divide by 1000 to get microseconds*/
	long long nanoseconds = tp.tv_sec*1000LL + tp.tv_nsec; // caculate nanoseconds  
    return nanoseconds;
}


int main(int argc, char *argv[]) {
    int i = 0;
    char bufferin[MAXLINES*8+1] ;
    char line[1024];
    long long tic_c, toc_c, tic_asm, toc_asm;

    char bufferout_c[MAXLINES*8+1] = "" ;
    char bufferout_asm[MAXLINES*8+1] = "" ;

    FILE *inputFile = fopen(argv[1], "r");

    if(argc != 3)
    {
        fprintf(stderr, "Syntax ./test <input_file> <output_file>\n");
        exit(1);
    }

    if (inputFile == 0)
    {
        fprintf(stderr, "failed to open the input file. Syntax ./test <input_file> <output_file>\n");
        exit(1);
    }

    while (i < MAXLINES && fgets(line, sizeof(line), inputFile))
    {
        i = i + 1;
        strcat( bufferin, line) ;
    }

    bufferin[MAXLINES*8] = '\0' ;

    fclose(inputFile);


    /* ELABORAZIONE in C */
    tic_c = current_timestamp();

  	/* supponendo di avere una variabile bufferin = "0,0,001\n0,0,013\n1,0,045\n...\n1,1,067\n\0" contenente i valori letti dal file */
  	int c = 0, j = 0;
  	int init, reset, ph, nck=0 ;
  	char st = '-', vlv[2] = "--", oldst = '-' ;
  	char tmpout[8] ;
  	char nckstr[2] ;

  	i = 0;
  	while ( bufferin[i] != '\0' ) {
      init = bufferin[i] - '0' ;
  		reset = bufferin[i+2] - '0' ;
    	ph = (bufferin[i+4]-'0')*100 + (bufferin[i+5]-'0')*10 + (bufferin[i+6]-'0') ;

      strcpy(tmpout, "-,--,--\n") ;

      /* printf("i=%d, init: %d, reset: %d, ph: %d, tmpout: %s\n", i, init, reset, ph, tmpout) ; */
    	if ( init == 0 || reset == 1) {
          oldst = '-' ;
    	}
    	else
    	{
      		/* determino lo stato attuale */
      		if ( ph < 60 ) {
      			st = 'A' ;
      		}
      		else if ( ph >= 60 && ph <= 80 ) {
        		st = 'N' ;
      		}
      		else if ( ph > 80 ) {
        		st = 'B' ;
            }

      		/* aggiorno il contatore dei cicli */
      		if ( st != oldst ) {
        		nck = 0 ;
      		}
      		else
      		{
        		nck = nck + 1 ;
      		}
      		sprintf(nckstr,"%.2d",nck) ;
      		/* determino lo stato della valvola */
      		if ( st == 'A' && nck>4 ) {
        		strcpy(vlv,"BS") ;
            /* vlv[0] = 'B' ;
            vlv[1] = 'S' ; */
      		}
      		else if ( st == 'B' && nck>4 ) {
            strcpy(vlv,"AS") ;
            /* vlv[0] = 'A' ;
            vlv[1] = 'S' ; */
      		}
      		else
      		{
            strcpy(vlv,"--") ;
            /* vlv[0] = '-' ;
            vlv[1] = '-' ; */
      		}
      		/* aggiorno oldst */
      		oldst = st ;
      		/* genero la stringa di output */
      		tmpout[0] = st ;
      		tmpout[2] = nckstr[0] ;
      		tmpout[3] = nckstr[1] ;
      		tmpout[5] = vlv[0] ;
      		tmpout[6] = vlv[1] ;
    	}

    	strcat( bufferout_c, tmpout) ;
    	i = i + 8 ;
  	}

    toc_c = current_timestamp();

  	long long c_time_in_nanos = toc_c - tic_c;

    /* FINE ELABORAZIONE C */


    /* INIZIO ELABORAZIONE ASM */

    tic_asm = current_timestamp();

    /* Assembly inline:
    Inserite qui il vostro blocco di codice assembly inline o richiamo a funzioni assembly.
    Il blocco di codice prende come input 'bufferin' e deve restituire una variabile stringa 'bufferout_asm' che verrà poi salvata su file. */
    
    __asm__ ("\
            .section .data\n\
                hellos:\n\
                    .ascii \"helloo\n\"\n\
                hello:\n\
                    .ascii \"Hello, world!\n\"\n\
                hello_len:\n\
                    .long . - hello\n\
            .section .text\n\
                .global _initialization\n\
            _initialization:\n\
                mov $0, %%bl #index for the string\n\
                mov $0, %%cl #counter\n\
                movl %[buffer], %%ecx #input string\n\
                jmp check_end_string\n\
            loop:\n\
                mov %%ecx[%%bl + 1],%%al #reset\n\
                cmp %%al, $0\n\
                je reset\n\ #reset true\n\
                mov %%ecx[%%bl + 0],%%al #start\n\
                cmp %%al, $0\n\
                je not_started\n\ #started false\n\
                #calc ph\n\
                #increment or put 0 into %%cl\n\
                cmp %%cl, $5\n\
                jl write_out_row\n\
                #else set valvole BS or AS\n\
                write_out_row\n\
            write_out_row:\n\
                write in buffer out (PH[1],counter(sarebbe %%cl)[2],valvole[2]\n\
                addl %%bl, $8 #next line\n\
                jmp check_end_string\n\
            reset:\n\
                #write in buffer out (-,--,--)\n\
                addl %%bl, $8 #next line\n\
                jmp check_end_string\n\
            not_started: #same of reset - check se si puo' usare la stessa etichetta\n\
                #write in buffer out (-,--,--)\n\
                addl %%bl, $8 #next line\n\
                jmp check_end_string\n\
            check_end_string:\n\
                testb %%ecx, %%ecx # Controlla se la stringa è finita (tutte le stringhe terminano con 0). \n\
                jz end\n\
                jmp loop #continua\n\
            end:\n\
                movl $1, %%eax \n\
                movl $0, %%ebx # Solito blocco di codice per la chiamata alla \n\
                int $0x80     # system call exit per uscire dal programma. \n\
            "
            ://output
            :[buffer]"g"(bufferin)//input
            );


    toc_asm = current_timestamp();

      long long asm_time_in_nanos = toc_asm - tic_asm;

    /* FINE ELABORAZIONE ASM */


    printf("C time elapsed: %lld ns\n", c_time_in_nanos);
    printf("ASM time elapsed: %lld ns\n", asm_time_in_nanos);

    /* Salvataggio dei risultati ASM */
  	FILE *outputFile;
    outputFile = fopen (argv[2], "w");
    fprintf (outputFile, "%s", bufferout_asm);
    fclose (outputFile);

    return 0;
}
