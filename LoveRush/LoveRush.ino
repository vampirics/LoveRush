#include <Arduboy2.h>
#include <ArduboyTones.h>

#include "Images.h"
#include "List.h"

#define EEPROM_START_C1                 (EEPROM_STORAGE_SPACE_START + 128)
#define EEPROM_START_C2                 (EEPROM_START_C1 + 1)
#define EEPROM_SCORE                    (EEPROM_START_C1 + 2)

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
Sprites sprite;

// setting up variables
enum class ObjectType
{
	None,
	Player,
	Heart,
	DamagedHeart,
	Enemy,
	Explosion,
	Fuel,
	LoveBomb,
	LaserBullet,
};

class Object
{
public:
  int16_t x = 0;
  int16_t y = 0;
  ObjectType type = ObjectType::None;
  uint8_t frame = 0;

public:
  // Make the default constructor set everything to zero
  Object(void) = default;

  Object(int16_t x, int16_t y, ObjectType type)
	: x(x), y(y), type(type)
  {
  }
  
  Object(int16_t x, int16_t y, ObjectType type, uint8_t frame)
	: x(x), y(y), type(type), frame(frame)
  {
  }
};

bool operator ==(const Object & left, const Object & right)
{
	return (left.type == right.type) && (left.x == right.x) && (left.y == right.y);
}

List<Object, 16> objects;

Object player;
List<Object, 4> laserBullets;
bool loveTrigger = false;

struct Position {
  int8_t x;
  int8_t y;
};

Position cloudl = { 0, 0 };
Position cloudr = { 103, 0 };

const uint8_t FPS = 60;
uint8_t powerUpCounter = 0;
uint8_t powerUpCounter2 = 0;
uint8_t selection = 0;

uint8_t state = 0;

// variable for Cloud backdrop
int8_t backdropx = -3;
int8_t backdropy = 0;

uint8_t heartFrame = 0;
uint8_t fuelFrame = 0;
uint16_t loveFrame = 0;
uint8_t LowfuelFrame = 0;
uint8_t delayFrame = 0;

int8_t shield = 3;
bool primed = false;

int8_t speed = 1;
uint16_t fuelcount = 0;

uint16_t ledTimer = 0;
uint16_t ledTimerg = 0;
uint16_t ledTimerb = 0;
uint16_t flashtimer = 0;

uint8_t introDelay = 0;

int8_t heartcounter = 0;
uint8_t blinkCounter = 0;

uint8_t ee1 = 0;

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

  if (state == 0)  { vsBoot(); }
  else if (state == 1)  { pharapBoot(); }
  else if (state == 2)  { updateSplashState(); }
  else if (state == 3)  { gameplay(); }
  else if (state == 4)  { updateGameOverState(); }
  else if (state == 5)  { updatePauseState(); }
  else if (state == 6)  { updateHighscoreScreen(); }
  else if (state == 7)  { ee1Ascreen(); }
  
  arduboy.display();
}

void updateHighscoreScreen()
{
  // Only need 5 for a uint16_t
  uint8_t digits[5];
  
  drawBackground2();
  arduboy.fillRect(0, 16, 128, 31, BLACK);
  arduboy.drawLine(0, 17, 128, 17, WHITE);
  arduboy.drawLine(0, 45, 128, 45, WHITE);
  arduboy.drawLine(0, 48, 128, 48, BLACK);
  arduboy.drawLine(0, 50, 128, 50, BLACK);

    // Reset highScore value option
  if(!primed)
  {
    if (arduboy.justPressed(A_BUTTON))
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
    else if (arduboy.justPressed(A_BUTTON))
    {
      primed = false;
    }
    // Display a warning
    arduboy.setCursor(0, 53);
    arduboy.print(F("  DOWN:DEL."));
    arduboy.setCursor(66, 53);
    arduboy.print(F(" A:CANCEL   "));
  }
  
  arduboy.setCursor(15, 28);
  arduboy.print(F("HIGH SCORE:"));
  arduboy.setCursor(81, 28);
  extractDigits(digits, highScore);
  for(uint8_t i = 5; i > 0; --i)
  arduboy.print(digits[i - 1]);

  if (arduboy.justPressed(UP_BUTTON) && arduboy.justPressed(DOWN_BUTTON))
  {
    sound.tone(NOTE_C4,50, NOTE_B3,100, NOTE_C5,50);
    state = 7;
  }
  
  // If 'A' button is pressed move to splash
  if (arduboy.justPressed(B_BUTTON))
  {
    state = 2;
  }
}

