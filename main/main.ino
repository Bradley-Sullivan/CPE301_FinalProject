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
#define LCD_D4    6
#define LCD_D5    5
#define LCD_D6    4
#define LCD_D7    3

#define DHT_PIN   7
#define DHTTYPE   DHT11

#define REV_STEPS 2038

#define FAN_MASK  0x10

#define GLED_MASK 0x80
#define YLED_MASK 0x20
#define RLED_MASK 0x08
#define BLED_MASK 0x02

#define ST_BTN    0x08
#define RES_BTN   0x04
#define CTRL_BTN  0x02

#define PCI_MASK  0x0C    // Use to enable PC-Interrupts on PCINT2 and PCINT3 (ST and RES buttons)

// Registers for Digital Pins 50 - 53, 10 - 13 (Buttons)
volatile unsigned char *PORT_B  = (unsigned char *) 0x25;
volatile unsigned char *DDR_B   = (unsigned char *) 0x24;
volatile unsigned char *PIN_B   = (unsigned char *) 0x23;

// Registers for Digital Pins 30 - 37         (LEDs)
volatile unsigned char *PORT_C  = (unsigned char *) 0x28;
volatile unsigned char *DDR_C   = (unsigned char *) 0x27;
volatile unsigned char *PIN_C   = (unsigned char *) 0x26;

// Registers for ADC                          (Level Sensor)
volatile unsigned char* my_ADMUX = (unsigned char*) 0x7C;
volatile unsigned char* my_ADCSRB = (unsigned char*) 0x7B;
volatile unsigned char* my_ADCSRA = (unsigned char*) 0x7A;
volatile unsigned int* my_ADC_DATA = (unsigned int*) 0x78;

// Registers for UART0                        (Serial Printing)
volatile unsigned char *UCSR_0A = (unsigned char *) 0x00C0;
volatile unsigned char *UCSR_0B = (unsigned char *) 0x00C1;
volatile unsigned char *UCSR_0C = (unsigned char *) 0x00C2;
volatile unsigned char *UBRR_0  = (unsigned char *) 0x00C4;
volatile unsigned char *UDR_0   = (unsigned char *) 0x00C6;

// Registers for Pin Change Interrupts
volatile unsigned char *myPCICR   = (unsigned char *) 0x68;
volatile unsigned char *myPCIFR   = (unsigned char *) 0x3B;
volatile unsigned char *myPCMSK0  = (unsigned char *) 0x6B;


typedef enum STATE {DISABLED, IDLE, ERROR, RUNNING} STATE;

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
Stepper step = Stepper(REV_STEPS, 28, 26, 24, 22);
DHT dht(DHT_PIN, DHTTYPE);
RTC_DS3231 rtc;

STATE dev_state = DISABLED;
STATE prev_state;

char lcd_buf[LCD_LEN], err_msg[LCD_LEN];
char state_map[4][16] = {"(DISABLED)", "IDLE", "ERROR", "RUNNING"};
unsigned char led_mask_map[4] = {YLED_MASK, GLED_MASK, RLED_MASK, BLED_MASK};

const unsigned int temp_threshold = 42, wtr_threshold = 400, max_steps = 200;
unsigned int wtr_level = 0, update_timer = 0, step_ct = 0;
int step_dir = 1;

void setup() {
  lcd.begin(16, 2);
  dht.begin();
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  lcd.clear();
  step.setSpeed(2);

  IO_INIT();
  ADC_INIT();
  UART0_INIT(19200);
}

