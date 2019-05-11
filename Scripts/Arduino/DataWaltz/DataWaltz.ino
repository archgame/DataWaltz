#define PIN 7
#include <SPI.h>
#include <WiFi101.h>
#include <MQTTClient.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#include <ArduinoJson.h>

//ADJUSTABLE VARIABLES
//const char *side = "/EAST"; //Uncomment East side of the gallery.
const char *side = "/WEST"; //Uncomment West side of the gallery.

const char *ssid = "ATT4n579w9"; //Uncomment for gallery wifi name.
const char *pass = "3x276z6t446n"; //Uncomment for gallery wifi password.
//const char *ssid = "This is a WiFi network"; //Uncomment for home wifi name.
//const char *pass = "jedamskiwifi"; //Uncomment for home wifi password.

const int LEDS = 480; //480//Number of LEDs on a strip, 480 for the gallery
const uint16_t wavecolor1Hue = 592; //Wave color hue, set between 0-767.  Original: 592
const uint16_t wavecolor2Hue = 450; //Wave color hue, set between 0-767.  Original: 511
const uint16_t waveFreq = 16; //Wave frequency, the higher the number the more waves
const int pause = 20; //50 //Wave speed, set this number first as it will effect all other speeds (the number is in milliseconds).  Original: 100
const uint8_t whitepulse = 5;//5 //Los Angeles update pulse speed, effected by "pause variable"  Original: 16
const uint8_t LEDLAdT = 8; //10//fade speed before and after Los Angeles update, effected by "pause variable"
const int pulseLength = 10; //Update blink fade out speed, effected by "pause variable"
const int pulseWidthMax = 100; //60//Maximum pulse LED width.  Original: 10
const int pulseWidthMin = 30; //15//Minimum pulse LED width.  Original: 3
const int wordcountMin = -200; //Cap for minimum word count, at this number and above blink is set to "pulseWidthMax".
const int wordcountMax = 200; //Cap for maximum word count, at this number and above blink is set to "pulseWidthMax".

//errors happening around pulsewidthmax/min

//STATIC VARIABLES
int t = 0;
int dT = 0;
int lastT = 0;
int degreeMin = 0;
int degreeMax = 180;

uint8_t wavecolor1[3];
uint8_t wavecolor2[3];
int LEDR[LEDS];
int LEDG[LEDS];
int LEDB[LEDS];
uint8_t LEDdT[LEDS];
uint8_t LEDLA = 0;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS, PIN, NEO_GRB + NEO_KHZ800);
WiFiClient net;
MQTTClient client;

//START
void setup() { 
  
  Serial.begin(9600);
  randomSeed(analogRead(0));
  
  //WIFI
  WiFi.begin(ssid, pass);
  client.begin("broker.shiftr.io", net);
  connect(side);

  //LED STRIP
  pinMode(LED_BUILTIN, OUTPUT);
  strip.begin();
  strip.show();  

  //ARRAY SETUP
  for(int i=0; i<strip.numPixels(); i++) 
  {
    LEDR[i] = 0;
    LEDG[i] = 0;
    LEDB[i] = 0;
    LEDdT[i] = 0;
  }

  //COLORS
  hsb2rgb(wavecolor1Hue,255,255,wavecolor1);
  hsb2rgb(wavecolor2Hue,255,255,wavecolor2);
}

//LOOP
void loop() {
  client.loop(); //Why do we loop here?
  if(!client.connected()){connect(side);} 
  t = millis();
  if(t - lastT > pause)
  {
    lastT = t;
    color();
  } 
}


//FUNCTIONS

