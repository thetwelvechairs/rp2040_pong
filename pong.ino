#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ST7789.h>      // Hardware-specific library for ST7789
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define SD_CS     8   // SD card select pin
#define TFT_DC    9
#define TFT_CS    10
#define TFT_RST   -1  // Or set to -1 and connect to Arduino RESET pin

#define ST77XX_PINK       0xFB9B
#define ST77XX_DARKGREEN  0x0BA8

#define USE_SD_CARD

#define PIN        16
#define NUMPIXELS 1

#define POTENTIOMETER_PIN1 A1
#define POTENTIOMETER_PIN2 A3


SdFat SD;                         // SD card filesystem
Adafruit_ImageReader reader(SD);  // Image-reader object, pass in SD filesys

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_Image img;     // An image loaded into RAM
int32_t imageWidth = 0; // BMP image dimensions
int32_t imageHeight = 0;

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

const int width = 320;
const int height = 170;

const int STARTX1 = (width / 4) - 12;
const int STARTX2 = (width - STARTX1) - 6;

int charWidth;
int charHeight;
int charMaxWidth;
int charMaxHeight;
int charMinWidth = 0;
int charMinHeight = 0;

const uint16_t backgroundColor = ST77XX_DARKGREEN;

bool playIntro = true;
bool ledOn = true;

int lastPlayer = 1;

int brightness = 0;
int fadeAmount = 5;

uint16_t localTime = millis();

struct player1 {
    int16_t x = 10;
    int16_t y = 60;
    int16_t width = 5;
    int16_t height = 50;
    int16_t color = ST77XX_PINK;
    int score = 0;
} player1{};

struct player2 {
    int16_t x = 305;
    int16_t y = 80;
    int16_t width = 5;
    int16_t height = 50;
    int16_t color = ST77XX_CYAN;
    int score = 0;
} player2{};

struct ball {
    int16_t x = 10;
    int16_t y = 60;
    int16_t radius = 4;
    int ballDirection = 1;
    int ballUp = 1;

} ball{};

void updateScore1(){
    tft.setTextSize(2);

    tft.fillRect(STARTX1, 2, 30, 18, backgroundColor);

    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(STARTX1 + 1, 5);
    tft.print(player1.score);

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(STARTX1, 4);
    tft.print(player1.score);
}

void updateScore2(){
    tft.setTextSize(2);

    tft.fillRect(STARTX2, 2, 30, 18, backgroundColor);

    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(STARTX2 + 1, 5);
    tft.print(player2.score);

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(STARTX2, 4);
    tft.print(player2.score);
}

void changeDirectionX(){
    ball.ballDirection *= -1;
    ledOn = true;
}

void changeDirectionY(){
    ball.ballUp *= -1;
    ledOn = true;
}

void drawField(){
    tft.drawFastHLine(0, 0, width, ST77XX_WHITE);
    tft.drawFastHLine(0, height-1, width, ST77XX_WHITE);

    tft.drawFastHLine(0, 20, width, ST77XX_WHITE);
    tft.drawFastHLine(0, height-20, width, ST77XX_WHITE);

    tft.drawFastVLine(0, 0, height, ST77XX_WHITE);
    tft.drawFastVLine(width - 1, 0, height, ST77XX_WHITE);

    tft.drawFastVLine(width/2, 0, height, ST77XX_WHITE);
}

bool checkCollision(){
    bool playerCollision = false;

    // Player 1 paddle collision
    if ((ball.x - ball.radius == player1.x + player1.width + 2) ||
        (ball.x + ball.radius == player1.x - player1.width)){
        if (ball.y >= player1.y && ball.y <= (player1.y + player1.height)){
            playerCollision = true;
            lastPlayer = 1;
            changeDirectionX();
        }
    }
    // Player 2 paddle collision
    if ((ball.x + ball.radius == player2.x - 2) ||
        (ball.x - ball.radius == player2.x + player2.width)){
        if (ball.y >= player2.y && ball.y <= (player2.y + player2.height)){
            playerCollision = true;
            lastPlayer = 2;
            changeDirectionX();
        }
    }

    // Left wall collision
    if (ball.x == 0){
        if (lastPlayer == 2){
            if (player2.score < 99){
                player2.score++;
            }
            else {
                player2.score = 0;
            }
        }
        updateScore2();
        changeDirectionX();
    }

    // Right wall collision
    if (ball.x >= width){
        if (lastPlayer == 1){
            if (player1.score < 99){
                player1.score++;
            }
            else {
                player1.score = 0;
            }
        }
        updateScore1();
        changeDirectionX();
    }

    // Top wall collision
    if (ball.y == 0){
        changeDirectionY();
    }

    // Bottom wall collision
    if (ball.y >= height){
        changeDirectionY();
    }

    return playerCollision;
}

void cycleLed(){
    if (ledOn == true){
      pixels.setPixelColor(0, pixels.Color(255, 255, 255));
    }
    else {
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
    }
    pixels.show();
}

void setup() {
    ImageReturnCode stat;

    tft.init(height, width);
    tft.setRotation(3);
    tft.fillScreen(backgroundColor);

    if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
        Serial.println(F("SD begin() failed"));
        exit(1);
    }

    charMaxWidth = width / charWidth;
    charMaxHeight = height / charHeight;

    localTime = millis() - localTime;

    // Splash screen
    stat = reader.drawBMP("/sofia.bmp", tft, 14, 20);
    tft.setCursor(140, 30);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setTextSize(4);
    tft.println("Sofia's");
    tft.setCursor(140, 70);
    tft.println("Super");
    tft.setCursor(140, 110);
    tft.print("Tennis!");
    delay(5000);

    // Game screen init
    tft.fillScreen(backgroundColor);
    updateScore1();
    updateScore2();
}

void loop() {
    ledOn = false;
    
    drawField();
  
    tft.fillCircle(ball.x, ball.y, ball.radius + 1, backgroundColor);

    ball.x = ball.x + ball.ballDirection;
    ball.y = ball.y + ball.ballUp;

    tft.drawCircle(ball.x, ball.y, ball.radius + 1, ST77XX_BLACK);
    tft.fillCircle(ball.x, ball.y, ball.radius, ST77XX_YELLOW);

    auto player1Pin = map(analogRead(POTENTIOMETER_PIN1), 0, 1023, 2, 120);
    auto player2Pin = map(analogRead(POTENTIOMETER_PIN2), 1023, 0, 2, 120);

    bool collision = checkCollision();

    if (collision == true || (player1.y != player1Pin) || (player2.y != player2Pin) ){
        tft.fillRect(player1.x, player1.y, player1.width, player1.height, backgroundColor);
        player1.y = player1Pin;
        tft.fillRect(player1.x, player1.y, player1.width, player1.height, player1.color);

        tft.fillRect(player2.x, player2.y, player2.width, player2.height, backgroundColor);
        player2.y = player2Pin;
        tft.fillRect(player2.x, player2.y, player2.width, player2.height, player2.color);
    }

    if (ball.y >= 2 && ball.y <= 22){
      if (ball.x >= STARTX1 - 10 && ball.x <= STARTX1 + 30){
        updateScore1();
      }
      if (ball.x >= STARTX2 - 10 && ball.x <= STARTX2 + 30){
        updateScore2();
      }
    }

    cycleLed();

}