void ee1Ascreen()
{
    objects.add(Object(random(26, 87), random(-64, -14), ObjectType::Heart));
    transformObjects(ObjectType::Enemy, ObjectType::Heart, 2);
    updateObjects();
    drawBackground2();
    drawObjects();
    
    arduboy.fillRect(0, 23, 128, 15, BLACK);
    arduboy.drawLine(0, 24, 128, 24, WHITE);
    arduboy.drawLine(0, 36, 128, 36, WHITE);
    arduboy.drawLine(0, 39, 128, 39, BLACK);
    arduboy.drawLine(0, 41, 128, 41, BLACK);
    arduboy.setCursor(-2, 27);
    arduboy.print(F(" J&S FOREVER 09.03.16 "));

    if (arduboy.pressed(A_BUTTON))
    {
      arduboy.fillRect(0, 23, 128, 15, BLACK);
      arduboy.drawLine(0, 24, 128, 24, WHITE);
      arduboy.drawLine(0, 36, 128, 36, WHITE);
      arduboy.drawLine(0, 39, 128, 39, BLACK);
      arduboy.drawLine(0, 41, 128, 41, BLACK);
      arduboy.setCursor(8, 27);
      arduboy.print(F("I LOVE YOU JESSICA!"));
    }
    if(arduboy.everyXFrames(15))
    {
      arduboy.digitalWriteRGB(RED_LED, RGB_ON);
      arduboy.digitalWriteRGB(BLUE_LED, RGB_ON);    
    }
    else
    {
      arduboy.digitalWriteRGB(RED_LED, RGB_OFF);
      arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF);
    }   

    if (arduboy.justPressed(B_BUTTON))
    {
      arduboy.digitalWriteRGB(RED_LED, RGB_OFF);
      arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF);
      state = 6;
    }
}


void gameplay()
{
	uint16_t oldScore = score;
  
	handleInput();
  
	updateTimers();
    
	updatePowerup(); 
  updatePowerup2(); 
  
	handleCollisions();
	updateObjects();
	updatePlayer();
	updateLaserBullets();
  updateFuelGauge();	
  updateLoveFrame();
  updateLowfuelFrame();

  loveRushFlash();
	
	drawBackground();
	drawFuelGauge();
	drawObjects();
	drawPlayer();
	drawLaserBullets();
  drawLowFuel();
  drawScoreboard();
  
	updateSpeed(oldScore);
}

void resetGame()
{
  score = 0;
  speed = 1;
  shield = 3;
  fuelcount = 0;
  heartcounter = 0;
  powerUpCounter = random(10, 20);
  powerUpCounter2 = random(10, 20);
  
  player = Object(54, 50, ObjectType::Player);
  
  objects.clear();
  objects.add(Object(random(26, 87), random(-64, -14), ObjectType::Heart));
  //objects.add(Object(random(26, 87), -28, ObjectType::Heart));
  objects.add(Object(random(26, 87), -28, ObjectType::Enemy));
  objects.add(Object(random(26, 87), -28, ObjectType::Enemy));
  objects.add(Object(random(26, 87), -28, ObjectType::Fuel));
  objects.add(Object(random(26, 87), -28, ObjectType::LoveBomb));
}

