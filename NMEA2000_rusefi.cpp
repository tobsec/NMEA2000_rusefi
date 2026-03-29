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

//*****************************************************************************
tNMEA2000_rusefi::tNMEA2000_rusefi() : tNMEA2000()
{
    efiPrintf("* tNMEA2000_rusefi initialized");
}

//*****************************************************************************
bool tNMEA2000_rusefi::CANOpen()
{
    efiPrintf("* tNMEA2000_rusefi::CANOpen()");
    return true;
}

//*****************************************************************************
bool tNMEA2000_rusefi::CANSendFrame(unsigned long id, unsigned char len, const unsigned char *buf, bool wait_sent)
{
    CanTxMessage msg(CanCategory::NBC, id, len, true);
    msg.busIndex = 1;

    for (uint8_t i = 0; i < len && i < 8; i++)
    {
        msg[i] = buf[i];
    }

    return true;
}

//*****************************************************************************
bool tNMEA2000_rusefi::CANGetFrame(unsigned long &id, unsigned char &len, unsigned char *buf)
{
    // TODO: receive not yet implemented
    return false;
}

/********************************************************************
*	Bridge functions
**********************************************************************/
uint32_t millis(void)
{
    return TIME_I2MS(chVTGetSystemTime());
}



/*
 * NMEA2000 bump allocator.
 *
 * The NMEA2000 library allocates via operator new during initialization.
 * We provide a simple bump allocator from a static buffer because:
 * - ChibiOS heap (chHeapAlloc) is not available before chSysInit
 * - The NMEA2000 library never frees memory after init
 *
 * The global operator new/delete override is guarded by nmea2000HeapActive
 * so that only NMEA2000 init allocations use this heap. Any unexpected
 * new outside of init will trigger an assert.
 */

#define NMEA2000_USER_HEAP 8192

static char nmea2000UserHeap[NMEA2000_USER_HEAP];

bool nmea2000HeapActive = false;

class NMEA2kHeap {
public:
	void* alloc(size_t n) {
		if ((m_pos + n) <= m_size)
		{
			unsigned int old_pos = m_pos;
			m_pos += n;
			return &m_buffer[old_pos];
		}
		else
		{
			firmwareError(OBD_PCM_Processor_Fault, "NMEA2000 heap overflow: need %d, have %d", n, m_size - m_pos);
			return nullptr;
		}
	}

	template<size_t TSize>
	NMEA2kHeap(char (&buffer)[TSize])
		: m_buffer(buffer)
		, m_size(TSize)
		, m_pos(0)
	{
	}

	size_t used() const { return m_pos; }
	size_t size() const { return m_size; }

private:
	char* m_buffer;
	size_t m_size;
	unsigned int m_pos;
};

static NMEA2kHeap userHeap(nmea2000UserHeap);

void* operator new(size_t size)
{
	osalDbgAssert(nmea2000HeapActive, "unexpected operator new outside NMEA2000 init");
	return userHeap.alloc(size);
}

void* operator new[](size_t size)
{
	osalDbgAssert(nmea2000HeapActive, "unexpected operator new[] outside NMEA2000 init");
	return userHeap.alloc(size);
}

/* Bump allocator — nothing to free. No-op is safe since the library never frees after init. */
void operator delete(void*) { }
void operator delete[](void*) { }
void operator delete(void*, size_t) { }
void operator delete[](void*, size_t) { }
