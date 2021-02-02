/*
 * File:        TFT, keypad, DAC, LED, PORT EXPANDER test
 * Direct Digital Synthesis example 
 * DAC channel A has audio
 * DAC channel B has amplitude envelope
 * Author:      Bruce Land (modified by Hunter Adams)
 * For use with Sean Carroll's Big Board
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_3_2.h"
// threading library
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
// need for sine function
#include <math.h>
// The fixed point types
#include <stdfix.h>
////////////////////////////////////

// lock out timer interrupt during spi comm to port expander
// This is necessary if you use the SPI2 channel in an ISR
#define start_spi2_critical_section INTEnable(INT_T2, 0);
#define end_spi2_critical_section INTEnable(INT_T2, 1);

////////////////////////////////////
// some precise, fixed, short delays
// to use for extending pulse durations on the keypad
// if behavior is erratic
#define NOP asm("nop");
// 20 cycles 
#define wait20 NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;NOP;
// 40 cycles
#define wait40 wait20;wait20;
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

////////////////////////////////////
// Audio DAC ISR
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
#define DAC_config_chan_B 0b1011000000000000

// audio sample frequency
#define Fs 44000.0
// need this constant for setting DDS frequency
#define two32 4294967296.0 // 2^32 
// sine lookup table for DDS
#define sine_table_size 256
volatile _Accum sine_table[sine_table_size] ;
// phase accumulator for DDS
volatile unsigned int DDS_phase = 0;
// phase increment to set the frequency DDS_increment = Fout*two32/Fs
// For A above middle C DDS_increment =  = 42949673 = 440.0*two32/Fs
volatile unsigned int DDS_increment = 0; //42949673 ;
// waveform amplitude
volatile _Accum max_amplitude=2000;

// waveform amplitude envelope parameters
// rise/fall time envelope 44 kHz samples
volatile unsigned int attack_time=1000, decay_time=1000, sustain_time=3720, play_time=5720 ; 
//  0<= current_amplitude < 2048
volatile _Accum current_amplitude ;
// amplitude change per sample during attack and decay
// no change during sustain
volatile _Accum attack_inc, decay_inc ;

//== Timer 2 interrupt handler ===========================================
volatile unsigned int DAC_data_A, DAC_data_B ;// output values
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
volatile int spiClkDiv = 4 ; // 10 MHz max speed for port expander!!

// interrupt ticks since beginning of song or note
volatile unsigned int note_time, play_time;

// PORT B
#define EnablePullDownB(bits) CNPUBCLR=bits; CNPDBSET=bits;
#define DisablePullDownB(bits) CNPDBCLR=bits;
#define EnablePullUpB(bits) CNPDBCLR=bits; CNPUBSET=bits;
#define DisablePullUpB(bits) CNPUBCLR=bits;
//PORT A
#define EnablePullDownA(bits) CNPUACLR=bits; CNPDASET=bits;
#define DisablePullDownA(bits) CNPDACLR=bits;
#define EnablePullUpA(bits) CNPDACLR=bits; CNPUASET=bits;
#define DisablePullUpA(bits) CNPUACLR=bits;

#define Accum2int(a) ((int)(a))

// Macros for chirp and swoop
#define ChirpIncrement(a) (1.53e-04*a*a + 2000)*two32/Fs
#define SwoopIncrement(a) (-260*sin(-1*3.14159/5720 * a) + 1740)*two32/Fs


// semaphores for controlling threads
static struct pt_sem control_t1, control_t2 ;
int keypresses[50] = {0} ;
int num_presses, tot_presses = 0;
int pause =0;


void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    int junk;
    
    mT2ClearIntFlag();
    
    // generate  sinewave
    // advance the phase
    DDS_phase += DDS_increment;
    DAC_data_A = (int)(current_amplitude*sine_table[(DDS_phase>>24)]) + 2048 ;

    if(pause==0){
      // update amplitude envelope 
      if (note_time < (attack_time + sustain_time + decay_time)){
          current_amplitude = (note_time <= attack_time)? 
              current_amplitude + attack_inc : 
              (note_time <= attack_time + sustain_time)? current_amplitude:
                  current_amplitude - decay_inc ;
      }
      else {
          current_amplitude = 0 ;
      }
    }
    else{
      current_amplitude = 0 ;
    }

    DAC_data_B = (int) current_amplitude  ;
    
     // test for ready
     while (TxBufFullSPI2());
     
    // reset spi mode to avoid conflict with expander
    SPI_Mode16();
    // DAC-A CS low to start transaction
    mPORTBClearBits(BIT_4); // start transaction 
     // write to spi2
    WriteSPI2(DAC_config_chan_A | (DAC_data_A & 0xfff) );
    // fold a couple of timer updates into the transmit time
    note_time++ ;
    // test for done
    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
    // MUST read to clear buffer for port expander elsewhere in code
    junk = ReadSPI2(); 
    // CS high
    mPORTBSetBits(BIT_4); // end transaction
    
     // DAC-B CS low to start transaction
    mPORTBClearBits(BIT_4); // start transaction 
     // write to spi2
    WriteSPI2(DAC_config_chan_B | (DAC_data_B & 0xfff) );
    // test for done
    while (SPI2STATbits.SPIBUSY); // wait for end of transaction
    // MUST read to clear buffer for port expander elsewhere in code
    junk = ReadSPI2(); 
    // CS high
    mPORTBSetBits(BIT_4); // end transaction
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

// Predefined colors definitions (from tft_master.h)
//#define	ILI9340_BLACK   0x0000
//#define	ILI9340_BLUE    0x001F
//#define	ILI9340_RED     0xF800
//#define	ILI9340_GREEN   0x07E0
//#define ILI9340_CYAN    0x07FF
//#define ILI9340_MAGENTA 0xF81F
//#define ILI9340_YELLOW  0xFFE0
//#define ILI9340_WHITE   0xFFFF

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
char toggle_id, toggle_value ;
// current slider
int slider_id, slider_value ; // value could be large
// current string
char receive_string[64];


// === thread structures ============================================
// thread control structs
// static struct pt pt_timer, pt_key, pt_serial ;
// static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds ;
int key_index ;
int key ;

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
     // timer readout
     sprintf(buffer,"%s", "Number of valid button presses:\n");
     printLine(0, buffer, ILI9340_WHITE, ILI9340_BLACK);
     
     // set up LED to blink
     mPORTASetBits(BIT_0 );	//Clear bits to ensure light is off.
     mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
     
      while(1) {
        // yield time 1 second
        // PT_YIELD_TIME_msec(1000) ;
        //stop until thread 1 signals
        PT_SEM_WAIT(pt, &control_t2);
        for (key_index=0; key_index<num_presses; key_index++) {
          note_time = 0 ;
          current_amplitude = 0 ;
          key = keypresses[key_index];
          pause = 0;
          
          if(num_presses>0){
            while (note_time < (play_time)){
              if (key==1){
                DDS_increment = SwoopIncrement(note_time) ;
              }
              else if (key==2) {
                DDS_increment = ChirpIncrement(note_time) ;
              }
              else {
                pause=1 ;
              }
            }
          }
        sys_time_seconds++ ;
         // draw sys_time
        sprintf(buffer,"%d", tot_presses);
        printLine(1, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        }
        num_presses = 0;
        
        // toggle the LED on the big board
        mPORTBToggleBits(BIT_3);
        
        // !!!! NEVER exit while !!!!
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Buttons thread ==========================================================
// process buttons from Python for clear LCD and blink the on-board LED
static PT_THREAD (protothread_buttons(struct pt *pt))
{
    PT_BEGIN(pt);
    // set up LED port A0 to blink
    mPORTAClearBits(BIT_0 );  //Clear bits to ensure light is off.
    mPORTASetPinsDigitalOut(BIT_0);    //Set port as output
    while(1){
        PT_YIELD_UNTIL(pt, new_button==1);
        // clear flag
        new_button = 0;   
        // Button one -- control the LED on the big board
        if (button_id==1 && button_value==1) {
            mPORTASetBits(BIT_0); 
            keypresses[num_presses] = 1 ;
            num_presses += 1 ;
            if (toggle_value == 0) {
              PT_SEM_SIGNAL(pt, &control_t2) ;
            }
        }
        else if (button_id==1 && button_value==0) {
            mPORTAClearBits(BIT_0); 
        }
        // Button 2 -- clear TFT
        if (button_id==2 && button_value==1) {     
            keypresses[num_presses] = 2 ;
            num_presses += 1 ;
            if (toggle_value == 0) {
              PT_SEM_SIGNAL(pt, &control_t2) ;
            }
        }     
        if (button_id==3 && button_value==1) {
            mPORTASetBits(BIT_0); 
            keypresses[num_presses] = 3 ;
            num_presses += 1 ;
            if (toggle_value == 0) {
              PT_SEM_SIGNAL(pt, &control_t2) ;
            }
        }
        else if (button_id==3 && button_value==0) {
            mPORTAClearBits(BIT_0); 
        }
    } // END WHILE(1)   
    PT_END(pt);  
} // thread blink

// === Toggle thread ==========================================================
// process toggle from Python to change a dot color on the LCD
static PT_THREAD (protothread_toggles(struct pt *pt))
{
    PT_BEGIN(pt);
    static short circle_color = ILI9340_RED;
    //
    while(1){
        PT_YIELD_TIME_msec(100)
        //update dot color if toggle changed
        if (new_toggle == 1){
            // clear toggle flag
            new_toggle = 0;   
            // Toggle one -- put a  green makrer on screen
            if (toggle_id==1 && toggle_value==1){
                tft_fillCircle(160, 30, 10, ILI9340_GREEN);
                circle_color = ILI9340_GREEN;
            }
            // toggle 0 -- put a red dot on the screen
            else if (toggle_id==1 && toggle_value==0){
                tft_fillCircle(160, 30, 10, ILI9340_RED); 
                circle_color = ILI9340_RED;
                PT_SEM_SIGNAL(pt, &control_t2) ;
            }
        } // end new toggle
        // redraw if no new event
        if (new_toggle == 0 && circle_color != ILI9340_BLACK){
            tft_fillCircle(160, 30, 10, circle_color);       
        }
    } // END WHILE(1)   
    PT_END(pt);  
} // thread toggles



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
        }
        
        // pushbutton
        if (PT_term_buffer[0]=='b'){
            // signal the button thread
            new_button = 1;
            // subtracting '0' converts ascii to binary for 1 character
            button_id = (PT_term_buffer[1] - '0')*10 + (PT_term_buffer[2] - '0');
            button_value = PT_term_buffer[3] - '0';
        }
        
        // slider
        if (PT_term_buffer[0]=='s'){
            sscanf(PT_term_buffer, "%c %d %d", &junk, &slider_id, &slider_value);
            new_slider = 1;
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
  // at 40 MHz PB clock 
  // 40,000,000/Fs = 909 : since timer is zero-based, set to 908
  OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 908);

  // set up the timer interrupt with a priority of 2
  ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
  mT2ClearIntFlag(); // and clear the interrupt flag

  // SCK2 is pin 26 
  // SDO2 (MOSI) is in PPS output group 2, could be connected to RB5 which is pin 14
  PPSOutput(2, RPB5, SDO2);

  // control CS for DAC
  mPORTBSetPinsDigitalOut(BIT_4);
  mPORTBSetBits(BIT_4);

  mPORTBSetPinsDigitalOut(BIT_3);
  mPORTBSetBits(BIT_3);

  // divide Fpb by 2, configure the I/O ports. Not using SS in this example
  // 16 bit transfer CKP=1 CKE=1
  // possibles SPI_OPEN_CKP_HIGH;   SPI_OPEN_SMP_END;  SPI_OPEN_CKE_REV
  // For any given peripherial, you will need to match these
  // clk divider set to 4 for 10 MHz
  SpiChnOpen(SPI_CHANNEL2, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , 4);
   // end DAC setup
    
  // 
  // build the sine lookup table
  // scaled to produce values between 0 and 4096
  int i;
  for (i = 0; i < sine_table_size; i++){
        sine_table[i] = (_Accum)(sin((float)i*6.283/(float)sine_table_size));
   }
 
   // build the amplitude envelope parameters
   // bow parameters range check
	if (attack_time < 1) attack_time = 1;
	if (decay_time < 1) decay_time = 1;
	if (sustain_time < 1) sustain_time = 1;
	// set up increments for calculating bow envelope
	attack_inc = max_amplitude/(_Accum)attack_time ;
	decay_inc = max_amplitude/(_Accum)decay_time ;
    
  // init the display
  // NOTE that this init assumes SPI channel 1 connections
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();
  
  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === now the threads ====================
  // init  the thread control semaphores
  PT_SEM_INIT(&control_t1, 0); // start blocked
  PT_SEM_INIT(&control_t2, 0); // start unblocked

  pt_add(protothread_timer, 0) ;
  pt_add(protothread_serial, 0) ;
  pt_add(protothread_buttons, 0) ;
  pt_add(protothread_toggles, 0) ;

  PT_INIT(&pt_sched) ;
  pt_sched_method = SCHED_ROUND_ROBIN ;

  PT_SCHEDULE(protothread_sched(&pt_sched)) ;

  } // main

// === end  ======================================================
