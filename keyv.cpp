#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <unistd.h>

#define MAX_PAGE_SIZE 4096

/*
bitcask disk entry format
[checksum, timestamp, keysize, valuesize, key, value]
*/

struct keydir_entry
{
    std::streampos value_position;
    uint32_t value_size;

    keydir_entry(std::streampos value_pos, uint32_t value_size) :
    value_position(value_pos),
    value_size(value_size)
    {}
};

std::vector<uint8_t> encode_to_bytes(const std::string& key, const std::string& value)
{
    std::vector<uint8_t> buffer;

    uint32_t key_length = key.size();
    uint32_t value_length = value.size();

    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&key_length), reinterpret_cast<uint8_t*>(&key_length) + sizeof(key_length));
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&value_length), reinterpret_cast<uint8_t*>(&value_length) + sizeof(value_length));
    buffer.insert(buffer.end(), key.begin(), key.end());
    buffer.insert(buffer.end(), value.begin(), value.end());

    return buffer;
}

std::pair<std::string, std::string> decode_bytes(std::vector<uint8_t> buffer)
{
    size_t pos = 0;

    //decode key_length
    uint32_t key_length;
    std::memcpy(&key_length, &buffer[pos], sizeof(uint32_t));
    pos += sizeof(uint32_t);

    //decode value_length
    uint32_t value_length;
    std::memcpy(&value_length, &buffer[pos], sizeof(uint32_t));
    pos += sizeof(uint32_t);

    //decode key and value
    std::string key(buffer.begin() + pos, buffer.begin() + pos + key_length);
    pos += key_length;
    std::string value(buffer.begin() + pos, buffer.begin() + pos + value_length);

    return {key, value};
}

keydir_entry write_to_disk(std::vector<uint8_t> buffer)
{
    std::string filename = "entries.db";
    std::ofstream outFile(filename, std::ios::binary | std::ios::app);

    if(!outFile)
    {
        std::cerr << "error opening the file" <<std::endl;
    }
    size_t pos = 0;
    uint32_t key_length;
    uint32_t value_length;
    std::streampos value_position;

    std::memcpy(&key_length, &buffer[pos], sizeof(uint32_t));
    pos += sizeof(uint32_t);
    std::memcpy(&value_length, &buffer[pos], sizeof(uint32_t));

    //write key length and value length to disk
    outFile.write(reinterpret_cast<const char*>(buffer.data()), (buffer.size() - value_length));
    value_position = outFile.tellp(); //save the position in file of the value of this entry
    //write the rest of the data
    outFile.write(reinterpret_cast<const char*>(buffer.data() + ((sizeof(uint32_t) * 2) + key_length)), buffer.size() - ((sizeof(uint32_t) * 2) + key_length)); 
    outFile.close();
    outFile.flush();
    return keydir_entry(value_position, value_length);
}

std::vector<char> read_from_disk(std::streampos value_position, uint32_t value_size)
{
    std::ifstream inFile("entries.db", std::ios::binary);
    if(!inFile)
    {
        std::cerr << "Error opening file" << std::endl;
    }
    inFile.seekg(value_position);
    std::vector<char> buffer(value_size);
    inFile.read(buffer.data(), buffer.size());

    return buffer;
}


int main()
{
    std::vector<uint8_t> buffer = encode_to_bytes("username", "cassie h");
    keydir_entry entry = write_to_disk(buffer);

    std::vector<char> read_buffer = read_from_disk(entry.value_position, entry.value_size);
    for(char c : read_buffer)
    {
        std::cout << c;
    }
}