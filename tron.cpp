#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS 6
#define TFT_DC 7
#define TFT_RST 8
#define LCD_WIDTH 160
#define LCD_HEIGHT 128
#define JOYSTICK_BUTTON_PIN 9
#define JOYSTICK_MOVE_X_PIN 0 //pins x,y are reversed because of screen orientation
#define JOYSTICK_MOVE_Y_PIN 1
typedef struct {
  uint8_t x;
  uint8_t y;
} position_t;

typedef struct {
  int8_t x;
  int8_t y;
} movement_t;

typedef struct {
  position_t currentPosition;
  movement_t direction;
} player_t;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

int joystickXCentre;
int joystickYCentre;
uint8_t wallPositions[128][20] = { 0 };
bool debug = false;
bool gameStarted = false;
player_t player1;
player_t player2;

/* Takes no input, returns position_t of the amount of *increase* required to add to x,y
 *  eg. movement_t { x = -2; y = 0 } would mean decrease x by 1. It will always return one value zero
 *  the other always -2 or 2 unless the joystick is at identical x,y
 * */
movement_t getJoystickInput();

/* Takes the current movement_t taken from the joystick and the old movement_t that maintains speed.
 * valid inputs have: a direction and cannot reverse directions on the spot. they must turn
 */
bool validInput(movement_t in, movement_t old);
/*
 * TODO DOCS
 */
bool legalPosition(position_t pos);
/*
 * TODO DOCS
 */
bool intersects(position_t dotPlace);
/*
 * TODO DOCS
 */
void addWallPosition(position_t pos);

bool getWallPosition(uint8_t x, uint8_t y);

void printWalls();
/*
 * Takes a string and outputs it to both the serial monitor and LCD display
 */
void dualPrint(char *s);

void drawGUI();

void drawPointer();

bool waitUntil(int pin, bool pos);

bool startNetwork();

void setSpawns(player_t *p1, player_t *p2);

void getSpawns(player_t *p1, player_t *p2); 

uint8_t getUint();

int8_t getInt();
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  tft.initR(INITR_REDTAB);
  randomSeed(analogRead(4));
  joystickXCentre = analogRead(JOYSTICK_MOVE_X_PIN) - 512;
  joystickYCentre = analogRead(JOYSTICK_MOVE_Y_PIN) - 512;
  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
  tft.setRotation(1); //because our screen is nonstandard rotation
  tft.fillScreen(ST7735_BLACK);
}

void loop() {
  if (!gameStarted) {
    drawGUI();
    while(!waitUntil(JOYSTICK_BUTTON_PIN, false));
    if (!startNetwork()) {
    }
    setSpawns(&player1, &player2);
    Serial.println(player1.currentPosition.x);
    gameStarted = true;
  }

  // Check if the position is acceptable
  if(!legalPosition(player1.currentPosition)) {
    tft.setCursor(0, 80);
    tft.println("YOU SUPER LOSE");
    printWalls();
    while(1);
  }
  // add a wall ad draw car at current position
  addWallPosition(player1.currentPosition); 
  tft.fillRect(player1.currentPosition.x, player1.currentPosition.y, 2, 2, ST7735_RED);
  movement_t temp = getJoystickInput();
  if (validInput(temp, player1.direction)) player1.direction = temp;
  else temp = player1.direction; 
  player1.currentPosition.x += temp.x;
  player1.currentPosition.y += temp.y;
  delay(75);
 
}
movement_t getJoystickInput() {
  int8_t joystickXMap;
  int8_t joystickYMap;
  joystickXMap = constrain(map(analogRead(JOYSTICK_MOVE_X_PIN) - joystickXCentre, 1023, 0, -100, 101), -100, 100);
  joystickYMap = constrain(map(analogRead(JOYSTICK_MOVE_Y_PIN) - joystickYCentre, 0, 1023, -100, 101), -100, 100);
  
  if (joystickXMap * joystickXMap  > joystickYMap * joystickYMap){
    joystickYMap = 0;
    joystickXMap = (joystickXMap < 0) ? -2 : 2;
  } else if (joystickXMap == joystickYMap) {
    joystickXMap = 0;
    joystickYMap = 0;
  } else {
    joystickXMap = 0;
    joystickYMap = (joystickYMap < 0) ? -2 : 2;
  }
  movement_t temp = {joystickXMap, joystickYMap};
  return  temp ;
}

bool legalPosition(position_t pos) {
  bool hasWall = false;
  for (uint8_t i = 0; i < 2; i++) {
    for (uint8_t j = 0; j < 2; j++) {
      hasWall |= getWallPosition(pos.x + i, pos.y + j);
      }
    }
  if (hasWall) {
    return false;
  }
  else if (pos.x == LCD_WIDTH || pos.x == 0 || pos.x +1 == LCD_WIDTH || pos.x +1 == 0) { // TODO UINT comparisons for -1 ?
    return false;
  }
  else if (pos.y == LCD_HEIGHT || pos.y == 0 || pos.y +1 == LCD_HEIGHT || pos.y +1 == 0) { 
    return false;
  }
  else return true;
}

