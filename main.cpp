#include "hFramework.h"
#include "stddef.h"
#include "stdio.h"
#include "math.h"
#include "Lego_Ultrasonic.h"
#include "Lego_Touch.h"

#define DEBUG
// #define telegraf 

#define IDEAL_DISTANCE 10
#define INACURRACY_DISTANCE 1
#define EXTRA_SPEED 50
#define MIN_SPEED 100
#define NO_MOTION 0
#define POS_LIMIT 200

using namespace hSensors;

hLegoSensor_i2c ls1(hSens1);
Lego_Ultrasonic dist_sensor(ls1);

hLegoSensor_simple ls2(hSens5);
Lego_Touch legoBtn(ls2);

int current_dist;       // distance sensor data
int engine_power;       // power data for engine
int current_pos;        // engine encoder data
int default_pos;    // encoder starting value

bool engine_run = false;
bool only_backwards = false;
bool only_forwards = false;

#ifdef telegraf
bool current_lego_btn = false;
bool previous_lego_btn = false;
bool current_hBtn1 = false;
bool current_hBtn2 = false;
bool engine_run = false;
uint8_t press_num = 0u;
#endif

void hMain()
{
    sys.setLogDev(&Serial);
    hMot1.setEncoderPolarity(Polarity::Reversed);
    default_pos = (int)hMot1.getEncoderCnt();    // get initial encoder value at start

    while (1)
    {
        current_dist = dist_sensor.readDist();
        current_pos = (int)hMot1.getEncoderCnt() - default_pos;

        if (current_pos > POS_LIMIT && !only_backwards)  // block movement forwards
        {
            // hMot1.setPower(NO_MOTION);
            only_backwards = true;
        }
        else if (current_pos < (-POS_LIMIT) && !only_forwards)  // block movement backwards
        {
            only_forwards = true;
        }
        else if (current_dist < 20)  // run if something is in the range of 20, do nothing otherwise
        {
            if ((current_dist > (IDEAL_DISTANCE + INACURRACY_DISTANCE)))         // eliminate unnessecary osillation
            {                                                                         // by tolerating +-1 inaccuracy
                if (!only_backwards)                                                                                          
                    engine_power = MIN_SPEED + (int)(EXTRA_SPEED * (current_dist / 20.0));     // calculate engine power (min=100, max=150)       
            }
            else if ((current_dist < (IDEAL_DISTANCE - INACURRACY_DISTANCE)))
            {
                if (!only_forwards)
                   engine_power = (-1) * (MIN_SPEED + (int)(EXTRA_SPEED * (current_dist / 20.0)));
            }
            else    // position achieved or stuck at movement limit zone 
            {
                engine_power = NO_MOTION;
            }
        }
        else    // motor encoder within limits but the nearest object more than 20 units away
        {
            engine_power = NO_MOTION;
        }

        hMot1.setPower(engine_power);

        if (current_pos <= POS_LIMIT && current_pos >= (-POS_LIMIT))
        {
            only_forwards = only_backwards = false;
        }

        #ifdef DEBUG
        printf("\n\rdist: %d  ", current_dist); 
        printf("pos: %d  ", (int)hMot1.getEncoderCnt());
        printf("power: %d  ", engine_power);
        only_forwards ? printf("only_forwards") : NULL;
        only_backwards ? printf("only_backwards") : NULL;
        #endif

        #ifdef telegraf
        current_lego_btn = legoBtn.isPressed();

        if (current_lego_btn && !previous_lego_btn)   //if rising slope is detected on button input
        {   
            if (press_num%2 == 0)
                hMot2.setPower(100);
            else
                hMot2.setPower(-100);                     //toggle engine state and increment number of presses
            // hMot2.rotRel(-90, 100);      
            printf("Lego button pressed %u times\r\n", ++press_num);
        }
        else if (!current_lego_btn && previous_lego_btn)
        {     
            hMot2.setPower(0);  //toggle engine state and increment number of presses
            // hMot2.rotRel(90, 200);  
            printf("Lego button released %u times\r\n", press_num);    
        }   
        previous_lego_btn = current_lego_btn;
        printf("dist: %d\r\n", current_dist);   // used for debugging
        printf("pos:  %d\r\n", current_pos);
        #endif

        sys.delay(50);
    }
}