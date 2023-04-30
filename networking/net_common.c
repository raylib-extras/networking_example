/**********************************************************************************************
*
*   raylib_networking_smaple * a sample network game using raylib and enet
*
*   LICENSE: ZLIB
*
*   Copyright (c) 2021 Jeffery Myers
*
*   Permission is hereby granted, free of charge, to any person obtaining a copy
*   of this software and associated documentation files (the "Software"), to deal
*   in the Software without restriction, including without limitation the rights
*   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*   copies of the Software, and to permit persons to whom the Software is
*   furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included in all
*   copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
*
**********************************************************************************************/

#include "net_common.h"


// Utility functions to read data out of a packet
// Optimally this would go into a library that was shared by the client and the server

/// <summary>
/// Read one byte out of a packet, from an offset, and update that offset to the next location to read from
/// </summary>
/// <param name="packet">The packet to read from</param>
/// <param name="offset">A pointer to an offset that is updated, this should be passed to other read functions so they read from the correct place</param>
/// <returns>The byte read</returns>
uint8_t ReadByte(ENetPacket* packet, size_t* offset)
{
	// make sure we have not gone past the end of the data we were sent
	if (*offset > packet->dataLength)
		return 0;

	// cast the data to a byte so we can increment it in 1 byte chunks
	uint8_t* ptr = (uint8_t*)packet->data;

	// get the byte at the current offset
	uint8_t data = ptr[(*offset)];

	// move the offset over 1 byte for the next read
	*offset = *offset + 1;

	return data;
}

/// <summary>
/// Read a signed short from the network packet
/// Note that this assumes the packet is in the host's byte ordering
/// In reality read/write code should use ntohs and htons to convert from network byte order to host byte order, so both big endian and little endian machines can play together
/// </summary>
/// <param name="packet">The packet to read from<</param>
/// <param name="offset">A pointer to an offset that is updated, this should be passed to other read functions so they read from the correct place</param>
/// <returns>The signed short that is read</returns>
int16_t ReadShort(ENetPacket* packet, size_t* offset)
{
	// make sure we have not gone past the end of the data we were sent
	if (*offset > packet->dataLength)
		return 0;

	// cast the data to a byte at the offset
	uint8_t* data = (uint8_t*)packet->data;
	data += (*offset);

	// move the offset over 2 bytes for the next read
	*offset = (*offset) + 2;

	// cast the data pointer to a short and return a copy
	return *(int16_t*)data;
}
