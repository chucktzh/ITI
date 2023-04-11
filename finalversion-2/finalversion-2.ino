#include <LiquidCrystal.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include <SD.h>
#include <math.h>
#include <Audio.h>

File myFile;


// Cache audio files
File audioFile;

#define LED_PIN_1 13
#define LED_PIN_2 12
#define LED_PIN_3 11
#define LED_PIN_4 10
#define NUM_LEDS 30
#define DELAY 10


// Initialize LCD pins
const int rs = 24, en = 25, d4 = 26, d5 = 27, d6 = 29, d7 = 28;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


// Initialising the WS2812B strip
Adafruit_NeoPixel strip4(NUM_LEDS, LED_PIN_1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(NUM_LEDS, LED_PIN_2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(NUM_LEDS, LED_PIN_3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip1(NUM_LEDS, LED_PIN_4, NEO_GRB + NEO_KHZ800);

// For storing strips of lights that need to flash
int blinkingLedStrip = -1;

// For storing the flashing status of the light strip
bool ledBlinkState = false;

// For recording the time of the last strip flashing switch
unsigned long lastBlinkTime = 0;

// For setting the flashing interval of the light strip
const int blinkInterval = 500;

// Initialize button pins
const int button1 = A0;
const int button2 = A1;
const int button3 = A2;
const int button4 = A3;
const int button5 = A4;
const int buttonSD = A5;

// SD card pins
const int chipSelect = 4;
const int speakerPin = DAC0;

// Used to control the ability to go back to the previous question after the score is displayed
unsigned long displayScoresStartTime = 0;
bool displayScoresActive = false;


File musicFile;
File dataFile;

// Audio file mapping table
const char* musicFiles[][2] = {
  { "TRAVEL-P.WAV", "TRAVEL-N.WAV" },
  { "FOOD-P.WAV", "FOOD-N.WAV" },
  { "TRASH-P", "TRASH-N" },
  { "SHOPPI~1.WAV", "SHOPPI~2.WAV" }
};

// Questions and options
String questions[][2] = {
  { "Are you happy with your current eco-related actions?", "Answer 1" },                                //1
  { "From the four areas below, is there one that you would like to improve?", "Answer 2" },             //2
  { "Are you ready for your first round of weekly challenges?", "Answer 3" },                            //3
  { "try a car free lifestyle for a week ", "Answer 4" },                                                // travel4
  { "try a weekly plant based diet", "Answer 5" },                                                       // food5
  { "Segregate your bio trash and take it out (for a week)", "Answer 6" },                               // nature6
  { "Buy only essentials for the week (no outside stuff)", "Answer 7" },                                 // living
  { "Next Week! From the four areas below, is there one that you would like to improve?", "Answer 8" },  //8
  { "Are you ready for your first round of weekly challenges?", "Answer 9" },
  { "Walk/bike to your grocery store instead of going by car yourself ", "Answer 10" },  // travel
  { "Eat 7 locally grown fruit/ vegetables this week", "Answer 11" },                    // food
  { "Donate one thing you don’t need anymore", "Answer 12" },                            // nature
  { "Take your own bag to the stores instead of agetting a platic bag", "Answer 13" }    // living
};

int focusCategory = -1;
int currentQuestion = 0;
String options[][5] = {
  { "1:YES", "2:NO" },
  { "1: TRAVEL", "2:FOOD", "3:WASTE", "4:SHOPPING", "5:RAMDOMLY" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1: Travel", "2:FOOD", "3:WASTE", "4:SHOPPING", "5:RAMDOMLY" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" },
  { "1:YES", "2:NO" }
};

int selectedOption = -1;
bool displayQuestion = true;

// Variables for scrolling display
unsigned long previousMillis = 0;
const long interval = 300;
int scrollPosition = 0;
int n = 0;

// Score variable
int scores[4] = { 0, 0, 0, 0 };
const char* categories[] = { "Travel", "Food", "Waste", "Shopping" };



void setup() {

  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);
  pinMode(button4, INPUT_PULLUP);
  pinMode(button5, INPUT_PULLUP);
  pinMode(buttonSD, INPUT_PULLUP);

  // Initialisation of the LED strip and display
  strip1.begin();
  strip2.begin();
  strip3.begin();
  strip4.begin();

  // LCD card initialization
  lcd.begin(16, 2);
  lcd.clear();

  SerialUSB.begin(9600);
  while (!SerialUSB) {}
  pinMode(speakerPin, OUTPUT);

  // Initialise the SD card
  if (!SD.begin(chipSelect)) {
    SerialUSB.println("SD 卡初始化失败");
    return;
  }
  SerialUSB.println("SD 卡初始化成功");

  // Read and parse the score file
  readScores();

  // Updating the LED light display
  updateLEDs();

  displaySDCardInfo();

  // Check if the music file exists, if not, output an error message and return
  if (!SD.exists("0002TR~2.WAV")) {
    SerialUSB.println("音乐文件不存在");
    return;
  }




  // Open music file
  musicFile = SD.open("0002TR~2.WAV");
  if (!musicFile) {
    SerialUSB.println("无法打开音乐文件");
    return;
  }
  SerialUSB.println("音乐文件已打开");

  // Set the DAC resolution to 12 bits (0~4095)
  analogWriteResolution(12);

  // hi-speed SPI transfers
  SPI.setClockDivider(4);

  // 44100 Hz stereo => 88200 sample rate
  // 100 mSec of prebuffering.
  Audio.begin(64000, 1);

  // displaySDCardInfo();
}

// Used to determine if the button is pressed twice in a row within 1 second 6
int button6PressCount = 0;
unsigned long button6LastPressTime = 0;
const int resetThreshold = 1000;  // 1秒内连续按动两次按钮6


void loop() {

  if (displayQuestion) {
    // Scroll through the questions and options
    scrollDisplay(questions[currentQuestion][0], options[currentQuestion], 5);
  }

  // Detects if buttons 1-5 are pressed
  for (int i = 0; i < 5; i++) {
    if (digitalRead(button1 + i) == LOW) {
      playMusicByName("CLICK1.WAV");
      selectedOption = i;
      displayQuestion = !displayQuestion;
      delay(500);
      break;
    }
  }

  // Cyclic detection of whether the leds need to be made to blink
  blinkLEDs(blinkingLedStrip);

  // Master flow of questions
  if (selectedOption != -1) {
    lcd.clear();
    if (currentQuestion == 0) {
      currentQuestion++;
    } else if (currentQuestion == 1 || currentQuestion == 8) {
      focusCategory = -1;
      String categoryName[] = { "TRAVEL", "FOOD", "WASTE", "SHOPPING" };
      String predefinedTexts[] = { "Focus on TRAVEL", "Focus on FOOD", "Focus on WASTE", "Focus on SHOPPING", "Focus RAMDOMLY" };

      if (selectedOption != 4) {
        focusCategory = selectedOption;
      }
      if (selectedOption == 4) {
        int randomCategory = random(0, 4);
        focusCategory = randomCategory;
        predefinedTexts[selectedOption] = "Focus on " + categoryName[randomCategory];
      }


      lcd.print(predefinedTexts[selectedOption]);

      blinkingLedStrip = focusCategory;
      delay(1500);
      currentQuestion++;

    } else if (currentQuestion == 2 || currentQuestion == 9) {
      if (selectedOption == 0) {
        currentQuestion++;
      } else {
        lcd.print("Come back later!");
        delay(3000);
        return;
      }
      // Stop flashing and set the light back to normal
      blinkingLedStrip = -1;
      focusCategory = -1;
      updateLEDs();
    } else {
      if ((currentQuestion >= 3 && currentQuestion <= 6) || (currentQuestion >= 10 && currentQuestion <= 13)) {


        int categoryIndex;

        if (currentQuestion >= 3 && currentQuestion <= 6) {
          categoryIndex = (currentQuestion - 3) % 4;
        } else if (currentQuestion >= 10 && currentQuestion <= 13) {
          categoryIndex = (currentQuestion - 10) % 4;
        }

        // Adjustment of score changes according to the options in question 2 and the current question
        int scoreChange = 2;
        if (focusCategory == categoryIndex) {
          scoreChange = 4;
        }
        scoreChange = (selectedOption == 0) ? scoreChange : -scoreChange;


        // Update and save scores
        updateScores(categoryIndex, scoreChange);
        saveScores();

        // Updating the LED light display
        updateLEDs();


        playMusic(categoryIndex, selectedOption == 0);
      }

      currentQuestion++;

      if (currentQuestion >= sizeof(questions) / sizeof(questions[0])) {
        lcd.clear();
        lcd.print("The End");
        delay(3000);
        return;
      }
    }
    delay(2000);
    selectedOption = -1;
    displayQuestion = !displayQuestion;
    scrollPosition = 0;
  }

  // detect if button 6 is pressed, display SD card information
  // When pressed twice, call resetScores() function to reset all scores to 0 and update the score.txt file
  if (digitalRead(buttonSD) == LOW) {
    playMusicByName("CLICK1.WAV");
    unsigned long currentTime = millis();
    if (currentTime - button6LastPressTime < resetThreshold) {
      button6PressCount++;
    } else {
      button6PressCount = 1;
    }
    button6LastPressTime = currentTime;

    if (button6PressCount == 1) {
      displayCategoriesAndScores();
      displayScoresStartTime = millis();
      displayScoresActive = true;
    } else if (button6PressCount == 2) {
      resetScores();
      button6PressCount = 0;
    }
  }

  if (displayScoresActive && (millis() - displayScoresStartTime > 5000)) {
    displayScoresActive = false;
    // Button 6 is pressed and after the score is displayed, the screen continues to show the question you are currently in
    scrollDisplay(questions[currentQuestion][0], options[currentQuestion], 5);
  }
}

void readScores() {
  if (SD.exists("scores.txt")) {
    File scoreFile = SD.open("scores.txt", FILE_READ);
    for (int i = 0; i < 4; i++) {
      scores[i] = scoreFile.parseInt();
    }
    scoreFile.close();
  } else {
    saveScores();
  }
}

void saveScores() {
  File scoreFile = SD.open("scores.txt", FILE_WRITE);
  for (int i = 0; i < 4; i++) {
    scoreFile.print(scores[i]);
    if (i < 3) {
      scoreFile.print(',');
    }
  }
  scoreFile.println();
  scoreFile.close();
}

void updateScores(int categoryIndex, int scoreChange) {
  scores[categoryIndex] += scoreChange;
}


// function to set the colour mode of the strip
void setLEDColorMode(Adafruit_NeoPixel& strip, int category, int score) {
  // Two-dimensional array storing the colour values of each LED bead in different modes for each category
  uint32_t colorModes[4][8][5] = {
    {
      // Travel
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236) },            // 一般
      { strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10) },       // 较好 淡黄色
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
    },
    {
      // Food
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236) },            // 一般
      { strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10) },       // 较好 淡黄色
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
    },
    {
      // Waste
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236) },            // 一般
      { strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10) },       // 较好 淡黄色
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
    },
    {
      // Shopping
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255), strip.Color(255, 255, 255) },  // 最差 worest
      { strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236), strip.Color(43, 97, 236) },            // 一般
      { strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10), strip.Color(193, 239, 10) },       // 较好 淡黄色
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
      { strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0), strip.Color(250, 255, 0) },            // 好
    }
  };

  // Calculating the brightness of the breathing effect
  unsigned long currentTime = millis();
  float brightness = 0.5 * (1.0 + sin(2 * M_PI * currentTime / 2000));  // 呼吸周期为2000毫秒



  int colorMode = 0;
  if (score >= -8 && score < -6) {
    colorMode = 0;
  } else if (score >= -6 && score < -4) {
    colorMode = 1;
  } else if (score >= -4 && score < -2) {
    colorMode = 2;
  } else if (score >= -2 && score < 0) {
    colorMode = 3;
  } else if (score >= 0 && score < 2) {
    colorMode = 4;
  } else if (score >= 2 && score < 4) {
    colorMode = 5;
  } else if (score >= 4 && score < 6) {
    colorMode = 6;
  } else if (score >= 6 && score <= 8) {
    colorMode = 7;
  }

  for (int i = 0; i < 5; i++) {
    uint32_t color = colorModes[category][colorMode][i];
    strip.setPixelColor(i, color);
  }
  strip.show();
}

