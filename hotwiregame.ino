// The hot buzzer wire game (German: Der heiße Draht)
// The game ist built with 3 signals:
// green  - A: The "host of the buzzer slope
//             Touching this with the buzzer slope will cause reset to 0 for the time
//             Releasing it will start measurement - player moves onto B
// red    - B: This is the touch point on the other side of the board. Touching it confirms that the player moved 
//             on the complete wire.
// yellow - D: The wire. Touching this will increment the touch cont. Target ist to move the buzzer slope from A to 
//             B and back to A without touching D.
// As the trigger inputs are low active, you can add this board to an existing circuit with 5V LEDs easily (The slope is GND)

#include "U8glib.h"
#include "pitches.h"
#include "TimerOne.h"

// notes in the melody:
int melody[] = {
  8, NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  0,4, 8, 8, 4, 4, 4, 4, 4
};

int greenPin = 2;
int redPin = 3;
int yellowPin = 4;

#define RESET 0
#define FORWARD 1
#define BACKWARD 2

volatile int imillis = 0;
unsigned char iSec = 0;
unsigned char iMin = 0;
unsigned char touchCount = 0;
volatile char mode = 1;
char timeSinceStart[16];
char lastTimeString[16];
char lastTouched[16];
char lowRow[16];

volatile char main_INTMSK = 0;
char last_main_INTMSK = 0;
#define INT_A_CAUGHT  0x1
#define INT_D_CAUGHT  0x2
#define PLAY_TOUCHED  0x4
#define PLAY_FINISHED 0x8

int INT_A_LockTimer;
int INT_D_LockTimer;
int soundPlayTimer;
volatile int thisNote = 0;
volatile int noteDuration = 0;

int mainDiffTimer = 0;

U8GLIB_SSD1306_128X32 u8g(U8G_I2C_OPT_NONE); // Just for 0.91”(128*32)
//U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);// for 0.96” and 1.3”


