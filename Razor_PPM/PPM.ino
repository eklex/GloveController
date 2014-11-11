/* This file is part of the mod Razor AHRS Firmware */
/* eklex - 2014/10/18                               */
/* Released under GNU GPL v3.0 as original firmware */

/****************** PPM CONFIGURATION ****************************/

#define ppm_count         4     /* PPM channel count */
#define ppm_default_val   1500  /* PPM default length (us) */
#define ppm_max           2000  /* PPM max length (us) */
#define ppm_min           1000  /* PPM min length (us) */
#define ppm_frame_len     22500 /* PPM frame length (us) */
#define ppm_pulse_len     400   /* PPM pulse length (us) */
#define ppm_polarity      1     /* PPM pulse polarity (1=pos, 0=neg) */
#define ppm_pin           11    /* PPM signal output pin */

/*****************************************************************/

static uint16_t ppm[ppm_count];

void ppm_setup()
{
  /* Initialize default ppm values */
  for(int i=0; i<ppm_count; i++)
  {
    ppm[i]= ppm_default_val;
  }
  
  /* Configure the output pin */
  pinMode(ppm_pin, OUTPUT);
  digitalWrite(ppm_pin, !ppm_polarity);
  
  /**************************/
  /* Enter CRITICAL section */
  /**************************/
  
  /* Disable global interrupt */
  cli();
  
  /* Reset Timer1's configuration */
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 0;
  OCR1B  = 0;
  TIMSK1 = 0;
  TIFR1  = 0;
  
  /* Set timer configuration */
  TCCR1B |= (1 << WGM12) | /* CTC mode enable */
            (1 << CS11);   /* 8 prescaler: 0.5us at 16MHz 
                            * or 1us at 8MHz */
  
  /* Reset timer */
  TCNT1 = 0;
  
  /* Set compare register to its max */
  OCR1A = 0xFFFF;
  
  /* Enable timer compare interrupt */
  TIMSK1 |= (1 << OCIE1A);
  
  /* Enable global interrupt */
  sei();
  
  /*************************/
  /* Exit CRITICAL section */
  /*************************/
}

void ppm_main()
{
  /* Roll / Ail */
  ppm[0] = (uint16_t)map(constrain(TO_DEG(roll), -90, 90), -90, 90, ppm_min, ppm_max);
  /* Pitch / Ele */
  ppm[1] = (uint16_t)map(constrain(TO_DEG(pitch), -90, 90), -90, 90, ppm_max, ppm_min);
  /* Throttle */
  /* not use */
  ppm[2] = ppm_default_val;
  /* Yaw / Rud */
  ppm[3] = (uint16_t)map(constrain(TO_DEG(yaw), -180, 180), -180, 180, ppm_min, ppm_max);
}

/* Timer 1 reg OCR1A comparaison interrupt */
/* The PPM signal is created by this interrupt handler. */
ISR(TIMER1_COMPA_vect)
{
  static boolean state = true;
  static uint16_t counter = 0;
  static uint8_t cur_chan_numb = 0;
  static uint16_t calc_rest = 0;
  
  /* Reset the timer */
  TCNT1 = 0;
  
  if(state) /* start pulse */
  {
    digitalWrite(ppm_pin, ppm_polarity);
    counter = ppm_pulse_len;
    state = false;
  }
  else /* end pulse and calculate when to start the next pulse */
  {
    digitalWrite(ppm_pin, !ppm_polarity);
    state = true;

    if(cur_chan_numb >= ppm_count)
    {
      cur_chan_numb = 0;
      calc_rest = calc_rest + ppm_pulse_len;
      counter = (ppm_frame_len - calc_rest);
      calc_rest = 0;
    }
    else
    {
      counter = (ppm[cur_chan_numb] - ppm_pulse_len);
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }
  }
#if F_CPU == 16000000
  OCR1A = counter * 2;
#elif F_CPU == 8000000
  OCR1A = counter;
#else
  #error "CPU frequency not supported";
#endif
}