void updateLEDs() {
  static unsigned long lastUpdateTime = 0;
  static float brightness = 0.0;
  static int brightnessDirection = 1;

  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= 20) {
    brightness += 0.01 * brightnessDirection;
    if (brightness >= 1.0) {
      brightness = 1.0;
      brightnessDirection = -1;
    } else if (brightness <= 0.0) {
      brightness = 0.0;
      brightnessDirection = 1;
    }
    SerialUSB.print("Score 1: ");
    SerialUSB.print(scores[0]);
    SerialUSB.print(", Score 2: ");
    SerialUSB.print(scores[1]);
    SerialUSB.print(", Score 3: ");
    SerialUSB.print(scores[2]);
    SerialUSB.print(", Score 4: ");
    SerialUSB.println(scores[3]);
    // Set the colour pattern of the strip according to the score
    setLEDColorMode(strip1, 0, scores[0]);
    setLEDColorMode(strip2, 1, scores[1]);
    setLEDColorMode(strip3, 2, scores[2]);
    setLEDColorMode(strip4, 3, scores[3]);

    lastUpdateTime = currentTime;
  }
}

void playMusicByName(const char* musicFileName) {
  SerialUSB.println("调用函数 ");

  SerialUSB.println(musicFileName);

  // Check if the music file exists, if not, output an error message and return
  if (!SD.exists(musicFileName)) {
    SerialUSB.println("音乐文件 ");
    SerialUSB.println(musicFileName);
    SerialUSB.println(" 不存在");
    return;
  }


  File musicFile = SD.open(musicFileName);
  if (!musicFile) {
    SerialUSB.println("无法打开音乐文件 ");
    SerialUSB.println(musicFileName);
    return;
  }

  const int S = 1024;  // Number of samples to read in block
  short buffer[S];

  SerialUSB.print("Playing");
  int count = 0;

  // until the file is not finished
  while (musicFile.available()) {
    // read from the file into buffer
    musicFile.read(buffer, sizeof(buffer));

    // Prepare samples
    int volume = 1024;
    Audio.prepare(buffer, S, volume);
    // Feed samples to audio
    Audio.write(buffer, S);

    // Every 100 block print a '.'
    count++;
    if (count == 100) {
      SerialUSB.print(".");
      count = 0;
    }
  }

  musicFile.close();
}


