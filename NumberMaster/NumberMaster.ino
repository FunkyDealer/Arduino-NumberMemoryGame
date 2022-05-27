#include <arduino-timer.h>
#include <Key.h>
#include <Keypad.h>
#include <Wire.h>
#include <DFRobot_LCD.h>
#include <EEPROM.h>
#include "pitches.h"

#define ROWS 4
#define COLS 3
#define BUZZER 11
#define LEDRED 13
#define LEDGREEN 12

//Lcd stuff
DFRobot_LCD lcd(16,2);  //16 characters and 2 lines of show

//keypad stuff
char keys[ROWS][COLS] = {
  { '1', '2', '3'},
  { '4', '5', '6'},
  { '7', '8', '9'},
  { '*', '0', '#'}
};
byte rowPins[ROWS] = { 3, 8, 7, 5}; //row pinouts of the keypad
byte colPins[COLS] = { 4, 2, 6}; //column pinouts of the keypad

//create a new keypad based on the parameters defined above
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//Create timer instance
auto timer = timer_create_default();

enum {Initiating, Output, Input, LevelUp, LevelDown, GameOver}; //game States
unsigned char GameState;  // GameState

//Numbers outputing
int currentLevel = 1;
int currentAmmountDifficulty = 2; //current ammount of outputed numbers
int maxAmmountDifficulty = 4; //max ammount of output numbers
int generatedOutPutNumber = -1; //Machine output number //-1 indicates nothing outputed yet
int currentInputNumber = -1; //Player input number //-1 indicates nothing inputed yet
bool isReady = false;
bool sessionDone = false;

int timeDifficulty = 300; //Time between numbers showing up
byte timeCut = 30; //ammount to cut between level
byte minTimeDifficulty = 50;
byte wrongs = 0; //ammount of wrongs done in session
byte gameOverCounter = 0; //2 levels downs in a row = game over

int currentLevelPoints = 0;

//Score
int currentScore = 0;
int highScore = 0;
byte highScoreAdress = 1;

