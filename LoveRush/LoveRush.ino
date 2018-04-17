#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "Images.h"

#define EEPROM_START_C1                 (EEPROM_STORAGE_SPACE_START + 128)
#define EEPROM_START_C2                 (EEPROM_START_C1 + 1)
#define EEPROM_SCORE                    (EEPROM_START_C1 + 2)

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Sprites sprite;

// setting up variables

struct Position {
  int8_t x;
  int8_t y;
};

Position player = { 54, 40 };
Position cloudl = { 0, 0 };
Position cloudr = { 103, 0 };
Position en1 = { 0, 0 };
Position en2 = { 0, 0 };
Position expl1 = { 0, 0 };
Position expl2 = { 0, 0 };
Position heartminus = { 0, 0 };
Position fuelpower = { 50, 0 };

// Globals
const uint8_t FPS = 60;
uint8_t powerUpCounter = 0;

uint8_t state = 0;

// variable for Cloud backdrop
int8_t backdropx = -3;
int8_t backdropy = 0;

// variable for enemy position
int8_t enemy1x = random(26,87);
int8_t enemy1y = -28;
int8_t enemy2x = random(26,87);
int8_t enemy2y = -28;

// variable for heart position
int8_t heartx = random(26,87);
int8_t hearty = -14;

uint8_t shipFrame = 0;
uint8_t enemyFrame = 0;
uint8_t heartFrame = 0;
uint8_t heartminusFrame = 0;
uint8_t explosionFrame = 0;
uint8_t fuelFrame = 0;
uint8_t powerup1Frame = 0;

int8_t shield = 3;
bool primed = false;

bool hit = false;
bool hearthit = false;
bool gotpowerup = false;

int8_t speed = 1;
uint16_t fueltimer = 0;
uint16_t fuelcount = 0;

uint16_t ledTimer = 0;
uint16_t ledTimerg = 0;
uint16_t ledTimerb = 0;
uint16_t flashtimer = 0;

int8_t ee1 = 0;
int8_t ee2 = 0;

int8_t heartcounter = 0;

uint8_t fadeWidth;

// score variables
uint16_t score = 0;
uint16_t highScore = 0;

// Extract individual digits of a uint8_t
template< size_t size > void extractDigits(uint8_t (&buffer)[size], uint8_t value)
{
  for(uint8_t i = 0; i < size; ++i)
  {
    buffer[i] = value % 10;
    value /= 10;
  }
}

// Extract individual digits of a uint16_t
template< size_t size > void extractDigits(uint8_t (&buffer)[size], uint16_t value)
{
  for(uint8_t i = 0; i < size; ++i)
  {
    buffer[i] = value % 10;
    value /= 10;
  }
}

void initEEPROM()
{
  char c1 = EEPROM.read(EEPROM_START_C1);
  char c2 = EEPROM.read(EEPROM_START_C2);
  
  if (c1 != 'L' || c2 != 'Z')
  {
    highScore = 0;
    EEPROM.update(EEPROM_START_C1, 'L');
    EEPROM.update(EEPROM_START_C2, 'Z');
    EEPROM.update(EEPROM_SCORE, highScore);
  }
  else
  {
    EEPROM.get(EEPROM_SCORE, highScore);
  }
}

// Resets the fade out effect
void resetFade()
{
  fadeWidth = 0;
}

// Resets the fade in effect
void resetFadeIn()
{
  fadeWidth = WIDTH;
}

// fade in function
bool fadeIn()
{
  for(uint8_t i = 0; i < (HEIGHT / 2); ++i)
  {
    arduboy.drawFastHLine(0, (i * 2), fadeWidth, BLACK);
    arduboy.drawFastHLine((WIDTH - fadeWidth), (i * 2) + 1, fadeWidth, BLACK);
  }

  // If fade isn't complete, decrease the counter
  if(fadeWidth > 0)
  {
    fadeWidth = fadeWidth - 4;
    return false;
  }
  else
    return true;
}

// fade out function
bool fadeOut()
{
  for(uint8_t i = 0; i < (HEIGHT / 2); ++i)
  {
    arduboy.drawFastHLine(0, (i * 2), fadeWidth, BLACK);
    arduboy.drawFastHLine((WIDTH - fadeWidth), (i * 2) + 1, fadeWidth, BLACK);
  }
  // If fade isn't complete, increase the counter
  if(fadeWidth < WIDTH)
  {
    ++fadeWidth;
    return false;
  }
  else
    return true;
  
}

