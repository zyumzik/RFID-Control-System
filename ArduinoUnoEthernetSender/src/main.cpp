/*
 ARDUINO UNO ETHERNET SENDER {ethernet shield 2 + rs485}
 Name:		ArduinoUnoEthernetSender.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <Ethernet.h>
#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <SPI.h>

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

#pragma region GLOBAL_SETTINGS

#define         program_version "0.8.5"
#define         serial_baud     115200          // debug serial baud speed
#define         broadcast_id    999             // id for receiving broadcast messages (for all devices)

void(* resetBoard) (void)       = 0;            // reset Arduino Uno function
EasyTransfer    easy_transfer;                  // RS485 message exchanger
Message         message;                        // exchangeable object

#pragma endregion //GLOBAL_SETTINGS

#pragma region V_RS485

#define         rs_rx_pin       2               // receive pin
#define         rs_tx_pin       3               // transmit pin
#define         rs_pwr_pin      9               // power (5v) pin
#define         rs_baud         9600            // baud speed
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // custom rx\tx serial

#pragma endregion //V_RS485

#pragma region V_ETHERNET

#define         reconnect_delay 1000            // delay for try to reconnect ethernet
EthernetClient  client;                         // object for connecting server as client
byte            mac[] = { 0x54, 0x34, 
                          0x41, 0x30, 
                          0x30, 0x35 };

#pragma endregion //V_ETHERNET

#pragma region V_SERVER

#define         srvr_port   80                  // default HTTP server port
#define         srvr_name   "skdmk.fd.mk.ua"
#define         srvr_rqst   "GET /skd.mk/baseadd2.php?"
#define         srvr_rcv    500                 // time for waiting response from server

#pragma endregion //V_SERVER

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

// returns true if ethernet connection established
bool ethernetConnected();

// recursive function for establishing DHCP connection
void ethernetConnect();

// send data to server
void sendServer();

// receive response from server
void receiveServer();

// send message to slave
void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id);

// send broadcast message (for all of devices connected by RS485)
void sendBroadcast(unsigned short state_id, unsigned short other_id);

#pragma endregion //F_DECLARATION

void setup()
{
    pinMode(rs_pwr_pin, OUTPUT);
    digitalWrite(rs_pwr_pin, HIGH);
    rs485.begin(rs_baud);
    easy_transfer.begin(details(message), &rs485);
    SPI.begin();

    #if DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    debug_s("\n\n\n\t---Arduino Uno Ethernet Sender v.");
    debug(program_version);
    debugln_s("\t---");
    debug_s("\t---debug serial speed: ");
    debug(serial_baud);
    debugln_s("\t\t---");
    #endif //DEBUG

    ethernetConnect();
}

void loop()
{
    if (!ethernetConnected())
    {
        ethernetConnect();
    }

    if (easy_transfer.receiveData())
    {
        debugln_f("\nET << \t[ %u; %lu; %u; %u ]", 
            message.device_id, message.card_id, message.state_id, message.other_id);

        sendServer();
        receiveServer();
    }
}

#pragma region F_DESCRIPTION

bool ethernetConnected()
{
    if (Ethernet.linkStatus() == LinkON && 
        Ethernet.hardwareStatus() != EthernetNoHardware)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ethernetConnect()
{
    sendBroadcast(er_no_ethr_cnctn, 0);

    debug_s("connecting ethernet. link status = ");
    debug(Ethernet.linkStatus());
    debug_s("; hardware status = "); 
    debugln(Ethernet.hardwareStatus());
    while(!ethernetConnected())
    {
        delay(reconnect_delay);
    }
    debugln_s("connected");

    // connecting to network (using mac address and DHCP)
    if (Ethernet.begin(mac) == 0)
    {
        debugln_s("critical error: DHCP failed. reloading board\n");

        delay(reconnect_delay);

        resetBoard();
    }

    debugln_f("DHCP succeeded. ip: %u.%u.%u.%u", 
        Ethernet.localIP()[0], Ethernet.localIP()[1], Ethernet.localIP()[2], Ethernet.localIP()[3]);

    sendBroadcast(er_no_ethr_cnctn, 1);
    debugln();
}

void sendServer()
{
    // no ethernet or server connection, trying to reconnect
    if (!client.connect(srvr_name, srvr_port))
    {
        sendData(
            message.device_id, 
            message.card_id, 
            er_no_srvr_cnctn,
            0
        );
        return;
    }
    
    // connection established
    client.print(srvr_rqst);
    client.print("id=");
    client.print(message.card_id);
    client.print("&kod=");
    client.print(message.device_id);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(srvr_name);
    client.println("Connection: close");
    client.println();
    client.println();

    debug_s("web  >> skdmk.fd.mk.us/skd.mk/baseadd2.php?id=");
    debug(message.card_id);
    debug_s("&kod=");
    debugln(message.device_id);

    // rare error check
    if (!client.find("\r\n\r\n"))
    {
        client.stop();
        sendData(
            message.device_id,
            message.card_id,
            er_request,
            0
        );
    }
}

void receiveServer()
{
    // smart receive waiting
    unsigned short counter = 0;
    while (counter <= srvr_rcv)
    {
        if (client.available())
            break;

        delay(1);
        counter++;
    }

    // available data in cleint for read
    if (client.available())
    {
        StaticJsonDocument<256> json;
        DeserializationError error = deserializeJson(json, client);

        // deserialization error
        if (error)
        {
            debug_s("json error: ");
            debugln(error.c_str());
            
            sendData(
                message.device_id, 
                message.card_id, 
                er_json, 
                0
            );
        }
        // successfully received response from server
        else
        {
            unsigned short  json_device_id = strtoul(json["kod"].as<const char*>(), NULL, 0);
            unsigned long   json_card_id   = strtoul(json["id"].as<const char*>(), NULL, 0);
            unsigned short  json_state_id  = json["status"].as<unsigned short>();

            debugln_f("json <<\t{ 'kod='%u; 'id'=%lu, 'status'=%u }", 
                json_device_id, json_card_id, json_state_id);

            // no response from server
            if (json_device_id == 0 && json_card_id == 0 && json_state_id == 0)
            {
                sendData(
                    message.device_id,
                    message.card_id,
                    er_no_response,
                    0
                );
            }
            // correct response
            else
            {
                sendData(
                    json_device_id,
                    json_card_id,
                    json_state_id,
                    0
                );
            }
        }
    }
    else
    {
        sendData(
            message.device_id,
            message.card_id,
            er_timeout,
            0
        );
    }

    client.stop();
}

void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id)
{
    message.set(device_id, card_id, state_id, other_id);

    debugln_f("ET >> \t[ %u; %lu; %u; %u ]", 
        message.device_id, message.card_id, message.state_id, message.other_id);

    easy_transfer.sendData();

    message.clean();
}

void sendBroadcast(unsigned short state_id, unsigned short other_id)
{
    message.set(broadcast_id, 0, state_id, other_id);

    debugln_f("ET >>> \t[ %u; %lu; %u; %u ]", 
        message.device_id, message.card_id, message.state_id, message.other_id);

    easy_transfer.sendData();

    message.clean();
}

#pragma endregion //F_DESCRIPTION
