//DailyQA.h
#pragma once

/* Local Dependencies */
#include <unordered_set>
#include <unordered_map>
#include "spreadsheet.h"

// Uncomment to enable debugging output in run()
//#define DEBUG

//! DailyQA
/*! This class parses and formats the URL spinner daily report and outputs the correctly formatted data so that the click rates
can easily be copy/pasted into the daily QA sheet.
 */
class DailyQA
{
public:
    template<typename ... Args>
    static DailyQA& get_singleton(Args... args)
    {
        static DailyQA doc{args...};
        return doc;
    }

    /**
     * run()
     *
     * @brief Runs the main program and outputs into respective files
     */
    void run();

private:

    /**
     * DailyQA()
     *
     * @param names File name of sheet with projects/companies
     * @param data File name of URL spinner daily report
     * @param throughput File name of sheet with throughput entries
     * @param providers File name of sheet with provider mapping
     */
    DailyQA(std::string_view names,
            std::string_view data,
            std::string_view throughput,
            std::string_view providers,
            std::string_view thresholds,
            std::string_view evening_data,
            std::string_view afternoon_data);

    //Stages
    //TODO: Add Afternoon + Evening QA, + possible others
    void morning_QA();
    void throughput();

    //Key:
    //Set b = true for afternoon QA
    //Set b = false for evening QA
    template<bool b>
    void other_QA();

    //Adds entries to respective hash tables (increase modularity, reduce dependencies)
    void add_throughput_entries();
    void add_name_entries();
    void thresholds();

    //Sheets that are opened
    Spreadsheet names_sheet, morning_sheet, throughput_sheet; // For the names and data and throughput
    Spreadsheet providers_sheet;			      // Stores the mapping between provider name and number
    Spreadsheet threshold_sheet;			      // Stores the threshholds for DailyQA setup
    Spreadsheet evening_sheet;				      // Evening data sheet
    Spreadsheet afternoon_sheet;

    using LineEntries = std::unordered_map<std::string, Line>;
    using LineLookup = std::unordered_set<std::string>;

    template<typename T1, typename T2>
    using LineMap = std::unordered_map<std::string, std::pair<T1, T2>>;

    //Parsed data stored in hash tables for easy lookup
    LineEntries morning_entries{};    // Stores morning data entries
    LineEntries afternoon_entries{};  // Stores afternoon data entries
    LineEntries evening_entries{};    // Stores evening data entries
    LineEntries name_entries{};       // Stores name entries (for throughput sheet)
    LineLookup throughput_entries{};       // Stores name entries (for throughput sheet)
    LineMap<std::string, int> provider_entries{}; 	// Stores provider mappings
    LineMap<double, double> threshold_entries{};     // Stores threshold entries (for throughput sheet)

    //Output files (no need for it to be a Spreadsheet)
    std::ofstream outfile{"output/output.csv"}; //Output file
    std::ofstream t_outfile{"output/t_outfile.csv"}; //Throughput output file
    std::ofstream log{"output/log.csv"}; //Log throughput stats
};