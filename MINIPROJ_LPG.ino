/*
  ESP32 MQ6 LPG Gas and PPM Calculation
  http:://www.electronicwings.com
*/
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Arduino.h>
#include "HX711.h"
#include <LiquidCrystal.h>

const int rs = 22, en = 21, d4 = 1, d5 = 3, d6 = 5, d7 = 17;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


#define BLYNK_TEMPLATE_ID "TMPL3wy5wBjpR"
#define BLYNK_TEMPLATE_NAME "LPG MONITORING SYSTEM"

// Wi-Fi credentials
char ssid[] = "SANCH_1809";
char password[] = "sanchit18";

// Blynk authentication token
char auth[] = "_Rnc_rEHz62iePe1p_TWmIVreTySVqri";

#define BUZZER_PIN 13

byte MQ6_Pin = A0;          /* Define A0 for MQ Sensor Pin */
float Referance_V = 3300.0; /* ESP32 Referance Voltage in mV */
float RL = 1.0;             /* In Module RL value is 1k Ohm */
float Ro = 10.0;            /* The Ro value is 10k Ohm */
float mVolt = 0.0;
const float Ro_clean_air_factor = 10.0;
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 16;
const int LOADCELL_SCK_PIN = 4;

HX711 scale;

void setup() 
{
  Serial.begin(9600);       /* Set baudrate to 9600 */

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  pinMode(BUZZER_PIN, OUTPUT); 
  pinMode(MQ6_Pin, INPUT);  /* Define A0 as a INPUT Pin */
  delay(500);
  // Serial.println("Wait for 30 sec warmup");
  // delay(30000);             /* Set the warmup delay wich is 30 Sec */
  // Serial.println("Warmup Complete");

  for(int i=0; i<30; i++)
  {
    mVolt += Get_mVolt(MQ6_Pin);
  }
  mVolt = mVolt /30.0;      /* Get the volatage in mV for 30 Samples */
  Serial.print("Voltage at A0 Pin = ");
  Serial.print(mVolt);
  Serial.println("mVolt");
  Serial.print("Rs = ");
  Serial.println(Calculate_Rs(mVolt));
  Ro = Calculate_Rs(mVolt) / Ro_clean_air_factor;
  Serial.print("Ro = ");
  Serial.println(Ro);
  Serial.println(" ");
  mVolt = 0.0;

  //Load Cell
  Serial.println("HX711 Demo");

  Serial.println("Initializing the scale");

  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the scale:");
  Serial.print("read: \t\t");
  Serial.println(scale.read());      // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));   // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
            // by the SCALE parameter (not set yet)
            
  scale.set_scale(98.248);
  //scale.set_scale(-471.497);                      // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();               // reset the scale to 0

  Serial.println("After setting up the scale:");

  Serial.print("read: \t\t");
  Serial.println(scale.read());                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(scale.read_average(20));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(scale.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(scale.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
            // by the SCALE parameter set with set_scale

  Serial.println("Readings:");
  // Set up Blynk
  Blynk.begin(auth, ssid, password);
  Serial.println("Blynk connected");
}

void loop() 
{
  for(int i=0; i<500; i++)
  {
    mVolt += Get_mVolt(MQ6_Pin);
  }
  mVolt = mVolt/500.0;      /* Get the volatage in mV for 500 Samples */
  Serial.print("Voltage at A0 Pin = ");
  Serial.print(mVolt);      /* Print the mV in Serial Monitor */
  Serial.println(" mV");

  float Rs = Calculate_Rs(mVolt);
   Serial.print("Rs = ");
   Serial.println(Rs);       /* Print the Rs value in Serial Monitor */
  float Ratio_RsRo = Rs/Ro;

   Serial.print("RsRo = ");
   Serial.println(Ratio_RsRo);

  int danger_limit = 50;

  Serial.print("LPG ppm = ");
  unsigned int LPG_ppm = LPG_PPM(Ratio_RsRo);
  unsigned int danger_percent = (LPG_ppm)/0.5;
  Serial.println(LPG_ppm);   /* Print the Gas PPM value in Serial Monitor */
  Blynk.virtualWrite(V0, danger_percent);
  lcd.setCursor(0,0);
  lcd.print("Dang_per:");
  lcd.print(danger_percent);

  WidgetLED LED(V3);
  if (danger_percent> 15)
  {
    lcd.print("(S)");
    Blynk.logEvent("gas_alert", "WARNING! LEAKAGE DETECTED!");
    LED.on();
    Serial.println("LPG level is Alarming!!!");
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else
  {
    lcd.print("(S)");
    LED.off();
  }
  Serial.print("one reading:\t");
  Serial.print(scale.get_units(), 1);
  Serial.print("\t| average:\t");
  Serial.println(scale.get_units(10), 5);
  unsigned int Level_read = scale.get_units(10);
  // if(Level_read<)
  // {
  //   Level_read = 0;
  // }
  // if(Level_read>12000)
  // {
  //   Level_read = 1000;
  // }
  int LPG_percent = ((Level_read)/10); 
  lcd.setCursor(0,1);
  lcd.print("Quantity:");
  lcd.print(LPG_percent);
  Blynk.virtualWrite(V1, LPG_percent);

  scale.power_down();             // put the ADC in sleep mode
  delay(5000);
  scale.power_up();
     
  WidgetLED LED1(V4);
  WidgetLED LED2(V5);

  if(LPG_percent <15)
  {
    Blynk.logEvent("orange_alert", "WARNING! ORANGE ALERT!");
    LED1.on();
    Serial.println("ORANGE ALERT!!!");
    lcd.print("(O)");
  }
  else
  {
    LED1.off();
  }
  if(LPG_percent<5)
  {
    Blynk.logEvent("red_alert", "WARNING! RED ALERT!");
    LED2.on();
    Serial.println("RED ALERT!!!");
    lcd.print("(R)");
  }
  else
  {
    LED2.off();
  }
  Serial.println("");
  mVolt = 0.0;              /* Set the mVolt variable to 0 */
  delay(5000);
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
}

float Calculate_Rs(float Vo) 
{
/* 
 *  Calculate the Rs value
 *  The equation Rs = (Vc - Vo)*(RL/Vo)
 */
  float Rs = (Referance_V - Vo) * (RL / Vo); 
  return Rs;
}


unsigned int LPG_PPM(float RsRo_ratio) 
{
/*
 * Calculate the PPM using below equation
 * LPG ppm = [(Rs/Ro)/18.446]^(1/-0.421)
 */
  float ppm;
  ppm = pow((RsRo_ratio/18.446), (1/-0.421));
  return (unsigned int) ppm;
}


float Get_mVolt(byte AnalogPin) 
{
/* Calculate the ADC Voltage using below equation
 *  mVolt = ADC_Count * (ADC_Referance_Voltage / ADC_Resolution)
 */
  int ADC_Value = analogRead(AnalogPin); 
  delay(1);
  float mVolt = ADC_Value * (Referance_V / 4096.0);
  return mVolt;
}