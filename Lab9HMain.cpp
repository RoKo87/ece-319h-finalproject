// Lab9HMain.cpp
// Runs on MSPM0G3507
// Lab 9 ECE319H
// Your name
// Last Modified: January 12, 2026

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/SlidePot.h"
#include "../inc/DAC5.h"
#include "../inc/DAC.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "UART1.h"
#include "UART2.h"
#include "images/images.h"

extern "C" void __disable_irq(void);
extern "C" void __enable_irq(void);
extern "C" void TIMG12_IRQHandler(void);
extern "C" void GROUP0_IRQHandler(void);

// ****note to ECE319K students****
// the data sheet says the ADC does not work when clock is 80 MHz
// however, the ADC seems to work on my boards at 80 MHz
// I suggest you try 80MHz, but if it doesn't work, switch to 40MHz
void PLL_Init(void){ // set phase lock loop (PLL)
  // Clock_Init40MHz(); // run this line for 40MHz
  Clock_Init80MHz(0);   // run this line for 80MHz
}

uint32_t M=1;
uint32_t Random32(void){
  M = 1664525*M+1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32()>>16)%n;
}


//IMPORTANT GLOBAL VARIABLES

int gState = 0;
int level = 1; //level in game
int score = 0;
int killCount = 0;
int timer = 1830;
char printBuf[256]; //idk if i need this

const uint32_t paindices[32] = {
    PA0INDEX,  PA1INDEX,  PA2INDEX,  PA3INDEX,  PA4INDEX, 
    PA5INDEX,  PA6INDEX,  PA7INDEX,  PA8INDEX,  PA9INDEX,
    PA10INDEX, PA11INDEX, PA12INDEX, PA13INDEX, PA14INDEX, 
    PA15INDEX, PA16INDEX, PA17INDEX, PA18INDEX, PA19INDEX,
    PA20INDEX, PA21INDEX, PA22INDEX, PA23INDEX, PA24INDEX, 
    PA25INDEX, PA26INDEX, PA27INDEX, PA28INDEX, PA29INDEX,
    PA30INDEX, PA31INDEX
};


const uint32_t pbindices[32] = {
    PB0INDEX,  PB1INDEX,  PB2INDEX,  PB3INDEX,  PB4INDEX, 
    PB5INDEX,  PB6INDEX,  PB7INDEX,  PB8INDEX,  PB9INDEX,
    PB10INDEX, PB11INDEX, PB12INDEX, PB13INDEX, PB14INDEX, 
    PB15INDEX, PB16INDEX, PB17INDEX, PB18INDEX, PB19INDEX,
    PB20INDEX, PB21INDEX, PB22INDEX, PB23INDEX, PB24INDEX, 
    PB25INDEX, PB26INDEX, PB27INDEX, 0, 0,
    0, 0
};


SlidePot Sensor(2040,-20); // copy calibration from Lab 7
int position = 0;




// CUSTOM CLASSES AND STRUCTS



// pincat classes and variables
class PinCat {
private:
  int isb;
  volatile unsigned int *outps;
  volatile unsigned int *oeps;
  volatile unsigned int *tglps;


public:
  volatile unsigned int *inps;
  PinCat() {
    isb = 0;
    outps = &(GPIOA->DOUT31_0);
    oeps = &(GPIOA->DOE31_0);
    inps = &(GPIOA->DIN31_0);
    tglps = &(GPIOA->DOUTTGL31_0);
  }

  PinCat(int x) {
    if (x == 0) {
      isb = 0;
      outps = &(GPIOA->DOUT31_0);
      oeps = &(GPIOA->DOE31_0);
      inps = &(GPIOA->DIN31_0);
      tglps = &(GPIOA->DOUTTGL31_0);
    }
    else {
      isb = 1;
      outps = &(GPIOB->DOUT31_0);
      oeps = &(GPIOB->DOE31_0);
      inps = &(GPIOB->DIN31_0);
      tglps = &(GPIOB->DOUTTGL31_0);
    }
  }

  int enableP(unsigned char pin) {
    if (pin > 31) return 0;
    *oeps |= (1 << pin); return 1;
  }

  int disableP(unsigned char pin) {
      if (pin > 31) return 0;
      *oeps &= ~(1 << pin); return 1;
  }

  int setP(unsigned char pin) {
    if (pin > 31) return 0;
    *outps |= (1 << pin); return 1;
  }

