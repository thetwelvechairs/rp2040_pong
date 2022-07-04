#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_ST7789.h>      // Hardware-specific library for ST7789
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <SdFat.h>                // SD card & FAT filesystem library
#include <SPI.h>

#define SD_CS     8   // SD card select pin
#define TFT_DC    9
#define TFT_CS    10
#define TFT_RST   -1  // Or set to -1 and connect to Arduino RESET pin

#define ST77XX_PINK       0xFB9B
#define ST77XX_DARKGREEN  0x0BA8

#define USE_SD_CARD

#define POTENTIOMETER_PIN1 A0
#define POTENTIOMETER_PIN2 A1

SdFat SD;                         // SD card filesystem
Adafruit_ImageReader reader(SD);  // Image-reader object, pass in SD filesys

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_Image img;     // An image loaded into RAM
int32_t imageWidth = 0; // BMP image dimensions
int32_t imageHeight = 0;

int width = 320;
int height = 170;

int charWidth;
int charHeight;
int charMaxWidth;
int charMaxHeight;
int charMinWidth = 0;
int charMinHeight = 0;

uint16_t backgroundColor = ST77XX_DARKGREEN;

bool playIntro = true;

int startX1;
int startX2;

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

void updateScore(){
    tft.setTextSize(2);
    
    startX1 = width / 4;
    startX2 = width - startX1;

    if (player1.score < 10){
        startX1 = startX1 - 8;
        startX2 = startX2 - 4;
    }
    else if (player1.score >= 10 && player1.score < 100){
        startX1 = startX1 - 12;
        startX2 = startX2 - 6;
    }
    
    tft.fillRect(startX1, 2, 30, 18, backgroundColor);
    tft.fillRect(startX2, 2, 30, 18, backgroundColor);

    tft.setTextColor(ST77XX_BLACK);
    tft.setCursor(startX1 + 1, 5);
    tft.print(player1.score);
    tft.setCursor(startX2 + 1, 5);
    tft.print(player2.score);

    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(startX1, 4);
    tft.print(player1.score);
    tft.setCursor(startX2, 4);
    tft.print(player2.score);
}

void changeDirectionX(){
    ball.ballDirection *= -1;
}

void changeDirectionY(){
    ball.ballUp *= -1;
}

void drawNet(){
    tft.drawFastHLine(0, 0, width, ST77XX_WHITE);
    tft.drawFastHLine(0, height-1, width, ST77XX_WHITE);

    tft.drawFastHLine(0, 20, width, ST77XX_WHITE);
    tft.drawFastHLine(0, height-20, width, ST77XX_WHITE);

    tft.drawFastVLine(0, 0, height, ST77XX_WHITE);
    tft.drawFastVLine(width - 1, 0, height, ST77XX_WHITE);

    tft.drawFastVLine(width/2, 0, height, ST77XX_WHITE);
}

void checkCollision(){
    // Player 1 paddle collision
    if (ball.ballDirection < 0 && ball.x == (player1.x + player1.width)){
        if (ball.y >= player1.y && ball.y <= (player1.y + player1.height)){
            changeDirectionX();
        }
    }
    // Player 2 paddle collision
    if (ball.ballDirection > 0 && ball.x == (player2.x)){
        if (ball.y >= player2.y && ball.y <= (player2.y + player2.height)){
            changeDirectionX();
        }
    }

    // Left wall collision
    if (ball.x == 0){
        if (player2.score < 99){
            player2.score++;
        }
        else {
            player2.score = 0;
        }
        updateScore();
        delay(500);
        changeDirectionX();
    }

    // Right wall collision
    if (ball.x >= width){
        if (player1.score < 99){
            player1.score++;
        }
        else {
            player1.score = 0;
        }
        updateScore();
        delay(500);
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
}

void setup() {
    ImageReturnCode stat;

    tft.init(height, width);
    tft.setRotation(3);
    tft.fillScreen(backgroundColor);

    if(!SD.begin(SD_CS, SD_SCK_MHZ(10))) { // Breakouts require 10 MHz limit due to longer wires
        Serial.println(F("SD begin() failed"));
        for(;;); // Fatal error, do not continue
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
    delay(1000);

    // Game screen init
    tft.fillScreen(backgroundColor);
    updateScore();
}

void loop() {
    drawNet();

    tft.fillCircle(ball.x, ball.y, ball.radius + 1, backgroundColor);

    ball.x = ball.x + ball.ballDirection;
    ball.y = ball.y + ball.ballUp;

    tft.drawCircle(ball.x, ball.y, ball.radius + 1, ST77XX_BLACK);
    tft.fillCircle(ball.x, ball.y, ball.radius, ST77XX_YELLOW);

    checkCollision();

    auto player1Pin = map(analogRead(POTENTIOMETER_PIN1), 0, 1023, 2, 120);
    if (player1.y != player1Pin){
        tft.fillRect(player1.x, player1.y, player1.width, player1.height, backgroundColor);
        player1.y = player1Pin;
        tft.fillRect(player1.x, player1.y, player1.width, player1.height, player1.color);
    }

    auto player2Pin = map(analogRead(POTENTIOMETER_PIN2), 0, 1023, 2, 120);
    if (player2.y != player2Pin){
        tft.fillRect(player2.x, player2.y, player2.width, player2.height, backgroundColor);
        player2.y = player2Pin;
        tft.fillRect(player2.x, player2.y, player2.width, player2.height, player2.color);
    }
}
