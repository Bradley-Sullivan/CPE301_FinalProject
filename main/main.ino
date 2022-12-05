#include <LiquidCrystal.h>
#include <Stepper.h>
#include <RTClib.h>
#include <string.h>
#include <DHT.h>

#define RBE       0x80
#define TBE       0x20

#define LCD_LEN   16
#define LCD_RS    12
#define LCD_EN    11
#define LCD_D4    5
#define LCD_D5    4
#define LCD_D6    3
#define LCD_D7    2

#define DHT_PIN   7
#define DHTTYPE   DHT11

#define REV_STEPS 2038

#define GLED_MASK 0x80
#define YLED_MASK 0x20
#define RLED_MASK 0x08
#define BLED_MASK 0x02

#define ST_BTN    0x80
#define RES_BTN   0x20
#define CTRL_BTN  0x08

#define WTR_MASK  0x01

// Registers for Digital Pins 30 - 37   (LEDs)
volatile unsigned char *PORT_C  = (unsigned char *) 0x28;
volatile unsigned char *DDR_C   = (unsigned char *) 0x27;
volatile unsigned char *PIN_C   = (unsigned char *) 0x26;

// Registers for Digital Pins 42 - 49   (Buttons)
volatile unsigned char *PORT_L  = (unsigned char *) 0x10B;
volatile unsigned char *DDR_L   = (unsigned char *) 0x10A;
volatile unsigned char *PIN_L   = (unsigned char *) 0x109;

// Registers for Analog Pins 0 - 7      (Level Sensor)
volatile unsigned char *PORT_F  = (unsigned char *) 0x31;
volatile unsigned char *DDR_F   = (unsigned char *) 0x30;
volatile unsigned char *PIN_F   = (unsigned char *) 0x2F;

// Registers for UART0                  (Serial Printing)
volatile unsigned char *UCSR_0A = (unsigned char *) 0x00C0;
volatile unsigned char *UCSR_0B = (unsigned char *) 0x00C1;
volatile unsigned char *UCSR_0C = (unsigned char *) 0x00C2;
volatile unsigned char *UBRR_0  = (unsigned char *) 0x00C4;
volatile unsigned char *UDR_0   = (unsigned char *) 0x00C6;

typedef enum STATE {DISABLED, IDLE, ERROR, RUNNING} STATE;

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Stepper step = Stepper(REV_STEPS, 28, 26, 24, 22);
DHT dht(DHT_PIN, DHTTYPE);
RTC_DS3231 rtc;

STATE dev_state = DISABLED;

char lcd_buf[LCD_LEN], err_msg[LCD_LEN];

const int temp_threshold = 69, wtr_threshold = 128;

void setup() {
  lcd.begin(16, 2);
  dht.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  lcd.clear();
  step.setSpeed(2);

  *DDR_C  |= GLED_MASK;
  *DDR_C  |= YLED_MASK;
  *DDR_C  |= RLED_MASK;
  *DDR_C  |= BLED_MASK;

  *PORT_L |= ST_BTN;
  *DDR_L  &= ~(ST_BTN);

  *PORT_L |= RES_BTN;
  *DDR_L  &= ~(RES_BTN);

  *PORT_L |= CTRL_BTN;
  *DDR_L  &= ~(CTRL_BTN);
}

void loop() {
  load_ht(lcd_buf);
  switch (dev_state) {
    case DISABLED:
      *PORT_C |= YLED_MASK;
      break;
    case IDLE:
      break;
    case ERROR:
      *PORT_C |= RLED_MASK;
      lcd.setCursor(0, 0);
      lcd.print(err_msg);
      break;
    case RUNNING:
      *PORT_C |= BLED_MASK;
      // start fan motor
      unsigned int wtr_level;    // read from A0
      if ((int)dht.readTemperature(true) < temp_threshold) {
        dev_state = IDLE;
      }
      if (wtr_level < wtr_threshold) {
        snprintf(err_msg, LCD_LEN, "(ERR) Low water!");
        dev_state = ERROR;
      }
      break;
  }
}

void load_ht(char *buf) {
  int h_read = (int)dht.readHumidity();
  int t_read = (int)dht.readTemperature(true);

  snprintf(buf, LCD_LEN, "H:%d T:%dF", h_read, t_read);
}

void UART0_INIT(unsigned long baud) {
  unsigned long FCPU = 16000000;
  unsigned int tbaud = (FCPU / 16 / baud - 1);

  *UCSR_0A = 0x20;
  *UCSR_0B = 0x18;
  *UCSR_0C = 0x06;
  *UBRR_0  = tbaud;
}

void UART0_PUTCHAR(unsigned char c) {
  while ((*UCSR_0A * TBE) == 0) {};
  *UDR_0 = c;
}

void UART0_PUTSTR(unsigned char *s, int len) {
  for (int i = 0; i < len && s[i] != '\0'; i++) {
    while ((*UCSR_0A * TBE) == 0) {};
    *UDR_0 = s[i];
  }
}