  int clearP(unsigned char pin) {
    if (pin > 31) return 0;
    *outps &= ~(1 << pin); return 1;
  }

  int toggleP(unsigned char pin) {
    if (pin > 31) return 0;
    *tglps |= (1 << pin); return 1;
  }

  bool ibit(unsigned char pin) {
    if (pin > 31 || !(*inps & (1 << pin))) return 0;
    return 1;
  }

  int initP(unsigned char pin, int regval) {
    if (pin > 31) return 0;
    if (isb) IOMUX->SECCFG.PINCM[pbindices[pin]] = regval;
    else IOMUX->SECCFG.PINCM[paindices[pin]] = regval;
    return 1;
  }
};

PinCat PA(0); PinCat PB(1);

// gobj classes and variables
class GObj {
  public:
  Image *img;
  int x, y;       // current logical position
  char alive, prevAlive;
  int moveWait, mWMax;
  int kflag, rflag;

  GObj() {
    img = NULL; x = 0; y = 0;
    alive = 0; prevAlive = 0;
    moveWait = 20; mWMax = 20; kflag = 0; rflag = 0;
  }
  //dead at initialization
  GObj(Image *i, int xp, int yp) {
    img = i;
    x = xp; y = yp;
    alive = 0; prevAlive = 0;
    moveWait = 20; mWMax = 20; kflag = 0; rflag = 0;
  }
  //alive at initialization
  GObj(Image *i, int xp, int yp, char a) {
    img = i;
    x = xp; y = yp;
    alive = a; prevAlive = 0;
    moveWait = 20; mWMax = 20; kflag = 0; rflag = 0;
  }

  // --- ISR-safe (no LCD) ---
  void ISpawn() { alive = 1; rflag = 1; }
  void IKill()  { kflag = 1; rflag = 1; }

  void IMoveIf(signed char dx, signed char dy) {
    if (alive) {
      moveWait--;
      if (moveWait == 0) {
        x += dx; y += dy;
        moveWait = mWMax;
        rflag = 1; // only flag when movement actually occurs
      }
    }
  }

  // --- LCD (call from main only) ---
  void Render() {
    if (!rflag) return; // nothing changed, skip LCD
    rflag = 0;
    if (kflag) {
      alive = 0;
      ST7735_FillRect(x, y, img->w, img->h, ST7735_GREEN); kflag = 0;
    } else if (alive) {
      ST7735_DrawBitmap(x, y + img->h, img->bmap, img->w, img->h);
    }
    prevAlive = alive;
  }

  // kept for LaunchProj (called from main)
  void Spawn() { alive = 1;
    ST7735_DrawBitmap(x, y + img->h, img->bmap, img->w, img->h); }


  void Kill()  { alive = 0;
    ST7735_FillRect(x, y, img->w, img->h, ST7735_GREEN); }


  void Position(int xn, int yn) { x = xn; y = yn;
    ST7735_DrawBitmap(xn, yn + img->h, img->bmap, img->w, img->h); }

    
  void MoveIf(signed char dx, signed char dy) {
    if (alive) { moveWait--;
      if (moveWait == 0) {
        ST7735_FillRect(x, y, img->w, img->h, ST7735_GREEN);
        x += dx; y += dy;
        ST7735_DrawBitmap(x, y + img->h, img->bmap, img->w, img->h);
        moveWait = mWMax;
      }
    }
  }

  void Rate(int n) { mWMax = n; }
};

class Zombie : public GObj {
public:
  int initX, initY;
  int dy;
  int ztimer;
  bool useRandomX;
  int unlock; //level that it unlocsk

  Zombie(Image* img, int x, int y, int dy, int initTimer, bool randomX = false, int minLvl = 1)
    : GObj(img, x, y), initX(x), initY(y), dy(dy), ztimer(initTimer), useRandomX(randomX), unlock(minLvl) {}

  void Respawn() {
    IKill();
    ztimer = 90; // 3 seconds at 30Hz
  }

  void Update() {
    if (!alive && gState == 1 && level >= unlock) {
      if (ztimer > 0) ztimer--; //spawn timer (is dead, still waiting to spawn)
      else { //spawn if dead (spawn now)
        x = useRandomX ? (int)Random(128 - img->w) : initX;
        y = initY;
        ISpawn();
      }
    } else if (alive) { //move if alive
      IMoveIf(0, (signed char)dy);
    }
  }
};

GObj plant(&plantI, 0, 150, 1);

