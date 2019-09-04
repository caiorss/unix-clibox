#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <cstring> // strtok
#include <regex>

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;

namespace fileutils
{
     template<typename Predicate, typename Action>
     void iterate_dirlist(std::string path, Predicate&& pred, Action&& act)
     {
         for(auto& p: fs::directory_iterator(path))
             if(pred(p)) {
                 try { act(p); }
                 catch(fs::filesystem_error& ex)
                 {
                     std::cerr << ex.what() << "\n";
                 }
             }
     }

     /** Higher-order function for reading a file line-by-line */
     template<typename Action>
     void process_line(std::string const& filename, Action&& line_processor)
     {
         using namespace std::string_literals;

         auto fs = std::ifstream(filename);
         // Report error to the caller
         if(!fs) {
             throw std::logic_error(" Error: failed to open file: "s + filename);
         }
         std::string line;
         while(std::getline(fs, line)) { line_processor(line); }
     }

     bool contains_string(std::string pattern, std::string text)
     {
         return text.find(pattern) != std::string::npos;
     }

     bool contains_string2(std::string const& s, const std::string& cont)
     {
         return cont.end() != std::search(cont.begin(), cont.end()
                                         ,std::boyer_moore_searcher(s.begin(), s.end()));
     }

     std::string
     to_lowercase(std::string const& text)
     {
         std::string t = text;
         std::transform( t.begin(),
                         t.end(),
                         t.begin(),
                         [](char x){ return std::toupper(x); }
                        );
         return t;
     }

     std::string
     right_trim(const std::string& s)
     {
         size_t end = s.find_last_not_of(" \n\r\t\f\v");
         return (end == std::string::npos) ? "" : s.substr(0, end + 1);
     }

     template<typename MATCHER>
     void search_file(std::string pattern, std::string filename, MATCHER&& matcher)
     {
         using namespace std::string_literals;
         long line_number = 0;
         bool pattern_found = false;

         process_line(filename,
                      [&](std::string const& line)
                      {
                          if(matcher(line))
                          {
                              auto p = fs::path(filename);

                              if(!pattern_found) {
                                  pattern_found = true;
                                  std::cout << "\n\n"
                                            << "  => File: "s + p.filename().string() << "\n";
                                  std::cout << "  " << std::string(50, '-') << "\n";
                              }
                              std::cout << std::setw(10) << line_number
                                        << " "
                                        << std::setw(10) << line
                                        << "\n";
                          }
                          ++line_number;
                      });
     }

     void search_file_for_text(std::string pattern, std::string filename)
     {
         search_file(pattern, filename, [=](std::string const& line)
                     {
                         return contains_string(pattern, line) ;
                     });
     }

} // * --- End of namespace fileutils --- * //



int main(int argc, char** argv)
{
    CLI::App app{ "text-search"};
    //app.footer("\n Creator: Somebody else.");

    std::string pattern;
    app.add_option("<PATTERN>", pattern, "Text pattern")->required();

    // Sets directory that will be listed
    std::vector<std::string> filepaths;
    app.add_option("<FILE>", filepaths, "File to be searched")->required();


    // ----- Parse Arguments ---------//
    try {
        app.validate_positionals();
        CLI11_PARSE(app, argc, argv);
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }        

    //------ Program Actions ---------//

    std::cout << "\n Seach results for pattern: '" << pattern << "'";

    for (auto const& fname : filepaths)
    {
        try
        {
            fileutils::search_file_for_text(pattern, fname);
        } catch (std::logic_error& ex)
        {
            std::cerr << " [ERROR / FILE] " << ex.what() << "\n";
            return  EXIT_FAILURE;
        } catch  (std::regex_error& ex)
        {
            std::cerr << " [ERROR / REGEX] " << ex.what() << "\n";
            return EXIT_FAILURE;
        }
    }


    return EXIT_SUCCESS;
}
