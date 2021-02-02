/*
 * File:        ADC > FFT > TFT
 * Author:      Bruce Land
 * 
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
#include <math.h>
#include <stdfix.h>
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

// === the fixed point macros ========================================
typedef signed short fix14 ;
#define multfix14(a,b) ((fix14)((((long)(a))*((long)(b)))>>14)) //multiply two fixed 2.14
#define float2fix14(a) ((fix14)((a)*16384.0)) // 2^14
#define fix2float14(a) ((float)(a)/16384.0)
#define absfix14(a) abs(a) 
// === input arrays ==================================================
#define nSamp 512
#define nPixels 239//256
// FFT
#define N_WAVE          512    /* size of FFT 512 */
#define LOG2_N_WAVE     9     /* log2(N_WAVE) 0 */
#define begin {
#define end }
_Accum v_in[nSamp] ;
_Accum adcValue ;
int k = 0 ;
int ready = 0 ;

_Accum Sinewave[N_WAVE]; // a table of sines for the FFT
_Accum window[N_WAVE]; // a table of window values for the FFT
_Accum fr[N_WAVE], fi[N_WAVE];
int pixels[nPixels] ;

_Accum fi_init[N_WAVE] = {0} ;

volatile int value = 2500;

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
int slider_value = 2500; // value could be large
// current string
char receive_string[64];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
// static struct pt pt_fft, pt_serial ;
// static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds ;

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    adcValue = ReadADC10(0) ;
    AcquireADC10(); // not needed if ADC_AUTO_SAMPLING_ON below

    if (k < N_WAVE) {
        v_in[k] = (adcValue * window[k]) ;
        k++ ;
    }
    else {
        ready = 1 ;
    }
    
    mT2ClearIntFlag();
}

