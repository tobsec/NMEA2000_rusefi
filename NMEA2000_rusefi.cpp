/*
NMEA2000_rusefi.cpp

2017 Copyright (c) Al Thomason   All rights reserved

Support the RUSEFI targeted CPUs
See: https://github.com/tobsec/NMEA2000_rusefi
     https://github.com/ttlappalainen/NMEA2000


Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

See also NMEA2000 library.
*/

#include "pch.h"

#include "NMEA2000_rusefi.h"

#include "can_msg_tx.h"

/**
 * Executes the BKPT instruction that causes the debugger to stop.
 * If no debugger is attached, this will be ignored
 */
#define bkpt() __asm volatile("BKPT #0\n")

static volatile bool retValDebug = false;

static volatile uint16_t accCounttNMEA2000_esp32 = 0u;

static volatile uint32_t cur_time = 0u;

static volatile uint32_t timeBuf[16];

//*****************************************************************************
tNMEA2000_esp32::tNMEA2000_esp32() : tNMEA2000()
{
    efiPrintf("* tNMEA2000_esp32::tNMEA2000_esp32() called!");
    accCounttNMEA2000_esp32++;
}

static volatile uint16_t accCountCANOpen = 0u;

//*****************************************************************************
bool tNMEA2000_esp32::CANOpen()
{
    // TODO: Check can baud rate and give warning if not set correctly at 250kbps!
    // Maybe check channel assignment and if CAN is actually enabled?
    // Maybe don't to this checks here?
accCountCANOpen++;
    efiPrintf("* tNMEA2000_esp32::CANOpen() called!");

    return true;
}



static volatile uint16_t accCountCANSendFrame = 0u;

//*****************************************************************************
bool tNMEA2000_esp32::CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent)
{
    // CANMessage output;
    // bool result;

accCountCANSendFrame++;


static uint8_t it = 0;
    // output.format=CANExtended;
    // output.type=CANData;
    // output.id = id;
    // output.len = len;
    // for (unsigned char i=0; i<len && i<8; i++) output.data[i]=buf[i];


    // result=can1.write(output);


    // if (wait_sent){
	//      //---- You can put something here to assure these special class of messages are indeed sent in FIFO order.
    //    //   See above in ::CANOpen()

	//      }

    // return result;

    {
        CanTxMessage msg(CanCategory::NBC, id, len, true);
        msg.busIndex = 1;

        for (uint8_t i=0; i<len && i<8; i++)
        {
            msg[i] = buf[i];
        }
    }

    if (id == 0x9f20016)
    {
        if (it<16)
        {
            timeBuf[it++] = cur_time;
        }
    }
    // efiPrintf("* tNMEA2000_esp32::CANSendFrame(id=%lu, unsigned char len=%hhu, buf, wait_sent=%hhu) called!");

    return true;
}

static volatile uint16_t accCountCANGetFrame = 0u;

//*****************************************************************************
bool tNMEA2000_esp32::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)
{
    // bool HasFrame=false;
    // CANMessage incoming;

    // if (can1.read(incoming)) {           // check if data coming
    //     id=incoming.id;
    //     len=incoming.len;
    //     for (int i=0; i<len && i<8; i++) buf[i]=incoming.data[i];
    //     HasFrame=true;
    // }
accCountCANGetFrame++;
    // return HasFrame;

    return retValDebug;
}




/********************************************************************
*	Other 'Bridge' functions and classes
*
*
*
**********************************************************************/
// void delay(const uint32_t ms)
// {
//     // TODO!
//     // HAL_Delay(ms);
// };


uint32_t millis(void)
{
    // TODO!
    // return HAL_GetTick();
    cur_time = TIME_I2MS(chVTGetSystemTime());

    // return TIME_I2MS(chVTGetSystemTime());
    return cur_time;
};



// chThdCreateStatic(gpsThreadStack, sizeof(gpsThreadStack), LOWPRIO, (tfunc_t)(void*) GpsThreadEntryPoint, NULL);
// static THD_WORKING_AREA(gpsThreadStack, UTILITY_THREAD_STACK_SIZE);

//------ called chHeapAlloc before initialized -----
// see https://forum.chibios.org/viewtopic.php?f=2&t=2550&sid=9f885cfba06f67eee40e3b57ced90eee&start=10
// -> probably like  "I use alloc before halinit and chSysInit"

#define NMEA2000_USER_HEAP 8192

static char nmea2000UserHeap[NMEA2000_USER_HEAP];

    static volatile unsigned int myHeap_pos; // put here for debugging

class NMEA2kHeap {
public:
	// memory_heap_t m_heap;
    char * myHeap_p;
    // unsigned int myHeap_pos;

	size_t m_memoryUsed = 0;
	size_t m_size;

	void* alloc(size_t n) {
		// return chHeapAlloc(&m_heap, n);
        // return malloc(size); // probably a bad idea. will return NULL in case of error. handle this?
        if ((myHeap_pos+n) < NMEA2000_USER_HEAP)
        {
            myHeap_pos += n;
            return &myHeap_p[myHeap_pos];
        }
        else
        {
            // TODO: error handling
            bkpt();
	        NVIC_SystemReset();
            return NULL;
        }
	}

	void free(void* obj) {
        /* dont support free operations up to now */
                    bkpt();
	        NVIC_SystemReset();
		// chHeapFree(obj);
	}

public:
	template<size_t TSize>
	NMEA2kHeap(char (&buffer)[TSize])
		: m_size(TSize)
	{
		// chHeapObjectInit(&m_heap, buffer, TSize);
        myHeap_p = buffer;
        myHeap_pos = 0u;
	}

	size_t size() const {
		return m_size;
	}

	size_t used() const {
		return m_memoryUsed;
	}
};

static NMEA2kHeap userHeap(nmea2000UserHeap);



void* operator new (size_t size)
{
    return userHeap.alloc(size);
    //return chCoreAlloc(size); -> will give memory from __ram0_free__ but is 0xFFFFFFFF - size ??? :-/
}
void* operator new[] (size_t size)
{
    return userHeap.alloc(size);
    //return chCoreAlloc(size); -> will give memory from __ram0_free__ but is 0xFFFFFFFF - size ??? :-/
}

void operator delete( void *p ) { userHeap.free(p); }
void operator delete[]( void *p ) { userHeap.free(p); }
void operator delete( void *p , size_t size) { userHeap.free(p); }
void operator delete[]( void *p , size_t size) { userHeap.free(p); }

// void* operator new( size_t size ) { return chHeapAlloc( 0x0, size ); }
// void* operator new[]( size_t size ) { return chHeapAlloc( 0x0, size ); }
// void operator delete( void *p ) { chHeapFree( p ); }
// void operator delete[]( void *p ) { chHeapFree( p ); }