void playMusic(int musicIndex, bool yesOption) {
  SerialUSB.println("调用新版本的播放音乐函数！");


  const char* musicFileName = musicFiles[musicIndex][yesOption ? 0 : 1];

  SerialUSB.println(musicFileName);


  if (!SD.exists(musicFileName)) {
    SerialUSB.println("音乐文件 ");
    SerialUSB.println(musicFileName);
    SerialUSB.println(" 不存在");
    return;
  }


  File musicFile = SD.open(musicFileName);
  if (!musicFile) {
    SerialUSB.println("无法打开音乐文件 ");
    SerialUSB.println(musicFileName);
    return;
  }

  const int S = 1024;  // Number of samples to read in block
  short buffer[S];

  SerialUSB.print("Playing");
  int count = 0;

  // until the file is not finished
  while (musicFile.available()) {
    // read from the file into buffer
    musicFile.read(buffer, sizeof(buffer));

    // Prepare samples
    int volume = 1024;
    Audio.prepare(buffer, S, volume);
    // Feed samples to audio
    Audio.write(buffer, S);

    // Every 100 block print a '.'
    count++;
    if (count == 100) {
      SerialUSB.print(".");
      count = 0;
    }
  }

  musicFile.close();
}


void scrollDisplay(String text1, String text2[], int numOptions) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(text1.substring(scrollPosition, scrollPosition + 16));
    lcd.setCursor(0, 1);
    String optionsLine = "";
    for (int i = 0; i < numOptions; i++) {
      optionsLine += text2[i] + " ";
    }
    lcd.print(optionsLine.substring(scrollPosition, scrollPosition + 16));

    scrollPosition++;
    // Reset scrollPosition to 0 when scrolling to the end of the text
    if (scrollPosition > max(text1.length(), optionsLine.length())) {
      scrollPosition = 0;
    }
  }
}


