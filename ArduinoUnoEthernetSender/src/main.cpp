/*
 ARDUINO UNO ETHERNET SENDER {ethernet shield 2 + rs485}
 Name:		ArduinoUnoEthernetSender.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <EthernetClient.h>
#include <Ethernet2.h>
#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <SPI.h>

#pragma region GLOBAL_SETTINGS

#define DEBUG                                   // comment this line to not write anything to Serial as debug
const String    program_version = "0.9.0";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

EasyTransfer    easy_transfer;                  // object for exchanging data using RS485
Message         message;                        // exchangeable object for EasyTransfer

#pragma endregion //GLOBAL_SETTINGS

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 2;                  // RS485 receive pin
uint8_t         rs_tx_pin = 3;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485

#pragma endregion //RS485_SETTINGS

#pragma region ETHERNET_SETTINGS

EthernetClient  client;                         // object for connecting ethernet as client
byte            mac[] = { 0x54, 0x34, 
                          0x41, 0x30, 
                          0x30, 0x35 };         // mac address of this device. must be unique in local network
uint8_t         reconnect_delay = 20;           // delay for try to reconnect ethernet

#pragma endregion //ETHERNET_SETTINGS

#pragma region SERVER_SETTINGS

int             server_port             = 80;   // server port
char*           server_name =                 
                "skdmk.fd.mk.ua";               // server address
String          server_request = 
                "GET /skd.mk/baseadd.php?";     // GET request address

// smart receive
unsigned long   server_receive_max_wait = 600;  // time for waiting response from server
uint8_t         server_receive_counter  = 0;    // counter for receive waiting

#pragma endregion //SERVER_SETTINGS

#pragma region SERVER_STATES

const uint8_t   state_denied        = 0;        // access denied                //
const uint8_t   state_no_connection = 95;       // no ethernet connection       // !client.connect(server, port)
const uint8_t   state_request_error = 96;       // wrong server request         // !client.find("\r\n\r\n")
const uint8_t   state_no_response   = 97;       // no response from server      // json {"id":0,"kod":0,"status":0}
const uint8_t   state_json_error    = 98;       // json deserialization error   // if (DeserializationError)
const uint8_t   state_timeout_error = 99;       // server connection timeout    // !client.available()
const uint8_t   state_ok            = 1;        // accessed                     // 

#pragma endregion //SERVER_STATES

// recursive function for connecting ethernet using DHCP
bool ethernetConnect();

// send data to server
void sendServer();

// receive response from server
void receiveServer();

// send message to Arduino Nano (RFID Reader)
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id);

void setup()
{
    rs485.begin(rs_baud);
    SPI.begin();
    easy_transfer.begin(details(message), &rs485);

#ifdef DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    Serial.println(
        String("Arduino Uno Ethernet Sender") +
        String("\nstart working...") + 
        String("\nprogram version: ") + program_version +
        String("\nserial baud speed: ") + serial_baud);
#endif //DEBUG

    ethernetConnect();
}

void loop()
{
    if (easy_transfer.receiveData())
    {
#ifdef DEBUG
        Serial.println("-received-");
        message.print();
        Serial.println("*****\n");
#endif //DEBUG

        sendServer();

        receiveServer();
    }
}

#pragma region FUNCTION_DESCRIPTION

bool ethernetConnect()
{
#ifdef DEBUG
    Serial.println("connecting ethernet...");
#endif //DEBUG

    if (Ethernet.begin(mac) == 0)
    {
#ifdef DEBUG
        Serial.println("connection via DHCP is not established");
        Serial.println("trying to reconnect...");
#endif //DEBUG
        delay(reconnect_delay);
        ethernetConnect();
    }

#ifdef DEBUG
    Serial.println("DHCP connection established");
    Serial.print("ip: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print(Ethernet.localIP()[i]);
        Serial.print(".");
    }
    Serial.println(", subnet mask: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print(Ethernet.subnetMask()[i]);
        Serial.print(".");
    }
    Serial.println("\n*****\n");
#endif //DEBUG

    return true;
}

void sendServer()
{
    // no ethernet or server connection, trying to reconnect
    if (!client.connect(server_name, server_port))
    {
#ifdef DEBUG
        Serial.println("client connect error");
        Serial.print("ethernet status: ");
        for (int i = 0; i < 8; i++)
        {
            Serial.print(Ethernet._state[i]);
            Serial.print(" ");
        }
        Serial.println();
        Serial.println(String("all statuses::") + 
                    "\nsock_close: " + Sock_CLOSE + 
                    "\nsock_connect: " + Sock_CONNECT + 
                    "\nsock_discon: " + Sock_DISCON + 
                    "\nsock_listen: " + Sock_LISTEN + 
                    "\nsock_open: " + Sock_OPEN + 
                    "\nsock_recv: " + Sock_RECV + 
                    "\nsock_send: " + Sock_SEND + 
                    "\nsock_send_keep: " + Sock_SEND_KEEP + 
                    "\nsock_send_mac: " + Sock_SEND_MAC);
#endif //DEBUG
        sendData(
            message.device_id, 
            message.card_id, 
            state_no_connection,
            0
        );
        if (ethernetConnect())
            return;

        // II way (smart)
        //if (ethernetConnect())
        //{
        //    sendServer();
        //    return;  
        //}
    }

    // connection established
    client.print(server_request);
    client.print("id=");
    client.print(message.card_id);
    client.print("&kod=");
    client.print(message.device_id);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server_name);
    client.println("Connection: close");
    client.println();
    client.println();

#ifdef DEBUG
    Serial.println("send http request:");
    Serial.print(server_request);
    Serial.print("id=");
    Serial.print(message.card_id);
    Serial.print("&kod=");
    Serial.print(message.device_id);
    Serial.println(" HTTP/1.1");
    Serial.print("Host: ");
    Serial.println(server_name);
    Serial.println("Connection: close");
    Serial.println();
    Serial.println();
#endif //DEBUG

    if (!client.find("\r\n\r\n"))
    {
#ifdef DEBUG
        Serial.println("invalid request (not sent)");
#endif //DEBUG
        client.stop();
        sendData(
            message.device_id,
            message.card_id,
            state_request_error,
            0
        );
        return;
    }
}

void receiveServer()
{
    // smart receive waiting
    server_receive_counter = 0;
    while (server_receive_counter <= server_receive_max_wait)
    {
        if (client.available())
            break;

        delay(1);
        server_receive_counter++;
    }

    // available data in cleint for read
    if (client.available())
    {
        StaticJsonDocument<256> json;
        DeserializationError error = deserializeJson(json, client);

        // deserialization error
        if (error)
        {
#ifdef DEBUG
            Serial.print("deserialization error: ");
            Serial.println(error.c_str());
#endif // DEBUG
            
            sendData(
                message.device_id, 
                message.card_id, 
                state_json_error, 
                0
            );
        }
        // successfully received response from server
        else
        {
            unsigned long json_device_id = strtoul(json["kod"].as<const char*>(), NULL, 0);
            unsigned long json_card_id   = strtoul(json["id"].as<const char*>(), NULL, 0);
            unsigned long json_state_id  = json["status"].as<int>();
            //uint8_t         json_other_id   = json["other"].as<uint8_t>();  // not exist
#ifdef DEBUG
            Serial.println("serialized data from json:");

            Serial.print("{id:");
            Serial.print(json_card_id);
            Serial.print(",kod:");
            Serial.print(json_device_id);
            Serial.print(",status:");
            Serial.print(json_state_id);
            Serial.println("}");

            Serial.println("*****\n");

            Serial.println("sending data from json...");
#endif //DEBUG
            // no response from server
            if (json_device_id == 0 &&
                json_card_id == 0 &&
                json_state_id == 0)
            {
#ifdef DEBUG
                Serial.println("error: no response from server");
#endif //DEBUG
                sendData(
                    0,
                    0,
                    state_no_response,
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
#ifdef DEBUG
        Serial.println("error: response timeout");
#endif // DEBUG
        sendData(
            message.device_id,
            message.card_id,
            state_timeout_error,
            0
        );
    }

    client.stop();
}

void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
    message.set(device_id, card_id, state_id, other_id);

#ifdef DEBUG
    Serial.println("---send---");
    message.print();
    Serial.println("*****\n");
#endif //DEBUG

    easy_transfer.sendData();

    message.clean();
}

#pragma endregion //FUNCTION_DESCRIPTION
