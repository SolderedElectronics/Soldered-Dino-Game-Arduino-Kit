#include <EEPROM.h>
#include "16x2-LCD-SOLDERED.h"
#include "sprites.h"
#include "string.h"

LCD lcd;

#define MAGIC_EEPROM 0xDEADBEEF

uint32_t start = 0;

char a[] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 0};
char b[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0};

int secsLen = 1, state = 0;
volatile bool jump = 0;
uint32_t lastFrame = 0, lastTimeUpdate = 0, stateAt = 0;
uint16_t frameCnt = 0;

void buttonPressed()
{
    jump = 1;
}

void end()
{
    lcd.clear();

    uint32_t seconds = (millis() - start) / 1000, oldSeconds;

    uint32_t magic;
    EEPROM.get(0, magic);

    if (magic != MAGIC_EEPROM)
    {
        EEPROM.put(0, MAGIC_EEPROM);
        oldSeconds = 0;
    }
    else
    {
        EEPROM.get(sizeof magic, oldSeconds);
    }

    lcd.setCursor(0, 0);
    lcd.print(seconds > oldSeconds ? "New High Score!" : "High score is:");

    lcd.setCursor(5, 1);
    lcd.print(seconds > oldSeconds ? seconds : oldSeconds);

    if (seconds > oldSeconds)
        EEPROM.put(sizeof magic, seconds);

    delay(1500);

    lcd.clear();

    lcd.setCursor(4, 0);
    lcd.print("Game Over!");

    char c[] = "Press button to restart!";
    for (int i = 0;; ++i)
    {
        lcd.setCursor(0, 1);

        if (i % 16 < 8)
            lcd.print(c + (i % 8));
        else
            lcd.print(c + (8 - i % 8));

        delay(500);

        start = millis();
        memset(a, 8, sizeof a - 1);
        memset(b, 1, sizeof b - 1);
        lastFrame = 0;
        state = 0;
        jump = 0;
        lastTimeUpdate = 0;
        stateAt = 0;
        frameCnt = 0;

        if (digitalRead(2) == LOW)
            return;
    }
}

void setup()
{
    Serial.begin(115200);
    lcd.begin();
    lcd.backlight();

    pinMode(2, INPUT_PULLUP);
    delay(10);
    attachInterrupt(digitalPinToInterrupt(2), buttonPressed, FALLING);
}

void loop()
{
    lcd.createChar(1, land);
    lcd.createChar(2, cactus);
    lcd.createChar(3, cactus2);
    lcd.createChar(4, dinos);
    lcd.createChar(5, dinoUp);
    lcd.createChar(6, bird);
    lcd.createChar(7, bird2);
    lcd.home();

    if (frameCnt % 4 == 0)
    {
        uint32_t seconds = (millis() - start) / 1000;
        int bound = 500, birdBound = 900;

        if (seconds > 60 * 5)
        {
            bound = 0;
            birdBound = 500;
        }
        else if (seconds > 60 * 4)
        {
            bound = 200;
            birdBound = 500;
        }
        else if (seconds > 60 * 3)
        {
            bound = 500;
            birdBound = 600;
        }
        else if (seconds > 60 * 2)
        {
            bound = 600;
            birdBound = 700;
        }
        else if (seconds > 60)
        {
            bound = 700;
            birdBound = 800;
        }
        else
        {
            bound = 800;
            birdBound = 800;
        }

        int r1 = rand() % 1000;
        int r2 = rand() % 1000;
        int r3 = rand() % 1000;

        if (r1 > bound)
        {
            if (r2 > birdBound)
            {
                a[15] = 6;
            }
            else
            {
                b[15] = (r3 > 500 ? 2 : 3);
            }
        }
    }

    if (state != 1)
        b[1] = 4;
    else
        b[1] = 1;

    for (int i = 0; i < 16; ++i)
    {
        if (a[i] == 6)
        {
            if (a[i - 1] == 5)
            {
                end();
                return;
            }
            if (i)
                a[i - 1] = 6;
            a[i] = 8;
        }
        if (b[i] == 2 || b[i] == 3)
        {
            if (b[i - 1] == 4)
            {
                end();
                return;
            }
            if (i)
                b[i - 1] = b[i];
            b[i] = 1;
        }
    }

    for (int i = 0; i < 16 - secsLen; ++i)
    {
        if (i == 1 && state == 1)
            continue;

        lcd.setCursor(i, 0);

        lcd.write(a[i] == 8 ? ' ' : (frameCnt % 2 ? 6 : 7));
    }

    lcd.setCursor(0, 1);
    lcd.print(b);

    uint32_t seconds = (millis() - start) / 1000;

    int delayMs = 400;
    if (seconds < 100)
        delayMs = 400 - 3 * seconds;
    else
        delayMs = 100;

    while ((millis() - start) < lastFrame + delayMs)
    {
        if (state == 0 && jump)
        {
            state = 1;

            lcd.setCursor(1, 0);
            lcd.write(5);
            lcd.setCursor(1, 1);
            lcd.write(1);

            stateAt = millis() - start;

            if (a[1] != 8)
            {
                end();
                return;
            }

            jump = 0;
        }

        if (state == 1 && (millis() - start) > stateAt + 600)
        {
            lcd.setCursor(1, 0);
            lcd.write(' ');
            lcd.setCursor(1, 1);
            lcd.write(4);

            if (digitalRead(2) == LOW)
            {
                stateAt = millis() - start;
                state = 3;
            }
            else
            {
                stateAt = millis() - start;
                state = 0;
            }
        }

        if (state == 3 && (millis() - start) > stateAt + 200)
        {
            if (digitalRead(2) == LOW)
            {
                jump = 1;
                state = 0;
                stateAt = millis() - start;
            }
            else
            {
                state = 0;
                stateAt = millis() - start;
            }
        }

        if ((millis() - start) > lastTimeUpdate + 1000)
        {
            uint32_t seconds = (millis() - start) / 1000;

            char t[10];
            itoa(seconds, t, 10);
            secsLen = strlen(t);
            lcd.setCursor(16 - strlen(t), 0);
            lcd.print(t);

            lastTimeUpdate = millis() - start;
        }

        delay(5);
    }

    lastFrame = millis() - start;
    ++frameCnt;
}
