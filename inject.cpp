//g++ inject.cpp -liconv -o inject.exe
#define NOMINMAX
#define JSON_DIAGNOSTICS 1
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <locale>
#include <iconv.h>
#include <filesystem>
#include <algorithm>
#include <nlohmann/json.hpp>

#if 0
TODO:
Some utf8 characters cannot be converted into shift-jis,
so do some replacement after translation.
This is a list of some of the characters found to cause error.

ï -> i
é -> e
ç -> c
#endif

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <decoded spt file> <json file>" << std::endl;
        return 1;
    }

    const char* filePath = argv[1];
    std::filesystem::path outFilePath{argv[1]};
    if(!std::filesystem::exists(outFilePath.parent_path() / "en"))
    {
        std::filesystem::create_directory(outFilePath.parent_path() / "en");
    }
    outFilePath = outFilePath.parent_path() / "en" / outFilePath.filename(); 

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open the file." << std::endl;
        return 1;
    }

    // Get the file size
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read the file contents into a vector
    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

    // Close the file
    file.close();

    auto icv = iconv_open("SHIFT_JIS", "UTF-8");

    if (icv == (iconv_t)-1) 
    {
        std::cerr << "Failed to open iconv." << std::endl;
        return 1;
    }

    std::string json_file{};
    if (argc == 2) 
    {
        std::filesystem::path outpath{argv[1]};
        outpath.replace_filename(outpath.stem().string() + "_en.json");
        json_file = outpath.string();
    }
    else
    {
        json_file = argv[2];
    }

    std::ifstream f(json_file);
    json j{};
    try
    {
        j = json::parse(f);
    }
    catch (const json::exception& e)
    {
        std::cerr << "File: " << argv[1] << '\n';
        std::cerr << e.what() << '\n';
    }

    std::vector<std::vector<uint8_t>> strings{};
    // Collect Strings
    std::vector<uint8_t> out{};
    for(auto& json_text: j)
    {
        std::string text{json_text["text"]};
        char* inptr{text.data()};
        size_t inbytesleft{text.size() + 1};
        size_t outbytesleft{inbytesleft * 4};
        out.resize(outbytesleft + 4);
        std::fill(out.begin(), out.end(), 0);
        char* outptr{reinterpret_cast<char*>(out.data())};
        int result = iconv(icv, &inptr, &inbytesleft, &outptr, &outbytesleft);
        if (result == (size_t)-1) 
        {
            perror("Error converting from utf8 to shift-jis");
            std::cerr << "File: " << argv[1] << std::endl;
            for(size_t i=0;i < std::min(inbytesleft,4ull);++i)
            {
                std::cerr << " '" << static_cast<uint8_t>(inptr[i]) << "' " << (static_cast<uint32_t>(inptr[i]) & 0x000000ff) << std::dec;
            }
                std::cerr << std::endl << inptr << std::endl;
            iconv_close(icv);
            return -1;
        }
        strings.push_back({});
        size_t string_lenght{strlen(reinterpret_cast<char*>(out.data()))};
        string_lenght = string_lenght + ( 4 - (string_lenght%4));
        strings.back().resize(string_lenght);
        std::fill(strings.back().begin(), strings.back().end(), 0);
        std::memcpy(strings.back().data(),out.data(),string_lenght);
    }

    uint8_t* offset{reinterpret_cast<uint8_t*>(buffer.data())};
    std::ofstream outfile(outFilePath.c_str(), std::ios::binary);
    size_t string_index{0};
    while (offset < buffer.data() + fileSize) 
    {
        uint32_t* tags{reinterpret_cast<uint32_t*>(offset)};
        //std::cout << std::hex << offset - buffer.data() << ": " << tags[0] << " " <<  tags[1] << " " << tags[2] << " " << tags[3]<< std::endl;
        if(tags[1] == 0x66660001 && tags[2] == 0x55550002 && (tags[3] == 0x44440002 /*|| tags[3] == 0x44440001*/))
        {
            size_t index{4};
            uint32_t string_count{0};
            while(offset + index * 4 < buffer.data() + tags[4] * 4)
            {
                ++string_count;
                ++index;
            }
            uint32_t chunk_size{(string_count + 4) * 4};
            for(uint32_t i = 0; i < string_count;++i)
            {
                chunk_size += strings[i + string_index].size();
            }
            uint32_t base_offset = static_cast<uint32_t>(outfile.tellp());
            chunk_size/=4;
            outfile.write(reinterpret_cast<char*>(&chunk_size),sizeof(uint32_t));
            outfile.write(reinterpret_cast<char*>(tags+1),sizeof(uint32_t)*3);
            chunk_size=base_offset+((string_count + 4) * 4);
            for(uint32_t i = 0; i < string_count;++i)
            {
                uint32_t string_offset{chunk_size/4};
                outfile.write(reinterpret_cast<char*>(&string_offset),sizeof(uint32_t));
                chunk_size += strings[i + string_index].size();
            }
            for(uint32_t i = 0; i < string_count;++i)
            {
                outfile.write(reinterpret_cast<char*>(strings[i + string_index].data()),
                strings[i + string_index].size());
            }
            string_index += string_count;
        }
        else
        {
            outfile.write(reinterpret_cast<char*>(offset),4 * *reinterpret_cast<uint32_t*>(offset));
        }
        offset += 4 * *reinterpret_cast<uint32_t*>(offset);
    }
    outfile.close();
    iconv_close(icv);
    std::cerr << "Injection completed." << std::endl;
    return 0;
}
