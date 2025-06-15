# system-nnn-translator
Simple tools to translate japanese visual novels based on the system-nnn engine.

### Current Status
Workflow works properly in Yami No Koe Zero files, and the game runs, but letters are too far appart and long lines are cut off.

### Overview
First of all, I appologize for the sloppy code. This is a collection of C++ "scripts" put together to automate the translation of visual novels based on the system-nnn engine. It is not a library, but a set of tools that can be used to extract and translate text from these games. There is very little error handling and the code is a mismash of different examples of the libraries used. For this reason it is important to follow the instructions carefully and to use the tools in the order specified.

### Tools
- `decode`: SPT files contain the dialogue text, but these have been "encoded" by XORing all bytes, this tool does that to generate a readable file.
- `extract`: Extracts the dialogue text from a decoded SPT file into a JSON file, shift-jis text is converted to utf8.
- `inject`: Injects the dialogue text from a JSON file into a decoded SPT file, utf8 text is converted to shift-jis.
- `azure-translate`: Takes a Japanese JSON file and translates it using Micorsoft Translate, an azure account and a translation service must have been set up ahead of time as the key and region are required for use of the service.
- `ollama-translate`: Takes a Japanese JSON file and translates it using Ollama, which must be installed and configured beforehand (Use the provided Modelfile).
- `formatter`: Takes a translated JSON file and formats the text such that there is a max of 3 lines per message.

### Requirements
- C++20 compiler
- CMake
- Microsoft Azure account with translation service set up (for `azure-translate`)
- nlohmann-json
- libcurl (for `azure-translate` and `ollama-translate`)
- libiconv (for `extract` and `inject`)
- Ollama (for `ollama-translate`)

### Workflow
1. Use `decode` to decode the SPT file. Decoded files have the .tps extension, that is spt backwards, this is just a preference to distinguish them which also reflects that the encoding is just XOR'ing the whole file, plus the engine uses .xtx for text files.
2. Use `extract` to extract the dialogue text into a JSON file.
3. Use `azure-translate` or `ollama-translate` to translate the JSON file into English.
4. Use `formatter` to fix translated messages.
5. Use `inject` to inject the translated text back into the decoded SPT file.
6. Use `decode` again to encode the SPT file back.
7. Replace the original SPT file with the newly encoded one.

In order do bulk translation all executables can take a single input file and infer the output file name from it, this way you can use 'find' to find all SPT files and process them in one go.

The following table shows posible inputs and outputs for each tool in order of execution:

| Tool            | Input File                | Output File                   |
|-----------------|---------------------------|-------------------------------|
| decode          | `*.spt`                   | `*.tps`                       |
| extract         | `*.tps`                   | `*_ja.json`                   |
| azure-translate | `*_ja.json`               | `*_en.json`                   |
| ollama-translate| `*_ja.json`               | `*_en.json`                   |
| inject          | `*.tps` (`*_en.json`)     | `en/*.tps`                    |
| decode          | `*.tps`                   | `*.spt`                       |
| formatter       | `*.json`                  | `*.json`(overwrites the file) |

Note: The `inject` tool will take a single path for the decoded SPT, but if so, the *_en.json for it must exist on the same directory.

### Example Workflow
To decode, extract, translate, inject and re-encode all SPT files in the current directory, you can use the following command:

```bash
find . -maxdepth 1 -name "*.spt" -exec decode {} \;
find . -maxdepth 1 -name "*.tps" -exec extract {} \;
find . -maxdepth 1 -name "*_ja.json" -exec azure-translate {} <Subscription-Key> <Subscription-Region> \;
find . -maxdepth 1 -name "*_en.json" -exec formatter {} \;
find . -maxdepth 1 -name "*.tps" -exec inject {} \;
find . -maxdepth 1 -name "en/*.tps" -exec decode {} \;
```
