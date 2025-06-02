//g++ -std=c++20 decode.cpp -o decode.exe
#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>

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

    std::string output_file{};
    if (argc == 2) 
    {
        std::filesystem::path outpath{argv[1]};
        if(outpath.extension()==".spt")
        {
            outpath.replace_extension(".tps");
        }
        else if(outpath.extension()==".tps")
        {
            outpath.replace_extension(".spt");
        }
        else
        {
            std::cerr << "File extension must be .tps or spt." << std::endl;
            return -1;
        }
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

    uint32_t w{};
    while (inputFile.read(reinterpret_cast<char*>(&w), sizeof(w))) {
        w = 0xffffffff ^ w;
        outputFile.write(reinterpret_cast<char*>(&w), sizeof(w));
    }

    inputFile.close();
    outputFile.close();

    std::cout << "File copied successfully." << std::endl;

    return 0;
}
