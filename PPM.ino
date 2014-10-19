/* This file is part of the mod Razor AHRS Firmware */
/* Based on this great example:                     */
/*   https://code.google.com/p/generate-ppm-signal  */
/* Mod to use the internal 8MHz osc.                */
/* eklex - 2014/10/18                               */
/* Released under GNU GPL v3.0 as original firmware */

/*****************************************************************/
/* PPM OPTIONS                                                   */
/*****************************************************************/
#define chanel_number 6  //set the number of chanels
#define default_servo_value 1500  //set the default servo value
#define chanel_max  2000
#define chanel_min  1000
#define PPM_FrLen 22500  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 11  //set PPM signal output pin on the arduino

// define the clock frequency in use
#define CLK_8MHZ
#undef  CLK_16MHZ

static int ppm[chanel_number];

void ppm_setup()
{
  //initialize default ppm values
  for(int i=0; i<chanel_number; i++)
  {
    ppm[i]= default_servo_value;
  }

  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)
  
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;
  
  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
                          //              1,0 microseconds at 8mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

void ppm_main()
{
  // Roll / Ail
  ppm[0] = map(constrain(TO_DEG(roll), -90, 90), -90, 90, chanel_min, chanel_max);
  // Pitch / Ele
  ppm[1] = map(constrain(TO_DEG(pitch), -90, 90), -90, 90, chanel_max, chanel_min);
  // Throttle
  ppm[2] = 1500;
  // Yaw / Rud
  ppm[3] = map(constrain(TO_DEG(yaw), -180, 180), -180, 180, chanel_min, chanel_max);
}

ISR(TIMER1_COMPA_vect)
{
  static boolean state = true;
  
  TCNT1 = 0;
  
  if(state) //start pulse
  {
    digitalWrite(sigPin, onState);
#ifdef CLK_16MHZ
    OCR1A = PPM_PulseLen * 2;
#else
  #ifdef CLK_8MHZ
    OCR1A = PPM_PulseLen;
  #endif
#endif
    state = false;
  }
  else //end pulse and calculate when to start the next pulse
  {
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(sigPin, !onState);
    state = true;

    if(cur_chan_numb >= chanel_number)
    {
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPM_PulseLen;
#ifdef CLK_16MHZ
      OCR1A = (PPM_FrLen - calc_rest) * 2;
#else
  #ifdef CLK_8MHZ
      OCR1A = (PPM_FrLen - calc_rest);
  #endif
#endif
      calc_rest = 0;
    }
    else
    {
#ifdef CLK_16MHZ
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * 2;
#else
  #ifdef CLK_8MHZ
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen);
  #endif
#endif
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}

