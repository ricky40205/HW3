#include "mbed.h"

#include "fsl_port.h"

#include "fsl_gpio.h"

#define UINT14_MAX        16383

// FXOS8700CQ I2C address

#define FXOS8700CQ_SLAVE_ADDR0 (0x1E<<1) // with pins SA0=0, SA1=0

#define FXOS8700CQ_SLAVE_ADDR1 (0x1D<<1) // with pins SA0=1, SA1=0

#define FXOS8700CQ_SLAVE_ADDR2 (0x1C<<1) // with pins SA0=0, SA1=1

#define FXOS8700CQ_SLAVE_ADDR3 (0x1F<<1) // with pins SA0=1, SA1=1

// FXOS8700CQ internal register addresses

#define FXOS8700Q_STATUS 0x00

#define FXOS8700Q_OUT_X_MSB 0x01

#define FXOS8700Q_OUT_Y_MSB 0x03

#define FXOS8700Q_OUT_Z_MSB 0x05

#define FXOS8700Q_M_OUT_X_MSB 0x33

#define FXOS8700Q_M_OUT_Y_MSB 0x35

#define FXOS8700Q_M_OUT_Z_MSB 0x37

#define FXOS8700Q_WHOAMI 0x0D

#define FXOS8700Q_XYZ_DATA_CFG 0x0E

#define FXOS8700Q_CTRL_REG1 0x2A

#define FXOS8700Q_M_CTRL_REG1 0x5B

#define FXOS8700Q_M_CTRL_REG2 0x5C

#define FXOS8700Q_WHOAMI_VAL 0xC7


I2C i2c( PTD9,PTD8);

Serial pc(USBTX, USBRX);

int m_addr = FXOS8700CQ_SLAVE_ADDR1;


void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len);

void FXOS8700CQ_writeRegs(uint8_t * data, int len);


Ticker ticker;
Thread thread;
DigitalOut blueLED(LED3);
InterruptIn button(SW2);
EventQueue queue(32 * EVENTS_EVENT_SIZE);

void blinking(){
    blueLED =!blueLED;
};



void logger(){

    ticker.attach(&blinking, 0.1); //ticker blink blueLED

    Timer t10s;

    pc.baud(115200);


    uint8_t who_am_i, data[2], res[6];

    int16_t acc16;

    float t[3];
    float s[3];
    int n = 0;
    // Enable the FXOS8700Q


    FXOS8700CQ_readRegs( FXOS8700Q_CTRL_REG1, &data[1], 1);

    data[1] |= 0x01;

    data[0] = FXOS8700Q_CTRL_REG1;

    FXOS8700CQ_writeRegs(data, 2);


    // Get the slave address

    FXOS8700CQ_readRegs(FXOS8700Q_WHOAMI, &who_am_i, 1);

    //pc.printf("Here is %x\r\n", who_am_i);

    //

    t10s.start();
    while (t10s.read()<=10){

        FXOS8700CQ_readRegs(FXOS8700Q_OUT_X_MSB, res, 6);

        //record origin vector
        if(n==0){
            acc16 = (res[0] << 6) | (res[1] >> 2);
            if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
            t[0] = ((float)acc16) / 4096.0f;
            s[0] = ((float)acc16) / 4096.0f;

            acc16 = (res[2] << 6) | (res[3] >> 2);
            if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
            t[1] = ((float)acc16) / 4096.0f;
            s[1] = ((float)acc16) / 4096.0f;

            acc16 = (res[4] << 6) | (res[5] >> 2);
            if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
            t[2] = ((float)acc16) / 4096.0f;
            s[2] = ((float)acc16) / 4096.0f;

            n++;
        }
        //record shifting vector
        else{
            acc16 = (res[0] << 6) | (res[1] >> 2);
            if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
            
            s[0] = ((float)acc16) / 4096.0f;

            acc16 = (res[2] << 6) | (res[3] >> 2);
            if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;
            
            s[1] = ((float)acc16) / 4096.0f;

            acc16 = (res[4] << 6) | (res[5] >> 2);
            if (acc16 > UINT14_MAX/2)
            acc16 -= UINT14_MAX;

            s[2] = ((float)acc16) / 4096.0f;
            n++;
        }

        float cos_x = (t[0]*s[0]+t[1]*s[1]+t[2]*s[2])/(sqrt(t[0]*t[0]+t[1]*t[1]+t[2]*t[2])*sqrt(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]));
        float cos45 = 1/(sqrt(2));

        if(cos_x <= cos45){

            pc.printf("%f\r\n%f\r\n%f\r\n%d\r\n", s[0], s[1], s[2], 1);
        }
        else{

            pc.printf("%f\r\n%f\r\n%f\r\n%d\r\n", s[0], s[1], s[2], 0);
        }
        wait(0.1);

    }

    ticker.detach();
    t10s.stop();

}

int main(){
    blueLED =1;

    thread.start(callback(&queue, &EventQueue::dispatch_forever));
    button.rise(queue.event(logger));
}

void FXOS8700CQ_readRegs(int addr, uint8_t * data, int len) {
   char t = addr;
   i2c.write(m_addr, &t, 1, true);
   i2c.read(m_addr, (char *)data, len);
}

void FXOS8700CQ_writeRegs(uint8_t * data, int len) {
   i2c.write(m_addr, (char *)data, len);
}