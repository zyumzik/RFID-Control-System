# RFIDControlSystem
RFID control system based on Arduino

In this project I used Arduino Nano with Wiegnad-34 and Arduino Uno with Ethernet Shield 2 connected to each other with RS485. 
Arduino Nano (ArduinoNanoReader) - conditional slave, so Arduino Uno (ArduinoUnoEthernetSender) - master.
Actually, in this system can be used any amount of 'slaves' (ArduinoNanoReader) because of getting to them special 'device_id' (check ArduinoNanoReader.ino file)

Target of all this counstruction in a nutshell:
1) Controlling different systems (tourniquets, for example) using RFID 
2) Keeping records of enter/exit via magnet card
3) Whatever you can imagine

Principle of working:
RFID reader scans magnet card (it has 34 bits of information, it's called ID (it is even almost unique!), looks like '0123456789'), Arduino Nano sends card ID to
Arduino Uno with RS485 (UART interface). I'm using 'Easy Transfer' library for better data exchanging via RS485, so Arduinos just send a 'Message' structure to each 
other. When Arduino Uno gets message (btw, it contains device id, card id, state id and other id (for other things...)), it sends data to server via GET requset (I use 
Ethernet Shield module for connecting local network), waits for response, which notices via state id if access is allowed, if it is unknown or if there was no response 
from server and other things. Getting server response Arduino Nano beeps some sound signal via RFID built-in zummer depending on state id which symbolize status of
access. It was main principles of work, you can find more details in code or connection scheme below.

Code written by me:

[ArduinoNanoReader code](https://github.com/zyumzik/RFID-Control-System/blob/main/ArduinoNanoReader/src/main.cpp)

[ArduinoUnoEthernetSender code](https://github.com/zyumzik/RFID-Control-System/blob/main/ArduinoUnoEthernetSender/src/main.cpp)

[Message class](https://github.com/zyumzik/RFID-Control-System/blob/main/ArduinoNanoReader/src/Message.h)


Connection scheme:

[scheme.png (raw)](https://raw.githubusercontent.com/zyumzik/RFID-Control-System/main/scheme.png?token=GHSAT0AAAAAABWIFRJOPWLKP57SIWGGNYUOYWD6E3Q)
