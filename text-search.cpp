#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <bitset>
#include <cstring> // strtok
#include <regex>
#include <algorithm>

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;

/// String utilties
namespace strutils
{
    bool contains_string2(std::string const& s, const std::string& cont)
    {
        return cont.end() != std::search( cont.begin()
                                        , cont.end()
                                        , std::boyer_moore_searcher(s.begin(), s.end()));
    }

    bool ends_with(std::string const & value, std::string const & ending)
    {
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
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
} // * ---- End of namespace strtutils --- * //

namespace fileutils
{
     using namespace strutils;

     template<typename Predicate, typename Action>
     void iterate_dirlist(  std::string path
                          , bool recursive
                          , Predicate&& pred
                          , Action&& act
                          )
     {
         if(!recursive)
         {
             for(auto& p: fs::directory_iterator(path))
                 if(pred(p)) {
                     try { act(p); }
                     catch(fs::filesystem_error& ex)
                     {
                         std::cerr << ex.what() << "\n";
                     }
                 }
             return;
         }

         for(auto& p: fs::recursive_directory_iterator(path))
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

         while(std::getline(fs, line))
         {
             if(!line_processor(line)) break;
         }
     }

     template<typename MATCHER>
     void search_file(  bool        not_show_lines
                      , bool        show_abspath
                      , std::string filename
                      , MATCHER&&   matcher
                      )
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

                                  auto file_path = show_abspath
                                                  ? fs::absolute(p).string()
                                                  : p.filename().string();


                                  std::cout << "\n\n"
                                            << "  => File: "s + file_path << "\n";
                                  std::cout << "  " << std::string(50, '-') << "\n";

                                  // Exit this lambda function
                                  if(not_show_lines) { return false; }
                              }

                              std::cout << std::setw(10) << line_number
                                        << " "
                                        << std::setw(10) << right_trim(line)
                                        << "\n";
                          }
                          ++line_number;

                          return true;
                      });
     }

     void search_file_for_text(std::string pattern, std::string filename, bool not_show_lines)
     {
         search_file(not_show_lines, true, filename,
                     [=](std::string const& line)
                     {
                         return contains_string2( to_lowercase(pattern)
                                                , to_lowercase(line)) ;
                     });
     }

     void search_file_for_regex( std::string pattern
                               , std::string filename
                               , bool not_show_lines)
     {
         std::regex reg{pattern};
         search_file(not_show_lines, true, filename, [=, &reg](std::string const& line)
                     {
                         return std::regex_search(line, reg) ;
                     });
     }

     void search_directory(  std::string pattern
                           , std::string directory
                           , bool recursive
                           , bool not_show_lines
                           , bool show_abspath
                           , std::vector<std::string> const& file_extensions)
     {
         std::puts("\n =========== Seaching files =============");

         iterate_dirlist(directory, recursive
             ,[=](fs::path const& p)
             {
                 if(!fs::is_regular_file(p)) return false;

                 auto it = std::find_if( file_extensions.begin()
                                       , file_extensions.end()
                                       , [=](std::string const& ext)
                                        {
                                            return ends_with(p.filename().string(), ext);
                                        });

                 return it != file_extensions.end();
             }
             ,[=](fs::path const& p)
             {
                // std::cout << " FIle = " << p.filename() << std::endl;

                 search_file(not_show_lines, show_abspath, fs::absolute(p)
                           , [=](std::string const& line)
                             {
                                 return contains_string2( to_lowercase(pattern)
                                                        , to_lowercase(line)) ;
                             });
             });
     }


} // * --- End of namespace fileutils --- * //

struct text_search_options
{
    std::string              pattern    = "";
    std::vector<std::string> filepaths  = {};
    bool                     use_regex  = false;
    bool                     show_abspath = false;
    bool                     noline     = false;
};

struct directory_search_options
{
    std::string              pattern          = "";
    std::string              directory         = ".";
    bool                     recursive         = false;
    bool                     use_regex         = false;
    bool                     noline            = false;
    bool                     not_show_abspath  = false;
    std::vector<std::string> file_extensions   = {};

};

int main(int argc, char** argv)
{
    CLI::App app{ "text-search"};
    //app.footer("\n Creator: Somebody else.");

    //-------------------------------------------------------------//
    //               Subcommand FILE                               //
    //-------------------------------------------------------------//

    auto cmd_file = app.add_subcommand("file",
                                       "Search a single or multiple file for some pattern.");


    text_search_options opt_file;

    cmd_file->add_option("<PATTERN>", opt_file.pattern, "Text pattern")->required();

    // Sets directory that will be listed
    cmd_file->add_option("<FILE>", opt_file.filepaths
                   , "File to be searched")->required();

    // If the flag is set (true), this application uses the regex
    // for searching in the target file instead of an input text.
    cmd_file->add_flag("--regex", opt_file.use_regex, "Use regex");

    // If this flag is set, this cmd_file-> does not show the line number
    // ,instead only print the file names where the pattern was found.
    cmd_file->add_flag("--noline", opt_file.noline, "Does not show lines");


    //------------------------------------------------------------------//
    //               Subcommand DIRECTORY                               //
    //------------------------------------------------------------------//
    // Search all files from a directory recusively matching
    // a given text pattern and a file pattern such as file textension.
    //

    directory_search_options dir_opt;

    auto cmd_dir = app.add_subcommand("dir",
                                      "Search files from a directory mathcing a file name and text patterns");

    cmd_dir->add_option("<PATTERN>", dir_opt.pattern, "Text patterb")->required();

    // Sets directory that will be listed
    cmd_dir->add_option("<DIRECTORY>", dir_opt.directory
                         , "Direcotry to be searched")->required();
    cmd_dir->add_flag("--regex", dir_opt.use_regex, "Use regex");
    cmd_dir->add_flag("--noline", dir_opt.noline, "Does not show lines");
    cmd_dir->add_flag("-r,--recursive", dir_opt.recursive, "Search all subdirectories too");
    cmd_dir->add_flag("--noabs", dir_opt.not_show_abspath, "Do not show absolute path");
    cmd_dir->add_option("-e,--extension", dir_opt.file_extensions, "File extensions to be searched");


    // ----- Parse Arguments ---------//
    try {
        app.require_subcommand();
        app.validate_positionals();        
        CLI11_PARSE(app, argc, argv);
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
        return EXIT_FAILURE;
    }        

    //------ Program Actions ---------//

    // std::cout << "\n Seach results for pattern: '" << opt_file.pattern << "'";

    // process subcommand: text-search file
    if(*cmd_file)
    {
        for (auto const& fname : opt_file.filepaths)
        {
            try
            {
                if(!opt_file.use_regex)
                    fileutils::search_file_for_text(opt_file.pattern, fname, opt_file.noline);
                else
                    fileutils::search_file_for_regex(opt_file.pattern, fname, opt_file.noline);
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
    }

    if(*cmd_dir)
    {
        std::cout << "   Pattern = " << dir_opt.pattern << std::endl;
        std::cout << " Directory = " << dir_opt.directory << std::endl;

        fileutils::search_directory(  dir_opt.pattern
                                    , dir_opt.directory
                                    , dir_opt.recursive
                                    , dir_opt.noline
                                    , !dir_opt.not_show_abspath
                                    , dir_opt.file_extensions
                                    );

        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}
