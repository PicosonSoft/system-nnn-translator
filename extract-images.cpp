//g++ -std=c++20 decode.cpp -o decode.exe
#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <vector>
#include <regex>
#include <windows.h>
#include <wingdi.h>

#if 0
	if (m_dwq[0x39] == '2')
	{
		m_maskFlag = TRUE;
		if ((*(ptr + 48 + 10)) == 'A') m_alreadyCutFlag = TRUE;
	}

	if (m_dwq[0x39] == '3')
	{
		m_packFlag = TRUE;
		m_maskFlag = TRUE;
		if ((*(ptr + 48 + 10)) == 'A') m_alreadyCutFlag = TRUE;
	}

	if (m_dwq[0x39] == '5')
	{
		m_jpegFlag = TRUE;
	}

	if (m_dwq[0x39] == '7')
	{
		m_jpegFlag = TRUE;
		m_maskFlag = TRUE;
		if ((*(ptr + 48 + 10)) == 'A') m_alreadyCutFlag = TRUE;
	}

	//8A�̂ݑ���
	if (m_dwq[0x39] == '8')
	{
//		m_jpegFlag = TRUE;
//		m_maskFlag = TRUE;
		m_pngFlag = TRUE;
		if ((*(ptr + 48 + 10)) == 'A') m_alreadyCutFlag = TRUE;
	}
#endif

struct DWQHeader
{
	char Type[32];
	int32_t IntData[4];
	char PackType[16];
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <gtb_file>" << std::endl;
        return 1;
    }

    std::ifstream gtb_file(argv[1], std::ios::binary);
    if (!gtb_file) {
        std::cerr << "Failed to open input file: " << argv[1] << std::endl;
        return 1;
    }
    std::filesystem::path outpath{argv[1]};

    std::vector<uint8_t> gtb{};

    std::string gpk_file{};
    if (argc == 2) 
    {
        if(outpath.extension()==".gtb")
        {
            outpath.replace_extension(".gpk");
            gtb_file.seekg(0, std::ios::end);
            std::streampos fileSize = gtb_file.tellg();
            gtb_file.seekg(0, std::ios::beg);
            gtb.resize(fileSize);
            gtb_file.read(reinterpret_cast<char*>(gtb.data()), fileSize);
            gtb_file.close();

        }
        else if(outpath.extension()!=".dwq")
        {
            std::cerr << "File extension must be gtb or dwq." << std::endl;
            return -1;
        }
        gpk_file = outpath.string();
    }

    std::ifstream gpk(gpk_file, std::ios::binary);
    if (!gpk) {
        std::cerr << "Failed to open input file: " << gpk_file << std::endl;
        return 1;
    }

    uint32_t file_count{gtb.empty() ? 1 : *(reinterpret_cast<uint32_t*>(gtb.data()))};
    uint32_t* string_offset_table{gtb.empty() ? nullptr : reinterpret_cast<uint32_t*>(gtb.data()) + 1};
    uint32_t* file_offset_table{gtb.empty() ? nullptr : reinterpret_cast<uint32_t*>(gtb.data()) + 1 + file_count};
    DWQHeader header{};
    std::vector<uint8_t> buffer{};
    for(uint32_t i = 0;i<file_count;++i)
    {
        //std::cerr << std::hex << string_offset_table[i] << " " << file_offset_table[i] << " " << (gtb.data() + 4 + file_count * 8) + string_offset_table[i] << std::endl;
        gpk.seekg(file_offset_table ? file_offset_table[i] : 0, std::ios::beg);
        gpk.read(reinterpret_cast<char*>(&header),sizeof(DWQHeader));
        header.Type[31]=0;
        header.PackType[15]=0;
        //std::cerr << header.Type << std::endl;
        //std::cerr << header.PackType << std::endl;
        // Pick an output file name
        if(reinterpret_cast<uint32_t*>(header.Type)[0]==0x4745504A)
        {
            //std::cerr << "Size: " << std::dec << header.IntData[0] << " Width: " << header.IntData[1] << " Height: " << header.IntData[2] << std::endl;
            buffer.resize(header.IntData[0]);
            auto path { string_offset_table ? outpath.parent_path() / reinterpret_cast<char*>((gtb.data() + 4 + file_count * 8) + string_offset_table[i]) : outpath };
            path.replace_extension(".jpg");
            //std::cerr << path << std::endl;
            gpk.read(reinterpret_cast<char*>(buffer.data()), header.IntData[0]);
            std::ofstream o(path.c_str(), std::ios::binary);
            o.write(reinterpret_cast<const char*>(buffer.data()), header.IntData[0]);
            o.close();
        }
        else if(reinterpret_cast<uint32_t*>(header.Type)[0]==0x20504D42)
        {
            //std::cerr << "Width: " << std::dec << header.IntData[1] << " Height: " << header.IntData[2] << std::endl;
            auto path { string_offset_table ? outpath.parent_path() / reinterpret_cast<char*>((gtb.data() + 4 + file_count * 8) + string_offset_table[i]) : outpath };
            path.replace_extension(".bmp");
            //std::cerr << path << std::endl;
            BITMAPFILEHEADER bmp_header{};
            BITMAPINFOHEADER bmp_info{};
            gpk.read(reinterpret_cast<char*>(&bmp_header), sizeof(BITMAPFILEHEADER));
            gpk.read(reinterpret_cast<char*>(&bmp_info), sizeof(BITMAPINFOHEADER));
            //std::cerr << "Size: " << std::dec << bmp_header.bfSize << " Offset: " << bmp_header.bfOffBits << std::endl;
            //std::cerr << "Data Start: " << sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) << " BitCount: " << bmp_info.biBitCount << " Width: " << bmp_info.biWidth << " Height: " << bmp_info.biHeight << std::endl;
            buffer.resize(bmp_header.bfSize - sizeof(BITMAPFILEHEADER) - sizeof(BITMAPINFOHEADER));
            gpk.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            std::ofstream o(path.c_str(), std::ios::binary);
            o.write(reinterpret_cast<char*>(&bmp_header), sizeof(BITMAPFILEHEADER));
            o.write(reinterpret_cast<char*>(&bmp_info), sizeof(BITMAPINFOHEADER));
            o.write(reinterpret_cast<char*>(buffer.data()), buffer.size());
            o.close();
        }
        else
        {
            std::cerr << "Unknown Type: " << std::regex_replace(std::string{header.Type}, std::regex("\\s+"), "\0" ) << " Path: " << outpath << std::endl;
        }
    }

    gpk.close();

    std::cout << "Images Extracted." << std::endl;

    return 0;
}
