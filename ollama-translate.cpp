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

using json = nlohmann::json;

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

    ollama::messages messages{};
    messages.reserve((j.size()*2)+5);
    messages.push_back({"user","I am going to give you game dialogue lines to translate to english, consider the context of the story, keep the structure of every line, use Romaji for given names."});
    ollama::response response{ollama::chat("7shi/llama-translate:8b-q4_K_M", messages)};
    size_t line_count{0};
    for(auto& i: j)
    {
        messages.push_back({"user",i["text"]});
        response = ollama::chat("7shi/llama-translate:8b-q4_K_M", messages);
        messages.push_back({"assistant",response});
        //std::cerr << response << std::endl;
        out.push_back(json::object({{"text",response}}));
        std::cerr << "\rLines Translated: " << ++line_count << " of " << j.size() << std::flush;
    }

  std::cerr << std::endl << "Writting " << output_file.c_str() << std::endl;
  std::ofstream o(output_file.c_str());
  o << std::setw(4) << out << std::endl;
  o.close();

  std::cerr << "Translation Complete" << std::endl;
  return 0;
}