void handleInput()
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
	
	if(arduboy.justPressed(A_BUTTON))
	{
		if(laserBullets.add(Object(player.x + 7, player.y - 2, ObjectType::LaserBullet)))
		{
			sound.tone(NOTE_C5, 50, NOTE_C3, 100, NOTE_C2, 200);
		}
	}
	
	if(arduboy.justPressed(B_BUTTON))
	{
		sound.tone(NOTE_C4, 70, NOTE_D5, 50, NOTE_E6, 70);
		state = 5;
	}
}

void updatePlayer()
{	
	if(arduboy.everyXFrames(2)) // when running at 60fps
	{
		++player.frame; // Add 1
		player.frame %= 2; // Remainder of dividing by 2
	}
}

void updateLaserBullet(Object & laserBullet)
{
	if(laserBullet.y > 12)
	{
		laserBullet.y -= 6;
	}
	else
	{
		laserBullets.remove(laserBullet);
	}
  
  // I don't know if there's an animation or not
  
  // if(arduboy.everyXFrames(30))
  // {
    // ++laserBullet.frame;
    // laserBullet.frame %= 2;
  // }
}

void updateLaserBullets()
{	
	for(uint8_t i = 0; i < laserBullets.getCount(); ++i)
	{
		if(laserBullets[i].y > 12)
		{
			laserBullets[i].y -= 6;
		}
		else
		{
			laserBullets.removeAt(i);
		}		
	}
}

void drawLaserBullets(void)
{
	for(uint8_t i = 0; i < laserBullets.getCount(); ++i)
		drawObject(laserBullets[i]);
}

void drawPlayer()
{
	Sprites::drawExternalMask(player.x, player.y, player1, player1mask, player.frame, player.frame);
}

void updateSpeed(uint16_t oldScore)
{
	// check if speed increase triggered
	if(score >= 1500 && oldScore < 1500)
  {
    speed = 2;
    handleSpeedUp();
  }
	else if(score >= 1150 && oldScore < 1150)
  {
    speed = 1;
    handleSpeedUp();
  }
	else if(score >= 900 && oldScore < 900)
	{
		speed = 2;
		handleSpeedUp();
	}
	else if(score >= 650 && oldScore < 650)
	{
		speed = 1;
		handleSpeedUp();
	}
	else if(score >= 500 && oldScore < 500)
	{
		speed = 2;
		handleSpeedUp();
	}
}

void updateTimers()
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

void updatePowerup()
{
  if(powerUpCounter > 0)
  {
    if(arduboy.everyXFrames(FPS))
    {
      --powerUpCounter;
    }
  }
}

void updatePowerup2()
{
  if(powerUpCounter2 > 0)
  {
    if(arduboy.everyXFrames(FPS))
    {
      --powerUpCounter2;
    }
  }
}

void updateFuelGauge()
{	
	// Fuel timer count
	if(arduboy.everyXFrames(60)) // when running at 60fps
	{
		fuelcount = fuelcount + 2; // Add 2
	}
	
	// make sure fuel don't overflow
	if(fuelcount < 1)
	{
		fuelcount = 0;
	}
	// GameOver if out of fuel
	if(fuelcount > 40)
	{
		state = 4;
	}
}

void updateLowfuelFrame()
{
    if(arduboy.everyXFrames(30)) // when running at 60fps
  {
    ++LowfuelFrame; // Add 1
    if(LowfuelFrame > 1) { LowfuelFrame = 0; } // resets every .5 seconds
  }
}

void drawLowFuel()
{
  // Fuel Low Warning if needed
  if(fuelcount > 30)
    {
      if(LowfuelFrame < 1)
      {
        Sprites::drawExternalMask(player.x - 4, player.y + 6, lowfuel, lowfuelmask, 0, 0);
      }
    }
}

bool loveRushLed = false;

void loveRushFlash()
{
  if (loveTrigger)
  {
    if(arduboy.everyXFrames(3))
    {
      Sprites::drawExternalMask(49, 25, loverush, loverushmask, 0, 0);
    }
	
	if(arduboy.everyXFrames(6))
	{
	  loveRushLed = !loveRushLed;
	  if(loveRushLed)
	  {
	    arduboy.digitalWriteRGB(RED_LED, RGB_ON);
	    arduboy.digitalWriteRGB(BLUE_LED, RGB_ON);	  
	  }
	  else
	  {
	    arduboy.digitalWriteRGB(RED_LED, RGB_OFF);
	    arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF);
      }	  
	}
  }
}

