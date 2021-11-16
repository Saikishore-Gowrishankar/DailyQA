/****************************************
 * DailyQA.cpp
 *
 * @brief Uses the spreadsheet parser to format
 * the DailyQA sheet
 *
 ****************************************/

/* Standard dependencies */
#include <string_view>
#include <unordered_map>
#include <iostream>
#include <regex>
#include <utility>
#include <algorithm>

/* Local Dependencies */
#include "spreadsheet.h"

// Uncomment to enable debugging output in run()
// #define DEBUG

//! DailyQA
/*! This class parses and formats the URL spinner daily report and outputs the correctly formatted data so that the click rates
can easily be copy/pasted into the daily QA sheet.
 */
class DailyQA
{
public:
    /**
     * DailyQA()
     *
     * @param names File name of sheet with projects/companies
     * @param data File name of URL spinner daily report
     * @param throughput File name of sheet with throughput entries
     * @param providers File name of sheet with provider mapping
     */
    DailyQA(std::string_view names, std::string_view data, std::string_view throughput, std::string_view providers, std::string_view thresholds)
             : names_sheet{names.data()},
               data_sheet{data.data()},
               throughput_sheet{throughput.data(), 10}, //Each line of throughput sheet has 10 cells
               providers_sheet{providers.data()},
               threshold_sheet{thresholds.data()}
    {
        // Insert data into a hash map for easy lookup
        for(auto&& line : data_sheet) data_entries.insert({line[0]+line[1], line});
        for(auto&& line : providers_sheet) provider_entries.insert({line[0], {line[1], 0} });
    }

