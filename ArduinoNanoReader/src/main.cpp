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
#include <Timer.h>
#include <Wiegand.h>
#include <WiegandSignal.h>

#define DEBUG true

#if DEBUG

char debug_buffer[64];
#define debug(v)        Serial.print(v)
#define debugln(v)      Serial.println(v)
#define debug_s(s)      Serial.print(F(s))
#define debugln_s(s)    Serial.println(F(s))
#define debugln_f(s, values...) { sprintf(debug_buffer, s, ##values); Serial.println(debug_buffer); }

#else

#define debug(v)
#define debugln(v)
#define debug_s(s)
#define debugln_s(s)
#define debugln_f(s, values...)

#endif

#define SET_DEV_ID  false
#define NEW_DEV_ID  999
#define WICKET      true
#define REED_SWITCH true

#pragma region GLOBAL_SETTINGS

#define         program_version "0.8.4"
#define         serial_baud     115200          // debug serial baud speed
#define         broadcast_id    999             // id for receiving broadcast messages
#define         handle_delay    0               // handle received response delay (for skipping default wiegand blink and beep)

unsigned long   device_id       = 803;          // unique ID of reader device
EasyTransfer    easy_transfer;                  // object for exchanging data using RS485
Message         message;                        // exchangeable object for EasyTransfer
bool            ethernet_flag   = true;         // flag of ethernet connection (true if connection established)

#pragma endregion //GLOBAL_SETTINGS

#pragma region V_RS485

#define         rs_rx_pin   4                   // receive pin
#define         rs_tx_pin   5                   // transmit pin
#define         rs_baud     9600                // baud rate (speed)
#define         rs_rspns    1500                // max waiting response time

SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485
bool            rs_flag = true;                 // true if response from master is being receiving
Timer           rs_wait_timer;                  // timer for waiting respinse from master

#pragma endregion //V_RS485

#pragma region V_WIEGAND

#define         w_rx_pin    2                   // receive pin
#define         w_tx_pin    3                   // transmit pin
#define         w_zum_pin   6                   // built-in zummer pin
#define         w_led_pin   7                   // built-in led pin
#define         w_delay     1000                // read card delay

WIEGAND         wiegand;                        // object for reading data from Wiegnad RFID
unsigned long   w_last_card;                    // last read card id
WiegandSignal   w_signal(w_led_pin, w_zum_pin); // object for signaling with led and zummer
Timer           w_timer;                        // read card timer

#pragma endregion //V_WIEGAND

#pragma region V_WICKET

#define         wicket_pin      8               // wicket relay pin
#define         wicket_time     150             // opened wicket time
Timer           wicket_timer;                   // timer for wicket open control

#pragma endregion //V_WICKET

#pragma region V_REED_SWITCH

#define         reed_pin        12              // generates high voltage if door opened
#define         reed_time       10000           // do alert time with opened door
Timer           reed_timer;                     // opened door timer
bool            reed_flag = true;               // 

#pragma endregion //V_REED SWITCH

#pragma region SERVER_STATES
                                                
// responses:
#define st_unknown          0                   // unknown state
#define st_allow            1                   // access allowed
#define st_re_entry         2                   // re-entry
#define st_denied           3                   // access denied
#define st_invalid          4                   // invalid card (database does not contain such card)
#define st_blocked          5                   // card is blocked

// errors:
#define er_no_srvr_cnctn    95                  // no server connection             | !client.connect(server, port)
#define er_request          96                  // wrong server request             | !client.find("\r\n\r\n")
#define er_no_response      97                  // no response from server          | json {"id":0,"kod":0,"status":0}
#define er_json             98                  // json deserialization error       | if (DeserializationError)
#define er_timeout          99                  // server connection timeout        | !client.available()
#define er_no_ethr_cnctn    100                 // no ethernet connection           | !ethernetConnected()

#pragma endregion //SERVER_STATES

#pragma region F_DECLARATION

// clear EEPROM
void clearMemory();

// save device id to EEPROM
void saveDeviceId(int address, unsigned long id);

// load device id from EEPROM
unsigned long loadDeviceId(int address);

// send message to master (Arduino Uno)
void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id);

// reverses code by bit
unsigned long wiegandToDecimal(unsigned long code);

// handle response from server
void handleResponse();

#pragma endregion //F_DECLARATION

void setup()
{
    #if SET_DEV_ID
    clearMemory();
    saveDeviceId(0, NEW_DEV_ID);
    #endif //SET_DEV_ID    

    #if WICKET
    pinMode(wicket_pin, OUTPUT);
    #endif //WICKET

    #if REED_SWITCH
    pinMode(reed_pin, INPUT_PULLUP);
    #endif //REED_SWITCH

    //device_id = loadDeviceId(0);  // defined in global settings

    wiegand.begin(w_rx_pin, w_tx_pin);
    pinMode(w_zum_pin, OUTPUT);
    pinMode(w_led_pin, OUTPUT);

    rs485.begin(rs_baud);
    easy_transfer.begin(details(message), &rs485);
    

    #if DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    debug_s("\n\n\n\t---Arduino Nano RFID Reader v.");
    debug(program_version);
    debugln_s("\t---");
    debug_s("\t---debug serial speed: ");
    debug(serial_baud);
    debugln_s("\t\t---");
    #endif //DEBUG

    // registering new connected device
    sendData(
        device_id,
        0,
        0,
        0
    );
    w_timer.begin(w_delay);
}