Zombie zomb0(&zomb0I,  0, 18, 1,   0, false);
Zombie zomb1(&zomb1I, 46, 18, 2,  120, true);
Zombie zomb2(&zomb2I, 90, 18, 3,  240, true);
Zombie zomb3(&zomb2I,  0, 18, 3,    0, true,  7); //spawns at lvl 7
Zombie zomb4(&zomb0I,  12, 18, 1,    0, true,  10); //spawns at lvl 10
Zombie zomb5(&zomb1I, 63, 18, 2,  120, true,  13); //spawns at lvl 13

Zombie* zombies[] = {&zomb0, &zomb1, &zomb2, &zomb3, &zomb4, &zomb5};

class Ptile : public GObj {
  public:
  Ptile() : GObj(&blet, plant.x, plant.y, 0) {
    this->Rate(1);
  }
};

Ptile* ptiles = new Ptile[10]();
int ptilei = 0;


// note arrays

const uint16_t SineWave[32] = {
  1024,1223,1415,1592,1747,1875,1969,2028,
  2047,2028,1969,1875,1747,1592,1415,1223,
  1024, 824, 632, 455, 300, 173,  78,  20,
     0,  20,  78, 173, 300, 455, 632, 824
};


Note sound0[30] =
  {{rest, 0, 0.5f}, {B, 0, 0.4f}, {rest, 0, 0.1f}, {B, 0, 0.4f}, {rest, 0, 0.1f}, {B, 0, 0.5f},
   {A, 0, 0.5f}, {Fs, 0, 0.4f}, {rest, 0, 0.1f}, {Fs, 0, 1.0f},
   {E, 0, 0.4f}, {rest, 0, 0.1f}, {E, 0, 0.5f}, {D, 0, 0.5f}, {E, 0, 0.5f},
   {Fs, 0, 1.0f}, {rest, 0, 0.5f}, {E, 0, 0.5f},
   {Fs, 0, 0.4f}, {rest, 0, 0.1f}, {Fs, 0, 0.5f}, {D, 0, 0.5f}, {E, 0, 0.5f},
   {Fs, 0, 0.75f}, {D, 0, 0.75f}, {E, 0, 0.5f},
   {Fs, 0, 0.75f}, {D, 0, 0.75f}, {E, 0, 0.5f}, {Fs, 0, 1.0f}};

Note soundKill[2] = {{C, 0, 0.5f}, {G, 0, 2.0f}};
Note soundLevel[3] = {{E, 0, 0.25f}, {G, 0, 0.25f}, {C, 1, 2.0f}};
Note soundCross[2] = {{D, 0, 0.5f}, {A, -1, 1.0f}};
Note soundDead[3] = {{G, -1, 1.0f}, {C, -1, 1.0f}, {G, -2, 1.0f}};
Note soundShoot[1] = {{G, 1, 0.1f}};




typedef enum {English, Spanish, Portuguese, French} Language_t;
Language_t myLanguage=English;
Language_t sellang = English;
typedef enum {HELLO, GOODBYE, LANGUAGE, SELECT, GAMEOVER, SCORESTR, RESTART, MAINMENU} phrase_t;
const char Hello_English[] ="Hello";
const char Hello_Spanish[] ="\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[] ="All\x83";
const char Goodbye_English[]="Goodbye";
const char Goodbye_Spanish[]="Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[] = "Au revoir";
const char Language_English[]="English";
const char Language_Spanish[]="Espa\xA4ol";
const char Language_Portuguese[]="Portugu\x88s";
const char Language_French[]="Fran\x87" "ais";

const char *Phrases[8][4]={
  {Hello_English,Hello_Spanish,Hello_Portuguese,Hello_French},
  {Goodbye_English,Goodbye_Spanish,Goodbye_Portuguese,Goodbye_French},
  {Language_English,Language_Spanish,Language_Portuguese,Language_French},
  {"Press SW1 to \nselect!", "Pulsa SW1 para \nseleccionar!", "Pressione SW1 para \nselecionar!", "Appuyez sur SW1 \npour s\x82lectionner!"},
  {"Game Over", "Juego terminado", "Fim de jogo", "Partie termin\x82 e"},
  {"Score: ", "Puntos: ", "Pontos: ", "Score: "},
  {"SW2 to restart", "SW2: reiniciar", "SW2: reiniciar", "SW2: recommencer"},
  {"SW1: languages", "SW1: idiomas", "SW1: idiomas", "SW1: langues"}};

