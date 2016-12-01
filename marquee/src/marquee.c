/* Display a string of text on the led display and have it slide across to fit the message. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <wiringPi.h>

#define d(x)   // printf(x "\n")
#define e(x,y) // printf(x "%d\n", y)

#define b10000000 0x80
#define b01000000 0x40
#define b00100000 0x20
#define b00010000 0x10
#define b00001000 0x08
#define b00000100 0x04
#define b00000010 0x02
#define b00000001 0x01

#define b11000000 192
#define b11111001 249
#define b10100100 164
#define b10110000 176
#define b10011001 153
#define b10010010 146
#define b10000010 130
#define b11111000 248
//#define b10000000 128
#define b10010000 144
#define b01111111 127
#define b11111111 255

int anode[] =
  {
    b10000000, // digit position 1
    b01000000, // digit position 2
    b00100000, // digit position 3
    b00010000, // digit position 4
    b00001000, // digit position 5
    b00000100, // digit position 6
    b00000010, // digit position 7
    b00000001  // digit position 8
  };

/* PGFEDCBA  p is the dot
 *
 *    A
 *   F B
 *    G
 *   E C
 *    D   p
 *
 */

int LATCH = 4; // RCK of 8x7segment module pin 16

int CLOCK = 16; // SCK of 8x7segment module pin 10

int DATA = 15; // DIO of 8x7segment module,  pin 8

typedef int bool;
typedef int boolean;
#define false 0
#define true (!false)

#define BLANKDIGIT 11

#define NOSPACES 1
#define SPACES 2

// this is the data that gets displayed by the display thread
int display[8];
int startpos; // this is the position of the character that goes in the leftmost display position

int filemode = 0;
char *filename = NULL;
char *sourcebuf = NULL;
char *patternbuf = NULL;


void latch()
  {
    digitalWrite(LATCH, HIGH);
    digitalWrite(LATCH, LOW);
  }

void init(void)
  {
    wiringPiSetup();
    pinMode(LATCH, OUTPUT);
    pinMode(CLOCK, OUTPUT);
    pinMode(DATA, OUTPUT);
    int lp;
    for (lp = 0; lp < 8; lp++)
      display[lp] = BLANKDIGIT;
    // get it in sync
    latch();
    latch();
    startpos = 0;
  }

boolean lastval = false;
void shift(boolean val)
  {
    if (lastval != val)
      {
        int state = val ? HIGH : LOW;
        digitalWrite(DATA, state);
      }
    lastval = val;
    digitalWrite(CLOCK, HIGH);
    digitalWrite(CLOCK, LOW);
  }

void writebits(int val)
  {
    // write each bit of val to register
    int i;
    for (i = 0; i < 8; i++)
      shift(((val << i) & b10000000) != 0);
  }

void displaypattern(int position, char value)
  {
    e("anode", position);
    int p = anode[position];
    writebits(p);
    writebits(value);
  }

void *display_thread(void *opt)
  {
    while (true)
      {
        int startdigit = 0;
        int lp;
        d("loop");

        for (lp = startdigit; lp < 8; lp++)
          {
            displaypattern(7 - lp, display[lp]);
            latch();
            delayMicroseconds(2000); // 1000000 is one second
          }
      }
    return NULL;
  }

void led(int p, int a, int b, int c, int d, int e, int f, int g, char *patterns, int *patternsize)
  {
    a = 1 - a;
    b = 1 - b;
    c = 1 - c;
    d = 1 - d;
    e = 1 - e;
    f = 1 - f;
    g = 1 - g;
    p = 1 - p;
    char out = 0;
    out |= (a << 0);
    out |= (b << 1);
    out |= (c << 2);
    out |= (d << 3);
    out |= (e << 4);
    out |= (f << 5);
    out |= (g << 6);
    out |= (p << 7);
    patterns[*patternsize] = out;
    (*patternsize)++;
  }

