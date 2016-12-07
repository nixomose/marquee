/* Display a string of text on the led display and have it slide across to fit the message. */

/* Same thing as the marquee for the 74hc595 but for the max 7219.
 * Put it into bcd decode off mode so we can write all the little segements the way we want
 * so we can write letters. */

// http://www.plingboot.com/2013/01/driving-the-max7219-with-the-raspberry-pi/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <wiringPi.h>

#define d(x)   // printf(x "\n")
#define e(x,y) // printf(x "%d\n", y)

#define b10000000 0x80


/* PGFEDCBA  p is the dot
 *
 *    A
 *   F B
 *    G
 *   E C
 *    D   p
 *
 */

/*
 * http://opensourceecology.org/w/images/e/e5/Rasp_pinout.png
 *
 * wiringpi pin 4  is gpio 23 which is raspberry pi pin 16
 * wiringpi pin 16 is gpio 15 which is raspberry pi pin 10
 * wiringpi pin 15 is gpio 14 which is raspberry pi pin 8
 */

int max_displays = 2; // how many 8 digit displays you have chained together.

/* So it turns out when daisy chaining displays together, the trick is just like it says, of course it makes
 * more sense after you understand it...
 * You write out a 16 bit data block for the first display, and if you have another display, you write out
 * another 16 bits, which will push the first 16 bits to the second display, THEN you latch.
 * So if you have three displays, you write out 3 sets of 16 bits, the first command sent goes to the last
 * display in the chain.
 * The other interesting point is that you address each display individually, so the 8th character in
 * display one is char #8. The 16th char in the string is char #8 also but in display two.
 * So the easiest thing to do is get your 16/24/whatever string of characters together
 * then write out #17, #9 #1, latch, because they are all position 1,
 * then write out #18, #10, #2, latch, and they are all position 2. and so on.
 */


int HC_LATCH = 4; // RCK of 8x7segment module pin 16

int HC_CLOCK = 16; // SCK of 8x7segment module pin 10

int HC_DATA = 15; // DIO of 8x7segment module,  pin 8

// same concept as 74hc595, you set the data, kick the clock once per bit and then kick the latch/load to denote end of byte
#define MAX_DATA        HC_DATA  // DIN on max 7419
#define MAX_CLOCK       HC_CLOCK // CLK on max 7419
#define MAX_LOAD        HC_LATCH // CS on max 7419


// The Max7219 has higher minded commands too... These are the first byte (first 8 of 16 bits) sent

#define DECODE_MODE   0x09
#define INTENSITY     0x0a
#define SCAN_LIMIT    0x0b
#define SHUTDOWN      0x0c
#define DISPLAY_TEST  0x0f

/* The big difference between the max7419 and the 74hc595 is that the max remembers what you tell it and will
 * maintain the display as you've sent it data. Much easier than the hc where you have to constantly refresh each
 * image because it can only display one 7 segment bit pattern at a time, so if you want different display images
 * you have to draw each one many times a second over and over. So for the max we don't need a separate thread to maintain
 * the display. */


typedef int bool;
typedef int boolean;
#define false 0
#define true (!false)

#define BLANKDIGIT 0

#define NOSPACES 1
#define SPACES 2

// this is the data buffer that gets displayed
char *display = NULL;
int startpos; // this is the position of the character that goes in the leftmost display position

int filemode = 0;
char *filename = NULL;
char *sourcebuf = NULL;
char *patternbuf = NULL;


void reset(void);
void displaypattern(char position, char value);
void latch(void);


void init(void)
  {
    wiringPiSetup();
    pinMode(MAX_LOAD, OUTPUT);
    pinMode(MAX_CLOCK, OUTPUT);
    pinMode(MAX_DATA, OUTPUT);
    display = malloc(max_displays * 8);
    int lp;
    for (lp = 0; lp < max_displays * 8; lp++)
      display[lp] = BLANKDIGIT;

    startpos = 0;
    reset();
  }

