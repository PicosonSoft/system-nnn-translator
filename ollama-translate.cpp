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
#include <filesystem>
#include <algorithm>
#include "nlohmann/json.hpp"
#include "ollama.hpp"

#define MODEL "visual-novel-translate"
using json = nlohmann::json;

std::vector<std::string> SplitLines(const std::string& str)
{
    std::regex regex("\r?\n");
    std::vector<std::string> list(std::sregex_token_iterator(str.begin(), str.end(), regex, -1),
                                  std::sregex_token_iterator());
    return list;
}

std::string Replace(const std::vector<std::function<std::string(const std::string&)>>& replacers, const std::string& input)
{
    std::string result{input};
    for(auto& i: replacers)
    {
        result = i(result);
    }
    return result;
}

std::string Match(const std::vector<std::function<std::string(const std::string&)>>& matchers, const std::string& input)
{
    for(auto& i: matchers)
    {
        auto result{i(input)};
        if(!result.empty())
        {
          return result;
        }
    }
    return {};
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " input.json [output.json]" << std::endl;
        return 1;
    }
    json charaname{};
    size_t charaname_index{0};
    std::string output_file{argv[1]};
    std::vector<std::function<std::string(const std::string&)>> charaname_replacers{};
    std::vector<std::function<std::string(const std::string&)>> charaname_matchers{};
    if (argc == 2 | argc == 3 && std::filesystem::path(argv[2]).filename()=="charaname.json")
    {
        output_file = std::regex_replace( output_file, std::regex("_ja"), "_en" );
        if(argc==3)
        {
          charaname_index = 2;
        }
    }
    else if(argc>2)
    {
        output_file = argv[2];
        if(argc>3)
        {
          charaname_index = 3;
        }
    }

    if(charaname_index!=0)
    {
      std::ifstream f(argv[charaname_index]);
      try
      {
          charaname = json::parse(f);
      }
      catch (const json::exception& e)
      {
          std::cerr << "File: " << argv[charaname_index] << '\n';
          std::cerr << e.what() << '\n';
          return -1;
      }
      if(charaname.size()!=0)
      {
        for(auto& i: charaname)
        {
          charaname_replacers.push_back(
          [i](const std::string& s) -> std::string
              {
                  return std::regex_replace( s, std::regex(std::string(i["from"])),std::string(i["to"]));
              }            
          );
          charaname_matchers.push_back(
          [i](const std::string& s) -> std::string
              {
                  return (s==std::string{i["from"]}) ? std::string{i["to"]} : std::string{};
              }            
          );
        }
      }
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
    ollama::setReadTimeout(900);
    ollama::setWriteTimeout(300);
    ollama::messages messages{};
    messages.reserve(16);
    /* 
    The model gets creative with onomatopoeias,
    adding acknowledgements or getting sassy when translating them
    so bait it with messages that better represents what is expected.

    NOTE: THIS IS PROBABLY NOT NECESSARY. 
    The halucinations and sassynes might have been the result of passing
    empty strings to translate which has already been addressed.
    */
    ollama::messages no_character_messages
    {
        ollama::message{"user","バタン！"},
        ollama::message{"assistant","BANG!"},
        ollama::message{"user","コンコン"},
        ollama::message{"assistant","Knock knock"},
        ollama::message{"user","チュプ、んん、チュパ"},
        ollama::message{"assistant","Chup, mmm, Chupa"},
        ollama::message{"user","ドンドン"},
        ollama::message{"assistant","Boom boom"},
        ollama::message{"user","ガッシャーン"},
        ollama::message{"assistant","Crash!"},
        ollama::message{"user","じょろじょろ"},
        ollama::message{"assistant","Jororo jororo"}
    };
    no_character_messages.reserve(no_character_messages.size()+1);
    ollama::response response{};

    size_t line_count{0};
    for(auto& i: j)
    {
        auto lines = SplitLines(i["text"]);
        bool no_character_name{!lines.empty() && lines[0].empty()};
        std::string line{};
        for(auto& k: lines)
        {
          if(k.empty())
          {
            line+="\r\n";
            continue;
          }
          // If we have a charaname file, avoid translating character names
          auto charname = Match(charaname_matchers,k);
          if(!charname.empty())
          {
            line+=charname+"\r\n";
            continue;
          }
          try
          {
            if(no_character_name)
            {
              no_character_messages.push_back({"user",Replace(charaname_replacers,k)});
              response = ollama::chat(MODEL, no_character_messages);
              no_character_messages.pop_back();
              line += std::string{response} + "\r\n";
            }
            else
            {
              if(messages.size() == messages.capacity())
              {
                std::shift_left(messages.begin(),messages.end(),2);
                messages.resize(messages.size() - 2);
              }
              messages.push_back({"user",Replace(charaname_replacers,k)});
              response = ollama::chat(MODEL, messages);
              messages.push_back({"assistant",response});
              line += std::string{response} + "\r\n";
            }
          } 
          catch(ollama::exception& e)
          {
            std::cerr << std::endl << e.what() << std::endl;
            for(auto& m: no_character_name ? no_character_messages : messages)
            {
              std::cerr << m << std::endl;
            }
            std::cerr << "On file:" << argv[1] << std::endl;
            return -1;
          }
        }
        //std::cerr << response << std::endl;
        out.push_back(json::object({{"text",line}}));
        std::cerr << "\rMessages Translated: " << ++line_count << " of " << j.size() << std::flush;
    }

    std::cerr << std::endl << "Writting " << output_file.c_str() << std::endl;
    std::ofstream o(output_file.c_str());
    o << std::setw(4) << out << std::endl;
    o.close();

    std::cerr << "Translation Complete" << std::endl;
    return 0;
}
