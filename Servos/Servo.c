/*
 * File:        TFT, keypad, DAC, LED, PORT EXPANDER test
 *              With serial interface to PuTTY console
 * Author:      Bruce Land
 * For use with Sean Carroll's Big Board
 * http://people.ece.cornell.edu/land/courses/ece4760/PIC32/target_board.html
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
//  TEST OLD CODE WITH NEW THREADS
//#include "config_1_2_3.h"
#include "config_1_3_2.h"
// threading library
//#include "pt_cornell_1_2_3.h"
#include "pt_cornell_1_3_2_python.h"
// yup, the expander
#include "port_expander_brl4.h"

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// need for sin function
#include <math.h>
////////////////////////////////////

// lock out timer 2 interrupt during spi comm to port expander
// This is necessary if you use the SPI2 channel in an ISR.
// The ISR below runs the DAC using SPI2
#define start_spi2_critical_section INTEnable(INT_T2, 0)
#define end_spi2_critical_section INTEnable(INT_T2, 1)

#define EnablePullUpA(bits) CNPDACLR=bits; CNPUASET=bits;
#define EnablePullUpB(bits) CNPDBCLR=bits; CNPUBSET=bits;

////////////////////////////////////

/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[60];

// === outputs from python handler =============================================
// signals from the python handler thread to other threads
// These will be used with the prototreads PT_YIELD_UNTIL(pt, condition);
// to act as semaphores to the processing threads
char new_string = 0;
char new_button = 0;
char new_toggle = 0;
char new_slider = 0;
// identifiers and values of controls
// curent button
char button_id, button_value ;
// current toggle switch/ check box
char toggle_id ;
char toggle_value = 1;
// current slider
int slider_id, slider_value ; // value could be large
// current string
char receive_string[64];

//== Timer 2 interrupt handler ===========================================
// volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
// volatile int spiClkDiv = 4 ; // 10 MHz max speed for port expander!!

volatile int control = 40000;
volatile int control2 = 60000 ;

volatile int stored_control = 40000 ;
volatile int stored_control2 = 60000 ;

volatile int pan = 40000 ;
volatile int tilt = 60000 ;

int hey = 0;
int increasing = 1 ;

unsigned int adcValue ;


void __ISR(_TIMER_3_VECTOR, ipl2) Timer2Handler(void)
{    
    // clear interrupt flag
    mT3ClearIntFlag();
    // toggle the LED on the big board
    mPORTAToggleBits(BIT_0);

    // Output PWM
    SetDCOC3PWM(control);
    SetDCOC4PWM(control2) ;

    adcValue = ReadADC10(0) ;
    AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below

    if ((adcValue > 600) && !toggle_value) {
      control = stored_control ;
      control2 = stored_control2 ;
      pan = stored_control ;
      tilt = stored_control2 ;
      hey = 500 ;
    }
   //    
}

// === print a line on TFT =====================================================
// print a line on the TFT
// string buffer
char buffer[60];
void printLine(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(1);
    tft_writeString(print_buffer);
}

void printLine2(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 20 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 16, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

static PT_THREAD(protothread_bored(struct pt *pt))
{
  PT_BEGIN(pt) ;

  while(1) {

    if (toggle_value == 0) {

      if (!hey) {

        if (tilt < 90000) {
          tilt += 100 ;
        }
        if (increasing == 1) {
          if (pan < 89900) {
            pan += 100 ;
          }
          else {
            pan -= 100 ;
            increasing = 0 ;
          }
        }
        else {
          if (pan > 30100) {
            pan -= 100 ;
          }
          else {
            pan += 100 ;
            increasing = 1 ;
          }
        }

        control2 = tilt ;
        control = pan ;
      }
      else {
        hey -= 1 ;
      }
    }

    else {
      pan = control ;
      tilt = control2 ;
    }

  // printf("%d\n", adcValue) ;
  PT_YIELD_TIME_msec(5) ;
  }
  PT_END(pt) ;
}

// === Python serial thread ====================================================
// you should not need to change this thread
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
    static char junk;
    //   
    //
    while(1){
        // There is no YIELD in this loop because there are
        // YIELDS in the spawned threads that determine the 
        // execution rate while WAITING for machine input
        // =============================================
        // NOTE!! -- to use serial spawned functions
        // you MUST edit config_1_3_2 to
        // (1) uncomment the line -- #define use_uart_serial
        // (2) SET the baud rate to match the PC terminal
        // =============================================
        
        // now wait for machine input from python
        // Terminate on the usual <enter key>
        PT_terminate_char = '\r' ; 
        PT_terminate_count = 0 ; 
        PT_terminate_time = 0 ;
        // note that there will NO visual feedback using the following function
        PT_SPAWN(pt, &pt_input, PT_GetMachineBuffer(&pt_input) );
        
        // Parse the string from Python
        // There can be toggle switch, button, slider, and string events
        
        // toggle switch
        if (PT_term_buffer[0]=='t'){
            // signal the button thread
            new_toggle = 1;
            // subtracting '0' converts ascii to binary for 1 character
            toggle_id = (PT_term_buffer[1] - '0')*10 + (PT_term_buffer[2] - '0');
            toggle_value = PT_term_buffer[3] - '0';
            if (toggle_value == 0) {
              stored_control = control ;
              stored_control2 = control2 ;
            }
        }
        
        // pushbutton
        if (PT_term_buffer[0]=='b'){
            // signal the button thread
            new_button = 1;
            // subtracting '0' converts ascii to binary for 1 character
            button_id = (PT_term_buffer[1] - '0')*10 + (PT_term_buffer[2] - '0');
            button_value = PT_term_buffer[3] - '0';
            if (button_value == 1) {
              control = stored_control ;
              control2 = stored_control2 ;
              pan = stored_control ;
              tilt = stored_control2 ;
              hey = 500 ;
            }
        }
        
        // slider
        if (PT_term_buffer[0]=='s'){
            sscanf(PT_term_buffer, "%c %d %d", &junk, &slider_id, &slider_value);
            new_slider = 1;
            if (toggle_value == 1) {
              if (slider_id == 1) {
                control = slider_value ;
              }
              else if (slider_id == 2) {
                control2 = slider_value ;
              }
              stored_control = control ;
              stored_control2 = control2 ;
            }
        }
        
        // string from python input line
        if (PT_term_buffer[0]=='$'){
            // signal parsing thread
            new_string = 1;
            // output to thread which parses the string
            // while striping off the '$'
            strcpy(receive_string, PT_term_buffer+1);
        }                                  
    } // END WHILE(1)   
    PT_END(pt);  
} // thread blink


// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // set up DAC on big board
  // timer interrupt //////////////////////////
  // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
  // at 30 MHz PB clock 60 counts is two microsec
  // 400 is 100 ksamples/sec
  // 2000 is 20 ksamp/sec
  OpenTimer23(T2_ON | T2_SOURCE_INT | T2_PS_1_1 | T2_32BIT_MODE_ON, (800000));

  // set up the timer interrupt with a priority of 2
  ConfigIntTimer23(T23_INT_ON | T23_INT_PRIOR_2);
  mT3ClearIntFlag(); // and clear the interrupt flag

  OpenOC3(OC_ON | OC_TIMER_MODE32 | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , 0, 0) ;
  PPSOutput(4, RPA3, OC3) ;

  OpenOC4(OC_ON | OC_TIMER_MODE32 | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE , 0, 0) ;
  PPSOutput(3, RPA2, OC4) ;  
   
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();


 // the ADC ///////////////////////////////////////
    
  // the ADC ///////////////////////////////////////
  // configure and enable the ADC
  CloseADC10(); // ensure the ADC is off before setting the configuration

  // define setup parameters for OpenADC10
  // Turn module on | ouput in integer | trigger mode auto | enable autosample
  // ADC_CLK_AUTO -- Internal counter ends sampling and starts conversion (Auto convert)
  // ADC_AUTO_SAMPLING_ON -- Sampling begins immediately after last conversion completes; SAMP bit is automatically set
  // ADC_AUTO_SAMPLING_OFF -- Sampling begins with AcquireADC10();
  #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //

  // define setup parameters for OpenADC10
  // ADC ref external  | disable offset test | disable scan mode | do 1 sample | use single buf | alternate mode off
  #define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
  //
  // Define setup parameters for OpenADC10
  // use peripherial bus clock | set sample time | set ADC clock divider
  // ADC_CONV_CLK_Tcy2 means divide CLK_PB by 2 (max speed)
  // ADC_SAMPLE_TIME_5 seems to work with a source resistance < 1kohm
  #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 //ADC_SAMPLE_TIME_15| ADC_CONV_CLK_Tcy2

  // define setup parameters for OpenADC10
  // set AN11 and  as analog inputs
  #define PARAM4  ENABLE_AN11_ANA // pin 24

  // define setup parameters for OpenADC10
  // do not assign channels to scan
  #define PARAM5  SKIP_SCAN_ALL

  // use ground as neg ref for A | use AN11 for input A     
  // configure to sample AN11 
  SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN11 ); // configure to sample AN11 
  OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above

  EnableADC10(); // Enable the ADC
  ///////////////////////////////////////////////////////




  // pt_add(protothread_adc, 0) ;
  pt_add (protothread_serial, 0) ;
  pt_add (protothread_bored, 0) ;

  PT_INIT(&pt_sched) ;

  pt_sched_method = SCHED_ROUND_ROBIN ;

  PT_SCHEDULE(protothread_sched(&pt_sched)) ;
  
  // // set up LED to blink
  mPORTASetBits(BIT_0 ); //Clear bits to ensure light is off.
  mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output

  // // round-robin scheduler for threads
  // while (1){
  //     // PT_SCHEDULE(protothread_serial(&pt_serial));
  //     PT_SCHEDULE(protothread_adc(&pt_adc)) ;
  //     }
  } // main

// === end  ======================================================

