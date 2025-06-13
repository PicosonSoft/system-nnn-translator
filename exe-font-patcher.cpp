#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <regex>
#include <vector>

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
    std::vector<uint8_t> buffer(fileSize);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    // Close the file
    inputFile.close();

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

    std::cout << "File patched successfully." << std::endl;

    return 0;
}
