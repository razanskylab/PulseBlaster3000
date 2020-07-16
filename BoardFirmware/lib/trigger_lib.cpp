#include "trigger_lib.h"

//NanoDelay %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// delay for a given number of nano seconds
// less sensitive to interrupts and DMA
// max delay is 4 seconds
// NOTE:  minimum pulse width is ~700 nsec, accuracy is ~ -0/+40 ns
// NOTE:  you can't trust this code:
//        compiler or library changes will change timing overhead
//        CPU speed will effect timing

// prepare before, so less delay later
static uint32_t nano_ticks;

// constexpr double CLOCK_RATE = 240.0E6; // MCU clock rate - measure it for best accuracy
constexpr double CLOCK_RATE = 240.0E6; // MCU clock rate - measure it for best accuracy
// constexpr unsigned NANO_OVERHEAD = 470;         // overhead - adjust as needed
constexpr unsigned NANO_OVERHEAD = 130; // overhead - adjust as needed
// constexpr unsigned NANO_JITTER = 18;            // adjusts for jitter prevention - leave at 18
constexpr unsigned NANO_JITTER = 0; // adjusts for jitter prevention - leave at 18

void setup_nano_delay(uint32_t nanos)
{
  // set up cycle counter
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;

  if (nanos < NANO_OVERHEAD) // we can't do less than this
    nanos = NANO_OVERHEAD;

  // how many cycles to wait
  nano_ticks = ((nanos - NANO_OVERHEAD) / (1.0E9 / CLOCK_RATE)) + .5;

  if (nano_ticks < NANO_JITTER)
    nano_ticks = NANO_JITTER;

} // Setup_Nano_Delay()

// Do the delay specified above.
// You may want to disable interrupts before and after
FASTRUN void wait_nano_delay(void)
{
  uint32_t start_time = ARM_DWT_CYCCNT;
  uint32_t loop_ticks = nano_ticks - NANO_JITTER;

  // loop until time is almost up
  while ((ARM_DWT_CYCCNT - start_time) < loop_ticks)
  {
    // could do other things here
  }

  if (NANO_JITTER)
  {                      // compile time option
    register unsigned r; // for debugging

    // delay for the remainder using single instructions
    switch (r = (nano_ticks - (ARM_DWT_CYCCNT - start_time)))
    {
    case 18:
      __asm__ volatile("nop"
                       "\n\t");
    case 17:
      __asm__ volatile("nop"
                       "\n\t");
    case 16:
      __asm__ volatile("nop"
                       "\n\t");
    case 15:
      __asm__ volatile("nop"
                       "\n\t");
    case 14:
      __asm__ volatile("nop"
                       "\n\t");
    case 13:
      __asm__ volatile("nop"
                       "\n\t");
    case 12:
      __asm__ volatile("nop"
                       "\n\t");
    case 11:
      __asm__ volatile("nop"
                       "\n\t");
    case 10:
      __asm__ volatile("nop"
                       "\n\t");
    case 9:
      __asm__ volatile("nop"
                       "\n\t");
    case 8:
      __asm__ volatile("nop"
                       "\n\t");
    case 7:
      __asm__ volatile("nop"
                       "\n\t");
    case 6:
      __asm__ volatile("nop"
                       "\n\t");
    case 5:
      __asm__ volatile("nop"
                       "\n\t");
    case 4:
      __asm__ volatile("nop"
                       "\n\t");
    case 3:
      __asm__ volatile("nop"
                       "\n\t");
    case 2:
      __asm__ volatile("nop"
                       "\n\t");
    case 1:
      __asm__ volatile("nop"
                       "\n\t");
    default:
      break;
    } // switch()
  }   // if
} // Nano_Delay()

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
TeensyTrigger::TeensyTrigger() {}
TeensyTrigger::~TeensyTrigger() {}

// setup IO PINs ---------------------------------------------------------------
void TeensyTrigger::setup_io_pins()
{
  // set two trigger inputs as digital input pins
  pinMode(TRIG_IN_PINS[0], INPUT);
  pinMode(TRIG_IN_PINS[1], INPUT);
  // set 8 trigger outputs and corresponding LEDs as digital output pins
  for (uint8_t i = 0; i < 8; i++)
  {
    pinMode(TRIG_OUT_PINS[i], OUTPUT);
    pinMode(LED_OUT_PINS[i], OUTPUT);
  }
  this->show_led_welcome();
}