// === print line for TFT ============================================
// print a line on the TFT
// string buffer
char buffer[60];
void printLine(int line_number, char* print_buffer, short text_color, short back_color, int h_pos){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10 ;
    // erase the pixels
    tft_fillRoundRect(h_pos, v_pos, 25, 8, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(h_pos, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

//=== FFT ==============================================================

void FFTfix(_Accum fr[], _Accum fi[], int m)
//Adapted from code by:
//Tom Roberts 11/8/89 and Malcolm Slaney 12/15/94 malcolm@interval.com
//fr[n],fi[n] are real,imaginary arrays, INPUT AND RESULT.
//size of data = 2**m
// This routine does foward transform only
begin
    int mr,nn,i,j,L,k,istep, n;
    _Accum qr,qi,tr,ti,wr,wi;

    mr = 0;
    n = 1<<m;   //number of points
    nn = n - 1;

    /* decimation in time - re-order data */
    for(m=1; m<=nn; ++m)
    begin
        L = n;
        do L >>= 1; while(mr+L > nn);
        mr = (mr & (L-1)) + L;
        if(mr <= m) continue;
        tr = fr[m];
        fr[m] = fr[mr];
        fr[mr] = tr;
        //ti = fi[m];   //for real inputs, don't need this
        //fi[m] = fi[mr];
        //fi[mr] = ti;
    end

    L = 1;
    k = LOG2_N_WAVE-1;
    while(L < n)
    begin
        istep = L << 1;
        for(m=0; m<L; ++m)
        begin
            j = m << k;
            wr =  Sinewave[j+N_WAVE/4];
            wi = -Sinewave[j];
            //wr >>= 1; do need if scale table
            //wi >>= 1;

            for(i=m; i<n; i+=istep)
            begin
                j = i + L;
                tr = (wr*fr[j]) - (wi*fi[j]);
                ti = (wr*fi[j]) + (wi*fr[j]);
                qr = fr[i] >> 1;
                qi = fi[i] >> 1;
                fr[j] = qr - tr;
                fi[j] = qi - ti;
                fr[i] = qr + tr;
                fi[i] = qi + ti;
            end
        end
        --k;
        L = istep;
    end
end

void drawRange(int maxval) {
    int firstStop = 60 * (slider_value*2) / (nSamp) ;
    int secondStop = 120 * (slider_value*2) / (nSamp) ;
    int thirdStop = 180 * (slider_value*2) / (nSamp) ;
    int fourthStop = 239 * (slider_value*2) / (nSamp) ;

    tft_fillRect(0, 0, 56, 240, 0x0000) ;
    tft_drawFastHLine(45, 0, 12, 0xFFFF) ;
    tft_drawFastHLine(45, 60, 12, 0xFFFF) ;
    tft_drawFastHLine(45, 120, 12, 0xFFFF) ;
    tft_drawFastHLine(45, 180, 12, 0xFFFF) ;
    tft_drawFastHLine(45, 239, 12, 0xFFFF) ;
    tft_setTextColor(0xFFFF); 

    tft_setCursor(0, 220);
    tft_setTextSize(2);
    tft_writeString("0 Hz");

    tft_setCursor(0, 171);
    sprintf(buffer, "%d", firstStop) ;
    tft_writeString(buffer) ;

    tft_setCursor(0, 111);
    sprintf(buffer, "%d", secondStop) ;
    tft_writeString(buffer) ;

    tft_setCursor(0, 51);
    sprintf(buffer, "%d", thirdStop) ;
    tft_writeString(buffer) ;

    tft_setCursor(0, 2);
    sprintf(buffer, "%d", fourthStop) ;
    tft_writeString(buffer) ;
}

// === FFT Thread =============================================
#define log_min 0x10   
unsigned int col=61 ;
static PT_THREAD (protothread_fft(struct pt *pt))
{
    PT_BEGIN(pt);
    static int sample_number ;
    static _Accum zero_point_4 = (_Accum)(0.4) ;
    // approx log calc ;
    static int sx, y, ly, temp ;

    unsigned short drawcolor ;
    
    while(1) {
        // Wait until input array is full
        PT_WAIT_UNTIL(pt, ready) ;
        // === Disable interrupts  ========
        INTDisableInterrupts();
        // load windowed input array
        memcpy(&fr, &v_in, sizeof(v_in)) ;
        memcpy(&fi, &fi_init, sizeof(&fi_init)) ;
        // reset index and ready signal
        k = 0 ;
        ready = 0 ;
        // Enable interrupts
        INTEnableInterrupts();
        // 
        // profile fft time
        WriteTimer4(0);
        
        // compute and display fft        
        // do FFT
        FFTfix(fr, fi, LOG2_N_WAVE);

        int max_fr = 0 ;
        int max_fr_dex = 0;
        
        // get magnitude and log
        // The magnitude of the FFT is approximated as: 
        //   |amplitude|=max(|Re|,|Im|)+0.4*min(|Re|,|Im|). 
        // This approximation is accurate within about 4% rms.
        // https://en.wikipedia.org/wiki/Alpha_max_plus_beta_min_algorithm
        for (sample_number = 2; sample_number < nPixels; sample_number++) {  
            // get the approx magnitude
            fr[sample_number] = abs(fr[sample_number]); //>>9
            fi[sample_number] = abs(fi[sample_number]);
            // reuse fr to hold magnitude
            fr[sample_number] = max(fr[sample_number], fi[sample_number]) + 
                    (min(fr[sample_number], fi[sample_number]) * zero_point_4); 

            if (fr[sample_number] > max_fr && sample_number>2) {
                max_fr = fr[sample_number] ;
                max_fr_dex = sample_number ;
            }
            
            // reuse fr to hold log magnitude
            // shifting finds most significant bit
            // then make approxlog  = ly + (fr-y)./(y) + 0.043;
            // BUT for an 8-bit approx (4 bit ly and 4-bit fraction)
            // ly 1<=ly<=14
            // omit the 0.043 because it is too small for 4-bit fraction
            
            // sx = fr[sample_number];
            // y=1; ly=0;
            // while(sx>1) {
            //    y=y*2; ly=ly+1; sx=sx>>1;
            // }
            // // shift ly into upper 4-bits as integer part of log
            // // take bits below y and shift into lower 4-bits
            // fr[sample_number] = ((ly)<<4) + ((fr[sample_number]-y)>>(ly-4) ) ;
            // // bound the noise at low amp
            // if(fr[sample_number]<log_min) fr[sample_number] = log_min;
        }
        tft_fillRect(70, 0, 250, 15, 0x0000);
        sprintf(buffer, "Max. freq: %d", max_fr_dex * (slider_value*2) / (nSamp)) ;
        printLine(0, buffer, ILI9340_WHITE, ILI9340_BLACK, 100);
        // sprintf(buffer, "%d", max_fr) ;
        // printLine(1, buffer, ILI9340_WHITE, ILI9340_BLACK, 100);
        
        // timer 4 set up with prescalar=8, 
        // hence mult reading by 8 to get machine cycles
        // sprintf(buffer, "FFT cycles %d", (ReadTimer4())*8);
        // printLine(10, buffer, ILI9340_WHITE, ILI9340_BLACK);
        
        // Display on TFT
        // erase, then draw
        tft_fillRect(col, 0, 5, 240, ILI9340_BLACK) ;
        for (sample_number = 0; sample_number < nPixels; sample_number++) {
            if (fr[sample_number] < 1){
                drawcolor=0x0000 ;
            }
            else if (fr[sample_number]<2) {
                drawcolor = 0x2945 ;
            }
            else if (fr[sample_number]<4) {
                drawcolor = 0x4a49 ;
            }
            else if (fr[sample_number]<8) {
                drawcolor = 0x738e ;
            }
            else if (fr[sample_number]<16) {
                drawcolor = 0x85c1 ;
            }
            else if (fr[sample_number]<32) {
                drawcolor = 0xad55 ;
            }
            else if (fr[sample_number]<64) {
                drawcolor = 0xc638 ;
            }
            else {
                drawcolor = 0xFFFF ;
            }
            tft_drawPixel(col, 239-sample_number, ILI9340_BLACK) ;
            tft_drawPixel(col, 239-sample_number, drawcolor) ;
            // tft_drawPixel(pixels[sample_number], sample_number, ILI9340_BLACK);
            // pixels[sample_number] = fr[sample_number] + 30 ;
            // tft_drawPixel(pixels[sample_number], sample_number, drawcolor);
            // reuse fr to hold magnitude 
        }    
        if (col<314){
            col = (col + 1) ;
        }
        else {
            col = 61 ;
        }
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // FFT thread

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

// ===  Slider thread =========================================================
// process slider from Python to draw a shor tline on LCD
static PT_THREAD (protothread_sliders(struct pt *pt))
{
    PT_BEGIN(pt);
    while(1){
        PT_YIELD_UNTIL(pt, new_slider==1);
        // clear flag
        new_slider = 0; 
        if (slider_id == 1){
            // printLine(10, 'a', ILI9340_WHITE, ILI9340_BLACK);
            DisableIntT2 ;
            CloseTimer2();
            drawRange(slider_value) ;
            OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, (int)(40000000/(slider_value*2)));
            // set up the timer interrupt with a priority of 2
            ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
            mT2ClearIntFlag(); // and clear the interrupt flag
        }
    } // END WHILE(1)   
    PT_END(pt);  
} // thread slider


// === Main  ======================================================

void main(void) {
    //SYSTEMConfigPerformance(PBCLK);

    ANSELA = 0;
    ANSELB = 0;

    // set up DAC on big board
    // timer interrupt //////////////////////////
    // Set up timer2 on,  interrupts, internal clock, prescalar 1, toggle rate
    // at 40 MHz PB clock 
    // 40,000,000/Fs = 909 : since timer is zero-based, set to 908
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, (int)(40000000/(2500*2)));

    // set up the timer interrupt with a priority of 2
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // and clear the interrupt flag

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

    // timer 4 setup for profiling code ===========================================
    // Set up timer2 on,  interrupts, internal clock, prescalar 1, compare-value
    // This timer generates the time base for each horizontal video line
    OpenTimer4(T4_ON | T4_SOURCE_INT | T4_PS_1_8, 0xffff);
        
    // === init FFT data =====================================
    // one cycle sine table
    //  required for FFT
    int ii;
    for (ii = 0; ii < N_WAVE; ii++) {
        Sinewave[ii] = (_Accum)(sin(6.283 * ((float) ii) / N_WAVE)*0.5);
        window[ii] = (_Accum)(0.5 * (1.0 - cos(6.283 * ((float) ii) / (N_WAVE))));
        //window[ii] = float2fix(1.0) ;
    }
    // ========================================================
    
    // init the threads
    // PT_INIT(&pt_fft);
    // PT_INIT(&pt_serial) ;

    pt_add(protothread_fft, 0) ;
    pt_add(protothread_serial, 0) ;
    pt_add(protothread_sliders, 0) ;

    PT_INIT(&pt_sched) ;
    pt_sched_method = SCHED_ROUND_ROBIN ;

    // init the display
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    //240x320 vertical display
    tft_setRotation(1); // Use tft_setRotation(1) for 320x240
    tft_fillRect(57, 0, 3, 240, 0xFFFF) ;

    drawRange(slider_value) ;

    PT_SCHEDULE(protothread_sched(&pt_sched)) ;

    // round-robin scheduler for threads
    // while (1) {
    //     PT_SCHEDULE(protothread_fft(&pt_fft));
    //     PT_SCHEDULE(protothread_serial(&pt_serial)) ;
    // }
} // main

// === end  ======================================================
