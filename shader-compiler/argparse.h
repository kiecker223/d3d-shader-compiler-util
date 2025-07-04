#pragma once
//
// @author : Morris Franken
//  https://github.com/morrisfranken/argparse
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
#include <cctype>              // for isdigit, tolower
#include <sstream>
#include <cstdlib>             // for size_t, exit
#include <algorithm>           // for max, transform, copy, min
#include <iomanip>             // for operator<<, setw
#include <iostream>            // for operator<<, basic_ostream, endl, ostream
#include <iterator>            // for ostream_iterator
#include <map>                 // for operator!=, map, _Rb_tree_iterator
#include <memory>              // for allocator, shared_ptr, __shared_ptr_ac...
#include <optional>            // for optional, nullopt
#include <stdexcept>           // for runtime_error, invalid_argument
#include <filesystem>          // for getting program_name from path
#include <string>              // for string, operator+, basic_string, char_...
#include <type_traits>         // for declval, false_type, true_type, is_enum
#include <utility>             // for move, pair
#include <vector>              // for vector
#include <codecvt>             // for std::wstring_convert
#include <locale>              // for std::wstring_convert

#if __has_include(<magic_enum.hpp>)
#include <magic_enum.hpp>      // for enum_entries
#define HAS_MAGIC_ENUM
#endif

#define ARGPARSE_VERSION 4

namespace argparse {
    class Args;
    using std::cout, std::cerr, std::endl, std::setw, std::size_t;

    template<typename T> struct is_vector : public std::false_type {};
    template<typename T, typename A> struct is_vector<std::vector<T, A>> : public std::true_type {};

    template<typename T> struct is_optional : public std::false_type {};
    template<typename T> struct is_optional<std::optional<T>> : public std::true_type {};

    template<typename T> struct is_shared_ptr : public std::false_type {};
    template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : public std::true_type {};

    template <typename, typename = void> struct has_ostream_operator : std::false_type {};
    template <typename T> struct has_ostream_operator<T, decltype(void(std::declval<std::ostream&>() << std::declval<const T&>()))> : std::true_type {};

    inline std::string bold(const std::string& input_str) {
#ifdef _WIN32
        return input_str; // no bold for windows
#else
        return "\033[1m" + input_str + "\033[0m";
#endif
    }

