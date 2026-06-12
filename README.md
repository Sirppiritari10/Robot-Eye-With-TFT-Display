# Robot-Eye-With-TFT-Display
Arduino code for running a set of robot eyes. Originally made with the XIAO Esp32S3 and two Adafruit 1.44" Color TFT LCD Display with MicroSD Card breakout - ST7735R
Also some python code for manually testing the eyes.
This repository will not be updated regularly. Do what you want with it I don't guarantee anything ;P

While the code is made for the specific display, it can be changed easily enough to work with other TFT displays if they are supported by the adafruit GFX library.

----------

How to control your eyes:
Controlling the eyes is done by sending JSON files, that contain commands and values for the specific commands.
These files can be sent either through serial or through a wifi connection depending on which version is 
running on your (assumably) esp32s3.


A correct command is a string, that contains(aka. can be parsed to) a JSON file. The read ends in a newline ("\n") character. 
(Note that some JSON to string functions might add newlines inside of the string for readability, causing errors)



example command strings (not correct syntax for Strings, but you get the point hopefully):
{"cmd":"look", "value":{"x": 0.34, "y": 0.117}}\n

{"cmd":"state", "value":"DEBUG"}\n

{"cmd":"mood", "value":"CONFUSED"}\n

{"cmd":"blink"}\n

{"cmd":"pupilR", "value":{"r":0.69}}\n

Different states and what they do:

"IDLE" = moves the eyes randomly, also blinks randomly
"CONV" = Small eye glances and blinks only
"ACTIVE" = no automated reactions or actions, full manual control
"DEBUG" = testing visuals instead of real animation, only meant for debugging


Look command:
x-y go from 0 to 1, where 0 is a corner (should be upper left iirc) and 1 is lower right.

"mood" command:
"NEUTRAL"
"HAPPY"
"SAD"
"SURPRISED"
"CONFUSED"

Changes the "frown" aka. how close the eylids are by default.

"pupilR" command:
Changes the radius of the pupils. Radius between 0 and 1. Constrained in code.


"blink" command:
Blinks once.

"glance" command:
if "value" field is empty or missing:
glances in random direction for abt 0.5 sec.
if "value" field has x and y parameters (similair to look):
glances in the look direction. x and y parameters constrained between -1 and 1.
(Note the x and y are translations instead of absolute positions in the glance command)