void displayCategoriesAndScores() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WEEKLY REFLECTION");
  delay(1000);

  for (int i = 0; i < 4; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(categories[i]);
    lcd.setCursor(0, 1);
    lcd.print("Score: ");
    lcd.print(scores[i]);
    delay(1250);
  }
}


void displaySDCardInfo() {
  SerialUSB.println("SD Card Info:");
  SerialUSB.println("File names:");

  File root = SD.open("/");
  if (root) {
    while (true) {
      File file = root.openNextFile();
      if (!file) {
        break;
      }
      SerialUSB.println(file.name());
      file.close();
    }
    root.close();
  }
}


void resetScores() {
  for (int i = 0; i < 4; i++) {
    scores[i] = 0;
  }
  saveScores();

  updateLEDs();
}

// Used to make a certain category of light strip flash
void blinkLEDs(int category) {
  if (blinkingLedStrip != -1) {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= blinkInterval) {
      ledBlinkState = !ledBlinkState;
      lastBlinkTime = currentTime;
    }

    Adafruit_NeoPixel* strip;
    switch (category) {
      case 0:
        strip = &strip1;
        break;
      case 1:
        strip = &strip2;
        break;
      case 2:
        strip = &strip3;
        break;
      case 3:
        strip = &strip4;
        break;
    }

    if (ledBlinkState) {
      setLEDColorMode(*strip, category, 8);
    } else {
      strip->clear();
      strip->show();
    }
  }
}