#include <iostream>
#include <string>
#include <filesystem>
#include <bitset>
#include <cstring> // strtok

#include <CLI/CLI.hpp>

using ByteArray = std::vector<char>;

/** Print byte array as string and non-printable chars as hexadecimal */
void print_char_bytes(std::ostream& os, const ByteArray& str)
{
    for(const auto& ch: str)
    {
        if(std::isprint(ch))
            os << ch << "";
        else
            os << "\\0x" << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(ch) << " "
               << std::dec;
    }
}

bool operator==(ByteArray const& rhs, ByteArray const& lhs)
{
    if(rhs.size() != lhs.size()) {
        return false;
    }
    return std::equal(rhs.begin(), rhs.end(), lhs.begin());
}

/** @brief Read some value T at some offset of a binary input stream.
  *
  * @tparam T      - type which will be read
  * @param  is     - Input stream opened in binary mode
  * @param  offset - Offset or position where data will be read.
  */
template<typename T>
auto read_at(std::istream& is, long offset = -1) -> T
{
    T value{};
    if(offset > 0) { is.seekg(offset); }
    is.read(reinterpret_cast<char*>(&value), sizeof (T));
    return value;
}

auto open_binary_file(std::string const& file) -> std::ifstream
{
    using namespace std::string_literals;
    auto ifs = std::ifstream (file, std::ios::in | std::ios::binary);
    if(!ifs) {
        throw std::runtime_error("Error: Unable to open file: "s + file);
    }
    return ifs;
}


/// Dump all printable characters for a binary file
void command_strings(std::string const& file)
{
    enum class strings_state {
         initial
        , printable
        , skip
        , end
    };

    auto ifs = open_binary_file(file);

    auto print_byte = [](std::uint8_t x)
    {
        std::cout << static_cast<char>(x);
       // std::cout << "(" << static_cast<int>(x) << ") ";
    };

    auto is_printable = [](std::uint8_t ch) -> bool
    {
        if(ch == '\r' || ch == '\n')
            return false;
        return std::isprint(ch);
    };

    std::uint8_t byte = 0x00;
    auto state = strings_state::initial;
    std::stringstream buffer;

    while(ifs)
    {
        if(state == strings_state::initial)
        {
            buffer.str("");

            ifs >> byte;
            if(is_printable(byte)){
                // print_byte(byte);
                buffer << byte;
                state = strings_state::printable;
                continue;
            }
            state = strings_state::skip;
            continue;
        }

        if(state == strings_state::skip)
        {
            ifs >> byte;
            if(is_printable(byte)){
                // print_byte(byte);
                buffer << byte;
                state = strings_state::printable;
                continue;
            }
            state = strings_state::skip;
            continue;
        }

        if(state == strings_state::printable)
        {
            ifs >> byte;
            if(is_printable(byte)){
                // print_byte(byte);
                buffer << byte;
                state = strings_state::printable;
                continue;
            }

            if(buffer.str().size() >= 3)
                std::cout << buffer.str() << std::endl;

            buffer.str(""); // Reset buffer
            state = strings_state::skip;
            continue;
        }

    }
}


enum class data_type
{
      t_byte
    , t_char
    , t_i8, t_i16, t_i32, t_i64
    , t_u8, t_u16, t_u32, t_u64
    , t_flt32
    , t_flt64,
};

template<typename T>
void dump_binary_t(std::string const& file, size_t size,  long offset)
{
    auto ifs = open_binary_file(file);
    if(offset > 0){ ifs.seekg(offset); }

    std::vector<T> arr(size);
    ifs.read(reinterpret_cast<char*>(arr.data()), arr.size() * sizeof(T));

    std::cout << "\n";

    if constexpr (std::is_same<T, char>::value)
    {
        print_char_bytes(std::cout, arr);
    } else if constexpr (std::is_same<T, std::uint8_t>::value)
    {
        std::cout << std::hex << std::uppercase;
        for(auto const& x: arr)
            std::cout << std::setfill('0') << std::setw(2)
                      << static_cast<int>(x) << " ";
        std::cout << "\n";
    } else {

        for(auto const& x: arr)
            std::cout << x << " ";
        std::cout << "\n";
    }
}

void dump_binary(std::string const& file, data_type type, size_t size,  long offset)
{
    if(type == data_type::t_char)
        dump_binary_t<char>(file, size, offset);

    if(type == data_type::t_byte)
        dump_binary_t<uint8_t>(file, size, offset);

    if(type == data_type::t_i16)
        dump_binary_t<uint16_t>(file, size, offset);
}

int main(int argc, char** argv)
{
    CLI::App app{ "hextool - Tool for analysis of binary files"};
    //app.footer("\n Creator: Somebody else.");

     // Dump printable characters of a binary file
    auto cmd_strings = app.add_subcommand("dump-strings",
                                          "Dump all printable strings of a binary file");

    std::string file;
    cmd_strings->add_option("<FILE>", file)->required();

    auto cmd_dump = app.add_subcommand("dump-bytes"
                                       , "Read binary file at some offset");

    cmd_dump->add_option("<FILE>", file)->required();

    long offset = -1;
    cmd_dump->add_option("--offset", offset);

    size_t size = 1;
    cmd_dump->add_option("--size", size);

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

    if(*cmd_strings)
    {
        std::cout << " Selected file: " << file << "\n";
        command_strings(file);

        return EXIT_SUCCESS;
    }

    if(*cmd_dump)
    {
        dump_binary(file, data_type::t_byte, size, offset);
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;
}
