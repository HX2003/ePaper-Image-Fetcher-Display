//Config
//ATtiny814
//Frequency 5MHz
//DAC disabled
#include <avr/sleep.h>
#include <Wire.h>
#include <SparkFun_MMA8452Q.h>
#define BATOUTEN 1
#define VBATCTRL 0
#define VBATSENSE 10 //PA3
#define PGOOD 9 //PA2 Low to indicate external power supply good
#define PCHG 8 //PA1 Low to indicate charging
#define ACCEL 2 //PA6

#define SLEEP_TIME 3200

int count = 0;
const int max_rtc_count = SLEEP_TIME / 32;
volatile int rtc_count = 0; //To enable sleep times of more than 32s
volatile byte INT_SRC = 0; //1 for pin change, 2 for rtc
volatile byte INT_FLAG = 1;

volatile byte PGOOD_INT_PIN;
volatile byte PCHG_INT_PIN;
volatile byte ACCEL_INT_PIN;

MMA8452Q accel;

void setup() {
  // set the data rate for the SoftwareSerial port
  Serial.begin(9600);
  Serial.println("Hello, world?");
  Serial.end();
  pinMode(VBATSENSE, INPUT);
  pinMode(PGOOD, INPUT);
  pinMode(PCHG, INPUT);

  digitalWrite(BATOUTEN, LOW);

  PGOOD_INT_PIN = digitalRead(PGOOD);
  PCHG_INT_PIN = digitalRead(PCHG);
  ACCEL_INT_PIN = digitalRead(ACCEL);

  Wire.begin();
  if (accel.begin(Wire, 0x1C) == false) {
    Serial.println("Accelerometer error");
  }
  Wire.end();

  PORTA.INTFLAGS = 0xff;
  PORTA.PIN1CTRL |= PORT_ISC0_bm; //PA1 sense BOTHEDGES
  PORTA.PIN2CTRL |= PORT_ISC0_bm; //PA2 sense BOTHEDGES
  //PORTA.PIN6CTRL |= PORT_ISC0_bm; //PA1 sense BOTHEDGES

  RTC_init();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
}
ISR(PORTA_PORT_vect) {
  byte flags = PORTA.INTFLAGS;
  PORTA.INTFLAGS = flags; //clear flags
  INT_SRC = 1;
  INT_FLAG = 1;
  if (flags & (1 << 2)) {
    PGOOD_INT_PIN = digitalRead(PGOOD);
  }
  if (flags & (1 << 1)) {
    PCHG_INT_PIN = digitalRead(PCHG);
  }
  if (flags & (1 << 6)) {
    ACCEL_INT_PIN = digitalRead(ACCEL);
  }
}
void RTC_init(void)
{
  /* Initialize RTC: */
  while (RTC.STATUS > 0)
  {
    ;                                   /* Wait for all register to be synchronized */
  }
  RTC.CLKSEL = RTC_CLKSEL_INT1K_gc;    /* 1.024kHz Internal Ultra-Low-Power Oscillator (OSCULP1K) */

  RTC.PITINTCTRL = RTC_PI_bm;           /* PIT Interrupt: enabled */

  //RTC.PITCTRLA = RTC_PERIOD_CYC1024_gc  //RTC Clock Cycles 32768, resulting in 32768/1024 = 1s
  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc /* RTC Clock Cycles 32768, resulting in 32768/1024 = 32s */
                 | RTC_PITEN_bm;                       /* Enable PIT counter: enabled */
}

ISR(RTC_PIT_vect)
{
  RTC.PITINTFLAGS = RTC_PI_bm;          /* Clear interrupt flag by writing '1' (required) */
  rtc_count++;
  if (rtc_count > max_rtc_count) {
    INT_FLAG = 1;
    INT_SRC = 2;
    rtc_count = 0;
  }
}

void loop() { // run over and over
  if (!INT_FLAG) {
    sleep_cpu();
  } else {
    delay(500); //Wait for power to stabilise
    pinMode(BATOUTEN, OUTPUT);
    pinMode(VBATCTRL, OUTPUT);
    digitalWrite(VBATCTRL, HIGH);
    pinMode(VBATSENSE, INPUT);
    delay(20);
    int batADC = analogRead(VBATSENSE);
    float batADCreading = batADC / 1024.0 * 3.30;
    float batVoltage = batADCreading * 1.47;
    digitalWrite(VBATCTRL, LOW);
    digitalWrite(BATOUTEN, HIGH); //Turning on ESP32
    Serial.begin(9600);
    delay(5000);
    Serial.println("test"); //Dumb text, first few characters not working?
    delay(100);
    Serial.println("");
    Serial.print("{\"v\":");
    Serial.print(batVoltage);
    Serial.print(",\"pgood\":");
    Serial.print(PGOOD_INT_PIN);
    Serial.print(", \"pchg\":");
    Serial.print(PCHG_INT_PIN);
    Serial.print(", \"src\":");
    Serial.print(INT_SRC);
    Serial.print(", \"count\":");
    Serial.print(count);
    Serial.println("}");

    /*
      Serial.print("Volt: " + String(batVoltage) + ",");
      Serial.print(String(PCHG_INT_PIN));
      Serial.print(", ");
      Serial.print(String(PGOOD_INT_PIN));
      Serial.print(", source: ");
      Serial.print(String(INT_SRC));
      Serial.print(", count: ");
      Serial.print(String(count));
      Serial.println();*/
    //char json[128];
    //char str_batVoltage[5];

    //dtostrf(batVoltage, 3, 2, str_batVoltage);
    //sprintf(json, "{\"v\": %s , \"pgood\": %i , \"pchg\": %d, \"src\": %d, \"count\": %d}", str_batVoltage, PGOOD_INT_PIN, PCHG_INT_PIN, INT_SRC, count);
    //String json = "{\"v\":" + String(batVoltage) + ", \"pgood\":" + String(PGOOD_INT_PIN) + ", \"pchg\":" + String(PCHG_INT_PIN) + ", \"src\":" + String(INT_SRC) + ", \"count\":" + String(count) + "}";

    //Serial.println(json);
    delay(15000);
    Serial.end();
    /*if(!PGOOD_INT_PIN){
      //External input source available, keep it on
      digitalWrite(BATOUTEN, HIGH); //Remain on ESP32
      }else{
      digitalWrite(BATOUTEN, LOW); //Turning off ESP32
      }*/
    digitalWrite(BATOUTEN, LOW);
    pinMode(4, INPUT);
    pinMode(5, INPUT);
    pinMode(BATOUTEN, INPUT);
    pinMode(VBATCTRL, INPUT);
    delay(500);
    count++;
    INT_FLAG = 0;
  }

}
