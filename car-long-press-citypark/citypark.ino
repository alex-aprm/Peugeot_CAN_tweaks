#include <SPI.h>
#include <mcp_can.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
unsigned char wbBuf[3];
unsigned char vfBuf[8];
unsigned char cr1Buf[8];
unsigned char cr2Buf[8];

#define CAN0_INT 2 // Set INT to pin 2
MCP_CAN CAN0(10); // Set CS to pin 10

void setup()
{
    Serial.begin(115200);

    if (CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK)
        Serial.println("MCP2515 Initialized Successfully!");
    else
        Serial.println("Error Initializing MCP2515...");

    CAN0.setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.

    pinMode(CAN0_INT, INPUT); // Configuring pin for /INT input
    pinMode(5, OUTPUT);
}

bool carBtn = false;
bool carBtnReacted = false;
unsigned long carBtnPressed = 0;

bool memBtn = false;
bool memBtnReacted = false;
bool memBtnReactedFirst = false;
unsigned long memBtnPressedFirst = 0;
unsigned long memBtnPressedSecond = 0;

bool navBtn = false;
bool navBtnReacted = false;
unsigned long navBtnPressed = 0;

bool musicOn = false;

void loop()
{
    if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
    {
        CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
        if ((rxId & 0x40000000) != 0x40000000) {
            if (rxId == 0x122 /* Piano message */) {
                if ((rxBuf[2] & 0x01) == 0x01 /* Car button */) {
                    if (!carBtn) {
                        carBtn = true;
                        carBtnReacted = false;
                        carBtnPressed = millis();
                    }
                } else {
                    carBtn = false;
                    carBtnReacted = false;
                }

                if ((rxBuf[2] & 0x04) == 0x04 /* Nav button */) {
                    if (!navBtn) {
                        navBtn = true;
                        navBtnReacted = false;
                        navBtnPressed = millis();
                    }
                } else {
                    navBtn = false;
                    navBtnReacted = false;
                }

            } else if (rxId == 0x1A9 /* control frame message */) {
                for (byte i = 0; i < len; i++) {
                    vfBuf[i] = rxBuf[i];
                }
            } else if (rxId == 0x21F /* wheel buttons */) {
                for (byte i = 0; i < len; i++) {
                    wbBuf[i] = rxBuf[i];
                }
            } else if (rxId == 0x094 /* cruise message */) {
                for (byte i = 0; i < len; i++) {
                    cr1Buf[i] = rxBuf[i];
                }

                if ((rxBuf[3] & 0x02) == 0x02 /* Mem button */) {
                    if (!memBtn) {
                        memBtn = true;
                        if (!memBtnReactedFirst) {
                            Serial.println("mem1");
                            memBtnReactedFirst = true;
                            memBtnPressedFirst = millis();
                        } else {
                            Serial.println("mem2");
                            memBtnReacted = false;
                            memBtnReactedFirst = false;
                            memBtnPressedSecond = millis();
                        }
                    }
                } else {
                    memBtn = false;
                }
            } else if (rxId == 0x165 /* status */) {
                if ((rxBuf[0] & 0xC0) == 0xC0) {
                    if (!musicOn) {
                        silence();
                        digitalWrite(5, HIGH);
                        setVolume();
                        musicOn = true;
                    }
                } else {
                    if (musicOn) {
                        digitalWrite(5, LOW);
                        musicOn = false;
                    }
                }
            }
        }
    }

    if (memBtnPressedSecond > 0 && !memBtnReacted && (millis() - memBtnPressedSecond > 300)
        && (memBtnPressedSecond - memBtnPressedFirst < 2000)) {
        memBtnReacted = true;
        memBtnPressedSecond = 0;
        Serial.println("Mem 2 reacted");

        for (int i = 0; i < 20; i++) {
            // delay(100);
            cr1Buf[3] = cr1Buf[3] | 0x20;
            CAN0.sendMsgBuf(0x094, 8, 8, cr1Buf);
            delay(200);
            // cr1Buf[3] = cr1Buf[3] & 0xDF;
            // CAN0.sendMsgBuf(0x094, 8, 8, cr1Buf);
        }
    }

    if (carBtn && !carBtnReacted && (millis() - carBtnPressed > 300)) {
        carBtnReacted = true;
        vfBuf[5] = vfBuf[5] | 0x04;
        CAN0.sendMsgBuf(0x1A9, 8, 8, vfBuf);
        Serial.println("Car button long press detected");
    }

    if (navBtn && !navBtnReacted && (millis() - navBtnPressed > 300)) {
        navBtnReacted = true;
        vfBuf[7] = vfBuf[7] | 0x80;
        CAN0.sendMsgBuf(0x1A9, 8, 8, vfBuf);
        Serial.println("nav button long press detected");
    }
}

void silence()
{
    delay(50);
    for (int i = 0; i < 35; i++) {
        wbBuf[0] = wbBuf[0] | 0x04;
        CAN0.sendMsgBuf(0x21F, 3, 3, wbBuf);
        wbBuf[0] = wbBuf[0] & 0xFB;
        CAN0.sendMsgBuf(0x21F, 3, 3, wbBuf);
        delay(10);
    }
}

void setVolume()
{
    delay(3000);
    for (int i = 0; i < 3; i++) {
        wbBuf[0] = wbBuf[0] | 0x08;
        CAN0.sendMsgBuf(0x21F, 3, 3, wbBuf);
        wbBuf[0] = wbBuf[0] & 0xF7;
        CAN0.sendMsgBuf(0x21F, 3, 3, wbBuf);
        delay(300);
    }
}