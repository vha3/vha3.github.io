/*
 * File:        Test of compiler fixed point
 * Author:      Bruce Land
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_3_2.h"
// threading library
#include "pt_cornell_1_3_2_python.h"

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// fixed point types
#include <stdfix.h>
#include <math.h>

// === DAC control vars ===========================================
// volatile SpiChannel spiChn = SPI_CHANNEL2 ; // the SPI channel to use
volatile int spiClkDiv = 2 ; // 20 MHz max speed for this DAC

// table size and timer rate determine frequency
// output freq = (timer rate)/(table size)
#define sine_table_size 256
// #define dma_table_size 512
// output valuea + control info
unsigned short DAC_data1[sine_table_size*2] ;
unsigned short DAC_data2[sine_table_size*2] ;
unsigned short raw_sin[sine_table_size*2] ;
unsigned short raw_sin2[sine_table_size*2] ;
unsigned short raw_sin3[sine_table_size*2] ;
unsigned short DAC_data3[sine_table_size*2] ;
// unsigned short DAC_data2[sine_table_size];
// unsigned short DMA_table[dma_table_size] ;
// A-channel, 1x, active
#define DAC_config_chan_A 0b0011000000000000
// B-channel, 1x, active
// #define DAC_config_chan_B 0b1011000000000000

static int adc_9 ;

#define hitBottom(b) (b>int2Accum(160))
#define hitTop(b) (b<int2Accum(50))
#define hitLeft(a) (a<int2Accum(50))
#define hitRight(a) (a>int2Accum(240))

// // #define hitBarrierEdge(a, b) (a>int2Accum(77) && a<int2Accum(83) && (b<int2Accum(61) || b>int2Accum(179)))
// #define hitBarrierEdge2(a, b) (a>int2Accum(115) && a<int2Accum(125) &&  b>int2Accum(139))
// #define hitBarrierEdge3(a, b) (a>int2Accum(135) && a<int2Accum(143) &&  b>int2Accum(139))

// #define hitGlassBottom1(a, b) (a>int2Accum(123) && a<int2Accum(137) &&  b>int2Accum(glassbottom))

// #define hitPaddle(a, b) (b>int2Accum(adc_9) && b<int2Accum(adc_9+20) && a<int2Accum(17))

#define EnablePullUpA(bits) CNPDACLR=bits; CNPUASET=bits;
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

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
// static struct pt pt_timer, pt_anim, pt_serial ;
// static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds = 0;
int ballcount = 0 ;
int sys_time_ms=  0 ;
int spare_time ;

float value ;

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
int slider_id ;
int slider_value; // value could be large
// current string
char receive_string[64];

#define maxballs 50
#define hit_thresh 30
#define frame_rate 33
#define float2Accum(a) ((_Accum)(a))
#define Accum2float(a) ((float)(a))
#define int2Accum(a) ((_Accum)(a))
#define Accum2int(a) ((int)(a))
static _Accum xc[maxballs] =  {0};
static _Accum yc[maxballs] =  {0};
static _Accum vxc[maxballs]=  {0};
static _Accum vyc[maxballs]=  {0};
static int hitcount[maxballs] = {hit_thresh};


static _Accum turnfactor = float2Accum(0.2) ;
volatile _Accum closeRangeSq = int2Accum(4) ;
volatile _Accum visualRange = int2Accum(20) ;
volatile _Accum visualRangeSq = int2Accum(400) ;

volatile _Accum centeringfactor = float2Accum(0.0005) ;
volatile _Accum avoidfactor = float2Accum(0.05) ;
volatile _Accum matchingfactor = float2Accum(0.05) ;

void spawnBall(_Accum* x, _Accum* y, _Accum* vx, _Accum* vy, int* hits)
{

  *x = ((_Accum)(rand() & 0xffff)>>9) + 96;
  *y = ((_Accum)(rand() & 0xffff)>>9) + 56;
  *vx = ((_Accum)(rand() & 0xffff)>>14);// - 4;
  *vy = ((_Accum)(rand() & 0xffff)>>13) - 4;

  *hits = hit_thresh ;
}

void wallsAndEdges(_Accum* x, _Accum* y, _Accum* vx, _Accum* vy, int* hits)
{
  _Accum speed ;

  if (hitTop(*y)) {
    *vy = (*vy + turnfactor) ;
    //*y = *y + 2 ;
  }
  if (hitRight(*x)) {
    *vx = (*vx - turnfactor) ;
    //*x = *x - 2 ;
  }
  if (hitLeft(*x)) {
    *vx = (*vx + turnfactor) ;
  }
  if (hitBottom(*y)) {
    *vy = (*vy - turnfactor) ;
  }  

  speed = sqrt((*vx)*(*vx) + (*vy)*(*vy)) ;

  if(speed>int2Accum(3)) {
    *vx = ((*vx)/speed) * int2Accum(3) ;
    *vy = ((*vy)/speed) * int2Accum(3) ;
  }
  if(speed<int2Accum(2)) {
    *vx = ((*vx)/speed) * int2Accum(2) ;
    *vy = ((*vy)/speed) * int2Accum(2) ;
  }

  *x = *x + *vx ;
  *y = *y + *vy ;
}

void otherBalls(_Accum* x, _Accum* y, _Accum* vx, _Accum* vy, int* hits, int ballnum)
{
  char temp[60] ;
  _Accum xpos_cum = 0;
  _Accum ypos_cum = 0;
  _Accum xvel_cum = 0;
  _Accum yvel_cum = 0;
  _Accum closex = 0;
  _Accum closey = 0;
  _Accum num_boids = 0;
  _Accum num_close_boids = 0;
  _Accum speed = 0;

  int j;

  // loop thru each boid
  for (j=(-ballnum); j<(ballcount-ballnum); j=(j+1)) {
    // make sure it's not yourself
    if(j!=0){
      // find difference in x and y coordinates
      _Accum dx = (*(x) - *(x+j)) ;
      _Accum dy = (*(y) - *(y+j)) ;
      // check that those distances are both less than the visual range
      if ((dx<visualRange) && (dx>(-visualRange)) && (dy<visualRange) && (dy>(-visualRange))){
        // find the squared distance
        _Accum rij_sq = (dx*dx) + (dy*dy) ;
        // check that distance is less than the squared visual range
        // if so, add information to cumulative variables
        if (rij_sq < closeRangeSq) {
          closex += (*(x) - *(x+j)) ;
          closey += (*(y) - *(y+j)) ;
          num_close_boids += 1;
        }
        else if (rij_sq < visualRangeSq) {
          xpos_cum += *(x+j) ;
          ypos_cum += *(y+j) ;
          xvel_cum += *(vx+j) ;
          yvel_cum += *(vy+j) ;
          num_boids += 1 ;
        }
      }
    }
    else{}
  }

  // if there are boids in the visual range, find the averages
  if (num_boids) {
    xpos_cum = xpos_cum/num_boids ;
    ypos_cum = ypos_cum/num_boids ;
    xvel_cum = xvel_cum/num_boids ;
    yvel_cum = yvel_cum/num_boids ;
    // if (num_close_boids){
    //   closex = closex/num_boids ;
    //   closey = closey/num_boids ;
    // }
    // update the boid's velocity
    *vx = (*vx) + ((xpos_cum-*(x))*centeringfactor) + ((xvel_cum-*(vx))*matchingfactor) + (closex*avoidfactor) ;
    *vy = (*vy) + ((ypos_cum-*(y))*centeringfactor) + ((yvel_cum-*(vy))*matchingfactor) + (closey*avoidfactor) ;
  }
}

void * drawArena() {
  tft_fillRect(0, 0, 2, 240, 65535);
  tft_fillRect(318, 0, 2, 240, 65535);
  tft_fillRect(0, 0, 320, 2, 65535) ;
  tft_fillRect(0, 238, 320, 2, 65535) ;
}

void * writeStats() {
  tft_fillRect(2, 2, 78, 40, ILI9340_BLACK);// x,y,w,h,radius,color
  tft_setCursor(25, 3);
  tft_setTextColor(ILI9340_WHITE); tft_setTextSize(1);
  sprintf(buffer,"Sec:%d", sys_time_seconds);
  tft_writeString(buffer);
  tft_setCursor(25, 11);
  tft_setTextColor(ILI9340_WHITE); tft_setTextSize(1);
  sprintf(buffer,"Boids:%d", ballcount);
  tft_writeString(buffer);
  tft_setCursor(25, 19);
  tft_setTextColor(ILI9340_WHITE); tft_setTextSize(1);
  sprintf(buffer,"FR:%d", 1000/frame_rate);
  tft_writeString(buffer);
  tft_setCursor(25, 27);
  tft_setTextColor(ILI9340_WHITE); tft_setTextSize(1);
  sprintf(buffer,"TS:%d", spare_time);
  tft_writeString(buffer);
}

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
    tft_fillScreen(ILI9340_BLACK);

      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(50) ;
        drawArena() ;
        writeStats() ;
        sys_time_ms++ ;
        if (sys_time_ms%20==0){
          sys_time_seconds += 1;
        }

        if (ballcount < maxballs) {
          spawnBall(&xc[ballcount], &yc[ballcount],
            &vxc[ballcount], &vyc[ballcount],
            &hitcount[ballcount]);
          ballcount += 1 ;
        }
      } // END WHILE(1)
  PT_END(pt);
} // timer thread


static PT_THREAD (protothread_anim(struct pt *pt))
{
    PT_BEGIN(pt);
    int i ;
      while(1) {
        int begin_time = PT_GET_TIME() ;
        // loop through each ball
        i = 0 ;
        while(i < (ballcount)) 
        {
          // erase ball
           tft_fillCircle(Accum2int(xc[i]), Accum2int(yc[i]), 1, ILI9340_BLACK);
           // tft_drawPixel(Accum2int(xc[i]), Accum2int(yc[i]), ILI9340_BLACK);
           // update ball
           otherBalls(&xc[i], &yc[i], &vxc[i], &vyc[i], &hitcount[i], i) ;
           wallsAndEdges(&xc[i], &yc[i], &vxc[i], &vyc[i], &hitcount[i]) ;
           //  draw ball
           tft_fillCircle(Accum2int(xc[i]), Accum2int(yc[i]), 1, 0xFFFF); 
           // tft_drawPixel(Accum2int(xc[i]), Accum2int(yc[i]), 0xFFFF);
           i+=1;
        }
        // delay in accordance with frame rate
        spare_time = frame_rate - (PT_GET_TIME() - begin_time) ;
        PT_YIELD_TIME_msec(spare_time) ;
       // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

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
            if (slider_id == 1) {
              closeRangeSq = int2Accum(slider_value) * int2Accum(slider_value) ;
            }
            else if (slider_id == 2) {
              visualRangeSq = int2Accum(slider_value) * int2Accum(slider_value) ;
              visualRange = int2Accum(slider_value) ;
            }
            else if (slider_id == 3) {
              centeringfactor = float2Accum((1./(float)slider_value)) ;
            }
            else if (slider_id == 4) {
              avoidfactor = float2Accum((1./(float)slider_value)) ;
            }
            else if (slider_id == 5) {
              matchingfactor = float2Accum((1./(float)slider_value)) ;
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

  // === timer2 =================================
  // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
  // at 30 MHz PB clock 60 counts is two microsec
  // 100 is 400 ksamples/sec  
  // 400 is 100 ksamples/sec
  // 2000 is 20 ksamp/sec
  OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 900);
  // !!!NO ISR TURNED ON!!!!!

  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // // init the threads
  // PT_INIT(&pt_timer);
  // PT_INIT(&pt_anim);
  // PT_INIT(&pt_serial);

  pt_add(protothread_timer, 0) ;
  pt_add(protothread_anim, 0) ;
  pt_add(protothread_serial, 0) ;

  PT_INIT(&pt_sched) ;
  pt_sched_method = SCHED_ROUND_ROBIN ;

  // init the display
  tft_init_hw();
  tft_begin();
  //320x240 horiz. display
  tft_setRotation(1); // Use tft_setRotation(1) for 320x240

  tft_fillScreen(ILI9340_BLACK);

  // seed random color
  srand(1);

  PT_SCHEDULE(protothread_sched(&pt_sched)) ;

  // round-robin scheduler for threads
  // while (1){
  //     PT_SCHEDULE(protothread_timer(&pt_timer));
  //     PT_SCHEDULE(protothread_anim(&pt_anim));
  //     PT_SCHEDULE(protothread_serial(&pt_serial));
  //     }
  } // main

// === end  ======================================================

