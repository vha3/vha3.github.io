import PySimpleGUI as sg
# on linux:
# sudo pip3 install pyserial
# /home/bruce/.local/lib/python3.5
# sudo python3.5 pic_target_1.py
#
# you can find out more about PySimleGUI at
# https://pysimplegui.readthedocs.io/en/latest/
#
# --event format to PIC--
# Four sharacters for each non-string event:
# pushbutton event 'b' + 2 digit button number + value (1,0)
# toggle sw event 't' + 2 digit button number + value (1,0)
# slider event 's' + 1-digit slider number + n digit value
# -- string --
# strings typed in the input line are sent in their entirety.
# -- reset --
# RESET has NO code on PIC!
# serial reset event sends a rs-232 BREAK which is connected
# through a filter to MCLR pin
#
# Python_TX_pin--(100ohm)--(+Schottky Diode-)------>(target MCLR pin)
#                                             |
#                                     (10uf)------(1kohm)
#                                        |           |
#                                        -------------
#                                             |
#                                         (PIC gnd)
#
import time
import serial
# open microcontroller serial port
# For windows the device will be 'COMx'
ser = serial.Serial('COM3', 38400)  # open serial port

#sg.theme('DarkAmber')   # Add a touch of color
# All the stuff inside your window.
# This a heirachical list of items to be displayd in the window
# First list is first row controls, etc
# Buttons:
#   Realtime buttons respond to push-events
#   After the window is defined below, release events may be bound to each realtime button
#   The 'key' for each button must be of the form 'pushbutNN', 
#   where 'NN' are digits 0-9 defining the button number
# Toggles:
#   Toggle switches are actually checkboxes
#   The 'key' for each checkbox must be of the form 'toggleNN', 
#   where 'NN' are digits 0-9 defining the checkbox number
# Sliders
#   The 'key' for each slider must be of the form 'sliderN', 
#   where 'N' is a digit 0-9 defining the slider number
#   Sliders can have any integer range which is handy for the application
# Text
#   The text input field acts like the one-line Arduino serial send box.
#   The multiline output box receives serial from the PIC. text typed here is ignored.
font_spec = 'Courier 24 bold'
layout = [  [sg.Text('Motor Controls', font='Helvetica 18')],
            # [sg.RealtimeButton('LED', key='pushbut01', font='Helvetica 12'),
            [sg.RealtimeButton('Return to position', key='pushbut01', font='Helvetica 12'),
             sg.Checkbox('Program position', key='toggle01', font='Helvetica 12',enable_events=True, default=True)],
             # sg.Checkbox('DDS', key='toggle02', font='Helvetica 12',enable_events=True, default=True)], 
            [sg.Text('Pan')],
            [sg.Slider(range=(30000,90000), default_value=60000, size=(22,15), key='slider1',
             orientation='horizontal', font=('Helvetica', 12),enable_events=True)],
            [sg.Text('Tilt')],
            [sg.Slider(range=(60000,90000), default_value=60000, size=(22,15), key='slider2',
             orientation='horizontal', font=('Helvetica', 12),enable_events=True)],
            [sg.Text('Serial data to PIC')],
            [sg.InputText('', size=(40,10), key='pic_input', do_not_clear=False,
                enable_events=False, focus=True),
             sg.Button('Send', key='pic_send', font='Helvetica 12')],
            [sg.Text('Serial data from PIC')],
            [sg.Multiline('', size=(50,10), key='console',
               autoscroll=True, enable_events=False)],
            [sg.Text('System Controls')],
            [sg.Button('Exit', font='Helvetica 12')],
            [ sg.Checkbox('reset_enable', key='r_en', 
                        font='Helvetica 8', enable_events=True),
             sg.Button('RESET PIC', key='rtg', font='Helvetica 8')
            ] 
         ]

# change the colors in any way you like.
sg.SetOptions(background_color='#9FB8AD',
       text_element_background_color='#9FB8AD',
       element_background_color='#475841',#'#9FB8AD',
       scrollbar_color=None,
       input_elements_background_color='#9FB8AD',#'#F7F3EC',
       progress_meter_color = ('green', 'blue'),
       button_color=('white','#475841'),
       )

# Create the Window
window = sg.Window('ECE4760 Interface', layout, location=(0,0), 
                    return_keyboard_events=True, use_default_focus=True,
                    element_justification='c', finalize=True)

# Bind the realtime button release events <ButtonRelease-1>
window['pushbut01'].bind('<ButtonRelease-1>', 'r')
# window['pushbut02'].bind('<ButtonRelease-1>', 'r')

# Event Loop to process "events" 
# event is set by window.read
event = 0
# toggles seem to be read differently than buttons
switch_state = [0, 0, 0]
#
#  button state machine variables
button_on = 0
button_which = '0'
#
while True:
    
    # time out paramenter makes the system non-blocking
    # If there is no event the call returns event  '__TIMEOUT__'
    event, values = window.read(timeout=2) # timeout=5
    #
    #print(event) for debugging
    # if user closes window using windows 'x' or clicks 'Exit' button  
    if event == sg.WIN_CLOSED or event == 'Exit': # 
        break
    #
    # read out the toggle switches
    switch_state[1] = window.Element('toggle01').get()
    # switch_state[2] = window.Element('toggle02').get()
    # switch_state[0] = window.Element('r_en').get()
    # The text from the one-line input field
    input_state = window.Element('pic_input').get()

    #
    # pushbutton events state machine
    if event[0:3]  == 'pus' and button_on == 0 :
       # 'b' for button, two numeral characters, a '1' for pushed, and a terminator
       ser.write(('b' + event[7:9] + '1' + '\r').encode()) 
       button_on = 1
       button_which = event[7:9]
    # releaase event signalled by the 'r'
    elif (button_on == 1 and event[7:10] == button_which +'r') :
       ser.write(('b'  + button_which + '0' + '\r').encode()) 
       button_on = 0
       button_which = ' '
    #
    # toggle switches
    if event[0:3]  == 'tog'  : 
       ser.write(('t' + event[6:8] + str(switch_state[int(event[6:8])]) + '\r').encode()) 
    #
    # silder events
    if event[0:3]  == 'sli'  :
       ser.write(('s ' + event[6] + " {:d}".format(int(values['slider'+ event[6]])) + '\r').encode()) 
    #
    # reset events
    if event[0:3] == 'rtg' and switch_state[0] == 1 :
       # drops the data line for 100 mSec
       ser.send_break() #optional duration; duration=0.01
    #
    # The one-line text input button event
    if event == 'pic_send':
       # add <cr> for PIC
       input_state = '$' + input_state + '\r'
       # zero the input field
       window['pic_input'].update('')
       # send to PIC protothreads
       ser.write((input_state).encode())
       #
    # character loopback from PIC
    if ser.in_waiting > 0:
       pic_char = chr(ser.read(size=1)[0])   
       if (pic_char) == '\r' :
          window['console'].update('\n', append=True)
       else :
          window['console'].update((pic_char), append=True)
       
# close port and Bail out
ser.close()             
window.close()





