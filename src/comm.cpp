#include "flags.h"
#include "comm.h"

/* TEScommand METHODS ======================================================= */

TEScommand::TEScommand()
{
	clear();
}

void TEScommand::addByte(uint8_t b)
{
	_data[_bytes_received] = b;
	_bytes_received++;

	if (_bytes_received >= COMMAND_LEN)
	{
		_full = true;
		_bytes_received = 0;
	}
	else
	{
		_full = false;
	}
}

void TEScommand::loadBytes(uint8_t *bytes)
{
	for (int i = 0; i < COMMAND_LEN; i++)
	{
		_data[i] = bytes[i];
	}
}

bool TEScommand::isFull()
{
	return _full;
}

uint8_t TEScommand::getCommand()
{
	return _command;
}

bool TEScommand::checkCRC()
{
	CRC16 crcGen;
	crcGen.add(_data, COMMAND_LEN-2);

	if (crcGen.getCRC() == _crc)
		return true;
	else
		return false;
}

void TEScommand::clear()
{
	for (int i = 0; i < COMMAND_LEN; i++)
		_data[i] = 0;

	_bytes_received = 0;
}

/* HARDWARE INIT FUNCTIONS ================================================== */

void initUSB(void)
{
	/**
     * Initialize USB connection to computer. Used to print debug messages.
     * Baud rate: 115200
     * Data bits: 8
     * Parity: none
     */
    SERCOM_USB.begin(115200);
    while (!SERCOM_USB);  // wait for initialization to complete
    SERCOM_USB.print("[system init]\tUSB interface initialized\r\n");
}

void initUART(void)
{
	/**
     * Initialize UART connection to satellite
     * Baud rate: 115200
     * Data bits: 8
     * Parity: odd (1 bit)
     */
    SERCOM_UART.begin(115200, SERIAL_8O1);
    while (!SERCOM_UART);  // wait for initialization to complete
	SERCOM_UART.setTimeout(10);
#if DEBUG
    SERCOM_USB.print("[system init]\tUART interface initialized\r\n");
#endif
}

void initI2C(void)
{
	/**
     * Initialize I2C network
     * Clock: 400 kHz
     */
    SERCOM_I2C.begin();
    SERCOM_I2C.setClock(400000);
#if DEBUG
	SERCOM_USB.print("[system init]\tI2C interface initialized\r\n");
#endif
}

/* HELPER FUNCTIONS ========================================================= */

/**
 * @brief
 * Converts a floating-point number to a fixed-point number with 5 bits for the
 * integer part and 3 bits for the fraction part. Resulting data cannot be
 * properly interpreted until converted back into a float.
 * 
 * @param[in] f  Float to convert
 * 
 * @return Fixed-point conversion
 */
fixed5_3_t floatToFixed(float f)
{
    fixed5_3_t fix;
    fix = (uint8_t)(f * (1 << 3));
    return fix;
}

/**
 * @brief
 * Converts a fixed-point number with 5 integer bits and 3 fraction bits to a
 * floating-point number. Resulting data can be properly interpreted with no
 * extra conversions.
 * 
 * @param[in] fix  Fixed-point number to convert
 * 
 * @return Float conversion
 */
float fixedToFloat(fixed5_3_t fix)
{
    float f;
    f = ((float)fix) / (1 << 3);
    return f;
}

void send_command(uint8_t cmd){
	#if DEBUG
		SERCOM_USB.print("TES -> ADCS [0x");
		SERCOM_USB.print(cmd, HEX);
		SERCOM_USB.println("]");
	#endif

	SERCOM_UART.write(cmd);
}