void drawFuelGauge()
{
	// display the fuel gauge
	Sprites::drawExternalMask(119, 15, fuelgauge, fuelgaugemask, 0, 0);
	arduboy.fillRect(121, 17, 3, fuelcount, BLACK);
}

void handlePlayerHeartCollision(Object & heart)
{
	score = score + 5;
	fuelcount = (fuelcount > 2) ? fuelcount - 2 : 0;
	++heartcounter;
	// check if you gain a shield
	if(heartcounter >= 15)
	{
		++shield;
		handleOneUp();
		//fuelcount = (fuelcount > 10) ? fuelcount - 10 : 0;
		heartcounter = 0;
	}
	
	heart.y = random(-64, -14);
	heart.x = random(26, 87);
	
	sound.tone(NOTE_E3, 80, NOTE_E4, 80, NOTE_E5, 80);
}

void handlePlayerEnemyCollision(Object & enemy)
{
    --shield;
    fuelcount = fuelcount + 4;
    if ( fuelcount > 40 )
    {
      fuelcount = 40;
    }
	handlePlayerHit();
	
	enemy.y = -28;
	enemy.x = random(26, 87);
}

void handlePlayerFuelCollision(Object & fuel)
{
	fuelcount = (fuelcount > 20) ? fuelcount - 20 : 0;

	resetFuel(fuel);

	sound.tone(NOTE_E6, 100, NOTE_E7, 100, NOTE_E8, 100);
}

void handlePlayerLoveBombCollision(Object & loveBomb)
{
  
  resetLoveBomb(loveBomb);
  loveTrigger = true;
  sound.tone(NOTE_E6, 100, NOTE_E7, 100, NOTE_E8, 100);
}

void handlePlayerCollision(Object & object)
{
	switch(object.type)
	{
		case ObjectType::Heart: handlePlayerHeartCollision(object); break;
		case ObjectType::Enemy: handlePlayerEnemyCollision(object); break;
		case ObjectType::Fuel: handlePlayerFuelCollision(object); break;
    case ObjectType::LoveBomb: handlePlayerLoveBombCollision(object); break;
		default: break;
	}
}

void handleLaserHeartCollision(Object & heart)
{
	//check if score would be lower then 10, if not -10 pts
	score = (score > 10) ? score - 10 : 0;
	heart.type = ObjectType::DamagedHeart;
	sound.tone(NOTE_C4, 100, NOTE_C4, 100, NOTE_C4, 100);
}

void handleLaserEnemyCollision(Object & enemy)
{
	score = score + 10;
	//fuelcount = (fuelcount > 1) ? fuelcount - 1 : 0;
	enemy.type = ObjectType::Explosion;
	objects.add(Object(random(26, 87), -28, ObjectType::Enemy));
	sound.tone(NOTE_C3,100, NOTE_C2,100, NOTE_C1,100);
}

void handleLaserCollision(Object & object)
{
	switch(object.type)
	{
		case ObjectType::Heart: handleLaserHeartCollision(object); break;
		case ObjectType::Enemy: handleLaserEnemyCollision(object); break;
		default: break;
	}
}

bool checkPlayerCollision(const Object & object)
{
	Rect playerRect = { player.x + 6, player.y + 4, 5, 5 };
	Rect objectRect;
	switch(object.type)
	{
		case ObjectType::Heart:
			objectRect = { object.x + 1, object.y + 1, 15, 13 };
			break;
		case ObjectType::Enemy:
			objectRect = { object.x + 1, object.y + 20, 13, 13 };
			break;
		case ObjectType::Fuel:
			objectRect = { object.x + 3, object.y + 3, 12, 12 };
			break;
     case ObjectType::LoveBomb:
      objectRect = { object.x + 3, object.y + 3, 12, 12 };
      break;
		default:
			return false;
	}
	return arduboy.collide(playerRect, objectRect);
}