void setup() {
  // initialize
  lcd.init();

  pinMode(LEDRED, OUTPUT);
  pinMode(LEDGREEN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  randomSeed(analogRead(A0));  

  SetDefaultScreenColor();

  lcd.setCursor(0,0);
  lcd.setCursor(0,1);
  lcd.print("Starting!");

  EEPROM.get(highScoreAdress, highScore);

  GoToInitiating();
}

void GoToInitiating() 
{
  GameState = Initiating;

  lcd.setCursor(0,0);
  lcd.print("HighScore: ");
  lcd.setCursor(11,0);
  lcd.print(highScore);

  lcd.setCursor(0,1);
  lcd.print("Press * to start");
}

void loop() {
  timer.tick();

switch(GameState) {
  case Initiating:
  Initialize();
  break;
  
  case Output:
  NumbersOutPut();
  break;

  case Input:
  PlayerInput();
  break;

  case LevelUp:
  LevelUpMode();
  break;

  case LevelDown:
  LevelDownMode();
  break;

  case GameOver:
  GameOverMode();
  break;

  default:
  lcd.setCursor(5,0);
  lcd.print("Error!!");
  break;
} 
}

void Initialize() {    
  char key = keypad.getKey();
  
  if (key == '*') {
    PlayBuzzer(1000);
    timer.in(100, StopBuzzer);

    GameState = Output;
    isReady = false;
    sessionDone = false;
    lcd.clear();
  }
}

void GoToOutputMode() {
  lcd.clear();
  GameState = Output;
  generatedOutPutNumber = -1;
  currentInputNumber = -1;
  sessionDone = false;
  SetDefaultScreenColor();

  digitalWrite(LEDGREEN, LOW);
  digitalWrite(LEDRED, LOW);

  lcd.setCursor(0,0);
  lcd.print("Numbers Output!");
}

//Number output mode - player needs to memorize the numbers presented during this time
void NumbersOutPut() {
  if (!sessionDone) {
    if (generatedOutPutNumber >= 0) { //if n time generating number
   int newGen = random(0, 10);
  generatedOutPutNumber = Concatenate(generatedOutPutNumber, newGen);
    }
    else { //if first time generating a number
      generatedOutPutNumber = random(0, 10);
    }

    //if generated number is now the same lenght as the dificulty specifies it should be
  if (GetIntLength(generatedOutPutNumber) == currentAmmountDifficulty) {
  timer.in(timeDifficulty, GoToInputMode);
  sessionDone = true; //end session
  }
  else {
  lcd.setCursor(0,1);
  lcd.print(String(generatedOutPutNumber));
  delay(timeDifficulty);
  }
  }

  lcd.setCursor(0,1);
  lcd.print(String(generatedOutPutNumber));
}

//Put game in player input mode
void GoToInputMode() {
  lcd.clear();
  sessionDone = false;
  GameState = Input;  

  lcd.setCursor(0,1);
  lcd.print("input numbers!");
}

//Player number input mode - player will input numbers during this mode
void PlayerInput() {
  
  lcd.setCursor(0,0);
  char key = keypad.getKey();
  
  if (key && key != '*' && key != '#') {
    
    PlayBuzzer(key);
    timer.in(100, StopBuzzer);

    if (currentInputNumber < 0) {
      currentInputNumber = key - '0'; //convert char to int
    }
    else {
      int x = key - '0'; //convert char to int
      currentInputNumber = Concatenate(currentInputNumber, x);
    }    
  }

  if (currentInputNumber > 0) {
    lcd.setCursor(0,0);
    lcd.print(String(currentInputNumber));

    if (GetIntLength(currentInputNumber) == GetIntLength(generatedOutPutNumber)) {

      if (currentInputNumber == generatedOutPutNumber)
      {        
        SetGreenScreenColor();
        digitalWrite(LEDGREEN, HIGH);
        delay(500);

        currentLevelPoints += 10;
        currentScore += 10;
        
        if (currentLevelPoints < 50)
        {
        GoToOutputMode();
        }
        else {
        GoToLevelUpMode();
        }     
      }
      else
      {
        SetRedScreenColor();
        digitalWrite(LEDRED, HIGH);
        delay(500);

        if (currentLevel > 1) {
          currentLevelPoints -= 10;
          wrongs++;
          currentScore -= 10;
        }
        if (wrongs < 3)
        {
        GoToOutputMode();
        }
        else {
          GoToLevelDownMode();
        }
      }
    }    
  }  
}

void GoToLevelUpMode() {
  lcd.clear();
  GameState = LevelUp;
  generatedOutPutNumber = -1;
  currentInputNumber = -1;
  sessionDone = false;
  SetDefaultScreenColor();
    wrongs = 0;

  currentLevelPoints = 0;
  currentLevel++;

  gameOverCounter = 0;

  if (currentLevel % 2 == 0 && currentAmmountDifficulty < maxAmmountDifficulty) currentAmmountDifficulty++;

  if (currentLevel % 3 == 0)
  {
  if (timeDifficulty > minTimeDifficulty) timeDifficulty = timeDifficulty - timeCut;
  if (timeDifficulty < minTimeDifficulty) timeDifficulty = minTimeDifficulty;
  }
}

void GoToLevelDownMode() {
  lcd.clear();
  GameState = LevelDown;
  generatedOutPutNumber = -1;
  currentInputNumber = -1;
  sessionDone = false;
  SetDefaultScreenColor();
  wrongs = 0;
  currentLevelPoints = 0;
  currentLevel--;

  gameOverCounter++;

  if (gameOverCounter == 2) {
    GoToGameOver();
  }

  if (currentLevel % 2 == 0) currentAmmountDifficulty--;
  if (currentLevel % 3 == 0) timeDifficulty = timeDifficulty + timeCut;

}

void LevelUpMode() {
  SetGreenScreenColor();
    
  lcd.setCursor(0,0);
  lcd.print("Level up! #");
  
  lcd.setCursor(0,1);
  lcd.print("Score: ");
  lcd.setCursor(8,1);
  lcd.print(currentScore);

  char key = keypad.getKey();  
  if (key == '#') {
    PlayBuzzer(1000);
    timer.in(100, StopBuzzer);

    GoToOutputMode();
  }
}

void LevelDownMode() {
  SetRedScreenColor();

  lcd.setCursor(0,0);
  lcd.print("Level Down! #");
  
  lcd.setCursor(0,1);
  lcd.print("Score: ");
  lcd.setCursor(8,1);
  lcd.print(currentScore);

  char key = keypad.getKey();
  if (key == '#') {
    PlayBuzzer(1000);
    timer.in(100, StopBuzzer);

    GoToOutputMode();
  }
}

void GoToGameOver() {
  lcd.clear();
  sessionDone = false;
  GameState = GameOver;
  
  SetRedScreenColor();
    
  lcd.setCursor(0,0);
  lcd.print("Game over");
  
  lcd.setCursor(0,1);
  lcd.print("Score: ");
  lcd.setCursor(8,1);
  lcd.print(currentScore);  

  if (currentScore > highScore) {
  highScore = currentScore;
  EEPROM.put(highScoreAdress, highScore);
  lcd.setCursor(0,1);
  lcd.print("New HiScore: ");
  lcd.setCursor(13,1);
  lcd.print(currentScore);  
  }
}


void GameOverMode() {
  SetRedScreenColor();

}



void PlayBuzzer(char key) {
  switch (key) {
  case '1':
    tone(BUZZER, NOTE_C3); //Send 1 KHz sound signal
    break;
  case '2':
    tone(BUZZER, NOTE_E3); //Send 1 KHz sound signal
    break;
      case '3':
    tone(BUZZER, NOTE_F3); //Send 1 KHz sound signal
    break;
          case '4':
    tone(BUZZER, NOTE_G3); //Send 1 KHz sound signal
    break;
          case '5':
    tone(BUZZER, NOTE_A3); //Send 1 KHz sound signal
    break;
          case '6':
    tone(BUZZER, NOTE_B3); //Send 1 KHz sound signal
    break;
          case '7':
    tone(BUZZER, NOTE_C4); //Send 1 KHz sound signal
    break;
          case '8':
    tone(BUZZER, NOTE_D4); //Send 1 KHz sound signal
    break;
          case '9':
    tone(BUZZER, NOTE_E4); //Send 1 KHz sound signal
    break;
          case '0':
    tone(BUZZER, NOTE_F4); //Send 1 KHz sound signal
    break;
          case '#':
    tone(BUZZER, NOTE_G4); //Send 1 KHz sound signal
    break;
          case '*':
    tone(BUZZER, NOTE_A4); //Send 1 KHz sound signal
    break;
  default:
    tone(BUZZER, 10); //Send 1 KHz sound signal
    break;
}  
}

void StopBuzzer() {
  noTone(BUZZER); //Stop buzzer
}

//concatenate 2 values
int Concatenate(int x, int y) {
    int pow = 10;
    while(y >= pow)
        pow *= 10;
    return x * pow + y; 
}

//Get the length of an Int
int GetIntLength(int x) {   
  return floor(log10(abs(x))) + 1;
}

void SetDefaultScreenColor() {
    lcd.setRGB(150, 200, 255);                  //Set R,G,B Value
}

void SetGreenScreenColor() {
    lcd.setRGB(0, 252, 0);                  //Set R,G,B Value
}

void SetRedScreenColor() {
    lcd.setRGB(255, 0, 0);                  //Set R,G,B Value
}