void loop() {
  DateTime now = rtc.now();

  wtr_level = ADC_READ(0);
  if (now.second() == 0) {
    load_ht(lcd_buf);
  }

  lcd.setCursor(0, 1);
  lcd.print(state_map[dev_state]);

  LED_UPDATE(dev_state);

  switch (dev_state) {
    case DISABLED:
      *PORT_B &= ~FAN_MASK;
      break;
    case IDLE:
      *PORT_B &= ~FAN_MASK;
      lcd.setCursor(0, 0);
      lcd.print(lcd_buf);
      if ((int)dht.readTemperature(true) >= temp_threshold) {
        dev_state = RUNNING;
      }
      if (wtr_level < wtr_threshold) {
        snprintf(err_msg, LCD_LEN, "Low water!");
        dev_state = ERROR;
      }
      break;
    case ERROR:
      *PORT_B &= ~FAN_MASK;
      lcd.setCursor(0, 0);
      lcd.print(err_msg);
      break;
    case RUNNING:
      lcd.setCursor(0, 0);
      lcd.print(lcd_buf);
      *PORT_B |= FAN_MASK;
      if ((unsigned int)dht.readTemperature(true) < temp_threshold) {
        dev_state = IDLE;
        *PORT_C &= ~BLED_MASK;
      }
      if (wtr_level < wtr_threshold) {
        snprintf(err_msg, LCD_LEN, "Low water!");
        dev_state = ERROR;
        *PORT_C &= ~BLED_MASK;
      }
      break;
  }

  if (dev_state != ERROR) {
    if (*PIN_B & CTRL_BTN) {
      unsigned char serial_buf[64];
      snprintf(serial_buf, 64, "\nVENT POSITION UPDATED\n");
      UART0_PUTSTR(serial_buf, strlen(serial_buf));
      step.step(1);
    }
  }

  if (prev_state != dev_state) {
    unsigned char serial_buf[256];
    snprintf(serial_buf, 256, "\nSTATE TRANSISTION: %s -> %s", state_map[prev_state], state_map[dev_state]);
    UART0_PUTSTR(serial_buf, strlen(serial_buf));
    snprintf(serial_buf, 256, "\nCurrent Time: %02d:%02d:%02d\nCurrent Date: %d/%d/%d\n", \
            now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    UART0_PUTSTR(serial_buf, strlen(serial_buf));
    if (dev_state == IDLE || dev_state == RUNNING) load_ht(lcd_buf);
    lcd.clear();
  }

  prev_state = dev_state;
}

ISR(PCINT0_vect) {
  if (*PIN_B & RES_BTN) {
    if (dev_state == ERROR) {
      dev_state = IDLE;
    }
  } else if (*PIN_B & ST_BTN) {
    if (dev_state == RUNNING || dev_state == IDLE || dev_state == ERROR) {
      dev_state = DISABLED;
    } else if (dev_state == DISABLED) {
      dev_state = RUNNING;
    }
  }
}

void load_ht(char *buf) {
  int h_read = (int)dht.readHumidity();
  int t_read = (int)dht.readTemperature(true);

  snprintf(buf, LCD_LEN, "H:%d T:%dF", h_read, t_read);
}

void LED_UPDATE(STATE state) {
  *PORT_C = led_mask_map[state];
}

void IO_INIT() {
  *DDR_C  |= GLED_MASK;
  *DDR_C  |= YLED_MASK;
  *DDR_C  |= RLED_MASK;
  *DDR_C  |= BLED_MASK;
  *DDR_B  |= FAN_MASK;

  *PORT_B |= ST_BTN;
  *DDR_B  &= ~(ST_BTN);

  *PORT_B |= RES_BTN;
  *DDR_B  &= ~(RES_BTN);

  *PORT_B |= CTRL_BTN;
  *DDR_B  &= ~(CTRL_BTN);

  *myPCICR |= 0x01;
  *myPCMSK0 |= PCI_MASK;
}

void ADC_INIT() {
  *my_ADCSRA |=  0b10000000; 
  *my_ADCSRA &=  0b11011111; 
  
  *my_ADCSRB &=  0b11110111; 
  *my_ADCSRB &=  0b11111000; 
  
  *my_ADMUX  &=  0b01111111; 
  *my_ADMUX  |=  0b01000000; 
  *my_ADMUX  &=  0b11011111; 
  *my_ADMUX  &=  0b11100000;
}

unsigned int ADC_READ(unsigned char adc_channel_num) {
  *my_ADMUX  &= 0b11100000;
  *my_ADCSRB &= 0b11110111;
  
  if(adc_channel_num > 7) {
    adc_channel_num -= 8;
    *my_ADCSRB |= 0b00001000;
  }
  
  *my_ADMUX  += adc_channel_num;
  *my_ADCSRA |= 0x40;
  
  while((*my_ADCSRA & 0x40) != 0);
  
  return *my_ADC_DATA;
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
  while ((*UCSR_0A & TBE) == 0) {};
  *UDR_0 = c;
}

void UART0_PUTSTR(unsigned char *s, int len) {
  while ((*UCSR_0A & TBE) == 0) {};
  for (int i = 0; i < len && s[i] != '\0'; i++) {
    while ((*UCSR_0A & TBE) == 0) {};
    *UDR_0 = s[i];
  }
}