void setup()
{
  // things that needs to be ran once
  arduboy.boot();
  arduboy.display();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.audio.begin();
  arduboy.clear();
  initEEPROM();
  
  // limits the frames per second
  arduboy.setFrameRate(60);
}

void loop()
{
  // Wait for the next frame
  if (!(arduboy.nextFrame())) return;
  
  arduboy.pollButtons();
  arduboy.clear();

  if (state == 0)  { vsboot(); }
  else if (state == 1)  { doSplash(); }
  else if (state == 2)  { gameplay(); }
  else if (state == 3)  { gameover(); }
  else if (state == 4)  { pause(); }
  
  arduboy.display();
}

void handleCollisions()
{
  Rect playerRect = { player.x + 6, player.y + 4, 5, 5 };
  Rect enemy1Rect = { enemy1x + 1, enemy1y + 20, 13, 13 };
  Rect enemy2Rect = { enemy2x + 1, enemy2y + 20, 13, 13 };
  Rect heartRect = { heartx + 1, hearty + 1, 15, 13 };

// collision with a heart  
  if(arduboy.collide(playerRect, heartRect))
  {
    score = score + 5;
    fuelcount = (fuelcount > 1) ? fuelcount - 1 : 0;
    ++heartcounter;
    sound.tone(NOTE_E3,80, NOTE_E4,80, NOTE_E5,80);
    hearty = -14;
    heartx = random(26,87);
    // check if you gain a shield
      if(heartcounter >= 15)
      {
        ++shield; oneup();
        fuelcount = (fuelcount > 10) ? fuelcount - 10 : 0;
        heartcounter = 0;
      }
  }

// collision with enemy 1
  if(arduboy.collide(playerRect, enemy1Rect))
  {
    --shield;
    fuelcount = fuelcount + 6;
    if ( fuelcount > 40 )
    {
      fuelcount = 40;
    }
  youarehit();
  enemy1y = -14;
  enemy1x = random(26,87);
  }
  
// collision with enemy 2  
  if(arduboy.collide(playerRect, enemy2Rect))
  {
    --shield;
    fuelcount = fuelcount + 6;
    if ( fuelcount > 40 )
    {
      fuelcount = 40;
    }
  youarehit();
  enemy2y = -28;
  enemy2x = random(26,87);
  }

// collision with fuel
  Rect fuelpowRect = { fuelpower.x + 3, fuelpower.y + 3, 12, 12 };  

  if(arduboy.collide(playerRect, fuelpowRect))
  {
    onCollideWithPowerup();
  }
  
// collisions with the laser  
  if(arduboy.justPressed(A_BUTTON))
  {
    sound.tone(NOTE_C5,50, NOTE_C3,100, NOTE_C2,200);
    arduboy.drawLine(player.x + 6, player.y - 1, player.x + 6, 10, WHITE);
    arduboy.drawLine(player.x + 8, player.y - 1, player.x + 8, 10, WHITE);
    arduboy.drawLine(player.x + 10, player.y - 1, player.x + 10, 10, WHITE);
    Rect laserRect = { player.x + 8, player.y - 64, 2, 64 };
      if(arduboy.collide(laserRect, enemy1Rect))
      {
        score = score + 10;
        fuelcount = (fuelcount > 1) ? fuelcount - 1 : 0;
        sound.tone(NOTE_C3,100, NOTE_C2,100, NOTE_C1,100);
        expl1.x = enemy1x;
        expl1.y = enemy1y + 12;
        hit = true;
        enemy1y = -14;
        enemy1x = random(26,87);
      }
          if(arduboy.collide(laserRect, enemy2Rect))
          {
            score = score + 10;
            fuelcount = (fuelcount > 1) ? fuelcount - 1 : 0;
            sound.tone(NOTE_C3,100, NOTE_C2,100, NOTE_C1,100);
            expl1.x = enemy2x + 2;
            expl1.y = enemy2y + 12;
            hit = true;
            enemy2y = -14;
            enemy2x = random(26,87);
          }
            if(arduboy.collide(laserRect, heartRect))
            {
              //check if score would be lower then 10, if not -10 pts
              score = (score > 10) ? score - 10 : 0;
              sound.tone(NOTE_C4,100, NOTE_C4,100, NOTE_C4,100);
              heartminus.x = heartx;
              heartminus.y = hearty;
              hearthit = true;
              hearty = -14;
              heartx = random(26,87);
            }
  }
}