bool validInput(movement_t in, movement_t old) {
  if (in.x == 0 && in.y == 0) return false;
  if ((in.x * -1 == in.x && in.x != 0)  || (in.y * -1 == in.y && in.y != 0)) return false;
  return true;
}

void addWallPosition(position_t pos) {
  uint8_t divisionOfXPosition = pos.x/8;
  uint8_t modOfXPosition = pos.x % 8;
  uint8_t divisionOfX2Position = (pos.x +1) / 8;
  uint8_t modOfX2Position = (pos.x +1) % 8;
  
  wallPositions[pos.y][divisionOfXPosition] |= (1 << modOfXPosition);
  wallPositions[pos.y][divisionOfX2Position] |= (1 << modOfX2Position);
  wallPositions[pos.y +1][(divisionOfXPosition)] |=  (1 << modOfXPosition);
  wallPositions[pos.y +1][divisionOfX2Position] |= (1 << modOfX2Position);
}

bool getWallPosition(uint8_t x, uint8_t y) {
  uint8_t val = wallPositions[y][x / 8];
  switch (x % 8) {
    case 0: return (val & 1); 
    case 1: return (val & 2);
    case 2: return (val & 4); 
    case 3: return (val & 8); 
    case 4: return (val & 16); 
    case 5: return (val & 32);
    case 6: return (val & 64);
    case 7: return (val & 128); 
  } 
  return 0;
}

void drawGUI() {
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0,0);
  tft.println("Press down joystick to search for partner");
}

void printWalls() {
  for (int i=0; i < 10; i++) {
    Serial.print(i);
    Serial.print(": ");
    for (int j=0; j < 20; j++) {
      Serial.print(wallPositions[i][j], BIN);
      Serial.print(", ");
    }
    Serial.println();
    Serial.println();
  }
}

bool waitUntil(int pin, bool pos) {
  while (digitalRead(JOYSTICK_BUTTON_PIN) != pos);
  return true;
}

bool startNetwork() {
  Serial1.write('r');
  for (int i=0; i < 10; i++) {

  if (Serial1.peek() == 'r') {
    Serial1.read();
    Serial.println("It has been here");
    dualPrint("Connection established!");
    return true;
  }
  else {
    // TODO fail case
    dualPrint("Establishing connection...");
    delay(500);
  }
  }
}

void setSpawns(player_t *p1, player_t *p2) {
  uint8_t myFlip = random(0, 255);
  uint8_t hisFlip = 0;
  Serial1.write(myFlip);
  while (hisFlip == 0) {
    if (Serial1.available()) {
      hisFlip = Serial1.read();
      Serial.print("myFlip: ");
      Serial.println(myFlip);
      Serial.print("hisFlip: ");
      Serial.println(hisFlip);
      if (myFlip > hisFlip) {
        getSpawns(p1, p2);
        Serial.println("I'm creating this");
        Serial1.write((uint8_t)p1->currentPosition.x);
        Serial1.write((uint8_t)p1->currentPosition.y);
        Serial1.write((int8_t)p1->direction.x);
        Serial1.write((int8_t)p1->direction.y);
        Serial1.write((uint8_t)p2->currentPosition.x);
        Serial1.write((uint8_t)p2->currentPosition.y);
        Serial1.write((int8_t)p2->direction.x);
        Serial1.write((int8_t)p2->direction.y);
      } else {
        p2->currentPosition.x = getUint();
        p2->currentPosition.y = getUint();
        p2->direction.x = getInt();
        p2->direction.y = getInt();
        p1->currentPosition.x = getUint();
        p1->currentPosition.y = getUint();
        p1->direction.x = getInt();
        p1->direction.y = getInt();
      }
    } else {
      Serial.println("Serial is not available");
    }
  }
  Serial.println("finished creation");
}

void getSpawns(player_t *p1, player_t *p2) {
  uint8_t randNum = random(0,2);
  Serial.println("randNum: ");
  Serial.println(randNum);
  if (randNum == 0) {
    player_t temp1 = {8, 64, 2, 0}; player_t temp2 = {152, 64, -2, 0};  
    (*p1) = temp1;
    (*p2) = temp2;
  } else {
    player_t temp1 = {80, 120, 0, -2}; player_t temp2 = {80, 8, 0, 2};
    (*p1) = temp1;
    (*p2) = temp2;
  }
}

void dualPrint(char *s) {
  Serial.println(s);
}

uint8_t getUint() {
  int val = 0;
  Serial.println("getUint");
  while (val == 0) {
    Serial.println("getU If");
    if (Serial1.available()) {
      Serial.println("iter");
      val = Serial1.read();
      return (uint8_t) val;
    }
  }
}

int8_t getInt() {
  int val = 0;
  while (val == 0) {
    if (Serial1.available()) {
      Serial.println("iter");
      val = Serial1.read();
      return (int8_t) val;
    }
  }
}

/*void printPlayer(player_t *p) {
  Serial.print("player has x: ")
}*/
