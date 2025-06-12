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
#include <regex>
#include "nlohmann/json.hpp"
#include "ollama.hpp"

#define MODEL "visual-novel-translate"
//#define MODEL "7shi/llama-translate:8b-q4_K_M"
using json = nlohmann::json;

std::vector<std::string> SplitLines(const std::string& str)
{
    std::regex regex("\r?\n");
    std::vector<std::string> list(std::sregex_token_iterator(str.begin(), str.end(), regex, -1),
                                  std::sregex_token_iterator());
    return list;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
      std::cout << "Usage: " << argv[0] << " input.json [output.json]" << std::endl;
      return 1;
  }

  std::string output_file{argv[1]};
  if (argc == 2)
  {
    output_file = std::regex_replace( output_file, std::regex("_ja"), "_en" );
  }
  else
  {
      output_file = argv[2];
  }
  json j{};
  std::ifstream f(argv[1]);
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

  json out{};
    ollama::setReadTimeout(300);
    ollama::setWriteTimeout(300);
    ollama::messages messages{};
    messages.reserve(16);
    ollama::response response{ollama::chat(MODEL, messages)};
    size_t line_count{0};
    for(auto& i: j)
    {
        //std::cerr << i["text"] << std::endl;
        auto lines = SplitLines(i["text"]);
        std::string line{};
        for(auto& k: lines)
        {
          try
          {
            if(messages.size() == messages.capacity())
            {
              std::shift_left(messages.begin(),messages.end(),2);
              messages.resize(messages.size() - 2);
            }
            messages.push_back({"user",k});
            response = ollama::chat(MODEL, messages);
            messages.push_back({"assistant",response});
            line += std::string{response} + "\r\n";
          } 
          catch(ollama::exception& e)
          {
            std::cerr << std::endl << e.what() << std::endl;
            return -1;
          }

        }
        //std::cerr << response << std::endl;
        out.push_back(json::object({{"text",line}}));
        std::cerr << "\rLines Translated: " << ++line_count << " of " << j.size() << std::flush;
    }

  std::cerr << std::endl << "Writting " << output_file.c_str() << std::endl;
  std::ofstream o(output_file.c_str());
  o << std::setw(4) << out << std::endl;
  o.close();

  std::cerr << "Translation Complete" << std::endl;
  return 0;
}