void onCollideWithPowerup()
{
  fuelcount = (fuelcount > 16) ? fuelcount - 16 : 0;
  gotpowerup = true;
  
  resetPowerup();
  
  sound.tone(NOTE_E6,100, NOTE_E7,100, NOTE_E8,100);
  
}

void resetPowerup()
{
  // Power up spawn code goes here
  fuelpower.x = random(26, 87);
  fuelpower.y = -28;
  
  // New power up at random interval between 30 and 60 seconds
  powerUpCounter = random(20, 30);
}

void drawPowerup()
{
  Sprites::drawExternalMask(fuelpower.x, fuelpower.y, fuelpowerup, fuelpowerupmask, powerup1Frame, powerup1Frame);
}

void updatePowerup()
{
  if(powerUpCounter > 0)
  {
    if(arduboy.everyXFrames(FPS))
    {
      --powerUpCounter;
    }
  }
  else
  {
    if(fuelpower.y <= HEIGHT)
    {
      ++fuelpower.y;
    }
    else
    {
      resetPowerup();
    }
  }

  if(arduboy.everyXFrames(15)) // when running at 60fps
  {
    ++powerup1Frame;
    powerup1Frame %= 2;
  }
}

void vsboot()
{
  // Vsoft logo display
  arduboy.drawBitmap(0, 0, bootlogo, 128, 64, WHITE);
  if(fadeOut())
  {
    resetFade();
    resetFadeIn();
    state = 1;
  }
}

  // scrolling background1 function
void scrollingbackground()
{
  arduboy.drawBitmap(backdropx, backdropy, cloudbackdrop, 128, 64, WHITE);
  arduboy.drawBitmap(backdropx, backdropy - 64, cloudbackdrop, 128, 64, WHITE);
  sprite.drawExternalMask(cloudl.x, cloudl.y, cloudborderl, cloudborderlmask, 0, 0);
  sprite.drawExternalMask(cloudr.x, cloudr.y, cloudborderr, cloudborderrmask, 0, 0);
  sprite.drawExternalMask(cloudl.x, cloudl.y - 64, cloudborderl, cloudborderlmask, 0, 0);
  sprite.drawExternalMask(cloudr.x, cloudr.y - 64, cloudborderr, cloudborderrmask, 0, 0);
  sprite.drawExternalMask(cloudl.x, cloudl.y + 64, cloudborderl, cloudborderlmask, 0, 0);
  sprite.drawExternalMask(cloudr.x, cloudr.y + 64, cloudborderr, cloudborderrmask, 0, 0);
  
  // background scrolling loop
  ++backdropy;
  if( backdropy > 64 )
  {
    backdropy = 0;
  }
  cloudl.y = cloudl.y +2;
  cloudr.y = cloudr.y +2;
  if( cloudl.y > 64 )
  {
    cloudl.y = 0;
  }
  if( cloudr.y > 64 )
  {
    cloudr.y = 0;
  }
}

void scrollingbackground2()
{
  arduboy.drawBitmap(backdropx, backdropy, cloudbackdrop2, 128, 64, WHITE);
  arduboy.drawBitmap(backdropx, backdropy - 64, cloudbackdrop2, 128, 64, WHITE);
  sprite.drawExternalMask(cloudl.x, cloudl.y, cloudborderl, cloudborderlmask, 0, 0);
  sprite.drawExternalMask(cloudr.x, cloudr.y, cloudborderr, cloudborderrmask, 0, 0);
  sprite.drawExternalMask(cloudl.x, cloudl.y - 64, cloudborderl, cloudborderlmask, 0, 0);
  sprite.drawExternalMask(cloudr.x, cloudr.y - 64, cloudborderr, cloudborderrmask, 0, 0);
  sprite.drawExternalMask(cloudl.x, cloudl.y + 64, cloudborderl, cloudborderlmask, 0, 0);
  sprite.drawExternalMask(cloudr.x, cloudr.y + 64, cloudborderr, cloudborderrmask, 0, 0);
  
  // background scrolling loop
  ++backdropy;
  if( backdropy > 64 )
  {
    backdropy = 0;
  }
  cloudl.y = cloudl.y +2;
  cloudr.y = cloudr.y +2;
  if( cloudl.y > 64 )
  {
    cloudl.y = 0;
  }
  if( cloudr.y > 64 )
  {
    cloudr.y = 0;
  }
}

  // Pause state
