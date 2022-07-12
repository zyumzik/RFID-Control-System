/*
 ARDUINO NANO READER {rfid + rs485}
 Name:		ArduinoNanoReader.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <EasyTransfer.h>
#include <EEPROM.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <Wiegand.h>
#include <Timer.h>

#pragma region GLOBAL_SETTINGS

#define DEBUG                                   // comment this line to not write anything to Serial as debug
const String    program_version = "0.9.4";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

unsigned long   broadcast_id    = 999;          // id for receiving broadcast messages
unsigned long   device_id       = 0;            // unique ID of reader device
unsigned long   read_delay      = 1000;         // reading card delay
unsigned long   read_last       = 0;            // last read card time
unsigned long   handle_delay    = 250;          // handle received response delay (for skipping default wiegand blink and beep)
EasyTransfer    easy_transfer;                  // object for exchanging data using RS485
Message         message;                        // exchangeable object for EasyTransfer
bool            ethernet_flag   = true;         // flag of ethernet connection (true if connection established)

#pragma endregion //GLOBAL_SETTINGS

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 4;                  // RS485 receive pin
uint8_t         rs_tx_pin = 5;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485
bool            rs_connected = true;            // true if response from Arduino Uno is being receiving
Timer           rs_wait_timer;                  // timer for waiting respinse from Arduino Uno
unsigned long   rs_last_send = 0;               // last send to Arduino Uno
unsigned long   rs_response_wait = 1500;        // time for waiting response from Arduino Uno

#pragma endregion //RS485_SETTINGS

#pragma region WIEGAND_SETTINGS

WIEGAND         wiegand;                        // object for reading data from Wiegnad RFID
uint8_t         w_rx_pin  = 2;                  // wiegand receive pin
uint8_t         w_tx_pin  = 3;                  // wiegand transmit pin
uint8_t         w_zum_pin = 6;                  // wiegand built-in zummer control pin
uint8_t         w_led_pin = 7;                  // wiegand built-in led control pin
unsigned long   w_last_card;                    // last read card id

bool            w_led_state = false;            // state of wiegand led (true if switched on)
bool            w_zum_state = false;            // state of wiegand zum (true if switched on)
Timer           w_led_timer;                    // timer for wiegand blinking
Timer           w_zum_timer;                    // timer for wiegand beeping
uint32_t        w_led_period = 100;             // period of led blinking
uint32_t        w_zum_period = 5000;            // period of zum beeping

#pragma endregion //WIEGAND_SETTINGS

#pragma region SERVER_STATES

// responses:
const uint8_t   st_denied           = 0;          // access denied
const uint8_t   st_allow            = 1;          // access allowed
const uint8_t   st_rest_denied      = 2;          // access denied for restricted area
const uint8_t   st_rest_allow       = 3;          // access allowed for restricted area
const uint8_t   st_temp_denied      = 4;          // access temporary denied

const uint8_t   st_reg_device       = 90;         // registration of device (check for similar device id in readers)
const uint8_t   st_set_device_id    = 92;         // setting device id

// errors:
const uint8_t   er_no_srvr_cnctn    = 95;         // no server connection             | !client.connect(server, port)
const uint8_t   er_request          = 96;         // wrong server request             | !client.find("\r\n\r\n")
const uint8_t   er_no_response      = 97;         // no response from server          | json {"id":0,"kod":0,"status":0}
const uint8_t   er_json             = 98;         // json deserialization error       | if (DeserializationError)
const uint8_t   er_timeout          = 99;         // server connection timeout        | !client.available()
const uint8_t   er_no_ethr_cnctn    = 100;        // no ethernet connection           | !ethernetConnected()

#pragma endregion //SERVER_STATES

#pragma region FUNCTION_DECLARATION

// clear EEPROM
void clearMemory();

// save device id to EEPROM
void saveDeviceId(uint8_t address, unsigned long id);

// load device id from EEPROM
unsigned long loadDeviceId(uint8_t address);

// send message to Arduino Uno (Ethernet Sender)
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id);

// reverses code by bit
unsigned long wiegandToDecimal(unsigned long code);

// handle response from server
void handleResponse();

// makes sound for beepMs time and delayMs delay
void wiegandBeep(unsigned long beepMs, unsigned long delayMs);

// blinks for beepMs time and delayMs delay
void wiegandBlink(unsigned long blinkMs, unsigned long delayMs);

#pragma endregion //FUNCTION_DECLARATION

void setup()
{
    // uncomment lines below to save some unique id to EEPROM
    // clearMemory();
    // saveDeviceId(0, -unique id-);    

    device_id = loadDeviceId(0);

    wiegand.begin(w_rx_pin, w_tx_pin);
    pinMode(w_zum_pin, OUTPUT);
    pinMode(w_led_pin, OUTPUT);
    w_led_timer.begin(w_led_period);
    w_zum_timer.begin(w_zum_period);
    rs485.begin(rs_baud);
    easy_transfer.begin(details(message), &rs485);
    
#ifdef DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    Serial.println(
        String("Arduino Nano AT328 RFID-WG Reader. V." + program_version) +
        String("\nstart working on ") + serial_baud + " baud speed" + 
        String("\ndevice id: ") + device_id);
#endif //DEBUG

    // registering new connected device
    sendData(
        device_id,
        0,
        st_reg_device,
        0
    );
}

void loop()
{
    if (rs_wait_timer.update())
    {
        rs_connected = false;
#ifdef DEBUG
        Serial.println("no RS485 connection...");
#endif //DEBUG
    }
    // received message from ArduinoUNO
    if (easy_transfer.receiveData())
    {
        rs_connected = true;
        rs_wait_timer.stop();
        handleResponse();
    }

    // blinking with wiegand led if Arduino Uno (master) has no ethernet connection or no RS485 connection
    if (!ethernet_flag || !rs_connected)
    {
        if (w_led_timer.update())
        {
            digitalWrite(w_led_pin, w_led_state);
            w_led_state = !w_led_state;
        }
        if (!rs_connected)
        {
            if (w_zum_timer.update())
            {
                digitalWrite(w_zum_pin,w_zum_state);
                w_zum_state = !w_zum_state;
            }
        }
    }

    // read a card
    if (wiegand.available())
    {
        read_last = millis();
        w_last_card = wiegandToDecimal(wiegand.getCode());
#ifdef DEBUG
        Serial.println("read card id: " + String(w_last_card));
        Serial.println("*****\n");
#endif //DEBUG
        if (ethernet_flag)
        {
            sendData(
                device_id, 
                w_last_card, 
                0, 
                0
            );
        }
    }
}

#pragma region FUNCTION_DESCRIPTION

void clearMemory()
{
    for (int i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.write(i, 0);
    }
}

void saveDeviceId(uint8_t address, unsigned long id)
{
    byte buf[4];
    (unsigned long&)buf = id;
    for(byte i = 0; i < 4; i++) 
    {
        EEPROM.write(address + i, buf[i]);
    }
}

unsigned long loadDeviceId(uint8_t address)
{
    byte buf[4];
    for(byte i = 0; i < 4; i++)
    { 
        buf[i] = EEPROM.read(address + i);
    }
    unsigned long &id = (unsigned long&)buf;
    return id;
}

void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
    rs_last_send = millis();
    rs_wait_timer.begin(rs_response_wait);

    message.set(device_id, card_id, state_id, other_id);

#ifdef DEBUG
    Serial.println("---send---");
    message.print();
    Serial.println("*****\n");
#endif //DEBUG

    easy_transfer.sendData();

    message.clean();
}

unsigned long wiegandToDecimal(unsigned long code)
{
    unsigned long code_decimal = 0;
    int t = 0;
    long unsigned int a = 0, b = 0;
    while(code > 0)
    {
        t++;
        a = code / 256;
        b = code - a * 256;
        code = a;
        code_decimal = code_decimal * 256 + b;
    }
    return code_decimal;
}

void handleResponse()
{
#ifdef DEBUG
    Serial.println("-received-");
    message.print();
    Serial.println("*****\n");
#endif // DEBUG
    // base data checking (if message was for this device)
    if (message.device_id != broadcast_id &&
        message.device_id != device_id)
    {
#ifdef DEBUG
        Serial.println("message for another device");
#endif //DEBUG
        return;
    }
        
    delay(handle_delay);

    switch (message.state_id)
    {
    case st_denied:
    {
        #ifdef DEBUG
        Serial.println("access denied");
        #endif //DEBUG
        break;
    }
    case st_allow:
    {
        #ifdef DEBUG
        Serial.println("access allowed");
        #endif // DEBUG
        break;
    }
    case st_rest_denied:
    {
        #ifdef DEBUG
        Serial.println("access denied in restricted area");
        #endif //DEBUG
        break;
    }
    case st_rest_allow:
    {
        #ifdef DEBUG
        Serial.println("access allowed in restricted area");
        #endif //DEBUG
        break;
    }
    case st_temp_denied:
    {
        #ifdef DEBUG
        Serial.println("access temporary denied");
        #endif //DEBUG
        break;
    }
    
    case st_set_device_id:
    {
        device_id = message.card_id;
        saveDeviceId(0, device_id);

        #ifdef DEBUG
        Serial.print("device id set to: ");
        Serial.println(device_id);
        #endif //DEBUG
        break;
    }
    
    // errors:
    case er_no_srvr_cnctn:
    {
        #ifdef DEBUG
        Serial.println("error: no server connection");
        #endif //DEBUG
        break;
    }
    case er_request:
    {
        #ifdef DEBUG
        Serial.println("error: wrong server request");
        #endif //DEBUG
        break;
    }
    case er_no_response:
    {
        #ifdef DEBUG
        Serial.println("error: no response from server");
        #endif //DEBUG
        break;
    }
    case er_json:
    {
        #ifdef DEBUG
        Serial.println("error: wrong json deserialization");
        #endif //DEBUG
        break;
    }
    case er_timeout:
    {
        #ifdef DEBUG
        Serial.println("error: server connection timeout");
        #endif //DEBUG
        break;
    }
    case er_no_ethr_cnctn:
    {
        if (message.other_id == 0)
        {
            ethernet_flag = false;
        }
        else if (message.other_id == 1)
        {
            ethernet_flag = true;
        }
        #ifdef DEBUG
        Serial.print("ethernet connection: ");
        Serial.println(message.other_id);
        #endif //DEBUG
        break;
    }
    
    // unknown state
    default:
    {
        #ifdef DEBUG
        Serial.println("unknown state. message not handled");
        #endif //DEBUG
        break;
    }
    }

    message.clean();
}

void wiegandBeep(unsigned long beepMs, unsigned long delayMs)
{
    digitalWrite(w_zum_pin, HIGH);
    delay(beepMs);
    digitalWrite(w_zum_pin, LOW);
    delay(delayMs);
}

void wiegandBlink(unsigned long blinkMs, unsigned long delayMs)
{
    digitalWrite(w_led_pin, HIGH);
    delay(blinkMs);
    digitalWrite(w_led_pin, LOW);
    delay(delayMs);
}

#pragma endregion //FUNCTION_DESCRIPTION