// make led blink --------------------------------------------------------------
void TeensyTrigger::set_all_led_brightness(uint8_t ledPower)
{
  for (int iLed = 0; iLed < 5; iLed++)
  {
    analogWrite(LED_OUT_PINS[iLed], ledPower);
  }
  analogWrite(LED_OUT_PINS[6], ledPower);
}

// make led blink --------------------------------------------------------------
void TeensyTrigger::do_nothing()
{
  if ((millis() - lastNothingCheck) >= 50)
  {
    lastNothingCheck = millis();
    if ((ledBrightness < 200) && fadeIn)
      ledBrightness++;
    else if (ledBrightness == 200)
    {
      fadeIn = 0;
      ledBrightness--;
    }
    else if (ledBrightness == 0)
    {
      fadeIn = true;
      ledBrightness++;
    }
    else
      ledBrightness--;
    set_all_led_brightness(ledBrightness);
  }
}

// make led blink --------------------------------------------------------------
void TeensyTrigger::show_led_welcome()
{
  for (uint8_t i = 0; i < 3; i++)
  { // blink 3 times
    for (uint8_t iPower = 0; iPower < 255; iPower++)
    {
      this->set_all_led_brightness(iPower);
      delay(1);
    }
    for (uint8_t iPower = 255; iPower > 0; iPower--)
    {
      this->set_all_led_brightness(iPower);
      delay(2);
    }
  }
  LED_PORT = 0b00000000; // disable all LEDs for now
}

// check for new command -------------------------------------------------------
FASTRUN uint_fast8_t TeensyTrigger::check_for_serial_command()
{
  // read a command if one was send
  if (Serial.available() >= 2)
  {
    this->currentCommand = serial_read_16bit_no_wait(); // read the incoming byte:
    return 1;
  }
  else
    return 0;
}

FASTRUN void TeensyTrigger::scope()
{
  uint_fast32_t lastCommandCheck = 0;
  uint_fast32_t triggerCounter = 0;
  uint_fast8_t doTrigger = true;
  uint32_t lastTriggerTime = 0;

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // read confing as send from matlab
  uint_fast32_t triggerPeriod = serial_read_32bit(); // trigger period in us
  // trigger how many times? per AOD cycle
  uint_fast32_t trigOnTime = serial_read_32bit();
  uint_fast32_t nTrigger = serial_read_32bit();

  setup_nano_delay(trigOnTime);
  serial_write_16bit(TRIGGER_STARTED); // send the "we are triggering" command

  while (doTrigger)
  {
    while ((micros() - lastTriggerTime) < triggerPeriod)
      {}; // here we wait...

    lastTriggerTime = micros();
    TRIG_OUT_PORT = 0b11111111; // all high
    wait_nano_delay();
    TRIG_OUT_PORT = 0b00000000; // all low
    triggerCounter++; 
    // if nTrigger = 0 we trigger forevere, otherwise check if we are done
    if (nTrigger && (triggerCounter >= nTrigger))
      doTrigger = 0;

    // check if we got a new serial command to stop triggering
    // COMMAND_CHECK_INTERVALL is high, so we only check once in a while
    if ((millis() - lastCommandCheck) >= COMMAND_CHECK_INTERVALL)
    {
      lastCommandCheck = millis();
      if (Serial.available() >= 2)
      {
        this->currentCommand = serial_read_16bit_no_wait();
        if (this->currentCommand == DISABLE_TRIGGER)
          doTrigger = false;
      }
    }
  } // while (doTrigger)

  LED_PORT = 0b00000000; // disable all LEDs
  ledBrightness = 0;
  serial_write_16bit(DONE); // send the "ok, we are done" command
  serial_write_32bit(triggerCounter);
  this->currentCommand = DO_NOTHING; // exit state machine
}