void pause()
{
  scrollingbackground2();
  arduboy.fillRect(0, 24, 128, 20, BLACK);
  arduboy.drawLine(0, 25, 128, 25, WHITE);
  arduboy.drawLine(0, 42, 128, 42, WHITE);
  arduboy.drawLine(0, 45, 128, 45, BLACK);
  arduboy.drawLine(0, 47, 128, 47, BLACK);
  arduboy.setCursor(49, 31);
  arduboy.print(F("PAUSE"));
  if(arduboy.everyXFrames(15)) // when running at 60fps
  {
    ++heartFrame; // Add 1
    if(heartFrame > 4)
    {
      heartFrame = 0; // resets frame to 0 if greater then 4
    }
  }
  Sprites::drawExternalMask(28, 27, heart, heartmask, heartFrame, heartFrame);
  Sprites::drawExternalMask(82, 27, heart, heartmask, heartFrame, heartFrame);
  arduboy.setCursor(15, 37);
  // If 'B' button is pressed move back to gameplay
  if (arduboy.justPressed(B_BUTTON))
  {
    state = 2;
  }

}

void resetfornewgame()
{
  score = 0;
  speed = 1;
  shield = 3;
  fuelcount = 0;
  fueltimer = 0;
  enemy1y = -28;
  enemy2y = -28;
  fuelpower.y = -28;
  hearty = -28;
  heartx = random(26,87);
}

void doSplash()
{
  // Reset highScore value option
  if(!primed)
  {
    if (arduboy.justPressed(B_BUTTON))
    {
      primed = true;
    }
  }
  else
  {
    if (arduboy.justPressed(DOWN_BUTTON))
    {
      highScore = 0;
      EEPROM.put(EEPROM_SCORE, highScore);
      primed = false;
      sound.tone(NOTE_E5,50, NOTE_E6,50, NOTE_E7,50);
    }
  else if (arduboy.justPressed(B_BUTTON))
  {
    primed = false;
  }
  // Display a warning
    arduboy.setCursor(16, 1);
    arduboy.print(F("DOWN:DEL."));
    arduboy.setCursor(66, 1);
    arduboy.print(F("B:CANCEL"));
  }
  
  if(arduboy.everyXFrames(15)) // when running at 60fps
  {
    ++heartFrame; // Add 1
    if(heartFrame > 4)
    {
      heartFrame = 0;
    }
  }
  Sprites::drawExternalMask(23, 31, heart, heartmask, heartFrame, heartFrame);
  Sprites::drawExternalMask(89, 31, heart, heartmask, heartFrame, heartFrame);
  arduboy.drawBitmap(0, 0, splash, 128, 64, WHITE);
  fadeIn();

  // If 'A' button is pressed move to gameplay
  if (arduboy.justPressed(A_BUTTON))
  {
    arduboy.initRandomSeed();
    resetfornewgame();
    state = 2; 
    resetFadeIn();
  }

}

void lightsoff()
{
  arduboy.digitalWriteRGB(RED_LED, RGB_OFF); // turn off LEDS
  arduboy.digitalWriteRGB(GREEN_LED, RGB_OFF); // turn off LEDS
  arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF); // turn off LEDS
}

// Gameover state
void gameover()
{
  lightsoff();
  
  // Only need 5 for a uint16_t
  uint8_t digits[5];
  
  if (score > highScore)
  {
    highScore = score;
    EEPROM.put(EEPROM_SCORE, highScore);
  }
  scrollingbackground();
  arduboy.fillRect(0, 16, 128, 31, BLACK);
  arduboy.drawLine(0, 17, 128, 17, WHITE);
  arduboy.drawLine(0, 45, 128, 45, WHITE);
  arduboy.drawLine(0, 48, 128, 48, BLACK);
  arduboy.drawLine(0, 50, 128, 50, BLACK);
  
  arduboy.setCursor(37, 5);
  arduboy.print(F("GAME OVER"));
  
  arduboy.setCursor(30, 23);
  arduboy.print(F("SCORE:"));
  arduboy.setCursor(65, 23);
  extractDigits(digits, score);
  for(uint8_t i = 5; i > 0; --i)
  arduboy.print(digits[i - 1]);
  
  arduboy.setCursor(15, 33);
  arduboy.print(F("HIGH SCORE:"));
  arduboy.setCursor(81, 33);
  extractDigits(digits, highScore);
  for(uint8_t i = 5; i > 0; --i)
  arduboy.print(digits[i - 1]);
  
  sprite.drawExternalMask(46, 52, pressb, pressbmask, 0, 0);

  // If 'A' button is pressed move to splash
  if (arduboy.justPressed(B_BUTTON))
  {
    state = 1;
  }
}