void translate(char c, char *patterns, int *patternsize)
  {
    /* PGFEDCBA  p is the dot
     *
     *    A
     *   F B
     *    G
     *   E C
     *    D   p
     *
     */

    switch (c)
      {
      case 'A':
        return led(0, 1, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'B':
        return led(0, 0, 0, 1, 1, 1, 1, 1, patterns, patternsize);

      case 'C':
        return led(0, 1, 0, 0, 1, 1, 1, 0, patterns, patternsize);

      case 'D':
        return led(0, 0, 1, 1, 1, 1, 0, 1, patterns, patternsize);

      case 'E':
        return led(0, 1, 0, 0, 1, 1, 1, 1, patterns, patternsize);

      case 'F':
        return led(0, 1, 0, 0, 0, 1, 1, 1, patterns, patternsize);

      case 'G':
        return led(0, 1, 1, 1, 1, 0, 1, 1, patterns, patternsize);

      case 'H':
        return led(0, 0, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'I':
        return led(0, 0, 0, 0, 0, 1, 1, 0, patterns, patternsize);

      case 'J':
        return led(0, 0, 1, 1, 1, 1, 0, 0, patterns, patternsize);

      case 'K':
        return led(0, 0, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'L':
        return led(0, 0, 0, 0, 1, 1, 1, 0, patterns, patternsize);

      case 'M':
               led(0, 1, 1, 0, 0, 1, 1, 0, patterns, patternsize);
        return led(0, 1, 1, 1, 0, 0, 1, 0, patterns, patternsize);

      case 'N':
        return led(0, 0, 0, 1, 0, 1, 0, 1, patterns, patternsize);

      case 'O':
        return led(0, 1, 1, 1, 1, 1, 1, 0, patterns, patternsize);

      case 'P':
        return led(0, 1, 1, 0, 0, 1, 1, 1, patterns, patternsize);

      case 'Q':
        return led(0, 1, 1, 1, 0, 0, 1, 1, patterns, patternsize);

      case 'R':
        return led(0, 0, 0, 0, 0, 1, 0, 1, patterns, patternsize);

      case 'S':
        return led(0, 1, 0, 1, 1, 0, 1, 1, patterns, patternsize);

      case 'T':
        return led(0, 0, 0, 0, 1, 1, 1, 1, patterns, patternsize);

      case 'U':
        return led(0, 0, 1, 1, 1, 1, 1, 0, patterns, patternsize);

      case 'V':
        return led(0, 0, 0, 1, 1, 1, 0, 0, patterns, patternsize);

      case 'W':
               led(0, 0, 0, 1, 1, 1, 1, 0, patterns, patternsize);
        return led(0, 0, 1, 1, 1, 1, 0, 0, patterns, patternsize);

      case 'X':
        return led(0, 0, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'Y':
        return led(0, 0, 1, 1, 1, 0, 1, 1, patterns, patternsize);

      case 'Z':
        return led(0, 1, 1, 0, 1, 1, 0, 1, patterns, patternsize);

        //lower case
      case 'a':
        return led(0, 1, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'b':
        return led(0, 0, 0, 1, 1, 1, 1, 1, patterns, patternsize);

      case 'c':
        return led(0, 1, 0, 0, 1, 1, 1, 0, patterns, patternsize);

      case 'd':
        return led(0, 0, 1, 1, 1, 1, 0, 1, patterns, patternsize);

      case 'e':
        return led(0, 1, 0, 0, 1, 1, 1, 1, patterns, patternsize);

      case 'f':
        return led(0, 1, 0, 0, 0, 1, 1, 1, patterns, patternsize);

      case 'g':
        return led(0, 1, 1, 1, 1, 0, 1, 1, patterns, patternsize);

      case 'h':
        return led(0, 0, 0, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'i':
        return led(0, 0, 0, 0, 0, 1, 1, 0, patterns, patternsize);

      case 'j':
        return led(0, 0, 1, 1, 1, 1, 0, 0, patterns, patternsize);

      case 'k':
        return led(0, 0, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'l':
        return led(0, 0, 0, 0, 1, 1, 1, 0, patterns, patternsize);

      case 'm':
               led(0, 1, 1, 0, 0, 1, 1, 0, patterns, patternsize);
        return led(0, 1, 1, 1, 0, 0, 1, 0, patterns, patternsize);

      case 'n':
        return led(0, 0, 0, 1, 0, 1, 0, 1, patterns, patternsize);

      case 'o':
        return led(0, 1, 1, 1, 1, 1, 1, 0, patterns, patternsize);

      case 'p':
        return led(0, 1, 1, 0, 0, 1, 1, 1, patterns, patternsize);

      case 'q':
        return led(0, 1, 1, 1, 0, 0, 1, 1, patterns, patternsize);

      case 'r':
        return led(0, 0, 0, 0, 0, 1, 0, 1, patterns, patternsize);

      case 's':
        return led(0, 1, 0, 1, 1, 0, 1, 1, patterns, patternsize);

      case 't':
        return led(0, 0, 0, 0, 1, 1, 1, 1, patterns, patternsize);

      case 'u':
        return led(0, 0, 1, 1, 1, 1, 1, 0, patterns, patternsize);

      case 'v':
        return led(0, 0, 0, 1, 1, 1, 0, 0, patterns, patternsize);

      case 'w':
               led(0, 0, 0, 1, 1, 1, 1, 0, patterns, patternsize);
        return led(0, 0, 1, 1, 1, 1, 0, 0, patterns, patternsize);

      case 'x':
        return led(0, 0, 1, 1, 0, 1, 1, 1, patterns, patternsize);

      case 'y':
        return led(0, 0, 1, 1, 1, 0, 1, 1, patterns, patternsize);

      case 'z':
        return led(0, 1, 1, 0, 1, 1, 0, 1, patterns, patternsize);

        //numbers
      case '0':
        return led(0, 1, 1, 1, 1, 1, 1, 0, patterns, patternsize);

      case '1':
        return led(0, 0, 1, 1, 0, 0, 0, 0, patterns, patternsize);

      case '2':
        return led(0, 1, 1, 0, 1, 1, 0, 1, patterns, patternsize);

      case '3':
        return led(0, 1, 1, 1, 1, 0, 0, 1, patterns, patternsize);

      case '4':
        return led(0, 0, 1, 1, 0, 0, 1, 1, patterns, patternsize);

      case '5':
        return led(0, 1, 0, 1, 1, 0, 1, 1, patterns, patternsize);

      case '6':
        return led(0, 1, 0, 1, 1, 1, 1, 1, patterns, patternsize);

      case '7':
        return led(0, 1, 1, 1, 0, 0, 0, 0, patterns, patternsize);

      case '8':
        return led(0, 1, 1, 1, 1, 1, 1, 1, patterns, patternsize);

      case '9':
        return led(0, 1, 1, 1, 1, 0, 1, 1, patterns, patternsize);

        //symbols
      case '.':
        return led(1, 0, 0, 0, 0, 0, 0, 0, patterns, patternsize);

      case ' ':
        return led(0, 0, 0, 0, 0, 0, 0, 0, patterns, patternsize);

      case '-':
        return led(0, 0, 0, 0, 0, 0, 0, 1, patterns, patternsize);

      default:
        return led(0, 0, 0, 0, 0, 0, 0, 0, patterns, patternsize);

      }
  }

void makestring(char *input, char **patterns, int *szout)
  {
    if (sourcebuf != NULL)
      free(sourcebuf);
    sourcebuf = NULL;
    if (patternbuf != NULL)
      free(patternbuf);
    patternbuf = NULL;
    int sz = strlen(input);
    patternbuf = malloc(sz * 2); // worst case every character is double wide
    sourcebuf = malloc(sz + 1);
    strcpy (sourcebuf, input);

    int patternsize = 0;
    int lp;
    for (lp = 0; lp < sz; lp++)
      {
        translate(input[lp], patternbuf, &patternsize);
      }
    *patterns = patternbuf;
    *szout = patternsize;
  }

void readfromfile(char **patterns, int *szout)
  {
    char buf[4096];
    FILE *f = fopen(filename, "r");
    if (f == NULL)
      {
        sprintf(buf, "Unable to open file %s, err: %d", filename, errno);
        return makestring(buf, patterns, szout);
      }
    memset(buf, 0, 4096);
    int ret = fread(buf, 1, 4096, f);
    if (ret == 0)
      {
        fclose(f);
        sprintf(buf, "Unable to read file %s, err: %d", filename, errno);
        return makestring(buf, patterns, szout);
      }
    makestring(buf, patterns, szout);
    fclose (f);
  }

void update_marquee(char *orig, char *patterns, int sz)
  {
    /* take the bit patterns and display the first 8,
     * then wait a bit and print the next 8 starting at position 2
     * taking care to deal with 8 char strings, and wrapping around. */

//    char dbuf[9];
//    dbuf[8] = '\0';
    while (true)
      {
        d("start update marquee loop");
        if (filemode)
          {
            if (startpos == 0) // reread from file
              readfromfile(&patterns, &sz);
          }
        int lp;
        for (lp = 0; lp < 8; lp++)
          {
            int readpos = startpos + lp;
            if (readpos >= sz)
              readpos -= sz;
            char pic = patterns[readpos];
            display[lp] = pic;
//            dbuf[lp] = orig[readpos];
          }
//        printf ("%s\n", dbuf);

        startpos++;
        if (startpos >= sz)
          startpos = 0;

        d("before sleep");
        usleep(300000); // don't need to update it that often...
        d("after sleep");
      }
  }

int main(int num, char *opts[])
  {
    // get the string to print
    if (num < 2)
      {
        printf("marquee <string>\n");
        printf("marquee -f <file>\n");
        printf("pass the string to print as a parameter.\n");
        printf("or pass the filename to read the string from.\n");
        return 1;
      }

    d("before init");
#ifndef onio
    init();
#endif
    d("after init");

    startpos = 0;
    char *buf = malloc(1);
    *buf = '\0';
    char *patterns = NULL;
    int patternsize = 0;

    if (strcmp(opts[1], "-f") == 0)
      {
        filemode = 1;
        if (num < 3)
          {
            printf("You must pass the filename to read the string from.\n");
            return 1;
          }
        filename = opts[2];
      }
    else
      {
        /* take all params and make one long string out of it. */
        int lp;

        for (lp = 1; lp < num; lp++)
          {
            int newlen = strlen(buf) + strlen(opts[lp]) + 2; // space + nt
            buf = realloc(buf, newlen);
            if (lp > 1)
              strcat(buf, " ");
            strcat(buf, opts[lp]);
          }

        if (strlen(buf) < 8) // must be at least 8 chars to marquee
          { // pad out with spaces.
            buf = realloc(buf, strlen(buf) + 10);
            while (strlen(buf) < 10)
              strcat(buf, " ");
          }

        // add the end of string delim "---"
        buf = realloc(buf, strlen(buf) + 6);
        strcat(buf, " --- ");

        d("displaying string:");d(buf);
        // now translate the string into a series of bit patterns
        int sz = strlen(buf);
        patternsize = 0;
        patterns = malloc(sz * 2); // worst case every character is double wide
        for (lp = 0; lp < sz; lp++)
          {
            translate(buf[lp], patterns, &patternsize);
          }
      } // if reading string from command line

    d("start thread");
    pthread_t t1;
    pthread_create(&t1, NULL, (void *) &display_thread, NULL);

    d("update marquee");
    update_marquee(buf, patterns, patternsize);
    // never gets here
    pthread_join(t1, NULL);

    return 0;
  }