void repeat_send(char cmd, char data, int repeat)
  {
    int lp;
    for (lp = 0; lp < repeat; lp++)
      displaypattern(cmd, data);
    latch();
  }

void reset(void)
  {
    /* BCD decode mode off : data bits correspond to the segments (A-G and DP) of the seven segment display.
       BCD mode on :  0 to 15 =  0 to 9, -, E, H, L, P, and ' '     */

    repeat_send(SCAN_LIMIT, 7, max_displays);    // set up to scan all eight digits
    repeat_send(DECODE_MODE, 0, max_displays);   // Set BCD decode mode off
    repeat_send(DISPLAY_TEST, 0, max_displays);  // Disable test mode
    repeat_send(INTENSITY, 1, max_displays);     // set brightness 0 to 15
    repeat_send(SHUTDOWN, 1, max_displays);      // come out of shutdown mode / turn on the digits
  }

void shift(boolean val)
  {
    /* to set a bit on the max7219, you set the clock low, set your data bit,
     * then set the clock high. Apparently it picks up the database as the clock raises. */
    int state = val ? HIGH : LOW;
    digitalWrite(MAX_CLOCK, LOW); // pi is slow enough that the chip can respond to us without adding a delay
    digitalWrite(MAX_DATA, state);
    digitalWrite(MAX_CLOCK, HIGH);
  }

void writebits(char val)
  {
    // write each bit of val to register
    char i;
    for (i = 0; i < 8; i++)
      shift(((val << i) & b10000000) != 0);
  }


void latch(void)
  {
    digitalWrite(MAX_LOAD, LOW);  // LOAD 0 to latch
    digitalWrite(MAX_LOAD, HIGH);  // set LOAD 1 to finish
  }


void displaypattern(char control, char value)
  {
    e("control", control);
    /* the max7219 takes 16 bits of information:
     * the first 8 bits is the register number, the second 8 bits is the data out. */
    writebits(control);
    writebits(value);
  }

void refresh_display()
  {
    int lp;
    d("loop");

    for (lp = 0; lp < 8; lp++) // always write 8 digits
      {
        int rp;
        for (rp = 0; rp < max_displays; rp++)
          {
            // loop through all the displays
            int invert = max_displays - rp; // we have to go from farthest display out towards the front because the first thing we send goes to the last display.
            int charpos = lp + (8 * invert);
            displaypattern(8 - lp, display[charpos]);
          }
        latch();
      }
    return;
  }

void led(int p, int a, int b, int c, int d, int e, int f, int g, char *patterns, int *patternsize)
  {
    char out = 0;
    out |= (g << 0);
    out |= (f << 1);
    out |= (e << 2);
    out |= (d << 3);
    out |= (c << 4);
    out |= (b << 5);
    out |= (a << 6);
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
        return led(0, 0, 0, 1, 1, 1, 0, 1, patterns, patternsize);

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
        return led(0, 0, 0, 1, 1, 1, 0, 0, patterns, patternsize);

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

    while (true)
      {
        d("start update marquee loop");
        if (filemode)
          {
            if (startpos == 0) // reread from file
              readfromfile(&patterns, &sz);
          }
        int lp;
        for (lp = 0; lp < (8 * max_displays); lp++)
          {
            int readpos = startpos + lp;
            if (readpos >= sz)
              readpos -= sz;
            char pic = patterns[readpos];
            display[lp] = pic;
          }

        startpos++;
        if (startpos >= sz)
          startpos = 0;

        /* Now that we've set our buffer to what we want to display, update the hardware... */

        refresh_display();

        d("before sleep");
        usleep(400000); // don't need to update it that often...
        d("after sleep");
        if (startpos == 0)
          reset(); // keep it from shutting down?
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

    d("update marquee");
    update_marquee(buf, patterns, patternsize);

    return 0;
  }

