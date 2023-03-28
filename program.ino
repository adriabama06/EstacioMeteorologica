// imports
// https://github.com/adafruit/Adafruit_Sensor
#include "DHT.h" // https://github.com/adafruit/DHT-sensor-library
#include <LiquidCrystal.h> // https://github.com/arduino-libraries/LiquidCrystal
#include <SFE_BMP180.h> // https://github.com/sparkfun/BMP180_Breakout
#include <Wire.h>
#include <SD.h> // https://github.com/arduino-libraries/SD



// defines

// DHT
#define DHTPIN 13
#define DHTTYPE DHT11 // DHT11 || DHT 21 || DHT22

// LCD rs, enable, ... digital pins ..., 
const int rs = 12, en = 11, d4 = 6, d5 = 5, d6 = 4, d7 = 3; // https://naylampmechatronics.com/blog/34_tutorial-lcd-conectando-tu-arduino-a-un-lcd1602-y-lcd2004.html

// BMP180
// No defines, use I2C       (barcelona)
double presion_nivel_del_mar = 1019.5; // presion sobre el nibel del mar en mbar

// SD
#define SD_CS_PIN 53



// global variables

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
SFE_BMP180 bmp180;



// functions

// return 0 no errors
int dht_sensor(float& humidity, float& temperature)
{
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if(isnan(humidity) || isnan(temperature)) {
        humidity = 0;
        temperature = 0;
        return 1;
    }

    return 0;
}

namespace c_lcd {
    String lines[2] = {String(""), String("")};

    void update(int i)
    {
        String& line = lines[i];

        if(Serial) {
            Serial.println(line);
        }

        lcd.setCursor(0, i);
        lcd.print(lines[i]);
    }

    void print(String line, int i)
    {
        if(i > 1 || i < 0) return;

        lines[i] = line;

        update(i);
    }
}

// return 0 no errors
int bmp_180_sensor(double& pression, double& altitude)
{
    char status;
    double temperature;

    status = bmp180.startTemperature();

    if(status != 0) {
        delay(status);

        status = bmp180.getTemperature(temperature);
 

        if (status != 0) {
            status = bmp180.startPressure(3);

            if(status != 0) {
                delay(status);
            
                status = bmp180.getPressure(pression, temperature);

                if(status != 0) {

                    altitude = bmp180.altitude(pression, presion_nivel_del_mar);

                    return 0;
                }
            }
        }
    }

    return 1;
}

namespace c_sd {
    File f;

    
}



// Setup

void setup()
{
    if(Serial) {
        Serial.begin(9600);
    }

    dht.begin();

    // columns, rows
    lcd.begin(16, 2);

    c_lcd::print(String("Temperature:"), 0);
}



// Loop

float humidity;
float temperature;
double pression;
double altitude;

void loop()
{
    while(dht_sensor(humidity, temperature) != 0) {
        Serial.println("Error on read DHT sensor");
        delay(7000);
    }

    while(bmp_180_sensor(pression, altitude) != 0) {
        Serial.println("Error on read BMP 180 sensor");
        delay(7000);
    }



    delay(6000);
}