// g++ azure-translate.cpp `curl-config --libs` -o azure-translate.exe
#define NOMINMAX
#define JSON_DIAGNOSTICS 1
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <locale>
#include <codecvt>
#include <regex>
#include <filesystem>
#include <algorithm>
#include <map>
#include "nlohmann/json.hpp"
#include "ollama.hpp"

#define DEBUG_OUTPUT false
#define MODEL "visual-novel-translate"
using json = nlohmann::json;

int main(int argc, char* argv[]) {

#ifdef _WIN32
    // Enable UTF-8 output on Windows console
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " input.json [output.json]" << std::endl;
        return 1;
    }

    std::filesystem::path input_path{argv[1]};
    std::filesystem::path charaname_path{input_path.parent_path() / "charaname.json"};
    if (!std::filesystem::exists(input_path)) {
        std::cerr << "Input file does not exist: " << input_path << std::endl;
        return 1;
    }
    if (argc == 3) 
    {
        charaname_path = argv[2];
        if (charaname_path == input_path) {
            std::cerr << "Output file cannot be the same as input file: " << charaname_path << std::endl;
            return 1;
        }
    }

    std::map<std::string, std::string> charaname_map;

    json charaname{};
    if(std::filesystem::exists(charaname_path))
    {
      std::ifstream f(charaname_path);
      try
      {
          charaname = json::parse(f);
      }
      catch (const json::exception& e)
      {
          std::cerr << "File: " << charaname_path << std::endl;
          std::cerr << e.what() << std::endl;
          return -1;
      }
      if(charaname.size()!=0)
      {
        for(auto& i: charaname)
        {
            charaname_map[std::string(i["from"])] = i["to"];
        }
      }
    }

    json j{};
    std::ifstream f(input_path);
    try
    {
        j = json::parse(f);
    }
    catch (const json::exception& e)
    {
        std::cerr << "File: " << argv[1] << '\n';
        std::cerr << e.what() << '\n';
        return -1;
    }
    if(j.size()==0)
    {
        std::cerr << "Empty Json File: " << argv[1] << std::endl;
        return -1;
    }

    ollama::setReadTimeout(900);
    ollama::setWriteTimeout(300);
    ollama::messages messages{};
    messages.reserve(16);
    ollama::response response{};

    size_t line_count{0};
    for(auto& i: j)
    {
      ++line_count;
        const std::regex base_regex("^([\\S]+)\\r?\\n");
        std::smatch base_match;
        std::string name{i["text"]};
        if (std::regex_search(name, base_match, base_regex))
        {
            if (base_match.size() == 2)
            {
                name = base_match[1].str();
                if(charaname_map.find(name) != charaname_map.end())
                {
                    continue; // Skip if name already exists in charaname_map
                }
            }
            else
            {
              continue;
            }
        }
        else
        {
          continue;
        }
        try
        {
            if(messages.size() == messages.capacity())
            {
              std::shift_left(messages.begin(),messages.end(),2);
              messages.resize(messages.size() - 2);
            }
            messages.push_back({"user",name});
            if(DEBUG_OUTPUT) {std::cerr << messages.back() << std::endl;}
            response = ollama::chat(MODEL, messages);
            messages.push_back({"assistant",response});
            if(DEBUG_OUTPUT) {std::cerr << messages.back() << std::endl;}
            charaname_map[name] = response.as_simple_string();
            std::cerr << "\r\x1b[2KName From: " << name << " Name To: " << response << " | Lines processed: " << line_count << " of " << j.size() << std::flush;
        } 
        catch(ollama::exception& e)
        {
          std::cerr << std::endl << e.what() << std::endl;
          for(auto& m: messages)
          {
            std::cerr << m << std::endl;
          }
          std::cerr << "On file:" << argv[1] << std::endl;
          return -1;
        }            
        //std::cerr << "\rNames Translated: " << ++line_count << " of " << j.size() << std::flush;
    }

    charaname.clear();
    for(auto& i: charaname_map)
    {
        json charaname_entry;
        charaname_entry["from"] = i.first;
        charaname_entry["to"] = i.second;
        charaname.push_back(charaname_entry);
    }

    std::cerr << "\r\x1b[2KWritting " << charaname_path << std::endl << std::flush;
    std::ofstream o(charaname_path.c_str());
    o << std::setw(4) << charaname << std::endl;
    o.close();

    std::cerr << "Translation Complete" << std::endl;
    return 0;
}
