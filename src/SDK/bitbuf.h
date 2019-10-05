#pragma once

class bf_write
{
public:
    char pad_0000[12]; //0x0000
    int32_t m_nDataBytes; //0x000C
    int32_t m_nDataBits; //0x0010
    int32_t m_iCurBit; //0x0014
    bool m_bOverflow; //0x0018
    bool m_bAssertOnOverflow; //0x0019
    char pad_001A[2]; //0x001A
    char* m_pDebugName; //0x001C
}; //Size: 0x0024

class bf_read // because fuck you. i'm lazy
{
public:
	
	uintptr_t base_address;
	uintptr_t cur_offset;

	bf_read(uintptr_t addr)
	{
		base_address = addr;
		cur_offset = 0;
	}

	void setOffset(uintptr_t offset)
	{
		cur_offset = offset;
	}

	void skip(uintptr_t length)
	{
		cur_offset += length;
	}

	int readByte()
	{
		auto val = *reinterpret_cast<char*>(base_address + cur_offset);
		++cur_offset;
		return val;
	}

	bool readBool()
	{
		auto val = *reinterpret_cast<bool*>(base_address + cur_offset);
		++cur_offset;
		return val;
	}

	std::string readString()
	{
		char buffer[256];

		auto str_length = *reinterpret_cast<char*>(base_address + cur_offset);
		++cur_offset;

		std::memcpy(buffer, reinterpret_cast<void*>(base_address + cur_offset), str_length > 255 ? 255 : str_length);

		buffer[str_length > 255 ? 255 : str_length] = '\0';
		cur_offset += str_length + 1;

		return std::string(buffer);
	}
};