#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <regex>
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
const size_t maxline{3};

#define REP(x,y) \
    [](const std::string& s) -> std::string\
    {\
        return std::regex_replace( s, std::regex(x), y );\
    }


std::vector<std::function<std::string(const std::string&)>> replacers
{
    REP("ï|ī", "i"),
    REP("é", "e"),
    REP("ç", "c"),
    REP("ū","u"),
    REP("~|～", "〜"),
    REP("—","-"),
    REP("\\.\\.\\.","…"),
    REP("　"," "),
    REP("？","?"),
    REP("「","\""),
    REP("」","\""),
    REP("ō","o"),
};

std::string ReplaceBadChars(const std::string& input)
{
    std::string result{input};
    for(auto& i: replacers)
    {
        result = i(result);
    }
    return result;
}
std::vector<std::string> SplitLines(const std::string& str)
{
    std::regex regex("\r?\n");
    std::vector<std::string> list(std::sregex_token_iterator(str.begin(), str.end(), regex, -1),
                                  std::sregex_token_iterator());
    return list;
}

std::string FormatLine(const std::string& str, size_t& not_formated, size_t& formated)
{
    const static std::regex empty_space("[\\n\\r\\s]+");
    const static std::regex double_quotes("\"[\\n\\r\\s]+\"");
    // We never modify the first line which is usually a character's name
    auto first_newline{str.find_first_of("\r\n")};
    if(first_newline==std::string::npos||first_newline+2==str.size())
    {
        ++not_formated;
        return ReplaceBadChars(first_newline==std::string::npos ? str + "\r\n" : str);
    }
    std::string first_line{ReplaceBadChars(str.substr(0,first_newline+2))};
    std::string second_line{ReplaceBadChars(str.substr(first_newline+2))};
    {
        auto lines = SplitLines(second_line);
        bool format_needed{false};
        for(auto& i: lines)
        {
            if(i.size() > static_cast<size_t>(48))
            {
                format_needed = true;
                break;
            }
        }
        if(!format_needed)
        {
            ++not_formated;
            return ReplaceBadChars(str);
        }
    }
    ++formated;
    second_line = std::regex_replace( second_line, empty_space, " " );
    second_line = std::regex_replace( second_line, double_quotes, " " );
    // Remove leading whitespace
    second_line.erase(second_line.begin(), std::find_if(second_line.begin(), second_line.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));

    // Remove trailing whitespace
    second_line.erase(std::find_if(second_line.rbegin(), second_line.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), second_line.end());

    size_t max_line_lenght{std::max(second_line.size()/maxline, static_cast<size_t>(48))};

    size_t index{max_line_lenght};
    size_t line_count{};
    while(index < second_line.size())
    {
        index = second_line.find_first_of(" ", index);
        if(index==std::string::npos)
        {
            break;
        }
        second_line[index] = '\n';
        second_line.insert(index,"\r");
        index += (max_line_lenght+1);
        ++line_count;
    }
    if(line_count>maxline)
    {
        std::cerr << "More than "<< maxline <<" lines:" << std::endl << second_line << std::endl;
    }

    return first_line+second_line+"\r\n";
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file> [output_file]" << std::endl;
        return 1;
    }

    size_t not_formated{0};
    size_t formated{0};
    std::string output_file{argc > 2  ? argv[2] : argv[1]};
    json input_json{};
    std::ifstream f(argv[1]);
    try
    {
        input_json = json::parse(f);
    }
    catch (const json::exception& e)
    {
        std::cerr << "File: " << argv[1] << '\n';
        std::cerr << e.what() << '\n';
        return -1;
    }

    if(input_json.size()==0)
    {
        std::cerr << "Empty Json File: " << argv[1] << std::endl;
        return -1;
    }
    f.close();

    json output_json{};

    for(auto& i: input_json)
    {
        output_json.push_back(json::object({{"text",FormatLine(i["text"],not_formated,formated)}}));
    }

    std::cerr << "Writting " << output_file.c_str() << std::endl;
    std::ofstream o(output_file.c_str());
    o << std::setw(4) << output_json << std::endl;
    o.close();

    std::cerr << "Formatting Complete, messages formated "<< formated<<" messages not formated " << not_formated << std::endl;

    return 0;
}