// custom trigger function for chen to trigger AOD and camera only -------------
FASTRUN void TeensyTrigger::chen_scope()
{
  set_all_led_brightness(0);
  uint_fast32_t lastCommandCheck = 0;
  uint_fast32_t triggerCounter = 0;
  uint_fast8_t doTrigger = true;

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // read confing as send from matlab
  // prime card, seems NI needs this
  uint_fast32_t nPreTrigger = serial_read_32bit();
  // trigger how many times? per AOD cycle
  uint_fast32_t nTrigger = serial_read_32bit();
  // trigger freq. in Hz
  uint_fast32_t triggerFreq = serial_read_32bit();
  uint_fast32_t postAcqDelay = serial_read_32bit();
  // camera is triggered slightly later than AOD
  uint_fast32_t camTrigDelay = serial_read_32bit();

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // calculate some dependent variables
  uint_fast32_t triggerPeriod = 1 / (triggerFreq * 1E-9);
  ; // trigger period in ns
  triggerPeriod = triggerPeriod / 2;
  setup_nano_delay(triggerPeriod);

  // firstHalf = do we need to trigger AOD in first or second half of
  // first full trigger period
  // bool firstHalf = (camTrigDelay * 1000 >= triggerPeriod);
  uint_fast32_t onWait = 0;  // wait this long, then trigger AOD
  uint_fast32_t offWait = 0; // after triggering AOD, wait this long to complete cycle
  const bool firstHalf = (camTrigDelay * 1000) <= triggerPeriod;
  if (firstHalf) // triggerPeriod in ns
  {
    onWait = camTrigDelay;
    offWait = triggerPeriod / 1000 - camTrigDelay;
  }
  else
  {
    onWait = camTrigDelay - triggerPeriod / 1000;
    offWait = triggerPeriod / 1000 - onWait;
  }

  // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // Here we start writing things to the port / trigger outputs and start the
  // action

  TRIG_OUT_PORT = 0b00000000; // all trigger pins low
  LED_PORT = 0b01010000;   // enabel Cam and AOD LEDs

  // we pre trigger the AOD n-times %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
  // then we turn on the camera and send out the remaining AOD triggers
  for (uint_fast8_t iTrig = 0; iTrig < nPreTrigger; iTrig++)
  {
    TRIG_OUT_PORT = 0b00000100; // AOD HIGH | PCO LOW
    wait_nano_delay();
    TRIG_OUT_PORT = 0b00000000; // all low
    wait_nano_delay();
  }

  // now we do the actual triggering to acquire data %%%%%%%%%%%%%%%%%%%%%%%%%%%
  // this might run for eternaty...
  // we wait 50% of the trigger period on, then of, so we need half the actual period
  while (doTrigger)
  {
    if (firstHalf)
    {
      TRIG_OUT_PORT = 0b00000100; // AOD HIGH | PCO LOW
      delayMicroseconds(onWait);
      TRIG_OUT_PORT = 0b00000110; // AOD HIGH | PCO HIGH
      delayMicroseconds(offWait);
      TRIG_OUT_PORT = 0b00000010; // AOD LOW | PCO HIGH
      wait_nano_delay();
    }
    else
    {
      TRIG_OUT_PORT = 0b00000100; // AOD HIGH | PCO LOW
      wait_nano_delay();
      TRIG_OUT_PORT = 0b00000000; // AOD LOW | PCO LOW
      delayMicroseconds(onWait);
      TRIG_OUT_PORT = 0b00000010; // AOD LOW | PCO HIGH
      delayMicroseconds(offWait);
    }

    // we have already triggered once (with a delay, so account for that)
    uint_fast32_t remainTrigger = nTrigger - 1; 
    for (uint_fast32_t iTrig = 0; iTrig < remainTrigger; iTrig++)
    {
      TRIG_OUT_PORT = 0b00000110; // AOD HIGH | PCO HIGH
      wait_nano_delay();
      TRIG_OUT_PORT = 0b00000010; // PCO high | AOD LOW
      wait_nano_delay();
    }
    delayMicroseconds(onWait); // extend camera trigger to match AOD exposure
    TRIG_OUT_PORT = 0b00000000; // all trigger pins low
    triggerCounter++;
    // delay after acq. is done for camera to prepare for next frame
    delayMicroseconds(postAcqDelay);
    // check if we got a new serial command to stop triggering
    // COMMAND_CHECK_INTERVALL is high, so we only check once in a while
    if ((millis() - lastCommandCheck) >= COMMAND_CHECK_INTERVALL)
    {
      lastCommandCheck = millis();
      if (Serial.available() >= 2)
      {
        this->currentCommand = serial_read_16bit_no_wait();
        if (this->currentCommand == DISABLE_LMI_MODE)
          doTrigger = false;
      }
    }
  } // while (doTrigger)
  LED_PORT = 0b00000000; // disable all LEDs
  ledBrightness = 0;
  serial_write_16bit(DONE); // send the "ok, we are done" command
  serial_write_32bit(triggerCounter);
  this->currentCommand = DO_NOTHING; // exit state machine
}

