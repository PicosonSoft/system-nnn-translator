#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <vector>
#include <array>
#include <tuple>

std::array<std::pair<std::vector<uint8_t>, std::vector<uint8_t>>,2>
replacers{{
    {{0x6A,0x00,0x6A,0x00,0x6A,0x00,0x68,0x80,0x00,0x00,0x00,0x6A,0x00,0x6A,0x00},
     {0x6A,0x00,0x6A,0x00,0x6A,0x00,0x6A,0x00,0x90,0x90,0x90,0x6A,0x00,0x6A,0x00}},
    {{0x82,0x6C,0x82,0x72,0x20,0x96,0xBE,0x92,0xA9,0x00},
     {'C', 'a', 'l', 'i', 'b', 'r', 'i', 0x00,0x00,0x00}}
}};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::ifstream inputFile(argv[1], std::ios::binary);
    if (!inputFile) {
        std::cerr << "Failed to open input file: " << argv[1] << std::endl;
        return 1;
    }
    // Get the file size
    inputFile.seekg(0, std::ios::end);
    std::streampos fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);
    // Read the file contents into a vector
    std::string buffer{};
    buffer.resize(fileSize);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    // Close the file
    inputFile.close();

    size_t replacements{0};
    for(size_t i=0; i<buffer.size(); ++i)
    {
        for(auto& j: replacers)
        {
            if(i+j.first.size()<buffer.size())
            {
                if(std::memcmp(buffer.data()+i,j.first.data(),j.first.size())==0)
                {
                    memcpy(buffer.data() + i,j.second.data(),j.second.size());
                    ++replacements;
                    i+=j.second.size();
                }
            }
        }
    }

    std::string output_file{};
    if (argc == 2) 
    {
        std::filesystem::path outpath{argv[1]};
        outpath.replace_filename(outpath.stem().string() + "_en.exe");
        output_file = outpath.string();
    }
    else
    {
        output_file = argv[2];
    }
    std::ofstream outputFile(output_file.c_str(), std::ios::binary);
    if (!outputFile) {
        std::cerr << "Failed to open output file: " << argv[2] << std::endl;
        return 1;
    }

    outputFile.write(reinterpret_cast<char*>(buffer.data()),buffer.size());
    outputFile.close();

    std::cout << "File patched successfully, " << replacements << " replacements." << std::endl;

    return 0;
}