void explosions()
{
  if(hit)
  {
    if(arduboy.everyXFrames(5)) // when running at 60fps
    {
      ++explosionFrame;
      if(explosionFrame > 3)
      {
        // resets frame to 0 and turn off explosion
        explosionFrame = 0;
        hit = false;
      }
    }
    Sprites::drawExternalMask(expl1.x, expl1.y, explosion, explosionmask, explosionFrame, explosionFrame);
    --expl1.y;
  }
}

void heartshot()
{
  if(hearthit)
  {
    if(arduboy.everyXFrames(5)) // when running at 60fps
    {
      ++heartminusFrame;
      if(heartminusFrame > 3)
      {
        // resets frame to 0 and turn off explosion
        heartminusFrame = 0;
        hearthit = false;
      }
    }
    Sprites::drawExternalMask(heartminus.x, heartminus.y, hearthurt, hearthurtmask, heartminusFrame, heartminusFrame);
    --heartminus.y;
  }
}

void youarehit()
{
  sound.tone(NOTE_C4,100, NOTE_C3,100, NOTE_C2,100);
  arduboy.invert(true);
  arduboy.digitalWriteRGB(RED_LED, RGB_ON);
  flashtimer = arduboy.frameCount + 2; // speed to the screen flash when hit
  ledTimer = arduboy.frameCount + 30;
  // check is game is over
  if(shield < 0)
  {
    speed = 1;
    state = 3;
    arduboy.invert(false); // display the screen no inverted
  }
}

void oneup()
{
  arduboy.digitalWriteRGB(GREEN_LED, RGB_ON);
  sound.tone(NOTE_E6,100, NOTE_E7,100, NOTE_E8,100);
  ledTimerg = arduboy.frameCount + 30;
}

void speedupdisplay()
{
arduboy.digitalWriteRGB(BLUE_LED, RGB_ON);
  ledTimerb = arduboy.frameCount + 30;
  sound.tone(NOTE_G4,100, NOTE_E5,100, NOTE_C6,100);
  sound.tone(NOTE_C5,100, NOTE_E4,100, NOTE_G3,100);
}

void timersupdate()
{
  if(ledTimer > 0 && arduboy.frameCount >= ledTimer)
  {
    arduboy.digitalWriteRGB(RED_LED, RGB_OFF);
    ledTimer = 0;
  }
  if(ledTimerg > 0 && arduboy.frameCount >= ledTimerg)
  {
    arduboy.digitalWriteRGB(GREEN_LED, RGB_OFF);
    ledTimerg = 0;
  }

  if(ledTimerb > 0 && arduboy.frameCount >= ledTimerb)
  {
    arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF);
    ledTimerb = 0;
  }
  if(flashtimer > 0 && arduboy.frameCount >= flashtimer)
  {
    arduboy.invert(false);
      flashtimer = 0;
  }
}

void animationframes()
{
  if(arduboy.everyXFrames(2)) // when running at 60fps
  {
    ++shipFrame; // Add 1
    shipFrame %= 2; // Remainder of dividing by 2
  }

  if(arduboy.everyXFrames(5)) // when running at 60fps
  {
    ++enemyFrame; // Add 1
    enemyFrame %= 2; // Remainder of dividing by 2
  }
  
  if(arduboy.everyXFrames(7)) // when running at 60fps
  {
    ++heartFrame; // Add 1
    if(heartFrame > 3) { heartFrame = 0; } // resets frame to 0 if greater then 3
  }
  
  if(arduboy.everyXFrames(30)) // when running at 60fps
  {
    ++fuelFrame; // Add 1
    if(fuelFrame > 1) { fuelFrame = 0; } // resets every .5 seconds
  }
}

void heartreset()
{
  // resetting position for next heart to fall
  hearty = hearty + speed + 1;
  if( hearty > 64 )
  {
    hearty = -28;
    heartx = random(26,87);
  }
}

