// Shota Nakaoji

#include <LiquidCrystal.h>

LiquidCrystal lcd(30, 31, 32, 33, 34, 35);


enum State {
  OFF,
  IDLE,
  ACTIVE,
  ERROR
};

volatile State currentState = OFF;


// PORTA (pins 22-25) LEDs
volatile unsigned char* ddr_a  = (unsigned char*) 0x21;
volatile unsigned char* port_a = (unsigned char*) 0x22;

// PORTB (pin12) buzzer
volatile unsigned char* ddr_b  = (unsigned char*) 0x24;
volatile unsigned char* port_b = (unsigned char*) 0x25;

// PORTE (pins2,3)
volatile unsigned char* ddr_e  = (unsigned char*) 0x2D;
volatile unsigned char* port_e = (unsigned char*) 0x2E;
volatile unsigned char* pin_e  = (unsigned char*) 0x2C;

// PORTG (pin4)
volatile unsigned char* ddr_g  = (unsigned char*) 0x33;
volatile unsigned char* port_g = (unsigned char*) 0x34;
volatile unsigned char* pin_g  = (unsigned char*) 0x32;


// ADC registers
volatile unsigned char* admux  = (unsigned char*) 0x7C;
volatile unsigned char* adcsra = (unsigned char*) 0x7A;
volatile unsigned char* adch   = (unsigned char*) 0x79;
volatile unsigned char* adcl   = (unsigned char*) 0x78;


void setup() {

  Serial.begin(9600);

  lcd.begin(16, 2);

  // LEDs
  *ddr_a |= (1 << 0);
  *ddr_a |= (1 << 1);
  *ddr_a |= (1 << 2);
  *ddr_a |= (1 << 3);

  // Buzzer (pin12)
  *ddr_b |= (1 << 6);

  // ON button
  *ddr_e &= ~(1 << 4);
  *port_e |= (1 << 4);

  // OFF button
  *ddr_e &= ~(1 << 5);
  *port_e |= (1 << 5);

  // RESET button
  *ddr_g &= ~(1 << 5);
  *port_g |= (1 << 5);

  attachInterrupt(digitalPinToInterrupt(2), startSystem, FALLING);

  adc_init();

  updateOutputs();
}


void loop() {

  switch(currentState) {

    case OFF:

      lcd.setCursor(0,0);
      lcd.print("SYSTEM OFF    ");

      lcd.setCursor(0,1);
      lcd.print("                ");

      break;


    case IDLE:

      {

        unsigned int adcValue = adc_read();

        Serial.println(adcValue);

        lcd.setCursor(0,0);
        lcd.print("HUMIDITY OK   ");

        lcd.setCursor(0,1);
        lcd.print("H: ");
        lcd.print(adcValue);
        lcd.print("      ");


        // OFF button
        if (((*pin_e) & (1 << 5)) == 0) {

          currentState = OFF;
          updateOutputs();
        }

        // ERROR condition
        else if (adcValue < 100 || adcValue > 900) {

          currentState = ERROR;
          updateOutputs();
        }

        // ACTIVE condition
        else if (adcValue < 500) {

          currentState = ACTIVE;
          updateOutputs();
        }
      }

      break;


    case ACTIVE:

      {

        unsigned int adcValue = adc_read();

        Serial.println(adcValue);

        lcd.setCursor(0,0);
        lcd.print("AIR TOO DRY   ");

        lcd.setCursor(0,1);
        lcd.print("H: ");
        lcd.print(adcValue);
        lcd.print("      ");


        // buzzer sound
        *port_b |= (1 << 6);

        for (volatile long i = 0; i < 50000; i++);

        *port_b &= ~(1 << 6);

        for (volatile long i = 0; i < 50000; i++);


        // OFF button
        if (((*pin_e) & (1 << 5)) == 0) {

          currentState = OFF;
          updateOutputs();
        }

        // ERROR condition
        else if (adcValue < 100 || adcValue > 900) {

          currentState = ERROR;
          updateOutputs();
        }

        // return to IDLE
        else if (adcValue >= 500) {

          currentState = IDLE;
          updateOutputs();
        }
      }

      break;


    case ERROR:

      lcd.setCursor(0,0);
      lcd.print("SENSOR ERROR  ");

      lcd.setCursor(0,1);
      lcd.print("PRESS RESET   ");


      // buzzer sound
      *port_b |= (1 << 6);

      for (volatile long i = 0; i < 50000; i++);

      *port_b &= ~(1 << 6);

      for (volatile long i = 0; i < 50000; i++);


      // RESET button
      if (((*pin_g) & (1 << 5)) == 0) {

        currentState = IDLE;
        updateOutputs();
      }

      // OFF button
      else if (((*pin_e) & (1 << 5)) == 0) {

        currentState = OFF;
        updateOutputs();
      }

      break;
  }
}


void startSystem() {

  if (currentState == OFF) {

    currentState = IDLE;
    updateOutputs();
  }
}


void updateOutputs() {

  // turn OFF all LEDs
  *port_a &= ~(1 << 0);
  *port_a &= ~(1 << 1);
  *port_a &= ~(1 << 2);
  *port_a &= ~(1 << 3);

  switch(currentState) {

    case OFF:

      *port_a |= (1 << 0);

      break;


    case IDLE:

      *port_a |= (1 << 1);

      break;


    case ACTIVE:

      *port_a |= (1 << 2);

      break;


    case ERROR:

      *port_a |= (1 << 3);

      break;
  }
}


void adc_init() {

  *admux = 0x40;

  *adcsra = 0x87;
}


unsigned int adc_read() {

  *adcsra |= (1 << 6);

  while ((*adcsra & (1 << 6)) != 0);

  unsigned int low  = *adcl;
  unsigned int high = *adch;

  return (high << 8) | low;
}