void loop()
{
    // wicket close
    #if WICKET
    if (wicket_timer.update())
    {
        digitalWrite(wicket_pin, HIGH);
        wicket_timer.stop();
        debugln_s("wicket closed");
    }
    #endif //WICKET

    #if REED_SWITCH
    if (reed_timer.update())
    {
        reed_flag = false;
        reed_timer.stop();
    }
    if (!reed_flag)
    {
        if (digitalRead(reed_pin) == LOW)
        {
            reed_flag = true;
        }
    }
    #endif //REED_SWITCH

    // too much time passed since last send (no response from master)
    if (rs_wait_timer.update())
    {
        rs_flag = false;
    }

    // received message from master
    if (easy_transfer.receiveData())
    {
        handleResponse();
    }

    // making signal if something is wrong
    if (!reed_flag)
    {
        w_signal.update(WiegandSignal::Length::s_short_short, false, true);
    }
    else if (!rs_flag)
    {
        w_signal.update(WiegandSignal::Length::s_short, false, true);
    }
    else if (!ethernet_flag)
    {
        w_signal.update(WiegandSignal::Length::s_medium, false, true);
    }
    else
    {
        w_signal.update();
    }

    // read a card
    if (wiegand.available())
    {
        w_last_card = wiegandToDecimal(wiegand.getCode());
        
        if (w_timer.update())
        {
            // check for ethernet connection, signaling state
            if (ethernet_flag && !w_signal.is_invoke)
            {
                debug_s("read card: ");
                debugln(w_last_card);
                sendData(
                    device_id, 
                    w_last_card, 
                    0, 
                    0
                );
            }
        }
    }
}

#pragma region F_DESCRIPTION

void clearMemory()
{
    for (int i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.write(i, 0);
    }
    debugln_s("EEPROM cleared");
}

void saveDeviceId(int address, unsigned long id)
{
    byte buf[4];
    (unsigned long&)buf = id;
    for(byte i = 0; i < 4; i++) 
    {
        EEPROM.write(address + i, buf[i]);
    }
    debug_s("new device id written: ");
    debugln(id);
}

unsigned long loadDeviceId(int address)
{
    byte buf[4];
    for(byte i = 0; i < 4; i++)
    { 
        buf[i] = EEPROM.read(address + i);
    }
    unsigned long &id = (unsigned long&)buf;
    return id;
}

void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id)
{
    rs_wait_timer.begin(rs_rspns);

    message.set(device_id, card_id, state_id, other_id);

    debugln_f("\nET >> \t[ %u; %lu; %u; %u ]", 
        message.device_id, message.card_id, message.state_id, message.other_id);

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
    debugln_f("\nET << \t[ %u; %lu; %u; %u ]", 
            message.device_id, message.card_id, message.state_id, message.other_id);

    // base data checking (if message was for this device)
    if (message.device_id != broadcast_id && message.device_id != device_id)
    {
        debugln("message not handled");
        return;
    }
        
    rs_flag = true;
    rs_wait_timer.stop();

    delay(handle_delay);

    // if error - long signal firstly
    if (message.state_id >= 90)
    {
        w_signal.invoke(WiegandSignal::Length::s_long_long, 1);
    }

    switch (message.state_id)
    {
    case st_unknown:
    {
        w_signal.invoke(WiegandSignal::Length::s_long, 3);
        debugln_s("unknown status");
        break;
    }
    case st_allow:
    {
        debugln_s("access allowed");

        // opening a wicket
        #if WICKET
        debugln_s("wicket opened");
        wicket_timer.begin(wicket_time);
        digitalWrite(wicket_pin, LOW);
        #endif //WICKET

        #if REED_SWITCH
        debugln_s("reed timer started");
        reed_timer.begin(reed_time);
        #endif //REEDSWITCH

        break;
    }
    case st_re_entry:
    {
        w_signal.invoke(WiegandSignal::Length::s_long, 2);
        debugln_s("re-entry");
        break;
    }
    case st_denied:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium, 10);
        debugln_s("access denied");
        break;
    }
    case st_invalid:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium, 5);
        debugln_s("invalid card");
        break;
    }
    case st_blocked:
    {
        w_signal.invoke(WiegandSignal::Length::s_short, 10);
        debugln_s("card blocked");
        break;
    }
    
    // errors:
    case er_no_srvr_cnctn:
    {
        w_signal.invoke(WiegandSignal::Length::s_long, 3);
        debugln_s("error: no server connection");
        break;
    }
    case er_request:
    {
        w_signal.invoke(WiegandSignal::Length::s_short, 5);
        debugln_s("error: wrong server request");
        break;
    }
    case er_no_response:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium , 3);
        debugln_s("error: no response from server");
        break;
    }
    case er_json:
    {
        w_signal.invoke(WiegandSignal::Length::s_short, 5);
        debugln_s("error: wrong json deserialization");
        break;
    }
    case er_timeout:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium, 3);
        debugln_s("error: server connection timeout");
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
        debug_s("ethernet connection: ");
        debugln(ethernet_flag);
        break;
    }
    
    // unknown state
    default:
    {
        debugln_s("unknown state. message not handled");
        break;
    }
    }

    message.clean();
}

#pragma endregion //F_DESCRIPTION