//main helper functions
void initialize();
int  Scene0();
int  Scene0();
int  Scene1(Language_t lang);
int  Scene2(Language_t lang);

// CUSTOM FUNCTIONS


// collision detector
int isCollide(GObj a, GObj b) {
  return a.alive && b.alive &&
         a.x < b.x + b.img->w && a.x + a.img->w > b.x &&
         a.y < b.y + b.img->h && a.y + a.img->h > b.y;
}


int FindDeadPtile() {
  for (int i = 0; i < 10; i++) {
    if (!ptiles[i].alive) return i;
  }
  return -1;
}

void LaunchProj() {
  ptilei = FindDeadPtile();
    if (ptilei != -1) { 
      ptiles[ptilei].Position(plant.x + (plant.img->w - ptiles[ptilei].img->w) / 2, plant.y);
      ptiles[ptilei].Spawn();
    }
  PlaySound(soundShoot, 1, 240, 0);
}

void ST7735_OutStringM(char *str, int x, int y) {
  int start = x;
  ST7735_SetCursor(x, y);
  ST7735_OutString((char*)"                     ");
  ST7735_SetCursor(x, y);
  for (int i = 0; str[i] != 0; i++) {
    if (str[i] != '\n') {
      ST7735_OutChar(str[i]); x++; 
    } else {
      x = start;
      y++;
      ST7735_SetCursor(x, y);
      ST7735_OutString((char*)"                     ");
    }
    ST7735_SetCursor(x, y);
  }
}

void ResetGame() {
  // reset all game state
      score = 0; level = 1; killCount = 0; timer = 1830;
      zomb0 = Zombie(&zomb0I,  0, 18, 1,   0, false);
      zomb1 = Zombie(&zomb1I, 46, 18, 2, 120, true);
      zomb2 = Zombie(&zomb2I, 90, 18, 3, 240, true);
      zomb3 = Zombie(&zomb2I,  0, 18, 3,   0, true, 7);
      zomb4 = Zombie(&zomb0I, 12, 18, 1,   0, true, 10);
      zomb5 = Zombie(&zomb1I, 63, 18, 2, 120, true, 13);
      plant.x = 0; plant.y = 150; plant.alive = 0; 
      plant.rflag = 0; plant.kflag = 0;
      for (int i = 0; i < 10; i++) {
        ptiles[i].alive = 0;
        ptiles[i].kflag = 0; ptiles[i].rflag = 0;
      }
}



// GAME ENGINE (INTERRUPTS)


int iflag = 0;
int isGreen = 0;
int plantx; //for debugging
void TIMG12_IRQHandler(void){uint32_t pos,msg;
  if((TIMG12->CPU_INT.IIDX) == 1){ // this will acknowledge
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
// game engine goes here
    // 1) sample slide pot
    // 2) read input switches
    // 3) move sprites
    // 4) start sounds
    // 5) set semaphore
    // NO LCD OUTPUT IN INTERRUPT SERVICE ROUTINES
    Sensor.Save(Sensor.In());
    position = Sensor.Convert(0);
    plantx = -30 + 1*(position/15);
    if (gState == 1) {
      plant.x = plantx; //move plant according to slide pot
      plant.rflag = 1;
    }
    if (gState == 1 && timer >= 0) timer--; //decrease timer
    for (Zombie* z : zombies) z->Update();


    for (int i = 0; i < 10; i++) { //projectile interaction
      ptiles[i].IMoveIf(0, -1);
      if (ptiles[i].y < 30) ptiles[i].IKill();
      for (Zombie* z : zombies) {

        if (isCollide(ptiles[i], *z) && gState == 1) {  //projectile-zombie collision
          ptiles[i].IKill();
          score += ((level + 8) * (plant.y - z->y)) >> 6;
          z->Respawn(); //respawn sprite logic
          killCount++;
          if (killCount % 3 == 0) { //leveling up
            level++;
            timer += 480;
            int newMax = (level > 20) ? 1 : (21 - level); //calculating zombie speed
            for (Zombie* zz : zombies) zz->mWMax = newMax;
            PlaySound(soundLevel, 3, 240, 0);
          }
          else PlaySound(soundKill, 2, 240, 0);
        }

      }
    }
    for (Zombie* z : zombies) 
      if (z->alive && z->y > 140 && gState == 1) { //zombie reaches bottom
        z->Respawn();
        int levbon = (level <= 21) ? 2 : level - 20;
        timer -= 1800 * (levbon/2); 
        PlaySound(soundCross, 2, 240, 0);
      }
    iflag = 1; //interrupt signal
    GPIOB->DOUTTGL31_0 = GREEN; // toggle PB27 (minimally intrusive debugging)
  }
}


uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80|((GPIOB->DOUT31_0>>26)&0x03));
}

void GROUP0_IRQHandler(void) {
    //for pa28
    if (GPIOA->CPU_INT.IIDX == 29) { // IIDX returns (pin_number + 1)
      if (gState == 0) Scene1(English);
      else {
        LaunchProj();
      }
    }
}













// THE ACTUAL GAME FUNCTION 





//WIRINGS FOR BREADBOARD FROM PCB:
//WIRE COLOR                PIN              DESTINATION       WHERE ON PCB
//-----------------------------------------------------------------------------
//green                     PB6              LCD2             row 4 pin 8
//green w/ mark             PB15             LCD1             row 4 pin 4
//gray                      PB8              LCD4             row 4 pin 6
//white                     PA13             LCD3             row 3 pin 10   
//yellow                    GND              LCD8             row 2 pin 2
//yellow w/ mark            PB9              LCD5             row 1 pin 7
//purple                    3V3              positive rail    row 1 pin 1
//blue                      PA15             AJ left          row 2 pin 10
//red                       5V               WEMOS: 5V        5V
//brown wires               PA8, PA22        WEMOS: RX, TX    -----------------

int runstate = -1;

int main() {
  initialize();
  // 3-way handshake with Wemos
  UART1_OutString("hi wemos");
  Clock_Delay1ms(500); // give Wemos time to receive and respond
  UART1_OutString("let's begin! :)");
  runstate = Scene0();
  while (1) {/*done!*/};
  return runstate;
}

void initialize() {
  __disable_irq();
  PLL_Init(); // set bus speed
  LaunchPad_Init();
  ST7735_InitPrintf(INITR_BLACKTAB); // INITR_REDTAB for AdaFruit, INITR_BLACKTAB for HiLetGo
  ST7735_FillScreen(ST7735_BLACK);
  Sensor.Init(); // PB18 = ADC1 channel 5, slidepot
  Switch_Init(); // initialize switches
  LED_Init();    // initialize LED
  Sound_Init(3, SineWave, 32);  // initialize sound
  UART1_Init(); // PA8  Tx to external device
  UART2_Init(); // PA22 Rx from external device
  // TExaS_Init(0,0,&TExaS_LaunchPadLogicPB27PB26); // PB27 and PB26
    // initialize interrupts on TimerG12 at 30 Hz
  TimerG12_IntArm(80000000/30, 1);
  
  // IOMUX->SECCFG.PINCM[PA28INDEX] = 0x00080001; //WHY?????

  
  IOMUX->SECCFG.PINCM[PA27INDEX] = 0x00040081; // PA27 as input with pull-up (PC=10)
  GPIOA->POLARITY31_16 = 0x01000000;  // Bits 25:24 in POLARITY31_16 set to 01, set rising edge for pa28

  GPIOA->CPU_INT.ICLR = 0x10000000; // clear bit 28
  GPIOA->CPU_INT.IMASK = 0x10000000; // arm PA28

  NVIC->IP[0] = (NVIC->IP[0] & (~0x000000FF)) | (2 << 6); // NVIC->IP[0] bits 7,6 are priority for IRQ 0

 
  NVIC->ISER[0] = 1 << 0;  // Enable Group 0 interrupt in NVIC

  // initialize all data structures
  __enable_irq();
}

int Scene0(void){ // final main
  gState = 0;
  int rState = 0;
  
  //SCENE 0: SELECT LANGUAGE SCREEN!
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetTextColor(ST7735_WHITE);
  ST7735_SetCursor(2, 2);
  ST7735_OutString("Select Language:");
  ST7735_SetCursor(2, 4);
  ST7735_OutString((char*)Phrases[2][0]); //english default
  ST7735_OutStringM((char*)Phrases[3][0], 2, 6); //select

  while(1){
    // wait for semaphore
      Sensor.Sync();
       // clear semaphore
       if (position/501 != sellang && position < 2000) {
          sellang = (Language_t)(position/501);
          ST7735_SetCursor(2, 4);
          ST7735_OutString((char*)"                     ");
          ST7735_SetCursor(2, 4);
          ST7735_OutString((char*)Phrases[2][sellang]);
          ST7735_OutStringM((char*)Phrases[3][sellang], 2, 6); //select
       }

       if (PA.ibit(28)) {
          M = TIMG12->COUNTERREGS.CTR; // seed RNG with timer value at button press
          rState = Scene1(sellang);
          return rState;
       }
       // update ST7735R
    // check for end game or level switch
  }

}

