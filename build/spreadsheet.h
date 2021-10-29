/******************************************************************************
 *  Spreadsheet Parser (spreadsheet.cpp)
 *
 *  Author: Saikishore G.
 *  Date: 05/26/2021
 *
 *  All trademarks belong to their respective owners. Lawyers love tautologies.
 *
 *  @brief This is a lightweight spreadsheet (CSV) parser for use with the DailyQA
 *  program.
 ******************************************************************************/

#pragma once

/* Standard dependencies */
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cmath>
#include <iterator>

/* Local dependencies */
#include "csv.h"

using cell = std::string;
class Spreadsheet;

/******************************************************************************
 *  Line
 ******************************************************************************/
/*! Represents the interface for a line in the spreadsheet*/

class Line
{
public:

    /* Implementation details*/
    using       iterator = std::vector<cell>::      iterator;
    using const_iterator = std::vector<cell>::const_iterator;

    /**
     * @brief Iterator methods for range-based for
     */
    iterator       begin()  { return std::begin(cells);  }
    iterator       end()    { return std::end(cells);    }
    const_iterator cbegin() { return std::cbegin(cells); }
    const_iterator cend()   { return std::cend(cells);   }

    /**
     * Line()
     *
     * @brief Extracts cells from raw line
     * @param raw Raw line data
     * @param line_length Number of cells each line should be. If size is less than line_length,
     * empty cells are created to extend the length.
     */
    Line(cell const& raw, int line_length)
    {
        using namespace std::string_literals;

        std::istringstream ss{raw};
        cell c;
        auto str = ss.str();
        if(str[0] == '"')
        {
            // @todo Fix output of names w/ comma
            ss.get();
            std::getline(ss, c, '"');

            cells.push_back("\""s+c+"\""s);
            ss.get();
        }
        while(std::getline(ss, c, ',')) cells.push_back(c);

        auto sz = std::size(cells);
        if(line_length > 0 && sz < (unsigned)line_length)
            for(int i = sz; i < line_length; ++i) cells.push_back("_");
    }

    /**
     * to_raw()
     *
     * @brief Extracts cells from raw line
     * @return std::string holding raw data
     */
    cell to_raw() const
    {
        std::ostringstream ss;
        std::copy(std::begin(cells), std::end(cells), std::ostream_iterator<cell>(ss, ","));
        auto val = ss.str();
        val.pop_back();
        return val;
    }

    /**
     * ::operator<<()
     *
     * @brief Output formatted line
     */
    friend std::ostream& operator<<(std::ostream& os, Line const& l)
    {
        std::copy(std::begin(l.cells), std::end(l.cells), std::ostream_iterator<cell>(os, " "));
        return os;
    }

    std::size_t size() const { return std::size(cells); }

    /**
     * operator[]()
     *
     * @brief Access individual cells
     */
    cell&       operator[] (std::ptrdiff_t off)         { return cells[off]; }
    cell const& operator[] (std::ptrdiff_t off) const   { return cells[off]; }


private:
    /* Parsed cells */
    std::vector<cell> cells{};
};

/******************************************************************************
 *  Spreadsheet
 ******************************************************************************/
 /*! Defines the interface for a spreadsheet parser */
class Spreadsheet
{
public:

    /* Implementation details*/
    friend class Line;
    using iterator       = std::vector<Line>::      iterator;
    using const_iterator = std::vector<Line>::const_iterator;

    //Iterator methods for range-based for
    iterator       begin()  { return std::begin(lines);  }
    iterator       end()    { return std::end(lines);    }
    const_iterator cbegin() { return std::cbegin(lines); }
    const_iterator cend()   { return std::cend(lines);   }

    /**
     * Spreadsheet()
     *
     * @param in The input file name
     * @param line_length The length of each line in cells
     **/
    Spreadsheet(std::string_view in, int line_length = -1) : infile{in}
    {
        if(!infile) { std::cerr << "could not open file " << in.data() << ". Abort.\n"; std::exit(1); }

        std::string raw_line{};
        infile.seekg(0); infile.seekp(0);
        while(std::getline(infile, raw_line))
        {
            lines.emplace_back(raw_line, line_length);
        }
    }

    /**
     * ::operator<< ()
     *
     * @brief Outputs formatted spreadsheet
     *
     **/
    friend std::ostream& operator<<(std::ostream& os, Spreadsheet const& s)
    {
        for(auto&& l : s.lines) os << l << '\n';
        return os;
    }

    auto erase(iterator pos) { return lines.erase(pos); }
    auto erase(const_iterator pos) { return lines.erase(pos); }
    auto erase(iterator first, iterator last ) { return lines.erase(first, last); }
    auto erase(const_iterator first, const_iterator last ) { return lines.erase(first, last); }

    template<typename UnaryPredicate>
    void remove_erase_if(UnaryPredicate p)
    {
        auto new_end = std::remove_if(std::begin(lines), std::end(lines), p);
        lines.erase(new_end, std::end(lines));
    }

    std::size_t size() const { return lines.size(); }

private:
    csv infile;			//Input file
    std::vector<Line> lines{};	//Parsed lines
};
