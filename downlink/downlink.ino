/*

WISNODE-LORA downlink example

Copyright (C) 2018
Xose Pérez <xose dot perez at gmail dot com>
for The Things Network Catalunya Wiki (http://thethingsnetwork.cat)

Requires RAK811 library (https://github.com/RAKWireless/RAK811-AT-Command/tree/master/Arduino/RAK811)

*/

#include "RAK811.h"
#include "credentials.h"

#define APP_PORT        9       // Application port
#define SEND_EVERY      60000   // Send message very 50 seconds
#define REQUIRE_ACK     1       // Require ACK
#define RETRIES         3       // Number of retries if message fails
#define RETRY_AFTER     5000    // Wait these many ms before retrying
#define RELAY_GPIO      13      // Digital GPIO for the LED

// -----------------------------------------------------------------------------

// The Leonardo uses Serial for USB and
// Serial1 for hardware RX/TX (in the header)
RAK811 rak811(Serial1);

// Holds the value to send
byte counter = 0;

// -----------------------------------------------------------------------------

void printResponse(String response) {
    if (response.startsWith("OK")) {
        response = response.substring(2);
    }
    Serial.println(response);
}

void radio_info() {

    // Configuration
    Serial.println("Configuring radio");
    rak811.rk_setConfig("adr", "on");
    rak811.rk_setConfig("duty", "on"); // LoRaWan compliant

    Serial.print("Module version: ");
    printResponse(rak811.rk_getVersion());

    Serial.print("Band: ");
    printResponse(rak811.rk_getBand());

    Serial.print("Power: ");
    printResponse(rak811.rk_getConfig("tx_power"));

    Serial.print("ADR: ");
    printResponse(rak811.rk_getConfig("adr"));

    Serial.print("Datarate: ");
    printResponse(rak811.rk_getConfig("dr"));

    Serial.print("Retrans: ");
    printResponse(rak811.rk_getConfig("retrans"));

    Serial.print("Duty: ");
    printResponse(rak811.rk_getConfig("duty"));

}

void ttn_join() {

    Serial.println("Setting work mode to LoRaWan");
    if (!rak811.rk_setWorkingMode(WORK_MODE)) {
        Serial.println("Error setting work mode");
        while(true);
    }

    #if JOIN_MODE == OTAA
        Serial.println("Init OTAA parameters");
        if (!rak811.rk_initOTAA(DEVEUI, APPEUI, APPKEY)) {
            Serial.println("Wrong configuration parameters");
            while(true);
        }
    #endif

    #if JOIN_MODE == ABP
        Serial.println("Init ABP parameters");
        if (!rak811.rk_initABP(DEVADDR, NWKSKEY, APPSKEY)) {
            Serial.println("Wrong configuration parameters");
            while(true);
        }
    #endif

    Serial.println("Connecting");
    if (!rak811.rk_joinLoRaNetwork(JOIN_MODE)) {
        Serial.println("Error joining network");
        return;
    }

    Serial1.setTimeout(8000);
    String response = Serial1.readStringUntil('\n');
    if (!response.startsWith(STATUS_JOINED_SUCCESS)) {
        Serial.print("Unexpected response: ");
        Serial.println(response);
        while(true);
    }
    Serial.println("Succesfully connected!!");

}


void ttn_send() {

    Serial.println("Sending message");

    // Send message
    unsigned char tries = 0;
    while (true) {

        // The payload has to be an HEX string
        char buffer[3];
        bool state = !digitalRead(RELAY_GPIO); // relay has inverse logic
        snprintf(buffer, sizeof(buffer), "%02X", state ? 0x31 : 0x30);

        // Sending test message
        if (rak811.rk_sendData(REQUIRE_ACK, APP_PORT, buffer)) {
            Serial.println("Message sent correctly!");
            return;
        }

        tries++;
        if (tries > RETRIES) break;

        Serial.println("Error, trying again...");
        delay(RETRY_AFTER);

    }

    Serial.println("Error sending message :(");

}

void ttn_receive() {

    // at+recv=<status>,<port>[,<rssi>][,<snr>],<len>[,<data>]\r\n
    // STATUS_RECV_DATA == 0
    String r = rak811.rk_recvData();
    if (r.length()) {

        Serial.print("Response: ");
        Serial.println(r);

        if (r.startsWith("at+recv=0,")) {

            bool state = r.endsWith(",31");
            Serial.print("Turning relay ");
            Serial.println(state ? "ON" : "OFF");
            digitalWrite(RELAY_GPIO, !state); // relay has inverse logic

        }

    }

}

void setup() {

    // Init debug serial1
    Serial.begin(115200);
    delay(5000);
    Serial.println();
    Serial.println("WisNode-LoRa downlink example");
    Serial.println("@xoseperez @ttncat");
    Serial.println();

    // Init UART connection to the RAK811 module
    Serial1.begin(115200);

    // Setup GPIO
    pinMode(RELAY_GPIO, OUTPUT);
    digitalWrite(RELAY_GPIO, true); // relay has inverse logic

    // Configure radio & get info
    radio_info();
    Serial.println();

    // Join TTN Network
    ttn_join();
    Serial.println();

}

void loop() {
    static unsigned long last = 0;
    if ((last == 0) || (millis() - last > SEND_EVERY)) {
        last = millis();
        ttn_send();
    }
    ttn_receive();
}