bool checkLaserCollision(const Object & object, const Object & laserBullet)
{
	Rect laserRect = { laserBullet.x, laserBullet.y, 3, 6 };
	Rect objectRect;
	switch(object.type)
	{
		case ObjectType::Heart:
			objectRect = { object.x + 1, object.y + 1, 15, 13 };
			break;
		case ObjectType::Enemy:
			objectRect = { object.x + 1, object.y + 20, 13, 13 };
			break;
		case ObjectType::Fuel:
			objectRect = { object.x + 3, object.y + 3, 12, 12 };
			break;
		default:
			return false;
	}
	return arduboy.collide(laserRect, objectRect);
}

void handleCollisions()
{
	for(uint8_t i = 0; i < objects.getCount(); ++i)
	{			
		if(checkPlayerCollision(objects[i]))
		{
			handlePlayerCollision(objects[i]);
		}
		
		// collisions with the laser  
		for(uint8_t j = 0; j < laserBullets.getCount(); ++j)
		{
			if(checkLaserCollision(objects[i], laserBullets[j]))
			{
				handleLaserCollision(objects[i]);
				laserBullets.removeAt(j);
				break;
			}
		}
	}
}

void updateFuelCounter(void)
{
	if(powerUpCounter > 0)
	{
		if(arduboy.everyXFrames(FPS))
		{
			--powerUpCounter;
		}
	}
}

void updateExplosion(Object & explosion)
{
	++explosion.y;
	if(explosion.y > 64)
	{
		// 'destroy' the explosion by removing it from the object list
		objects.remove(explosion);
	}
	else if(arduboy.everyXFrames(5)) // when running at 60fps
	{
		++explosion.frame;
		if(explosion.frame > 3)
		{
			explosion.frame = 0;
		}
	}
}

void updateHeart(Object & heart)
{
	// resetting position for next heart to fall
	heart.y = heart.y + speed + 1;
	if( heart.y > 64 )
	{
		heart.y = random(-64, -14);
		heart.x = random(26, 87);
	}
	
	if(arduboy.everyXFrames(7)) // when running at 60fps
	{
		++heart.frame; // Add 1
		heart.frame %= 2; // Remainder of dividing by 2
	}
}

void updateDamagedHeart(Object & heart)
{
	--heart.y;
	if( heart.y < -28 )
	{
		heart.y = random(-64, -14);
		heart.x = random(26, 87);
		heart.type = ObjectType::Heart;
		heart.frame = 0;
	}
	else if(arduboy.everyXFrames(5)) // when running at 60fps
	{
		++heart.frame;
		heart.frame %= 3;
	}
}

void updateEnemy(Object & enemy)
{
	// resetting position for next enemy to appear
	enemy.y = enemy.y + speed;
	if( enemy.y > 64 )
	{
		enemy.y = -28;
		enemy.x = random(26, 87);
	}
	
	if(arduboy.everyXFrames(5)) // when running at 60fps
	{
		++enemy.frame; // Add 1
		enemy.frame %= 2; // Remainder of dividing by 2
	}
}

void updateFuel(Object & fuel)
{
	if(powerUpCounter == 0)
	{
		if(fuel.y <= HEIGHT)
		{
			++fuel.y;
		}
		else
		{
			resetFuel(fuel);
		}
	}
	
	if(arduboy.everyXFrames(10)) // when running at 60fps
	{
		++fuel.frame;
		fuel.frame %= 2;
	}
}

void updateLoveBomb(Object & LoveBomb)
{
  if(powerUpCounter2 == 0)
  {
    if(LoveBomb.y <= HEIGHT)
    {
      ++LoveBomb.y;
    }
    else
    {
      resetLoveBomb(LoveBomb);
    }
  }
  
  if(arduboy.everyXFrames(10)) // when running at 60fps
  {
    ++LoveBomb.frame;
    LoveBomb.frame %= 2;
  }
}