    /**
     * run()
     *
     * @brief Runs the main program and outputs into the outfile.
     */
    void run()
    {
        std::string cached_company_name;
        std::cout << "Formatting Morning QA sheet...\n\n";
        for(auto&& line : names_sheet)
        {
            auto company = line[0], project = line[1];

            // The projects don't all have a company name listed next to them
            // For example, if the company has multiple carriers it only lists the
            // name next to the first project. This caches that name so that all
            // projects under the same company have a company name listed when 
            // the data is entered into the output file.
            if(company != "") cached_company_name = company;
            else if(company == "" && project != "") company = line[0] = cached_company_name;
            else if(company == "" && project == "_") { outfile << '\n'; continue; }

            // The lookup uses the concatenation of the company and the project names
            // Then, insert into name_entries hash table for later usage in setting up throughput sheet.
            auto str = company+project;
            if(company != "_" && project != "_") name_entries.insert({str, line});

            static int ws_count = 0; //Whitespace count (merely for logging purposes)
            if(auto search = data_entries.find(str); search != std::end(data_entries) && search->first != "")
            {
                #define out(x,y) ( (x)[y] == ""? "n/a" : (x)[y]  )
                outfile << out(search->second,0) << ',' << "->" << out(search->second,1) << ','
                        << out(search->second,4) << ',' << out(search->second,6) << ','<< out(search->second,8) << ',' << out(search->second,10)
                        << ",,no,n/a,no,n/a\n";
                #undef out
            }
            else if(project == "_" || company == "_" || project == "" || company == "")
            {
                std::cout << "Skipping ws (" << ++ws_count << ")\r";
                outfile << '\n';
            }
            else
            {
                ws_count = 0;
                std::cout << "\u001b[31;1mCould not find:\u001b[37;1m    " << company << "\u001b[0m - " << project << "    \n";
                outfile << company << ',' << project << ',' << "ERR,ERR,ERR,ERR\n";
            }
        }

        std::cout << "\n\u001b[32;1mMorning QA Sheet successfully formatted.\u001b[0m\n Formatting Throughput sheet...\n";

        //Counters
        int total_ATT{}, total_verizon{}, total_sprint{}, total_tmobile{}, total_entries{};
        int only_ATT{}, only_verizon{}, only_sprint{}, only_tmobile{};
        int all{};

        std::vector<Line> D2S{};

        std::regex r{R"~(\(([0-9]+)\).+)~"}; //Regular expression to extract provider number
        int unknown_providers{};

        //Column titles
        t_outfile << ",Company,Project,Campaign,Drip Name,Drip ID, ATT, Sprint,T-Mobile,Verizon,Tested,Edited\n";

        //Delete unnecessary projects on throughput sheet
        for(auto&& line : throughput_sheet)
        {
            bool ATT{}, verizon{}, sprint{}, tmobile{};
            auto str = line[1] + line[2]; //Lookup based on concatenation, as before
            if(auto search = name_entries.find(str); search != std::end(name_entries))
            {
                ++total_entries;

                //Add to list of projects that will be in final throughput sheet
                D2S.push_back(line);

                #define provider_block()\
                {\
                    if(std::smatch m; std::regex_match(dat, m, r))\
                    {\
                        if(auto search = provider_entries.find(m[1]); search != std::end(provider_entries)) ++search->second.second;\
                        else { ++unknown_providers; provider_entries.insert({m[1],{"UNKNOWN",1}}); }\
                    }\
                }

                //Update total blocks and provider blocks
                if(auto dat = line[6]; dat != "" && dat != "_"){ ATT     = ++total_ATT;     provider_block(); }
                if(auto dat = line[7]; dat != "" && dat != "_"){ sprint  = ++total_sprint;  provider_block(); }
                if(auto dat = line[8]; dat != "" && dat != "_"){ tmobile = ++total_tmobile; provider_block(); }
                if(auto dat = line[9]; dat != "" && dat != "_"){ verizon = ++total_verizon; provider_block(); }

                #undef provider_block

                //Update exclusive blocks
                if(ATT && !sprint && !tmobile && !verizon) ++only_ATT;
                if(!ATT && sprint && !tmobile && !verizon) ++only_sprint;
                if(!ATT && !sprint && tmobile && !verizon) ++only_tmobile;
                if(!ATT && !sprint && !tmobile && verizon) ++only_verizon;

                //Update blocks on all carriers
                if(ATT && sprint && tmobile && verizon) ++all;
            }
        }

        //Lines in throughput sheet are lexicographically sorted. 
        //Stable sort is used to preserve relative order of elements, but is slightly less efficient
        std::       sort(std::begin(D2S), std::end(D2S), [](auto&& a, auto&& b){ return a[3] < b[3]; }); //First sort by campaign name
        std::stable_sort(std::begin(D2S), std::end(D2S), [](auto&& a, auto&& b){ return a[2] < b[2]; }); //Then sort by project name
        std::stable_sort(std::begin(D2S), std::end(D2S), [](auto&& a, auto&& b){ return a[1] < b[1]; }); //Finally sort by company name

        //Output only D2S projects that are on the names sheet.
        for(auto&& line : D2S)
           for(int i = 0; i < 10; ++i) { auto val = line[i]; t_outfile << (val=="_"?" ":val) << (i < 9?',':'\n'); }

        //Output stats (to file and stdout)
        std::cout << "\u001b[32;1mThroughput sheet successfully formatted.\u001b[0m\n Outputting statistics to stdout...\n\n";

        auto tp_sz = throughput_sheet.size();
        std::ostream* out = &std::cout;
        bool outflag = false;
        if(!log) std::cerr << "\u001b[31;1mError: could not open \u001b[35;1moutput/log.csv\u001b[0m\n";
DUMP:
        (*out) << (!outflag?"\u001b[37;1m":"") << "Number of entries before formatting: " << (!outflag?"\u001b[0m":",") << tp_sz << "\n";
        (*out) << (!outflag?"\u001b[37;1m":"") << "Total number of entries deleted: " << (!outflag?"\u001b[0m":",") << tp_sz - total_entries << "\n";
        (*out) << (!outflag?"\u001b[37;1m":"") << "Final size of throughput sheet (entries): " << (!outflag?"\u001b[0m":",") << total_entries << "\n\n";

        (*out) << (!outflag?"\u001b[37;1m":"") << "Total ATT Blocks: " << (!outflag?"\u001b[0m":",") << total_ATT << '\n';
        (*out) << (!outflag?"\u001b[37;1m":"") << "Total Verizon Blocks: " << (!outflag?"\u001b[0m":",") << total_verizon << '\n';
        (*out) << (!outflag?"\u001b[37;1m":"") << "Total Sprint Blocks: " << (!outflag?"\u001b[0m":",") << total_sprint << '\n';
        (*out) << (!outflag?"\u001b[37;1m":"") << "Total T-Mobile Blocks: " << (!outflag?"\u001b[0m":",") << total_tmobile << "\n\n";

        (*out) << (!outflag?"\u001b[37;1m":"") << "Exclusive ATT Blocks: " << (!outflag?"\u001b[0m":",") << only_ATT << '\n';
        (*out) << (!outflag?"\u001b[37;1m":"") << "Exclusive Verizon Blocks: " << (!outflag?"\u001b[0m":",") << only_verizon << '\n';
        (*out) << (!outflag?"\u001b[37;1m":"") << "Exclusive Sprint Blocks: " << (!outflag?"\u001b[0m":",") << only_sprint << '\n';
        (*out) << (!outflag?"\u001b[37;1m":"") << "Exclusive T-Mobile Blocks: " << (!outflag?"\u001b[0m":",") << only_tmobile << "\n\n";

        (*out) << (!outflag?"\u001b[37;1m":"") << "All carrier Blocks: " << (!outflag?"\u001b[0m":",") << all << "\n\n";

        for(auto&& [code, provider] : provider_entries)
            (*out) << (!outflag?"\u001b[37;1m":"") << "Total " << provider.first << " (" << code << ") blocks:" << (!outflag?"\u001b[0m ":",") << provider.second << '\n';
        if(!outflag && log)
        {
            outflag = true;
            out = &log;
            std::cout << "\n\u001b[32;1mDone.\n\u001b[0m\nOutputting stats to \u001b[35;1moutput/log.csv\u001b[0m...";
            goto DUMP;
        }

        if(unknown_providers > 0) std::cerr << "\n\u001b[31;1m" << "There are " << unknown_providers << " unknown providers. Please update \u001b[35;1minput/providers.csv\u001b[0m" << '\n';
        std::cout.put('\n');
        std::cout << "\u001b[32;1mDone.\u001b[0m\n\n";

        //If debugging is necessary, add debug lines here, then #define DEBUG above
        #ifdef DEBUG
            for(auto&& [first, second] : name_entries) std::cout << second << '\n';
            std::cout << throughput_sheet << std::endl;
            for(auto&& line : throughput_sheet) std::cout << line.size() << ' ';
            std::cout.put('\n');
        #endif
    }

private:
    Spreadsheet names_sheet, data_sheet, throughput_sheet;    // For the names and data and throughput
    Spreadsheet providers_sheet;			      // Stores the mapping between provider name and number
    Spreadsheet threshold_sheet;			      // Stores the threshholds for DailyQA setup
    std::unordered_map<std::string, Line> data_entries{};     // Stores data entries
    std::unordered_map<std::string, Line> name_entries{};     // Stores name entries (for throughput sheet)
    std::unordered_map<std::string, std::pair<std::string, int>> provider_entries{}; // Stores provider mappings
    std::ofstream outfile{"output/output.csv"}; //Output file
    std::ofstream t_outfile{"output/t_outfile.csv"}; //Throughput output file
    std::ofstream log{"output/log.csv"}; //Log throughput stats
};

int main()
{
    std::cout << "\u001b[36;1m\n--------------------------------------------------------------------------\u001b[0m\n";
    std::cout << "|\u001b[33;1mDaily QA Sheet generator\u001b[0m\n";
    std::cout << "\u001b[36;1m--------------------------------------------------------------------------\u001b[0m\n";

    std::cout << "opening \u001b[35;1minput/names.csv\u001b[0m, \u001b[35;1minput/data.csv\u001b[0m, and \u001b[35;1minput/throughput.csv\u001b[0m\n";

    DailyQA doc{"input/names.csv", "input/data.csv","input/throughput.csv", "input/providers.csv", "input/thresholds.csv"};

    std::cout << "\u001b[32;1mSuccessfully opened aforementioned files.\u001b[0m\n\nRunning main program.\n";

    doc.run();

    std::cout << "\u001b[36;1m--------------------------------------------------------------------------\u001b[0m\n";
    std::cout << "\nFinished. Find results in \u001b[35;1moutput/output.csv\u001b[0m and throughput results in \u001b[35;1moutput/t_output.csv\u001b[0m. Terminating.\n\n";

    return 0;
}