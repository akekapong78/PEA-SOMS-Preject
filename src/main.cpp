#include <stdio.h>  // แก้ error printf ใน lib 
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

int dr = DR_SF12;     // set SF factor (7-12)

// Node1-Test  (ABP)   TTN               
static const PROGMEM u1_t NWKSKEY[16] = {  0x59, 0x25, 0x1E, 0xD0, 0x3C, 0x90, 0xC0, 0x05, 0xD8, 0xF2, 0xE8, 0x59, 0xBF, 0x67, 0xE7, 0x48};
static const u1_t PROGMEM APPSKEY[16] = { 0x22, 0x79, 0x1E, 0x54, 0x83, 0x2B, 0x04, 0x0D, 0xCE, 0x3A, 0x49, 0xC3, 0xCD, 0x49, 0xE3, 0xEC };
static const u4_t DEVADDR = 0x260B401F;


// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }      // ปล่อยว่างไว้ ไม่ต้องลบ
void os_getDevEui (u1_t* buf) { }      // ปล่อยว่างไว้ ไม่ต้องลบ
void os_getDevKey (u1_t* buf) { }      // ปล่อยว่างไว้ ไม่ต้องลบ


// Declare the job control structures
static osjob_t sendjob;

// Status Voltage
uint8_t status_A = 0x00;   // uint8 มีค่า 0-255 หรือ 0x00 - 0xFF (1111 1111) เท่ากับ 1 byte
uint8_t status_B = 0x00;   
uint8_t status_C = 0x00;   
// payload base64 -> AAAA  ,  base16  0x 00 00 00

// กำหนดขนาดของ payload data
static uint8_t mydata[3];


// Pin Voltage Pin
uint8_t pinA = A0;  // pin A0, ขา 18
uint8_t pinB = A1;  // pin A1, ขา 19
uint8_t pinC = A2;  // pin A2, ขา 20

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;


// For Arduio Pro mini   [nss=10, SCK=13, MOSI=11, MISO=12] 
const lmic_pinmap lmic_pins = {
    .nss = 10,  // pin 10 ตามชื่อบอร์ด                  
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = { 4, 5, 7 },     // pin ตามชื่อบนบอร์ด
};

/* *******************************************************************************
 * build data to transmit in dataTX ปรับค่าในตัวแปล ให้เป็นจำนวนเต็ม พร้อมเพื่อส่ง package
 * *******************************************************************************/
void build_data() {   
   
    // print data voltage
    if (status_A == 0xFF)
        Serial.println("Voltage A : On");
    else
        Serial.println("Voltage A : Off");
    if (status_B == 0xFF)
        Serial.println("Voltage B : On");
    else
        Serial.println("Voltage B : Off");
    if (status_C == 0xFF)
        Serial.println("Voltage C : On");
    else
        Serial.println("Voltage C : Off");
    

    //static uint8_t mydata[3];  // ทำเป็นตัวแปลโกลเบิล

    int index = 0;    // i++ จะเพิ่มค่าตนเองขึ้น 1 หลังจากจบประโยค ;
    mydata[index++] = status_A & 0xFF ;      
    mydata[index++] = status_B & 0xFF ;    
    mydata[index++] = status_C & 0xFF ;      
    
    Serial.print("My payload (size=");               // แสดง package ที่สร้างขึ้น
    Serial.print(sizeof(mydata)); 
    Serial.print(") : ");
    for(uint8_t i=0 ; i < sizeof(mydata) ; i++) {
      Serial.print(mydata[i],HEX);  
      Serial.print(" ");
    }
    Serial.println(" END.");
}  // end build data


// สั่งให้ส่ง Packet เมื่อพร้อม
void do_send(osjob_t* j){

    // เรียนใช้งาน เพื่อแสดงค่าตัวแปลล่าสุด
    build_data();


    // vvvvvv โค้ดตัวอย่าง
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {

        // Prepare upstream data transmission at the next possible time.
        // lib = int   LMIC_setTxData2   (u1_t port, xref2u1_t data, u1_t dlen, u1_t confirmed);
        //LMIC_setTxData2(1, mydata2, sizeof(mydata2)-1, 0);
        //LMIC_setTxData2(1, mydata, sizeof(mydata)-1, 0);   ********** size()-1 คือผิด  
        LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}


// สถานะการทำงาน แต่ละขั้นตอน (ปล่อยไว้ ไม่ต้องลบ)
void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}   // end fn onEvent


void setup() {

    // กำหนดโหมดของ IO
    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
    pinMode(pinC, INPUT);

    Serial.begin(9600);
    Serial.println(F("Starting"));

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    // Defined(CFG_as923)
    // แก้ package มา1หาย2  ด้วยการ fix f=923.2 ที่ฝั้ง GW และต้อง config node ให้ครบ 3 ch แบบนี้ ถึงจะปล่อยความพี่เดียว ไม่วิ่ง
    // in lib =  LMIC_setupBand (u1_t bandidx, s1_t txpow, u2_t txcap);
    LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band    
    LMIC_setupChannel(1, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(dr,14);

    // Start job
    do_send(&sendjob);

}  // end fn setup




void loop() {

    //Serial.println(digitalRead(pinA));
    //Serial.println(digitalRead(pinB));
    //Serial.println(digitalRead(pinC));
    // update data 3 ตัวแปร
    if (digitalRead(pinA) == HIGH)
        status_A = 0xFF;
    else    
        status_A = 0x00;
    if (digitalRead(pinB) == HIGH)
        status_B = 0xFF;
    else    
        status_B = 0x00;
    if (digitalRead(pinC) == HIGH)
        status_C = 0xFF;
    else    
        status_C = 0x00;
    
    os_runloop_once();     // การทำงานของ LoRa ต่าง ๆ โดยพิจารณา time interval แล้ว
}
