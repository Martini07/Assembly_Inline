#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>

/* Inserite eventuali extern modules qui */
void inlineasm(char *o, char *i);
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
	/**
    __asm__(
            "movl %eax,%ecx\n\t"
    );*/
    inlineasm(bufferout_asm,bufferin);
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
void inlineasm(char *o, char *i) {
    asm (
            "xorb %%cl, %%cl\n\t" //contatore dei cicli di clocl
			"xorb %%dl, %%dl\n\t" //stato soluzione
			"loop:\n\t"
                "lodsb\n\t" //leggo primo carattere (start)
				"cmpb $0, %%al\n\t"
				"je end_loop\n\t"
                "cmpb $48, %%al\n\t" //codice ascii 0: 48 vedere se conviene decrementare 48
                "je off\n\t" //se continua init a 1
				"#lodsb\n\t" //leggo primo , vedere come modificare esi per evitare di leggere
				"inc %%rsi\n\t"
				"lodsb\n\t" //leggo reset
				"cmpb $49, %%al\n\t" //codice ascii 1: 49 vedere se conviene decrementare di 49
				"je reset\n\t" //se continua init a 1 e reset a 0
				"inc %%rsi\n\t"
				"lodsb\n\t" //leggo prima cifra ph
				"cmpb $49, %%al\n\t" //attenzione se superiore a 199 comportamento imprevedibile
				"je basic\n\t"
				"lodsb\n\t" //leggo seconda cifra ph (sapendo che la prima è "0" (!=1))
				"cmpb $54, %%al\n\t" //confronto col 6 
				"jl acid\n\t"
				"cmpb $56, %%al\n\t" //confonto col 8
				"jg basic\n\t"
				"je verify_81\n\t"
				"neutro:\n\t"
				"cmpb $4, %%dl\n\t" //prima era neutro
				"je al_neutro\n\t"
				"movb $4, %%dl\n\t" //prima non era neutro aggiorno stato soluzione
				"xorb %%cl, %%cl\n\t" //azzero il contatore di cicli
				"movb $78,%%al\n\t" //so che è neutro scrivo N
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo terzo valore ph
				"movb $45, %%al\n\t" //scrivo - indico non muovere valvola (soluzione già neutra)
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return o conviene metterlo diretto e inc rsi?
				"stosb\n\t"
                "jmp loop\n\t"
			"al_neutro:\n\t"
				"incb %%cl\n\t" //incremento contatore cicli di clock
				"movb $78,%%al\n\t" //so che è neutro scrivo N
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo terzo valore ph
				"movb $45, %%al\n\t" //scrivo - indico non muovere valvola (soluzione già neutra)
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"acid:\n\t"
				"cmpb $1, %%dl\n\t" //confronto se prima era acido
				"je al_acid\n\t" //guardare se meglio vedere subito se sono passati i cinque (sei) cicli
				"xorb %%cl, %%cl\n\t" //azzero contatore cicli di clock prima non era acido
				"movb $1, %%dl\n\t" //imposto stato soluzione attuale acido
				"movb $65, %%al\n\t" //scrivo A di acido
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo terza cifra ph
				"movb $45, %%al\n\t" //scrivo - indico non muovere valvola
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"al_acid:\n\t"
				"incb %%cl\n\t" //incremento contatore cicli di clock
				"movb $65, %%al\n\t" //scrivo A di acido
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo terza cifra ph
				"cmpb $5, %%cl\n\t" //confronto se sono passati 6 cicli di clock (parto da 0)
				"jl notopen\n\t" //se continuo sono passati almeno 6 cicli di clock
				"movb $66, %%al\n\t" //scrivo B
				"stosb\n\t"
				"movb $83, %%al\n\t" //scrivo S
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"notopen:\n\t"
				"movb $45, %%al\n\t" //scrivo - indico non muovere valvola
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"basic:\n\t"
				"cmpb $2, %%dl\n\t" //confronto se prima era basico
				"je al_basic\n\t" //guardare se meglio vedere subito se sono passati i cinque (sei) cicli
				"#\n\t" //prima non era basico
				"xorb %%cl, %%cl\n\t" //azzero contatore cicli di clock
				"movb $2, %%dl\n\t" //imposto stato soluzione attuale basico
				"movb $66, %%al\n\t" //scrivo B di basico
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo terza cifra ph
				"movb $45, %%al\n\t" //scrivo - indico non muovere valvola
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"al_basic:\n\t"
				"incb %%cl\n\t" //incremento contatore cicli di clock
				"movb $66, %%al\n\t" //scrivo B di basico
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo terza cifra ph
				"cmpb $5, %%cl\n\t" //confronto se sono passati 6 cicli di clock (parto da 0)
				"jl notopen\n\t" //se continuo sono passati almeno 6 cicli di clock
				"movb $65, %%al\n\t" //scrivo A
				"stosb\n\t"
				"movb $83, %%al\n\t" //scrivo S
				"stosb\n\t"
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"verify_81:\n\t"
				"lodsb\n\t" //leggo terza cifra
				"cmpb $48, %%al\n\t" //confronto con 0
				"dec %%rsi\n\t" //torno indietro per poter usare neutro e basic
				"je neutro\n\t"
				"jmp basic\n\t"
            "end_loop:\n\t"
		        "movb $0, (%%rdi)\n\t" //insert end string character
		        "movq -0x8(%%rbp), %%rdi\n\t" //restore pointer
				"jmp end\n\t"
			"off:\n\t"
				"xorb %%cl, %%cl\n\t" //azzero contatore
				"xorb %%dl, %%dl\n\t" //azzero stato
				"movb $45, %%al\n\t" //scrivo - indico stato indifferente ( dispositivo spento)
				"stosb\n\t" 
				"lodsb\n\t" //leggo primo , vedere come modificare esi per evitare di leggere
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo reset
				"movb $45, %%al\n\t" //scrivo - indico numero cicli clock indifferente
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t" //leggo secondo ,
                "stosb\n\t" //scrivo secondo ,
				"inc %%rsi\n\t" //leggo primo valore ph
				"movb $45, %%al\n\t" //sostituisco con - chiudo valvole
				"stosb\n\t"
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo secondo valore ph
				"inc %%rsi\n\t" //leggo terzo valore ph
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"reset:\n\t"
				"xorb %%cl, %%cl\n\t" //azzero contatore
				"xorb %%dl, %%dl\n\t" //azzero stato
				"movb $45, %%al\n\t" //scrivo - indico macchina in reset
				"stosb\n\t"
				"lodsb\n\t" //leggo secondo ,
                "stosb\n\t" //scrivo secondo ,
				"inc %%rsi\n\t" //leggo primo valore ph
				"movb $45, %%al\n\t" //sostituisco con -
				"stosb\n\t"
				"stosb\n\t"
				"movb $44,%%al\n\t" //metto ,
				"stosb\n\t"
				"movb $45, %%al\n\t" //metto - nelle valvole
				"stosb\n\t"
				"stosb\n\t"
				"inc %%rsi\n\t" //leggo secondo valore ph
				"inc %%rsi\n\t" //leggo terzo valore ph
				"lodsb\n\t" //leggo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
			"end:\n\t"
            :"=D" (o)
            :"S" (i)
    );
    return;
}
/**
 asm (
            "loop:\n\t"
                "lodsb\n\t" //leggo primo carattere (start)
                "cmp $48, %%al\n\t" //codice ascii 0: 48 vederese convienedecrementare 48
				"cmp $0, %%al\n\t"
				"je end_loop\n\t"
                "je reset\n\t" //se continua init a 1
				"lodsb\n\t" //leggo primo ; vedere come modificare esi per evitare di leggere
				"lodsb\n\t" //leggo reset
				"cmp $49, %%al\n\t" //codice ascii 1: 49 vedere se conviene decrementare di 49
				"je reset\n\t" //se continua init a 1 e reset a 0
				"lodsb\n\t" //leggo secondo ,
				"mov $79, %%al\n\t" //scrivo O indico macchina accesa
                "stosb\n\t"
				"mov $44, %%al\n\t" //scrivo ;
                "stosb\n\t"
				"mov $48, %%al\n\t" //metto 0 nck
                "stosb\n\t"
				"mov $44, %%al\n\t" //scrivo ;
                "stosb\n\t"
				"lodsb\n\t" //copio primo valore ph
				"stosb\n\t"
				"lodsb\n\t" //copio secondo valore ph
				"stosb\n\t"
				"lodsb\n\t" //copio terzo valore ph
				"stosb\n\t"
				"lodsb\n\t" //scrivo carriage return
				"stosb\n\t"
                "jmp loop\n\t"
            "end_loop:\n\t"
            "movb $0, (%%rdi)\n\t" //insert end string character
            "movq -0x8(%%rbp), %%rdi\n\t" //restore pointer
			"reset:\n\t"
				"mov $45, %%al\n\t" //metto - in prima riga
				"stosb\n\t"
				"mov $44, %%al\n\t" //scrivo ;
                "stosb\n\t"
				"mov $45, %%al\n\t" //metto - in prima riga
				"stosb\n\t"
				"stosb\n\t"
				"mov $44, %%al\n\t" //scrivo ;
                "stosb\n\t"
				"lodsb\n\t"
				"lodsb\n\t"
				"lodsb\n\t"
				"mov $45, %%al\n\t" //metto - in prima riga
				"stosb\n\t"
				"stosb\n\t"
				"stosb\n\t"
				"lodsb\n\t"
				"stosb\n\t"
				"jmp loop\n\t"
            :"=D" (o)
            :"S" (i)
    );
**/