int finishMelody[] = {
  NOTE_G3, NOTE_B3, NOTE_C4, NOTE_D4, 0, NOTE_B3, NOTE_D4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int finishDurations[] = {
  7, 8, 8, 8, 8, 8, 8, 2
};

int noTouchfinishMelody[] = {
  NOTE_G3, 0, NOTE_G3, NOTE_G3, NOTE_G4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noTouchfinishDurations[] = {
  5, 8, 8, 8, 8, 2
};


void paint(void) {
  u8g.setFont(u8g_font_unifont);

  if (mode == RESET)
  {
    u8g.setPrintPos(0, 10); 
    u8g.print("Letzte Zeit");
    u8g.setPrintPos(0, 21); 
    u8g.print(lastTimeString);
    u8g.setPrintPos(0, 32); 
    u8g.print(lastTouched);
  }
  else 
    if (mode >= FORWARD)
    {
      sprintf(timeSinceStart, "Zeit: %02d:%02d.%03d", iMin, iSec, imillis);
      u8g.setPrintPos(0, 10); 
      u8g.print(timeSinceStart);
      sprintf(lowRow, "Beruehrt: %3d", touchCount);
      u8g.setPrintPos(0, 21); 
      u8g.print(lowRow);
    }
}

void playSnd()
{
  tone(A3, finishMelody[thisNote]);
  noteDuration = 1000 / (finishDurations[thisNote]);
}

void handleINT_A()
{
  if (!(main_INTMSK & INT_A_CAUGHT))
  {
    main_INTMSK |= INT_A_CAUGHT;
    INT_A_LockTimer = imillis;
  }
}

void handleINT_D()
{  
  if (!(main_INTMSK & INT_D_CAUGHT))
  {
    main_INTMSK |= INT_D_CAUGHT;
    INT_D_LockTimer = imillis;
    if (mode != RESET)
    {
      main_INTMSK |= PLAY_TOUCHED;
      tone(A3, NOTE_G3);
      soundPlayTimer = imillis;
      touchCount++;
    }
  }
}

void milliseconds() {
  imillis++;
  if (imillis == 1000)
  {
    imillis = 0;
    iSec++;
    if (iSec == 60)
    {
      iSec = 0;
      iMin++;
    }
  } 
}


void setup(void) {
  noInterrupts();           // disable all interrupts
  // flip screen, if required
  // u8g.setRot180();
  pinMode(greenPin, INPUT_PULLUP);        // sets the digital pin 7 as input
  pinMode(redPin, INPUT_PULLUP);        // sets the digital pin 7 as input
  pinMode(yellowPin, INPUT_PULLUP);        // sets the digital pin 7 as input

  memset(lowRow, 0, 16);
  mode = RESET;
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(155);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }


  Timer1.initialize(1000);
  Timer1.attachInterrupt(milliseconds);

// Activate falling Edge for A and D
  attachInterrupt(digitalPinToInterrupt(greenPin), handleINT_A, CHANGE);
  attachInterrupt(digitalPinToInterrupt(redPin), handleINT_D, FALLING);
  
  interrupts();             // enable all interrupts
}


int getDiffTime(int stamp)
{
  if (stamp > imillis)
  {
    return 1000 - (stamp - imillis);
  }
  else
  {
    return imillis - stamp;
  }
}

void playSound(int* melody, int* durations)
{
  int* durationStart = &durations[1];
  
  for (thisNote = 0; thisNote < durations[0]; thisNote++) {

  // to calculate the note duration, take one second divided by the note type.
  //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
  int noteDuration = 600 / durationStart[thisNote];
  if (melody[thisNote])
    tone(A3, melody[thisNote]);
  else
    noTone(A3);
  int soundPlayTimer = imillis;
  while (getDiffTime(soundPlayTimer) <= noteDuration)
  {}

  // to distinguish the notes, set a minimum time between them.
  // the note's duration + 30% seems to work well:
  int pauseBetweenNotes = noteDuration * 12 / 10;
  noTone(A3);
  soundPlayTimer = imillis;
  while (getDiffTime(soundPlayTimer) <= pauseBetweenNotes)
  {}
  // stop the tone playing:
}

}

void loop(void) {
  if (main_INTMSK & PLAY_TOUCHED)
  {
    if (getDiffTime(soundPlayTimer) >=250)
    {
      main_INTMSK &= ~PLAY_TOUCHED;
      noTone(A3);
    }
  }

  if (main_INTMSK & INT_A_CAUGHT)
  {
    if (getDiffTime(INT_A_LockTimer) >=50)
    {
      main_INTMSK &= ~INT_A_CAUGHT;
      int pinstate = digitalRead(greenPin);
      if (pinstate == HIGH)
      {
        if (mode == RESET)
        {
          mode = FORWARD;
          imillis = 0;
          iSec = 0;
          iMin = 0;
          touchCount = 0;
        }
      }
      else
      {
        if (mode == BACKWARD)
        {
          noTone(A3);
          main_INTMSK |= PLAY_FINISHED;
          main_INTMSK &= ~PLAY_TOUCHED; // clear possible touch sound
        }
        mode = RESET;
      }
    }
  }

  if (main_INTMSK & PLAY_FINISHED)
  {
    if (!(last_main_INTMSK & PLAY_FINISHED))
    {
      strncpy(lastTimeString, timeSinceStart, 16);
      strncpy(lastTouched, lowRow, 16);
      thisNote = 0;

      // picture loop
      u8g.firstPage();  
      do {
        paint();      
      } while( u8g.nextPage() );

      if (touchCount != 0)
      {
        playSound(finishMelody, finishDurations);
      }
      else
      {
        playSound(noTouchfinishMelody, noTouchfinishDurations);
      }
      noTone(A3);
      main_INTMSK &=  ~PLAY_FINISHED;
    }
  }
  else
  {
    // picture loop
    u8g.firstPage();  
    do {
      paint();      
    } while( u8g.nextPage() );
  }

  if (main_INTMSK & INT_D_CAUGHT)
  {
    if (getDiffTime(INT_D_LockTimer) >=700)
    {
      main_INTMSK &= ~INT_D_CAUGHT;
    }
  }

  if (digitalRead(yellowPin) == LOW)
  {
    mode = BACKWARD;
  }
  last_main_INTMSK = main_INTMSK;
}
