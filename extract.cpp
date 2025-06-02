//g++ extract.cpp -liconv -o extract.exe
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <locale>
#include <iconv.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Please provide the path to the binary file as an argument." << std::endl;
        return 1;
    }

    const char* filePath = argv[1];

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

    uint8_t* offset{ reinterpret_cast<uint8_t*>(buffer.data()) };

    auto icv = iconv_open("UTF-8", "SHIFT_JIS");

    if (icv == (iconv_t)-1) 
    {
        std::cerr << "Failed to open iconv." << std::endl;
        return 1;
    }

    std::vector<uint8_t> out{};
    json j{};
    bool has_text{false};
    while (offset < buffer.data() + fileSize) 
    {
        uint32_t* tags{reinterpret_cast<uint32_t*>(offset)};
        //std::cout << std::hex << offset - buffer.data() << ": " << tags[0] << " " <<  tags[1] << " " << tags[2] << " " << tags[3]<< std::endl;
        if(tags[1] == 0x66660001 && tags[2] == 0x55550002 && (tags[3] == 0x44440002 /*|| tags[3] == 0x44440001*/))
        {
            has_text = true;
            size_t index{4};
            while(offset + index * 4 < buffer.data() + tags[4] * 4)
            {
                char* inptr{reinterpret_cast<char*>(buffer.data() + tags[index] * 4)};
                size_t inbytesleft{std::strlen(inptr)};
                size_t outbytesleft{inbytesleft * 4};
                out.resize(outbytesleft+4);
                std::fill(out.begin(), out.end(), 0);
                char* outptr{reinterpret_cast<char*>(out.data())};
                int result = iconv(icv, &inptr, &inbytesleft, &outptr, &outbytesleft);
                if (result == (size_t)-1) {
                    perror("iconv failed");
                    iconv_close(icv);
                    return 1;
                }
                j.push_back(json::object({{"text",reinterpret_cast<char*>(out.data())}}));
                index++;
            }
        }
        offset += 4 * *reinterpret_cast<uint32_t*>(offset);
    }
    iconv_close(icv);

    if(!has_text)
    {
        std::cerr << "File " << argv[1] << " Contains no dialogue." << std::endl;
        return -1;
    }

    std::string output_file{};
    if (argc == 2) 
    {
        std::filesystem::path outpath{argv[1]};
        outpath.replace_filename(outpath.stem().string() + "_ja.json");
        output_file = outpath.string();
    }
    else
    {
        output_file = argv[2];
    }

    // write prettified JSON to another file
    std::ofstream o(output_file.c_str());
    o << std::setw(4) << j << std::endl;
    o.close();

    std::cerr << "Extraction completed." << std::endl;
    return 0;
}