int Scene1(Language_t lang) {
  __disable_irq();
  gState = 1;
  ST7735_FillScreen(ST7735_GREEN);
  isGreen = 0; //interrupt handling


  ST7735_FillRect(0, 0, 128, 18, ST7735_BLACK);
  ST7735_SetCursor(2, 0);
  ST7735_OutString((char*)Phrases[0][lang]); //hello
  ST7735_SetCursor(17, 0);
  ST7735_OutUDec(timer / 30);
  ST7735_OutChar('s');
  ST7735_SetCursor(2, 1);
  ST7735_OutString((char*)Phrases[SCORESTR][lang]);
  ST7735_OutUDec(score);
  ST7735_SetCursor(17, 1);
  ST7735_OutUDec(level);
  plant.alive = 1;
  plant.rflag = 1; // force first draw
  __enable_irq();
  

  PlaySound(sound0, 30, 180, 0);
  int held = 1;
  int lastScore = score;
  int lastTime  = timer / 30;
  int lastLevel = level;
  while(1) {
    if (iflag) {
      iflag = 0;
      plant.Render();
      for (int i = 0; i < 10; i++) {
        if (ptiles[i].kflag) { // about to erase — check for zombie overlap
          for (Zombie* z : zombies) {
            if (z->alive &&
                ptiles[i].x < z->x + z->img->w && ptiles[i].x + ptiles[i].img->w > z->x &&
                ptiles[i].y < z->y + z->img->h && ptiles[i].y + ptiles[i].img->h > z->y) {
              z->rflag = 1; // zombie will redraw after ptile erases
            }
          }
        }
        ptiles[i].Render();
      }
      for (Zombie* z : zombies) z->Render();

      //updated values in isr
      if (score != lastScore) {
        lastScore = score;
        ST7735_FillRect(0, 8, 128, 10, ST7735_BLACK);
        ST7735_SetCursor(2, 1);
        ST7735_OutString((char*)Phrases[SCORESTR][lang]);
        ST7735_OutUDec(score);
        ST7735_SetCursor(17, 1);
        ST7735_OutUDec(level);
      }
      if (timer / 30 != lastTime) {
        lastTime = timer / 30;
        ST7735_FillRect(17*6, 0, 128 - 17*6, 8, ST7735_BLACK);
        ST7735_SetCursor(17, 0);
        ST7735_OutUDec(lastTime);
        ST7735_OutChar('s');
      }
      if (level != lastLevel) {
        lastLevel = level;
        ST7735_FillRect(17*6, 8, 128 - 17*6, 10, ST7735_BLACK);
        ST7735_SetCursor(17, 1);
        ST7735_OutUDec(level);
      }
    }
    if (!PA.ibit(28)) held = 0;
    if (!held && PA.ibit(28)) {
      LaunchProj();
      held = 1;
    }
    if (timer < 0) return Scene2(lang);
  }
  return 1;
}

int Scene2(Language_t lang) {
  __disable_irq();
  PlaySound(soundDead, 3, 240, 0);
  gState = 2;
  
  // transmit score to Wemos over UART1
  char scoreBuf[16];
  snprintf(scoreBuf, sizeof(scoreBuf), "SCORE:%d", score);
  UART1_OutString(scoreBuf);
  
  ST7735_FillScreen(ST7735_BLACK);
  ST7735_SetCursor(2, 2);
  ST7735_OutString((char*)Phrases[GAMEOVER][lang]);
  ST7735_SetCursor(2, 4);
  ST7735_OutString((char*)Phrases[SCORESTR][lang]);
  ST7735_OutUDec(score);
  ST7735_SetCursor(2, 6);
  ST7735_OutString((char*)Phrases[MAINMENU][lang]);
  ST7735_SetCursor(2, 8);
  ST7735_OutString((char*)Phrases[RESTART][lang]);
  // wait for PA27 press to restart
  while (PA.ibit(27) || PA.ibit(28)) {} // wait for release if already held
  while (1) {
    if(PA.ibit(27)) {
      ResetGame();
      return Scene1(lang);
    }
    if(PA.ibit(28)) {
      ResetGame();
      return Scene0();
    }
  }
}


