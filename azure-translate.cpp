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
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

size_t print_response(char *ptr, size_t size, size_t nmemb, json *out){
    std::string response{ptr , size*nmemb};
    json r = json::parse(response);
    if(r.contains("error"))
    {
      std::cerr << r << std::endl;
      return 0;
    }
    for(auto& i: r)
    {
      for(auto& j: i["translations"])
      {
        out->push_back(json::object({{"text",j["text"]}}));
      }
    }
    return size * nmemb;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
      std::cout << "Usage: " << argv[0] << " input.json [output.json] Subscription-Key Subscription-Region" << std::endl;
      return 1;
  }

  std::string output_file{argv[1]};
  if (argc == 4) 
  {
    output_file = std::regex_replace( output_file, std::regex("_ja"), "_en" );
  }
  else
  {
      output_file = argv[-3];
  }
  CURL *curl;
  CURLcode res;
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
  curl_global_init(CURL_GLOBAL_ALL);
 
  curl = curl_easy_init();
  if(curl) 
  {
    curl_easy_setopt(curl, CURLOPT_URL, "https://api.cognitive.microsofttranslator.com/translate?api-version=3.0&from=ja&to=en");
    struct curl_slist *headers{nullptr};
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string OcpApimSubscriptionKey{"Ocp-Apim-Subscription-Key: " + std::string{argv[argc-2]}};
    headers = curl_slist_append(headers, OcpApimSubscriptionKey.c_str());
    std::string OcpApimSubscriptionRegion{"Ocp-Apim-Subscription-Region: " + std::string{argv[argc-1]}};
    headers = curl_slist_append(headers, OcpApimSubscriptionRegion.c_str());
 
    /* pass our list of custom made headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, print_response);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);

    size_t line_count{0};
    for(auto& i: j)
    {
        json post{i};
        std::string post_data{post.dump()};
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
          std::cerr << "Translation Failed: " << std::endl << argv[1] << std::endl << std::endl << post << std::endl << curl_easy_strerror(res) << std::endl;
          return -1;
        }
        std::cerr << "\rLines Translated: " << ++line_count << " of " << j.size() << std::flush;
    }
    curl_slist_free_all(headers); /* free the header list */ 
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();

  std::cerr << std::endl << "Writting " << output_file.c_str() << std::endl;
  std::ofstream o(output_file.c_str());
  o << std::setw(4) << out << std::endl;
  o.close();

  std::cerr << "Translation Complete" << std::endl;
  return 0;
}