//COLOR
void color(){


  //SET WAVE
  uint8_t fadecolor[3];
  for(int i=0; i<strip.numPixels(); i++) 
  {
    double ii;
    //if(i <= strip.numPixels()/2)
    //{
      //ii = i/(1.0*strip.numPixels()/2)*100;
    //}
    //else
    //{
      //ii = (strip.numPixels() - i)/(1.0*strip.numPixels()/2)*100;
    //}
    ii = sin(i*1.0/strip.numPixels()*waveFreq)*100;
    ii = (ii+100)/2.0;
    //Serial.println(ii);   
    twocolorfade(ii, wavecolor1, wavecolor2, fadecolor);
    int dTi = i+dT;
    
    if(dTi < strip.numPixels())
    {    
      LEDR[dTi] = constrain(fadecolor[0] + LEDR[dTi]*LEDdT[dTi]/pulseLength, 0, 255);
      LEDG[dTi] = constrain(fadecolor[1] + LEDG[dTi]*LEDdT[dTi]/pulseLength, 0, 255);
      LEDB[dTi] = constrain(fadecolor[2] + LEDB[dTi]*LEDdT[dTi]/pulseLength, 0, 255);
      if(LEDdT[dTi] > 0)
      {
        LEDdT[dTi] -= 1;
      }
    }
    else
    {
      LEDR[dTi-strip.numPixels()] = constrain(fadecolor[0] + LEDR[dTi-strip.numPixels()]*LEDdT[dTi-strip.numPixels()]/pulseLength, 0, 255);
      LEDG[dTi-strip.numPixels()] = constrain(fadecolor[1] + LEDG[dTi-strip.numPixels()]*LEDdT[dTi-strip.numPixels()]/pulseLength, 0, 255);
      LEDB[dTi-strip.numPixels()] = constrain(fadecolor[2] + LEDB[dTi-strip.numPixels()]*LEDdT[dTi-strip.numPixels()]/pulseLength, 0, 255);
      if(LEDdT[dTi-strip.numPixels()] > 0)
      {
        LEDdT[dTi-strip.numPixels()] -= 1;
      }   
    } 

      strip.setPixelColor(i, LEDR[i], LEDG[i], LEDB[i]); 
  }
  
    
  //STEPPING
  if(LEDLA == 0)
  {
    strip.show();
    dT += 1; 
    if(dT == LEDS)
    {
      dT = 0;
    }
  }
  else
  //SET LOS ANGELES
  {    
    //WHITE PULSE
    if(LEDLA == 1)
    {
      //for(int i=0; i < whitepulse; i++)
      for(int i=0; i < strip.numPixels()/4; i++)  
      {
        for(int j=0; j< strip.numPixels(); j++) 
        {         
          //int k = strip.numPixels()/2+(i*(strip.numPixels()/(1.0 * whitepulse))/2.0);
          //int l = strip.numPixels()/2-(i*(strip.numPixels()/(1.0 * whitepulse))/2.0);
          int k = strip.numPixels()/2+i*2;
          int l = strip.numPixels()/2-i*2;            
          if(j >= l && j <= k)
          {
            strip.setPixelColor(j, 255, 255, 255);
          }
          else
          {
            strip.setPixelColor(j, 0, 0, 0);   
          }
        }
        //delay(pause);
        strip.show();   
      }
      for(int i=0; i < strip.numPixels()/6; i++) 
      {
        for(int j=0; j< strip.numPixels(); j++) 
        {
          
          //int k = strip.numPixels()/2+(i*(strip.numPixels()/(1.0 * whitepulse))/2.0);
          //int l = strip.numPixels()/2-(i*(strip.numPixels()/(1.0 * whitepulse))/2.0);
          int k = strip.numPixels()/2+i*3;
          int l = strip.numPixels()/2-i*3;             
          if(j >= l && j <= k)
          {
            strip.setPixelColor(j, 0, 0, 0);
          }
          else
          {
            int randNumber = random(0, 255);
            if(randNumber > 127)
            {
              strip.setPixelColor(j, 255, 255, 255);
            }else{
              strip.setPixelColor(j, 0, 0, 0);             
            }
          }
        }
        delay(pause);
        strip.show();   
      }
      LEDLA = LEDLAdT*2;
    }
    //FADE OUT
    else if(LEDLA > 1 && LEDLA <= LEDLAdT)
    {
      uint8_t black[3] = {0,0,0};
      uint8_t fadecolor[3];
      double dT = ((float)LEDLA/(float)LEDLAdT)*100;
      for(int i=0; i< strip.numPixels(); i++) 
      {
        uint8_t stripcolor[3] = {LEDR[i],LEDG[i],LEDB[i]};        
        twocolorfade(dT, stripcolor, black, fadecolor); 
        strip.setPixelColor(i, fadecolor[0], fadecolor[1], fadecolor[2]);
      }
      strip.show();
      LEDLA -= 1;
    }
    //FADE IN
    else if(LEDLA > LEDLAdT)
    {
      double dT = ((float)(LEDLA-LEDLAdT)/(float)LEDLAdT)*100;
      uint8_t black[3] = {0,0,0};
      uint8_t fadecolor[3];
      for(int i=0; i< strip.numPixels(); i++) 
      {
        uint8_t stripcolor[3] = {LEDR[i],LEDG[i],LEDB[i]};                
        twocolorfade(dT, black, stripcolor, fadecolor); 
        strip.setPixelColor(i, fadecolor[0], fadecolor[1], fadecolor[2]);
      }
      strip.show();
      if(LEDLA == LEDLAdT+1)
      {
        LEDLA = 0;
      }
      else
      {
        LEDLA -= 1;
      }      
    }
  }
}






//WIKIPEDIA UPDATES
void messageReceived(String topic, String payload, char * bytes, unsigned int length) 
{
  if(LEDLA == 0)
  {   
    //PARSE CLIENT
    int Index1 = payload.indexOf(':');
    int Index2 = payload.indexOf(':', Index1+1);
    int Index3 = payload.indexOf(':', Index2+1);
    int Index4 = payload.indexOf(':', Index3+1);
    int Index5 = payload.indexOf(':', Index4+1);

    String city = payload.substring(Index1+2, Index2-6); //-6
    String edittype = payload.substring(Index2+2, Index3-8); 
    long wordcount = payload.substring(Index3+2, Index4-9).toFloat();
    double angle = payload.substring(Index4+2, Index5-10).toFloat();
    
    //JSON
    //Serial.println();
    //Serial.print(payload);
    //DynamicJsonBuffer jsonBuffer;
    //JsonObject& root = jsonBuffer.parseObject(payload);

    //const char* city = root["city"]; //const?
    //const char* edittype = root["type"]; //const?
    //long wordcount = root["change"]; //const?
    //double angle = root["azimuth"]; //const?

    int pulseWidth = (wordcount/wordcountMax)*pulseWidthMax;    
    pulseWidth = constrain(pulseWidth, pulseWidthMin, pulseWidthMax);
    
    //SET BLINK
    float k = angle*strip.numPixels()/180;
    int j = int(k +0.5);
    for(int i=0; i<pulseWidth; i++) 
    {
      int k = j - (pulseWidth/2) + i;
      if(k >= 0 && k < LEDS)
      {
        if(edittype == "edit")
        {
          LEDR[k] = 255;
          LEDG[k] = -255;
          LEDB[k] = -255;
          LEDdT[k] += pulseLength;
        }else{
          LEDR[k] = -255;
          LEDG[k] = 255;
          LEDB[k] = -255;
          LEDdT[k] += pulseLength;   
        }
      }     
    }

    //SET LOS ANGELES
    if(city == "Los Angeles")
    {
      LEDLA = LEDLAdT;
    }

  }
}