void updateLoveFrame()
{
  if(loveTrigger)
  {
    transformObjects(ObjectType::Enemy, ObjectType::Heart, 2);
    ++loveFrame;
  }
    if (loveFrame >= 480)
    {
      transformObjects(ObjectType::Heart, ObjectType::Enemy, 2);
      loveTrigger = false;
      loveFrame = 0;
	    arduboy.digitalWriteRGB(RED_LED, RGB_OFF);
	    arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF);
    }
}

void updateObject(Object & object)
{
	switch(object.type)
	{
		case ObjectType::Heart: updateHeart(object); break;
		case ObjectType::DamagedHeart: updateDamagedHeart(object); break;
		case ObjectType::Enemy: updateEnemy(object); break;
		case ObjectType::Explosion: updateExplosion(object); break;
		case ObjectType::Fuel: updateFuel(object); break;
		case ObjectType::LoveBomb: updateLoveBomb(object); break;
		case ObjectType::LaserBullet: updateLaserBullet(object); break;
		default: break;
	}
}

void updateObjects(void)
{
  for(uint8_t i = 0; i < objects.getCount(); ++i)
    updateObject(objects[i]);
}

void transformObjects(ObjectType fromType, ObjectType toType, uint8_t limit)
{
  uint8_t counter = 0;
  for(uint8_t i = 0; i < objects.getCount(); ++i)
  {
    if(objects[i].type == fromType)
    {
      objects[i].type = toType;
      ++counter;
      
      if(counter >= limit)
      {
        break;
      }
    }
  }
}

void drawObject(const Object & object)
{
	const unsigned char * image = nullptr;
	const unsigned char * mask = nullptr;
	switch(object.type)
	{
		case ObjectType::Heart:
			image = heart;
			mask = heartmask;
			break;
		case ObjectType::DamagedHeart:
			image = hearthurt;
			mask = hearthurtmask;
			break;
		case ObjectType::Enemy:
			image = enemy1;
			mask = enemy1mask;
			break;
		case ObjectType::Explosion:
			image = explosion;
			mask = explosionmask;
			break;
		case ObjectType::Fuel:
			image = fuelpowerup;
			mask = fuelpowerupmask;
			break;
		case ObjectType::LoveBomb:
			image = lovepowerup;
			mask = lovepowerupmask;
			break;
		case ObjectType::LaserBullet:
			image = laserbullet;
			mask = laserbulletmask;
			break;
		default: break;
	}
	
	// Using variables and having only one draw call saves around 100 bytes
	if(image != nullptr && mask != nullptr)
	{
		Sprites::drawExternalMask(object.x, object.y, image, mask, object.frame, object.frame);
	}
}

void drawObjects(void)
{
  for(uint8_t i = 0; i < objects.getCount(); ++i)
    drawObject(objects[i]);
}

void resetFuel(Object & fuel)
{  
  // New power up at random interval between 30 and 60 seconds
  powerUpCounter = random(10, 20);
  
  fuel.x = random(26, 87);
  fuel.y = -28;
}

void resetLoveBomb(Object & LoveBomb)
{  
  // New power up at random interval between 30 and 60 seconds
  powerUpCounter2 = random(30, 40);
  
  LoveBomb.x = random(26, 87);
  LoveBomb.y = -28;
}

void vsBoot()
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


void pharapBoot()
{
  // Pharap image display
    sprite.drawExternalMask(52, 8, Pharap, Pharapmask, 0, 0);
    arduboy.setCursor(37, 49);
    arduboy.print(F("OPTIMISED"));
    if(arduboy.everyXFrames(120)) // when running at 60fps
    {
      sprite.drawExternalMask(52, 8, Pharap, Pharapmask, 1, 0);
      ++blinkCounter;
      if (blinkCounter >= 3)
      {
        resetFade();
        resetFadeIn();
        state = 2;
        blinkCounter = 0;
      }
      
    }
      if (arduboy.justPressed(A_BUTTON))
      {
        state = 2;
      }
      fadeIn();
}

  // scrolling background1 function
