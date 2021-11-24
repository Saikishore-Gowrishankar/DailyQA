/****************************************
 * csv.h
 *
 * Performs preprocessing on any csv files
 *
 ****************************************/
#pragma once

/* Standard dependencies */
#include <fstream>
#include <string_view>
#include <string>
#include <iterator>
#include <iostream>
#include <regex>
#include <algorithm>

/*************************************************************************
 * csv
 *************************************************************************/
 /*!
 * A class that extends std::fstream to preprocess CSV files
 * as dictated by the RFC 4180 standard. This class performs preprocessing
 * on all CSV files by removing all double-quotes around fields and replacing
 * embedded comma characters with an underscore for easier processing.
 */
class csv : public std::fstream
{
public:
    /**
     * @param std::string_view Opens file s
     */
    csv(std::string_view s) : std::fstream{s.data()}
    {
        //Regular expressions
        std::regex symbol_with_comma{ R"~("([^"]+)(,)([^"]+)")~" };
        std::regex quotes_symbol{ R"~(".")~" };

        //Read line into buffer
        std::getline(*this, buf_, (char)EOF);

        //Replace commas in symbols with underscore
        buf_ = std::regex_replace(buf_, symbol_with_comma, "$1~$3");

        //Erase all double quote characters (remove-erase idiom)
        buf_.erase(std::remove(std::begin(buf_), std::end(buf_), '"'), std::end(buf_));

        //Write new contents.
        close(); open(s.data(), std::fstream::out | std::fstream::in | std::fstream::trunc);
        *this << buf_;
        seekp(0); seekg(0);
    }
private:
    std::string buf_{};
};