    template<typename T> std::string toString(const T& v) {
        if constexpr (std::is_convertible<T, std::wstring>::value) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996) // C4996 is the typical MSVC deprecation warning
#endif
            return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(v);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
        }
        else if constexpr (has_ostream_operator<T>::value) {
            return static_cast<std::ostringstream&&>((std::ostringstream() << std::boolalpha << v)).str();       // https://github.com/stan-dev/math/issues/590#issuecomment-550122627
        }
        else {
            return "unknown";
        }
    }

    std::vector<std::string> inline split(const std::string& str) {
        std::vector<std::string> splits;
        std::stringstream ss(str);
        std::string key;
        while (std::getline(ss, key, ',')) {
            if (!key.empty() && key.back() == '\0')
                key.pop_back(); // last variables contain a '\0', which is unexpected when comparing to raw string, e.g. value == "test" will fail when the last character is '\0'. Therefore we can remove it
            splits.emplace_back(std::move(key));
        }
        return splits;
    }

    template <typename T> std::string to_lower(const T& str_) { // both std::string and std::basic_string_view<char> (for magic_enum) are using to_lower
        std::string str(str_.size(), '\0');
        std::transform(str_.begin(), str_.end(), str.begin(), ::tolower);
        return str;
    }

    template<typename T> inline T get(const std::string& v);
    template<> inline std::string get(const std::string& v) { return v; }
    template<> inline std::wstring get(const std::string& v) { return std::wstring(v.begin(), v.end()); }
    template<> inline char get(const std::string& v) { return v.empty() ? throw std::invalid_argument("empty string") : v.size() > 1 ? v.substr(0, 2) == "0x" ? (char)std::stoul(v, nullptr, 16) : (char)std::stoi(v) : v[0]; }
    template<> inline int get(const std::string& v) { return std::stoi(v); }
    template<> inline short get(const std::string& v) { return std::stoi(v); }
    template<> inline long get(const std::string& v) { return std::stol(v); }
    template<> inline long long get(const std::string& v) { return std::stol(v); }
    template<> inline bool get(const std::string& v) { return to_lower(v) == "true" || v == "1"; }
    template<> inline float get(const std::string& v) { return std::stof(v); }
    template<> inline double get(const std::string& v) { return std::stod(v); }
    template<> inline unsigned char get(const std::string& v) { return get<char>(v); }
    template<> inline unsigned int get(const std::string& v) { return std::stoul(v); }
    template<> inline unsigned short get(const std::string& v) { return std::stoul(v); }
    template<> inline unsigned long get(const std::string& v) { return std::stoul(v); }
    template<> inline unsigned long long get(const std::string& v) { return std::stoul(v); }

    template<typename T> inline T get(const std::string& v) { // remaining types
        if constexpr (is_vector<T>::value) {
            const std::vector<std::string> splitted = split(v);
            T res(splitted.size());
            if (!v.empty())
                std::transform(splitted.begin(), splitted.end(), res.begin(), get<typename T::value_type>);
            return res;
        }
        else if constexpr (std::is_pointer<T>::value) {
            return new typename std::remove_pointer<T>::type(get<typename std::remove_pointer<T>::type>(v));
        }
        else if constexpr (is_shared_ptr<T>::value) {
            return std::make_shared<typename T::element_type>(get<typename T::element_type>(v));
        }
        else if constexpr (is_optional<T>::value) {
            return get<typename T::value_type>(v);
        }
        else if constexpr (std::is_enum<T>::value) {  // case-insensitive enum conversion
#ifdef HAS_MAGIC_ENUM
            constexpr auto& enum_entries = magic_enum::enum_entries<T>();
            const std::string lower_str = to_lower(v);
            for (const auto& [value, name] : enum_entries) {
                if (to_lower(name) == lower_str)
                    return value;
            }
            std::string error = "enum is only accepting [";
            for (size_t i = 0; i < enum_entries.size(); i++)
                error += (i == 0 ? "" : ", ") + to_lower(enum_entries[i].second);
            error += "]";
            throw std::runtime_error(error);
#else
            throw std::runtime_error("Enum not supported, please install magic_enum (https://github.com/Neargye/magic_enum)");
#endif
        }
        else {
            return T(v);
        }
    }

    struct ConvertBase {
        virtual ~ConvertBase() = default;
        virtual void convert(const std::string& v) = 0;
        virtual void set_default(const std::unique_ptr<ConvertBase>& default_value, const std::string& default_string) = 0;
        [[nodiscard]] virtual size_t get_type_id() const = 0;
        [[nodiscard]] virtual std::string get_allowed_entries() const = 0;
    };

    template <typename T> struct ConvertType : public ConvertBase {
        T data;
        ~ConvertType() override = default;
        ConvertType() : ConvertBase() {};
        explicit ConvertType(const T& value) : ConvertBase(), data(value) {};

        void convert(const std::string& v) override {
            data = get<T>(v);
        }

        void set_default(const std::unique_ptr<ConvertBase>& default_value, const std::string& default_string) override {
            if (this->get_type_id() == default_value->get_type_id())    // When the types do not match exactly. resort to string conversion
                data = ((ConvertType<T>*)(default_value.get()))->data;
            else
                data = get<T>(default_string);
        }

        [[nodiscard]] size_t get_type_id() const override {
            return typeid(T).hash_code();
        }

        [[nodiscard]] std::string get_allowed_entries() const override {
            std::stringstream ss;

#ifdef HAS_MAGIC_ENUM
            if constexpr (std::is_enum<T>::value) {
                for (const auto& [value, name] : magic_enum::enum_entries<T>()) {
                    ss << to_lower(name) << ", ";
                }
            }
#endif

            return ss.str();
        }
    };

    struct Entry {
        enum ARG_TYPE { ARG, KWARG, FLAG } type;

        Entry(ARG_TYPE type, const std::string& key, std::string help, std::optional<std::string> implicit_value = std::nullopt) :
            type(type),
            keys_(split(key)),
            help(std::move(help)),
            implicit_value_(std::move(implicit_value)) {
        }

        // Allow both string inputs and direct-type inputs. Where a string-input will be converted like it would when using the commandline, and the direct approach is to simply use the value provided.
        template <typename T> Entry& set_default(const T& default_value) {
            this->default_str_ = toString(default_value);
            if constexpr (!(std::is_array<T>::value || std::is_same<typename std::remove_all_extents<T>::type, char>::value)) {
                data_default = std::make_unique<ConvertType<T>>(default_value);
            }
            return *this;
        }

        Entry& multi_argument() {
            _is_multi_argument = true;
            return *this;
        }

        // Magically convert the value string to the requested type
        template <typename T> operator T& () {
            // Automatically set the default to nullptr for pointer types and empty for optional types
            if constexpr (is_optional<T>::value || std::is_pointer<T>::value || is_shared_ptr<T>::value) {
                if (!default_str_.has_value()) {
                    default_str_ = "none";
                    if constexpr (is_optional<T>::value) {
                        data_default = std::make_unique<ConvertType<T>>(T{ std::nullopt });
                    }
                    else {
                        data_default = std::make_unique<ConvertType<T>>((T) nullptr);
                    }
                }
            }

            datap = std::make_unique<ConvertType<T>>();
            return ((ConvertType<T>*)(datap.get()))->data;
        }

        // Force an ambiguous error when not using a reference.
        template <typename T> operator T() {} // When you get here  because you received an error, make sure all parameters of argparse are references (e.g. with `&`)

    private:
        std::vector<std::string> keys_;
        std::string help;
        std::optional<std::string> value_;
        std::optional<std::string> implicit_value_;
        std::optional<std::string> default_str_;
        std::string error;
        std::unique_ptr<ConvertBase> datap;
        std::unique_ptr<ConvertBase> data_default;
        bool _is_multi_argument = false;
        bool is_set_by_user = true;

        [[nodiscard]] std::string _get_keys() const {
            std::stringstream ss;
            for (size_t i = 0; i < keys_.size(); i++)
                ss << (i ? "," : "") << (type == ARG ? "" : (keys_[i].size() > 1 ? "--" : "-")) + keys_[i];
            return ss.str();
        }

        void _convert(const std::string& value) {
            try {
                this->value_ = value;
                datap->convert(value);
            }
            catch (const std::invalid_argument& e) {
                error = "Invalid argument, could not convert \"" + value + "\" for " + _get_keys() + " (" + help + ")";
            }
            catch (const std::runtime_error& e) {
                error = "Invalid argument \"" + value + "\" for " + _get_keys() + " (" + help + "). Error: " + e.what();
            }
        }

        void _apply_default() {
            is_set_by_user = false;
            if (data_default != nullptr) {
                value_ = *default_str_; // for printing
                datap->set_default(data_default, *default_str_);
            }
            else if (default_str_.has_value()) {   // in cases where a string is provided to the `set_default` function
                _convert(default_str_.value());
            }
            else {
                error = "Argument missing: " + _get_keys() + " (" + help + ")";
            }
        }

        [[nodiscard]] std::string info() const {
            const std::string allowed_entries = datap->get_allowed_entries();
            const std::string default_value = default_str_.has_value() ? "default: " + *default_str_ : "required";
            const std::string implicit_value = implicit_value_.has_value() ? "implicit: \"" + *implicit_value_ + "\", " : "";
            const std::string allowed_value = !allowed_entries.empty() ? "allowed: <" + allowed_entries.substr(0, allowed_entries.size() - 2) + ">, " : "";
            return " [" + allowed_value + implicit_value + default_value + "]";
        }

        friend class Args;
    };

    struct SubcommandEntry {
        std::shared_ptr<Args> subargs;
        std::string subcommand_name;

        explicit SubcommandEntry(std::string subcommand_name) : subcommand_name(std::move(subcommand_name)) {}

        template<typename T> operator T& () {
            static_assert(std::is_base_of_v<Args, T>, "Subcommand type must be a derivative of argparse::Args");

            std::shared_ptr<T> res = std::make_shared<T>();
            res->program_name = subcommand_name;
            subargs = res;
            return *(T*)(subargs.get());
        }

        // Force an ambiguous error when not using a reference.
        template <typename T> operator T() {} // When you get here  because you received an error, make sure all parameters of argparse are references (e.g. with `&`)
    };

    class Args {
    private:
        size_t _arg_idx = 0;
        std::vector<std::string> params;
        std::vector<std::shared_ptr<Entry>> all_entries;
        std::map<std::string, std::shared_ptr<Entry>> kwarg_entries;
        std::vector<std::shared_ptr<Entry>> arg_entries;
        std::map<std::string, std::shared_ptr<SubcommandEntry>> subcommand_entries;
        bool has_options() {
            return std::find_if(all_entries.begin(), all_entries.end(), [](auto e) { return e->type != Entry::ARG; }) != all_entries.end();
        };

    public:
        std::string program_name;
        bool is_valid = false;

        virtual ~Args() = default;

        /* Add a positional argument, the order in which it is defined equals the order in which they are being read.
         * help : Description of the variable
         *
         * Returns a reference to the Entry, which will collapse into the requested type in `Entry::operator T()`
         */
        Entry& arg(const std::string& help) {
            return arg("arg_" + std::to_string(_arg_idx), help);
        }

        /* Add a *named* positional argument, the order in which it is defined equals the order in which they are being read.
         * key : The name of the argument, otherwise arg_<position> will be used
         * help : Description of the variable
         *
         * Returns a reference to the Entry, which will collapse into the requested type in `Entry::operator T()`
         */
        Entry& arg(const std::string& key, const std::string& help) {
            std::shared_ptr<Entry> entry = std::make_shared<Entry>(Entry::ARG, key, help);
            // Increasing _arg_idx, so that arg2 will be arg_2, irregardless of whether it is preceded by other positional arguments
            _arg_idx++;
            arg_entries.emplace_back(entry);
            all_entries.emplace_back(entry);
            return *entry;
        }

        /* Add a Key-Worded argument that takes a variable.
         * key : A comma-separated string, e.g. "k,key", which denotes the short (-k) and long(--key) keys_
         * help : Description of the variable
         * implicit_value : Implicit values are used when no value is provided.
         *
         * Returns a reference to the Entry, which will collapse into the requested type in `Entry::operator T()`
         */
        Entry& kwarg(const std::string& key, const std::string& help, const std::optional<std::string>& implicit_value = std::nullopt) {
            std::shared_ptr<Entry> entry = std::make_shared<Entry>(Entry::KWARG, key, help, implicit_value);
            all_entries.emplace_back(entry);
            for (const std::string& k : entry->keys_) {
                kwarg_entries[k] = entry;
            }
            return *entry;
        }

        /* Add a flag which will be false by default.
         * key : A comma-separated string, e.g. "k,key", which denotes the short (-k) and long(--key) keys_
         * help : Description of the variable
         *
         * Returns reference to Entry like kwarg
         */
        Entry& flag(const std::string& key, const std::string& help) {
            return kwarg(key, help, "true").set_default<bool>(false);
        }

        /* Add a a subcommand
         * command : name of the subcommand, e.g. 'commit', if you wish to implement a function like 'git commit'
         *
         * Returns a reference to the Entry, which will collapse into the requested type in `Entry::operator T()`
         * Expected type *Must* be an std::shared_ptr of derivative of the argparse::Args class
         */
        SubcommandEntry& subcommand(const std::string& command) {
            std::shared_ptr<SubcommandEntry> entry = std::make_shared<SubcommandEntry>(command);
            subcommand_entries[command] = entry;
            return *entry;
        }

        virtual void welcome() {}       // Allow to overwrite the `welcome` function to add a welcome-message to the help output
        virtual void help() {
            welcome();
            cout << "Usage: " << program_name << " ";
            for (const auto& entry : arg_entries)
                cout << entry->keys_[0] << ' ';
            if (has_options()) cout << " [options...]";
            if (!subcommand_entries.empty()) {
                cout << " [SUBCOMMAND: ";
                for (const auto& [subcommand, subentry] : subcommand_entries) {
                    cout << subcommand << ", ";
                }
                cout << "]";
            }
            cout << endl;
            for (const auto& entry : arg_entries) {
                cout << setw(17) << entry->keys_[0] << " : " << entry->help << entry->info() << endl;
            }

            if (has_options()) cout << endl << "Options:" << endl;
            for (const auto& entry : all_entries) {
                if (entry->type != Entry::ARG) {
                    cout << setw(17) << entry->_get_keys() << " : " << entry->help << entry->info() << endl;
                }
            }

            for (const auto& [subcommand, subentry] : subcommand_entries) {
                cout << endl << endl << bold("Subcommand: ") << bold(subcommand) << endl;
                subentry->subargs->help();
            }
        }

        void validate(const bool& raise_on_error) {
            for (const auto& entry : all_entries) {
                if (!entry->error.empty()) {
                    if (raise_on_error) {
                        throw std::runtime_error(entry->error);
                    }
                    else {
                        std::cerr << entry->error << std::endl;
                        exit(-1);
                    }
                }
            }
        }

        /* parse all parameters and also check for the help_flag which was set in this constructor
         * Upon error, it will print the error and exit immediately if validation_action is ValidationAction::EXIT_ON_ERROR
         */
        void parse(int argc, const char* const* argv, const bool& raise_on_error) {
            auto parse_subcommands = [&]() -> int {
                for (int i = 1; i < argc; i++) {
                    for (auto& [subcommand, subentry] : subcommand_entries) {
                        if (subcommand == argv[i]) {
                            subentry->subargs->parse(argc - i, argv + i, raise_on_error);
                            return i;
                        }
                    }
                }
                return argc;
                };
            argc = parse_subcommands();   // argc_ is the number of arguments that should be parsed after the subcommand has finished parsing

            program_name = std::filesystem::path(argv[0]).stem().string();
            params = std::vector<std::string>(argv + 1, argv + argc);

            std::string help_keys = kwarg_entries.count("h") ? "?,help" : "?,h,help";
            bool& _help = flag(help_keys, "print help");

            auto is_value = [&](const size_t& i) -> bool {
                return params.size() > i && (params[i][0] != '-' || (params[i].size() > 1 && std::isdigit(params[i][1])));  // check for number to not accidentally mark negative numbers as non-parameter
                };
            auto parse_param = [&](size_t& i, const std::string& key, const bool is_short, const std::optional<std::string>& equal_value = std::nullopt) {
                auto itt = kwarg_entries.find(key);
                if (itt != kwarg_entries.end()) {
                    auto& entry = itt->second;
                    if (equal_value.has_value()) {
                        entry->_convert(equal_value.value());
                    }
                    else if (entry->implicit_value_.has_value()) {
                        entry->_convert(*entry->implicit_value_);
                    }
                    else if (!is_short) { // short values are not allowed to look ahead for the next parameter
                        if (is_value(i + 1)) {
                            std::string value = params[++i];
                            if (entry->_is_multi_argument) {
                                while (is_value(i + 1))
                                    value += "," + params[++i];
                            }
                            entry->_convert(value);
                        }
                        else if (entry->_is_multi_argument) {
                            entry->_convert("");    // for multiargument parameters, return an empty vector when not passing any more values
                        }
                        else {
                            entry->error = "No value provided for: " + key;
                        }
                    }
                    else {
                        entry->error = "No value provided for: " + key;
                    }
                }
                else {
                    if (raise_on_error)
                        throw std::runtime_error("unrecognised commandline argument :  " + key);
                    else
                        cerr << "unrecognised commandline argument :  " << key << endl;
                }
                };
            auto add_param = [&](size_t& i, const size_t& start) {
                size_t eq_idx = params[i].find('=');  // check if value was passed using the '=' sign
                if (eq_idx != std::string::npos) { // key/value from = notation
                    std::string key = params[i].substr(start, eq_idx - start);
                    std::string value = params[i].substr(eq_idx + 1);
                    parse_param(i, key, false, value);
                }
                else {
                    std::string key = std::string(params[i].substr(start));
                    parse_param(i, key, false);
                }
                };

            std::vector<std::string> arguments_flat;
            for (size_t i = 0; i < params.size(); i++) {
                if (!is_value(i)) {
                    if (params[i].size() > 1 && params[i][1] == '-') {  // long --
                        add_param(i, 2);
                    }
                    else { // short -
                        const size_t j_end = std::min(params[i].size(), params[i].find('=')) - 1;
                        for (size_t j = 1; j < j_end; j++) { // add possible other flags
                            const std::string key = std::string(1, params[i][j]);
                            parse_param(i, key, true);
                        }
                        add_param(i, j_end);
                    }
                }
                else {
                    arguments_flat.emplace_back(params[i]);
                }
            }

            // Parse all the positional arguments, making sure multi_argument positional arguments are processed last to enable arguments afterwards
            size_t arg_i = 0;
            for (; arg_i < arg_entries.size() && !arg_entries[arg_i]->_is_multi_argument; arg_i++) { // iterate over positional arguments until a multi-argument is found
                if (arg_i < arguments_flat.size())
                    arg_entries[arg_i]->_convert(arguments_flat[arg_i]);
            }
            size_t arg_j = 1;
            for (size_t j_end = arg_entries.size() - arg_i; arg_j <= j_end; arg_j++) { // iterate from back to front, to ensure non-multi-arguments in the front and back are given preference
                size_t flat_idx = arguments_flat.size() - arg_j;
                if (flat_idx < arguments_flat.size() && flat_idx >= arg_i) {
                    if (arg_entries[arg_entries.size() - arg_j]->_is_multi_argument) {
                        std::stringstream s;  // Combine multiple arguments into 1 comma-separated string for parsing
                        copy(&arguments_flat[arg_i], &arguments_flat[flat_idx] + 1, std::ostream_iterator<std::string>(s, ","));
                        std::string value = s.str();
                        value.back() = '\0'; // remove trailing ','
                        arg_entries[arg_i]->_convert(value);
                    }
                    else {
                        arg_entries[arg_entries.size() - arg_j]->_convert(arguments_flat[flat_idx]);
                    }
                }
            }

            // try to apply default values for arguments which have not been set
            for (const auto& entry : all_entries) {
                if (!entry->value_.has_value()) {
                    entry->_apply_default();
                }
            }

            if (_help) {
                help();
                exit(0);
            }

            validate(raise_on_error);
            is_valid = true;
        }

        void print() const {
            for (const auto& entry : all_entries) {
                std::string snip = entry->type == Entry::ARG ? "(" + (entry->help.size() > 10 ? entry->help.substr(0, 7) + "..." : entry->help) + ")" : "";
                cout << setw(21) << entry->_get_keys() + snip << " : " << (entry->is_set_by_user ? bold(entry->value_.value_or("null")) : entry->value_.value_or("null")) << endl;
            }

            for (const auto& [subcommand, subentry] : subcommand_entries) {
                if (subentry->subargs->is_valid) {
                    cout << endl << "--- Subcommand: " << subcommand << endl;
                    subentry->subargs->print();
                }
            }
        }

        virtual int run() { return 0; }       // For automatically running subcommands
        int run_subcommands() {
            for (const auto& [subcommand, subentry] : subcommand_entries) {
                if (subentry->subargs->is_valid) {
                    return subentry->subargs->run();
                }
            }

            std::cerr << "No subcommand provided" << std::endl;
            help();
            return -1;
        }
    };

    template <typename T> T parse(int argc, const char* const* argv, const bool& raise_on_error = false) {
        T args = T();
        args.parse(argc, argv, raise_on_error);
        return args;
    }
}