void drawBackground()
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
	if( cloudl.y > 64 )
	{
		cloudl.y = 0;
	}
	
	cloudr.y = cloudr.y +2;
	if( cloudr.y > 64 )
	{
		cloudr.y = 0;
	}
}

void drawBackground2()
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
	if( cloudl.y > 64 )
	{
		cloudl.y = 0;
	}

	cloudr.y = cloudr.y +2;
	if( cloudr.y > 64 )
	{
		cloudr.y = 0;
	}
}

  // Pause state
void updatePauseState()
{
	drawBackground2();
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
		state = 3;
	}
}

void updateSplashState()
{
	if(arduboy.everyXFrames(15)) // when running at 60fps
	{
		++heartFrame; // Add 1
		if(heartFrame > 4)
		{
			heartFrame = 0;
		}
	}
	
	Sprites::drawExternalMask(23, 27, heart, heartmask, heartFrame, heartFrame);
	Sprites::drawExternalMask(89, 27, heart, heartmask, heartFrame, heartFrame);
	arduboy.drawBitmap(0, 0, splash, 128, 64, WHITE);
  if (selection < 1 )
  {
    arduboy.drawBitmap(39, 47, startselected, 50, 14, WHITE);
  }
  else
  {
    arduboy.drawBitmap(39, 47, highscoreselected, 50, 14, WHITE);
  }
 
	fadeIn();

	// If 'A' button is pressed move to gameplay
	if (arduboy.justPressed(A_BUTTON))
	{
		arduboy.initRandomSeed();
		resetGame();
    sound.tone(NOTE_C5,50, NOTE_D4,50, NOTE_E3,50);
		if (selection < 1)
		{
		  state = 3; 
		}
   else
   {
    state = 6;
   }
		resetFadeIn();
	}
 if (arduboy.justPressed(UP_BUTTON)&& selection > 0)
 {
  sound.tone(NOTE_E5,50, NOTE_E2,50, NOTE_E3,50);
  --selection;
 }
  if (arduboy.justPressed(DOWN_BUTTON)&& selection < 1)
 {
  sound.tone(NOTE_E5,50, NOTE_E2,50, NOTE_E3,50);
  ++selection;
 }

}

void turnLightsOff()
{
  arduboy.digitalWriteRGB(RED_LED, RGB_OFF); // turn off LEDS
  arduboy.digitalWriteRGB(GREEN_LED, RGB_OFF); // turn off LEDS
  arduboy.digitalWriteRGB(BLUE_LED, RGB_OFF); // turn off LEDS
}


// Gameover state
void updateGameOverState()
{
	turnLightsOff();

	// Only need 5 for a uint16_t
	uint8_t digits[5];

	if (score > highScore)
	{
		highScore = score;
		EEPROM.put(EEPROM_SCORE, highScore);
	}
	drawBackground();
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

	// If 'B' button is pressed move to splash
	if (arduboy.justPressed(B_BUTTON))
	{
		state = 2;
	}
}

void handlePlayerHit()
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
		state = 4;
		arduboy.invert(false); // display the screen no inverted
	}
}

void handleOneUp()
{
	ledTimerg = arduboy.frameCount + 30;
	arduboy.digitalWriteRGB(GREEN_LED, RGB_ON);
	sound.tone(NOTE_E6, 100, NOTE_E7, 100, NOTE_E8, 100);
}

void handleSpeedUp()
{
	ledTimerb = arduboy.frameCount + 30;
	arduboy.digitalWriteRGB(BLUE_LED, RGB_ON);
	sound.tone(NOTE_G4, 100, NOTE_E5, 100, NOTE_C6, 100);
	sound.tone(NOTE_C5, 100, NOTE_E4, 100, NOTE_G3, 100);
}

void drawScoreboard()
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