void enemyreset()
{
  // resetting position for next enemy to appear
  enemy1y = enemy1y + speed;
  if( enemy1y > 64 )
  {
    enemy1y = -28;
    enemy1x = random(26,87);
  }
  
  enemy2y = enemy2y + speed;
  if( enemy1y > 64 )
  {
    enemy2y = -28;
    enemy2x = random(26,87);
  }
}

void fueltime()
{
  // Fuel timer count
  if(arduboy.everyXFrames(60)) // when running at 60fps
  {
    fuelcount = fuelcount + 2; // Add 2
  }
}

void drawhearts()
{
  // falling heart display
  Sprites::drawExternalMask(heartx, hearty, heart, heartmask, heartFrame, heartFrame);
}

void drawenemies()
{
  // enemies appearing
  Sprites::drawExternalMask(enemy1x, enemy1y, enemy1, enemy1mask, enemyFrame, enemyFrame);
  Sprites::drawExternalMask(enemy2x, enemy2y, enemy1, enemy1mask, enemyFrame, enemyFrame);
}

void drawplayer()
{
  // here i display the main sprite
  Sprites::drawExternalMask(player.x, player.y, player1, player1mask, shipFrame, shipFrame);
}

void drawscoreboard()
{
    // Only need 5 for a uint16_t
  uint8_t digits[5];
  arduboy.fillRect(0, 0, 128, 10, BLACK);
  arduboy.setCursor(1, 0);
  arduboy.print(F("SCORE:")); arduboy.setCursor(37, 0);
  extractDigits(digits, score);
  for(uint8_t i = 5; i > 0; --i)
  arduboy.print(digits[i - 1]);
  arduboy.setCursor(78, 0);
  arduboy.print(F("SHIELD:"));
  arduboy.setCursor(120, 0);
  arduboy.print(shield);
  arduboy.drawLine(0, 9, 128, 9, WHITE);
  arduboy.drawLine(0, 10, 128, 10, BLACK);
}

void drawfuelgauge()
{
  // display the fuel gauge
  Sprites::drawExternalMask(119, 15, fuelgauge, fuelgaugemask, 0, 0);
  arduboy.fillRect(121, 17, 3, fuelcount, BLACK);
}

void fuellow()
{
  // make sure fuel don't overflow
  if(fuelcount < 1)
  {
    fuelcount = 0;
  }
    // Fuel Low Warning if needed
    if(fuelcount > 30)
    {
      if(fuelFrame < 1)
      {
        Sprites::drawExternalMask(player.x - 4, player.y + 6, lowfuel, lowfuelmask, 0, 0);
      }
    }
}

void outoffuel()
{
  // GameOver if out of fuel
  if(fuelcount > 40)
  {
    state = 3;
  }
}


void speedincrease(uint16_t oldScore)
{
  // check if speed increase triggered
  if(score >= 1500 && oldScore < 1500)
  {
    speed = 2;
    speedupdisplay();
    }
  else if(score >= 1000 && oldScore < 1000)
  {
    speed = 1;
    speedupdisplay();
  }
  else if(score >= 500 && oldScore < 500)
  {
    speed = 2;
    speedupdisplay();
  }
}

void buttonpressed()
{
  // what is happening when we press buttons
  if(arduboy.pressed(LEFT_BUTTON) && player.x > 18)
  {
    --player.x;
  }
  if(arduboy.pressed(RIGHT_BUTTON) && player.x < 92)
  {
    ++player.x;
  }
  if(arduboy.pressed(UP_BUTTON) && player.y > 10)
  {
    --player.y; 
  }
  if(arduboy.pressed(DOWN_BUTTON) && player.y < 49)
  {
    ++player.y;
  }
    
  if(arduboy.justPressed(B_BUTTON))
  {
    sound.tone(NOTE_C4,70, NOTE_D5,50, NOTE_E6,70); state = 4;
  }
}

void gameplay()
{
  uint16_t oldScore = score;
  enemyreset();
  heartreset();
  fueltime();
  animationframes();
  timersupdate();
  handleCollisions();
  scrollingbackground();
  explosions();
  heartshot();
  drawPowerup();
  updatePowerup();
  drawhearts();
  drawenemies();
  drawplayer();
  drawscoreboard();
  drawfuelgauge();
  fuellow();
  outoffuel();
  buttonpressed();
  speedincrease(oldScore);  
}