// custom trigger function for chen to trigger AOD and camera only -------------
FASTRUN void TeensyTrigger::chen_cascade()
{
  set_all_led_brightness(0);
  trigOutChMask = 0b00000000;
  uint_fast32_t cycleTrigger = 0;
  uint_fast32_t nCycle = 0;   // keeps track of completed triggering cycles
  uint_fast32_t nTrigger = 0; // keeps track of triggering during
  uint_fast8_t currentTrigState = 0;
  uint_fast8_t lastTrigState = 0;
  uint_fast8_t waitForTrigger = 1;
  uint_fast8_t doTrigger = 0;
  uint_fast32_t lastCommandCheck = 0;

  uint_fast32_t daqDelay = serial_read_32bit();     // for now in us, could be changed
  uint_fast32_t trigDuration = serial_read_32bit(); // for now in us, could be changed
  uint_fast32_t camWait = serial_read_32bit();      // trigger cam ever n shots
  uint_fast32_t nBaselineWait = serial_read_32bit();
  // wait nshots before starting stimulus
  uint_fast32_t nRecordLength = serial_read_32bit();
  // total number of shots for which to record data
  uint_fast32_t nCycleLength = serial_read_32bit();
  // total number of shots after which whole cylce starts again

  while (waitForTrigger)
  {
    // FIXME - wait for next trigger signal here
    currentTrigState = TRIG_IN1;
    if (currentTrigState != lastTrigState)
    {
      // check for rising flank
      doTrigger = (currentTrigState & !lastTrigState);
      lastTrigState = currentTrigState;
    }

    if (doTrigger)
    {
      delayMicroseconds(daqDelay);
      trigOutChMask = 0b00000000;
      ledOutMask = 0b00000000;
      trigOutChMask |= (1UL << (DAQ_BIT - 1)); // always enable DAQ trigger pin
      ledOutMask |= (1UL << (1 - 1));          // first LED on
      // check if we need to enable camera trigger as well
      if (cycleTrigger % camWait == 0)
      {
        trigOutChMask |= (1UL << (ANDOR_BIT - 1)); // enable cam trigger ever n-shots
        ledOutMask |= (1UL << (2 - 1));            // 2nd LED on
      }
      // check if we need to activate the stimulus trigger
      if (cycleTrigger == nBaselineWait)
      {
        trigOutChMask |= (1UL << (STIM_BIT - 1)); // enable stim trigger
        ledOutMask |= (1UL << (3 - 1));           // 3rd LED on
      }

      // check if we need to activate the blocking trigger
      if (cycleTrigger >= nRecordLength)
      {
        TRIG_OUT_PORT = 0b00001000; // activate block, disable rest
        LED_PORT = 0b00001000;
      }
      else
      {
        TRIG_OUT_PORT = trigOutChMask;
        LED_PORT = ledOutMask;
        // writes trigger mask to output port, thus triggers
        delayMicroseconds(trigDuration);
        TRIG_OUT_PORT = 0b00000000;
      }

      // keep track of how often we have triggered
      if (cycleTrigger == nCycleLength)
      {
        cycleTrigger = 0; // disable all triggers here
        nCycle++;
      }
      else
        cycleTrigger++; // current cylce trigger counter

      nTrigger++; // overall trigger / shot counter
      // cycleTrigger++; // current cylce trigger counter
      // check if we got a new serial command to stop triggering
      // COMMAND_CHECK_INTERVALL is high, so we only check once in a while
      doTrigger = 0;
    } // if do trigger
    if ((millis() - lastCommandCheck) >= COMMAND_CHECK_INTERVALL)
    {
      lastCommandCheck = millis();
      if (Serial.available() >= 2)
      {
        this->currentCommand = serial_read_16bit_no_wait();
        if (this->currentCommand == DISABLE_CHEN_CASCADE)
          waitForTrigger = false;
      }
    }
  } // while (waitForTrigger)
  ledBrightness = 0;
  serial_write_16bit(DONE); // send the "ok, we are done" command
  serial_write_32bit(nTrigger);
  serial_write_32bit(nCycle);
  this->currentCommand = DO_NOTHING; // exit state machine
}
