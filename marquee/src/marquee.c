/* Display a string of text on the led display and have it slide across to fit the message. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
            delayMicroseconds(1000000); // 1000000 is one second
          }
      }
    return NULL;
  }

char led(int p, int a, int b, int c, int d, int e, int f, int g)
  {
    a = 1 - a;
    b = 1 - b;
    c = 1 - c;
    d = 1 - d;
    e = 1 - e;
    f = 1 - f;
    g = 1 - g;
    char out = 0;
    out |= (a << 0);
    out |= (b << 1);
    out |= (c << 2);
    out |= (d << 3);
    out |= (e << 4);
    out |= (f << 5);
    out |= (g << 6);
    out |= (p << 7);
    return out;
  }

char translate(char c)
  {
    switch (c)
      {
      case 'A':
        return led(0, 1, 1, 1, 0, 1, 1, 1);

      case 'B':
        return led(0, 0, 0, 1, 1, 1, 1, 1);

      case 'C':
        return led(0, 1, 0, 0, 1, 1, 1, 0);

      case 'D':
        return led(0, 0, 1, 1, 1, 1, 0, 1);

      case 'E':
        return led(0, 1, 0, 0, 1, 1, 1, 1);

      case 'F':
        return led(0, 1, 0, 0, 0, 1, 1, 1);

      case 'G':
        return led(0, 1, 1, 1, 1, 0, 1, 1);

      case 'H':
        return led(0, 0, 1, 1, 0, 1, 1, 1);

      case 'I':
        return led(0, 0, 0, 0, 0, 1, 1, 0);

      case 'J':
        return led(0, 0, 1, 1, 1, 1, 0, 0);

      case 'K':
        return led(0, 0, 1, 1, 0, 1, 1, 1);

      case 'L':
        return led(0, 0, 0, 0, 1, 1, 1, 0);

      case 'M':
        return led(0, 1, 0, 1, 0, 1, 0, 0);

      case 'N':
        return led(0, 0, 0, 1, 0, 1, 0, 1);

      case 'O':
        return led(0, 1, 1, 1, 1, 1, 1, 0);

      case 'P':
        return led(0, 1, 1, 1, 0, 0, 1, 1);

      case 'Q':
        return led(0, 1, 1, 1, 0, 0, 1, 1);

      case 'R':
        return led(0, 0, 0, 0, 0, 1, 0, 1);

      case 'S':
        return led(0, 1, 0, 1, 1, 0, 1, 1);

      case 'T':
        return led(0, 0, 0, 0, 1, 1, 1, 1);

      case 'U':
        return led(0, 0, 1, 1, 1, 1, 1, 0);

      case 'V':
        return led(0, 0, 0, 1, 1, 1, 0, 0);

      case 'W':
        return led(0, 0, 1, 0, 1, 0, 1, 0);

      case 'X':
        return led(0, 0, 1, 1, 0, 1, 1, 1);

      case 'Y':
        return led(0, 0, 1, 1, 1, 0, 1, 1);

      case 'Z':
        return led(0, 1, 1, 0, 1, 1, 0, 1);

        //lower case
      case 'a':
        return led(0, 1, 1, 1, 0, 1, 1, 1);

      case 'b':
        return led(0, 0, 0, 1, 1, 1, 1, 1);

      case 'c':
        return led(0, 1, 0, 0, 1, 1, 1, 0);

      case 'd':
        return led(0, 0, 1, 1, 1, 1, 0, 1);

      case 'e':
        return led(0, 1, 0, 0, 1, 1, 1, 1);

      case 'f':
        return led(0, 1, 0, 0, 0, 1, 1, 1);

      case 'g':
        return led(0, 1, 1, 1, 1, 0, 1, 1);

      case 'h':
        return led(0, 0, 1, 1, 0, 1, 1, 1);

      case 'i':
        return led(0, 0, 0, 0, 0, 1, 1, 0);

      case 'j':
        return led(0, 0, 1, 1, 1, 1, 0, 0);

      case 'k':
        return led(0, 0, 1, 1, 0, 1, 1, 1);

      case 'l':
        return led(0, 0, 0, 0, 1, 1, 1, 0);

      case 'm':
        return led(0, 1, 0, 1, 0, 1, 0, 0);

      case 'n':
        return led(0, 0, 0, 1, 0, 1, 0, 1);

      case 'o':
        return led(0, 1, 1, 1, 1, 1, 1, 0);

      case 'p':
        return led(0, 1, 1, 1, 0, 0, 1, 1);

      case 'q':
        return led(0, 1, 1, 1, 0, 0, 1, 1);

      case 'r':
        return led(0, 0, 0, 0, 0, 1, 0, 1);

      case 's':
        return led(0, 1, 0, 1, 1, 0, 1, 1);

      case 't':
        return led(0, 0, 0, 0, 1, 1, 1, 1);

      case 'u':
        return led(0, 0, 1, 1, 1, 1, 1, 0);

      case 'v':
        return led(0, 0, 0, 1, 1, 1, 0, 0);

      case 'w':
        return led(0, 0, 1, 0, 1, 0, 1, 0);

      case 'x':
        return led(0, 0, 1, 1, 0, 1, 1, 1);

      case 'y':
        return led(0, 0, 1, 1, 1, 0, 1, 1);

      case 'z':
        return led(0, 1, 1, 0, 1, 1, 0, 1);

        //numbers
      case '0':
        return led(0, 1, 1, 1, 1, 1, 1, 0);

      case '1':
        return led(0, 0, 1, 1, 0, 0, 0, 0);

      case '2':
        return led(0, 1, 1, 0, 1, 1, 0, 1);

      case '3':
        return led(0, 1, 1, 1, 1, 0, 0, 1);

      case '4':
        return led(0, 0, 1, 1, 0, 0, 1, 1);

      case '5':
        return led(0, 1, 0, 1, 1, 0, 1, 1);

      case '6':
        return led(0, 1, 0, 1, 1, 1, 1, 1);

      case '7':
        return led(0, 1, 1, 1, 0, 0, 0, 0);

      case '8':
        return led(0, 1, 1, 1, 1, 1, 1, 1);

      case '9':
        return led(0, 1, 1, 1, 1, 0, 1, 1);

        //symbols
      case '.':
        return led(1, 0, 0, 0, 0, 0, 0, 0);

      case ' ':
        return led(0, 0, 0, 0, 0, 0, 0, 0);

      default:
        return led(0, 1, 1, 1, 1, 1, 1, 1);

      }
  }

void update_marquee(char *orig, char *patterns, int sz)
  {
    /* take the bit patterns and display the first 8,
     * then wait a bit and print the next 8 starting at position 2
     * taking care to deal with 8 char strings, and wrapping around. */

    char dbuf[9];
    dbuf[8] = '\0';
    while (true)
      {
        d("start update marquee loop");
        int lp;
        for (lp = 0; lp < 8; lp++)
          {
            int readpos = startpos + lp;
            if (readpos >= sz)
              readpos -= sz;
            char pic = patterns[readpos];
            display[lp] = pic;
            dbuf[lp] = orig[readpos];
            printf ("%s\n", dbuf);
          }

        startpos++;
        if (startpos >= sz)
          startpos = 0;

        d("before sleep");
        sleep(1); // don't need to update it that often...
        d("after sleep");
      }
  }

int main(int num, char *opts[])
  {

    // get the string to print
    if (num < 2)
      {
        printf("marquee <string>\n");
        printf("pass the string to print as a parameter.\n");
        return 1;
      }
    /* take all params and make one long string out of it. */
    int lp;
    char *buf = malloc(1);
    *buf = '\0';
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
    char *patterns = malloc(sz);
    for (lp = 0; lp < sz; lp++)
      {
        patterns[lp] = translate(buf[lp]);
      }

    d("before init");
#ifndef onio
    init();
#endif
    d("after init");d("start thread");
    pthread_t t1;
    pthread_create(&t1, NULL, (void *) &display_thread, NULL);
    d("update marquee");
    update_marquee(buf, patterns, sz);
    // never gets here
    pthread_join(t1, NULL);

    return 0;
  }

