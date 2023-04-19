// imports
// https://github.com/adafruit/Adafruit_Sensor
#include "DHT.h" // https://github.com/adafruit/DHT-sensor-library
#include <LiquidCrystal.h> // https://github.com/arduino-libraries/LiquidCrystal
#include <SFE_BMP180.h> // https://github.com/sparkfun/BMP180_Breakout
#include <Wire.h>
#include <SPI.h>
#include <SD.h> // https://github.com/arduino-libraries/SD



// defines

// program
// every how many seconds check and save data
#define check_interval 30



// DHT
#define DHTPIN 2
#define DHTTYPE DHT11 // DHT11 || DHT 21 || DHT22

// LCD rs, enable, ... digital pins ..., 
const int rs = 12, en = 11, d4 = 6, d5 = 5, d6 = 4, d7 = 3; // https://naylampmechatronics.com/blog/34_tutorial-lcd-conectando-tu-arduino-a-un-lcd1602-y-lcd2004.html

// BMP180
// No defines, use I2C:
// BMP180     SCL SDA
// Uno/Nano   A5  A4
// Mega       21  20
//                          (barcelona)
double presion_nivel_del_mar = 1019.5; // presion sobre el nibel del mar en mbar

// SD
// 5v
// SD         CS MOSI SCK MISO
// Uno/Nano   4  11   13  12
// Mega     4/53 51   52  50
// 4 is in use, use 53, is close to the other cables
#define SD_CS_PIN 53
int has_sd_start = 0;



// macros

#define SD_FILE_LOG(id) String("DATA_") + String(id) + String(".TXT")

#define S String



// global variables

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
SFE_BMP180 bmp180;



// functions

// return 0 no errors
int dht_sensor(float& humidity, float& temperature)
{
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if(isnan(h) || isnan(t)) {
        return 1;
    }

    humidity = h;
    temperature = t;

    return 0;
}

namespace c_lcd {
    String lines[2] = {String(""), String("")};
    String clear = String("                ");

    void update(int i)
    {
        String& line = lines[i];

        if(Serial) {
            Serial.println(line);
        }

        lcd.setCursor(0, i);
        lcd.print(clear);
        delay(100);
        lcd.setCursor(0, i);
        delay(100);
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

                    altitude = bmp180.altitude(presion_nivel_del_mar, pression);

                    return 0;
                }
            }
        }
    }

    return 1;
}

namespace c_sd {
    File f;
    String filename;

    void save() {
        if(!has_sd_start) return;
        // to save content open and close the file
        f.close();
        f = SD.open(filename.c_str(), FILE_WRITE);
    }

    void write(String text) {
        if(has_sd_start) {
            f.write(text.c_str(), text.length());
        }

        Serial.println(text);
    
        delay(100);

        save();
    }

    void open() {
        if(!has_sd_start) return;

        filename = SD_FILE_LOG(1);

        for(unsigned int i = 1; SD.exists(filename); i++)
        {
            filename = SD_FILE_LOG(i);
        }

        f = SD.open(filename.c_str(), FILE_WRITE);

        c_lcd::print(String("Creant fitxer:"), 0);
        c_lcd::print(filename, 1);
    }
}

namespace c_time {
    unsigned long day = 0;
    unsigned long hour = 0;
    unsigned long minute = 0;
    unsigned long second = 0;

    void update() {
        while (second >= 60)
        {
            second -= 60;
            minute += 1;
        }
        
        while (minute >= 60)
        {
            minute -= 60;
            hour += 1;
        }

        while (hour >= 24)
        {
            hour -= 24;
            day += 1;
        }
    }
}



// Setup

unsigned long last_millis = 0;

void setup()
{
    last_millis = millis();

    if(Serial) {
        Serial.begin(9600);
    }

    // columns, rows
    lcd.begin(16, 2);

    c_lcd::print(String("LCD Ok"), 0);

    delay(((check_interval * 0.1) * 1000) - 200);

    dht.begin();

    float dht_test = dht.readHumidity();

    if(isnan(dht_test)) {
        c_lcd::print(String("DHT Error"), 0);
    } else {
        c_lcd::print(String("DHT Ok"), 0);
    }

    delay(((check_interval * 0.1) * 1000) - 200);

    if(bmp180.begin() == 0) {
        c_lcd::print(String("BMP180 Error"), 0);
    } else {
        c_lcd::print(String("BMP180 Ok"), 0);
    }

    delay(((check_interval * 0.1) * 1000) - 200);

    if(!SD.begin(SD_CS_PIN)) {
        c_lcd::print(String("SD Error"), 0);
    } else {
        has_sd_start = 1;
        c_lcd::print(String("SD Ok"), 0);
    }

    delay(((check_interval * 0.1) * 1000) - 200);

    c_sd::open();

    delay(((check_interval * 0.1) * 1000) - 400);

    c_sd::write(
        S("# m=time elapsed from the start of the program in day:hour:minute:seconds, t=temerature in Celcius, h=humidity in percent, p=pression in mbar, a=altitude relative to sea in barcelona in meters\n")
    );

    c_lcd::print(String("Tot preparat"), 0);

    delay(((check_interval * 0.1) * 1000) - 200);
}



// Loop

float humidity = 0;
float temperature = 0;
double pression = 0;
double altitude = 0;

void save_data()
{
    c_sd::write(
        S("m=") + S(c_time::day) + S(":") + S(c_time::hour) + S(":") + S(c_time::minute) + S(":") + S(c_time::second) + S(";") +
        S("t=") + S(temperature, 1) + S(";") +
        S("h=") + S(humidity, 0) + S(";") +
        S("p=") + S(pression, 2) + S(";") +
        S("a=") + S(altitude, 2) + S("\n")
    );
}

unsigned long loop_start_millis = 0;
unsigned long elapsed_millis = 0;
unsigned long elapsed_millis_seconds = 0;

void loop()
{
    loop_start_millis = millis();

    if(loop_start_millis < last_millis) {
        last_millis = loop_start_millis + elapsed_millis;
        return;
    }

    elapsed_millis = loop_start_millis - last_millis;
    elapsed_millis_seconds = elapsed_millis / 1000;

    if(elapsed_millis_seconds < check_interval) {
        return;
    }

    c_time::second += elapsed_millis_seconds;

    c_time::update();

    last_millis = loop_start_millis; // continue check async work

    if(dht_sensor(humidity, temperature) != 0) {
        c_lcd::print("DHT Error", 0);
    }

    if(bmp_180_sensor(pression, altitude) != 0) {
        c_lcd::print("BMP180 Error", 0);
    }

    save_data();

    c_lcd::print(
        String(temperature, 1) + String(" C - ") + String(humidity, 0) + String(" %")
    , 0);

    c_lcd::print(
        String(pression, 2) + String(" mbar")
    , 1);

    delay(((check_interval * 0.5) * 1000) - 600);

    c_lcd::print(
        String(altitude, 2) + String(" m")
    , 1);
}