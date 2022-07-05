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

EasyTransfer    easy_transfer;                  // object for exchanging data between RS485
Message         message;                        // object for exchanging data between ArduinoNano and ArduinoUno

#pragma endregion

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 0;                  // RS485 receive pin
uint8_t         rs_tx_pin = 1;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485

#pragma endregion

#pragma region ETHERNET_SETTINGS

EthernetClient  client;                         // object for connecting ethernet as client
byte            mac[] = { 0xFF, 0xFF, 
                          0xFF, 0xFF, 
                          0xFF, 0xFF };         // mac address of this device. must be unique in local network
uint8_t         reconnect_delay = 20;           // delay for try to reconnect ethernet

#pragma endregion

#pragma region SERVER_SETTINGS

int             server_port             = 80;   // server port
char*           server_name =                 
                "skdmk.fd.mk.ua";               // server address
String          server_request = 
                "GET /skd.mk/baseadd.php?";     // GET request address

// smart receive
unsigned long   server_receive_max_wait = 600;  // time for waiting response from server
uint8_t         server_receive_counter  = 0;    // counter for receive waiting

#pragma endregion

#pragma region SERVER_STATES

const uint8_t   state_no_response   = 97;       // no response from server
const uint8_t   state_json_error    = 98;       // json deserialization error
const uint8_t   state_timeout_error = 99;       // server connection timeout
const uint8_t   state_ok            = 200;      // accessed

#pragma endregion

void setup()
{
    rs485.begin(rs_baud);
    SPI.begin();
    easy_transfer.begin(details(message), &rs485);

#ifdef DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    Serial.println(
        String("start working...") + 
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
        Serial.println("received message: ");
        message.print();
#endif //DEBUG

        sendServer();
    }
}

// recursive function for connecting ethernet using DHCP
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
    Serial.print("ip: " + Ethernet.localIP());
    Serial.println(", subnet mask: " + Ethernet.subnetMask());
#endif //DEBUG

    return true;
}

// send data to server
void sendServer()
{
    if (client.connect(server_name, server_port))
    {
        client.print(server_request);
        client.print("kod=");
        client.print(message.device_id);
        client.print("&id=");
        client.print(message.card_id);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server_name);
        client.println("Connection: close");
        client.println();
        client.println();

        receiveServer();
    }
    else
    {
#ifdef DEBUG
        Serial.println("client connect error");
        Serial.println("ethernet status: " + int(Ethernet._state));
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
        sendData(message.device_id, message.card_id, state_timeout_error, 0);
    }

    client.stop();
}

// receive response from server
void receiveServer()
{
    // smart receive waiting
    server_receive_counter = 0;
    while (server_receive_counter < server_receive_max_wait)
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
            Serial.println("deserialization error: ");
            Serial.println(error.c_str());
#endif // DEBUG
            
            sendData(
                message.device_id, 
                message.card_id, 
                state_json_error, 
                0);
        }
        // successfully received response from server
        else
        {
            sendData(
                json["id"].as<uint_fast16_t>(),
                json["kod"].as<uint32_t>(),
                json["status"].as<uint8_t>(),
                0);
        }
    }
    else
    {
#ifdef DEBUG
        Serial.println("error: no response from server");
#endif // DEBUG
        sendData(
            message.device_id,
            message.card_id,
            state_no_response,
            0);
    }
}

// send message to ArduinoNano
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
    message.set(device_id, card_id, state_id, other_id);

    easy_transfer.sendData();

    message.clean();

#ifdef DEBUG
    Serial.println("send data:");
    message.print();
#endif // DEBUG
}