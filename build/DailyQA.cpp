/****************************************
 * DailyQA.cpp
 *
 * @brief Uses the spreadsheet parser to format
 * the DailyQA sheet
 *
 ****************************************/

/* Standard dependencies */
#include <string_view>
#include <sstream>
#include <iostream>
#include <regex>
#include <utility>
#include <algorithm>
#include <cstdlib>

/* Platform-specific dependencies */
#include <stdlib.h>
#include <signal.h>

/* Local dependencies */
#include "DailyQA.h"

//Returns the floating-point value of click rate percentage (format: "x.xx%" -> x.xx)
static constexpr double find_threshold(auto const& rate)
{
    return std::stod(rate == "n/a"?"0.0":rate.substr(0,'%')); //(std::stod(_7v) + std::stod(_3v) + std::stod(_2v) + std::stod(_1v))/4;
}

DailyQA::DailyQA(std::string_view names, std::string_view data, std::string_view throughput, std::string_view providers, std::string_view thresholds,
                 std::string_view evening_data, std::string_view afternoon_data)
         : names_sheet{names.data()},
           morning_sheet{data.data()},
           throughput_sheet{throughput.data(), 10}, //Each line of throughput sheet has 10 cells
           providers_sheet{providers.data()},
           threshold_sheet{thresholds.data(), 4},
           evening_sheet{evening_data.data(), 7},
           afternoon_sheet{afternoon_data.data(), 7}

{
    // Insert data into a hash map for easy lookup
    for(auto&& line : providers_sheet) provider_entries.insert({line[0], {line[1], 0} });

}

void DailyQA::run()
{
    while(true)
    {
        int in{}, ret{};
        std::cout << "\u001b[37;1mPlease select from the following menu:\u001b[0m\n"
                  << "1. Morning QA Setup\n"
                  << "2. Afternoon QA Setup\n"
                  << "3. Evening QA Setup\n"
                  << "4. Other Options\n"
                  << "5. Quit\n\n"
                  << "Your selection (number): ";
        std::cin >> in;
        if(!std::cin)
        {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize> ::max(), '\n');
            std::cerr << "\u001b[31;1mERROR:\u001b[0m Unrecognized option\n";
            continue;
        }

        switch(in)
        {
            case 1:
                std::cout << "Formatting Morning QA sheet...\n\n";
                for(auto&& line : morning_sheet) morning_entries.insert({line[0]+line[1], line});
                morning_QA();
                std::cout << "\n\u001b[32;1mMorning QA Sheet successfully formatted.\u001b[0m\n ";

                std::cout << "Formatting Throughput sheet...\n";
                throughput();
                std::cout << "\u001b[32;1mDone.\u001b[0m\n\n";
                return;
            case 2:
                for(auto&& line : afternoon_sheet) afternoon_entries.insert({line[0]+line[1], line});
                std::cout << " Afternoon QA report:  \n";
                other_QA<1>();
                std::cout << "\n\u001b[32;1mAfternoon QA Sheet successfully formatted.\u001b[0m\n ";

                return;
            case 3:
                for(auto&& line : evening_sheet) evening_entries.insert({line[0]+line[1], line});
                std::cout << " Evening QA report:  \n";
                other_QA<0>();
                std::cout << "\n\u001b[32;1mEvening QA Sheet successfully formatted.\u001b[0m\n ";
                return;
            case 4:
                ret = system("firefox https://saikishore-gowrishankar.github.io/DailyQA");
                if(WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT || WTERMSIG(ret) == SIGQUIT))
                    return;
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                [[fallthrough]];
            case 5: std::exit(0); break;
            default:
                //If debugging is necessary, add debug lines here, then #define DEBUG above
                #ifdef DEBUG
                    for(auto&& [first, second] : threshold_entries) std::cout << first << "---->" << second << '\n';
                    std::cout << threshold_sheet << std::endl;
                    for(auto&& line : threshold_sheet) std::cout << line.size() << ' ';
                    std::cout.put('\n');
                    return;
                #else
                    std::cerr << "\u001b[31;1mERROR:\u001b[37;1m invalid selection\n";
                    continue;
                #endif
        }
    }
}
/*
    TODO: Make thresholds() have high exception safety around calls to std::stod()
    TODO: Reduce/eliminate morning QA/threshold dependencies
*/
void DailyQA::thresholds()
{
    std::string cached_company_name;
    for(auto&& line : threshold_sheet)
    {
        auto company = line[0], project = line[1],
                       _1threshold = line[3], _2threshold = line[2];
        if(_1threshold.find("n") != std::string::npos) _1threshold = "0.00%";
        if(_2threshold.find("n") != std::string::npos) _2threshold = "0.00%";

        //Caches company name for projects w/ multiple carriers, see morning_QA()
        if(company != "") cached_company_name = company;
        else if(company == "" && project != "") company = line[0] = cached_company_name;
        else if(company == "" && project == "_") { continue; }

        //Remove '%' character
        _1threshold.pop_back(); _2threshold.pop_back();

        //auto _1 = std::stod(_1threshold);
        //auto _2 = std::stod(_2threshold);

        //Lookup based on concatenation of company and project
        auto str = company+project;
        try{
        if(company != "_" && project != "_")
            threshold_entries.insert({str, {std::stod(_1threshold), std::stod(_2threshold)}});
        }catch(...){std::cout << "_threshikds: " << _1threshold << "  " << _2threshold << "\n\n\n\n"; continue;}
    }
}
void DailyQA::add_throughput_entries()
{
    for(auto&& line : throughput_sheet)
    {
        auto str = line[1] + line[2]; //Lookup based on concatenation, as before
        if(auto search = name_entries.find(str); search != std::end(name_entries))
            throughput_entries.insert(str);
    }
}

void DailyQA::add_name_entries()
{
    std::string cached_company_name;
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
        else if(company == "" && project == "_") { continue; }

        // The lookup uses the concatenation of the company and the project names
        // Then, insert into name_entries hash table for later usage in setting up throughput sheet.
        auto str = company+project;
        if(company != "_" && project != "_") name_entries.insert({str, line});
    }
}

template<bool b>
void DailyQA::other_QA()
{
    LineEntries* entries = nullptr;

    if constexpr (b) entries = &afternoon_entries;
    else 	     entries = &evening_entries;

    add_name_entries();
    for(auto&& line : names_sheet)
    {
        auto company = line[0], project = line[1];
        auto str = company+project;
        if(auto search = entries->find(str); search != std::end(*entries))
        {
            auto& match = search->second;

            #define out(x,y) ( (x)[y] == ""? "n/a" : (x)[y]  )
            auto const& cr       = out(match, 2);
            auto const& cr_1day  = out(match, 4);
            auto const& cr_1week = out(match, 6);

            outfile << out(match,0) << ',' << out(match, 1) << ','
                    << cr << ',' << cr_1day << ',' << cr_1week << '\n';
            #undef out
        }
        else if(project == "_" || company == "_" || project == "" || company == "")
        {
            outfile.put('\n');
        }
        else
        {
            outfile << company << ',' << project << ",\n";
        }
    }
}
void DailyQA::morning_QA()
{
    thresholds();
    add_name_entries();
    add_throughput_entries();
    std::string cached_company_name;
    for(auto&& line : names_sheet)
    {
        auto company = line[0], project = line[1];
        auto str = company+project;

        static int ws_count = 0; //Whitespace count (merely for logging purposes)
        if(auto search = morning_entries.find(str); search != std::end(morning_entries) /*&& search->first != ""*/)
        {
            #define out(x,y) ( (x)[y] == ""? "n/a" : (x)[y]  )
            auto const& _7 = out(search->second,4);
            auto const& _3 = out(search->second,6);
            auto const& _2 = out(search->second,8);
            auto const& _1 = out(search->second,10);

            auto t_1 = find_threshold(_1);
            auto t_2 = find_threshold(_2);

            outfile << out(search->second,0) << ',' <<  out(search->second,1) << ','
                    << _7 << ',' << _3 << ','<< _2 << ',' << _1;
            if(auto s = threshold_entries.find(str); s != std::end(threshold_entries))
            {
                auto const& match = s->second;
                bool cond = (match.first > t_1 && match.second > t_2);
                if(throughput_entries.find(str) != std::end(throughput_entries))
                    outfile << ",***,";
                else
                    outfile << ",n/a,";
                outfile << (cond?"***,,***,\n":"no,n/a,no,n/a\n");
            }
            else
            {
                std::cerr << "\u001b[31;1mERROR:\u001b[37;1m    " << company << "\u001b[0m - " << project 
                    << " does not have a corresponding threshold entry\n";
                outfile << ",,ERR,ERR,ERR,ERR\n";
            }
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
            std::cerr << "\u001b[31;1mCould not find:\u001b[37;1m    " << company << "\u001b[0m - " << project << "    \n";
            outfile << company << ',' << project << ',' << "ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR,ERR\n";
        }
    }
}
void DailyQA::throughput()
{
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
        bool ATT{}, verizon{}, sprint{}, tmobile{}, val{};
        auto str = line[1] + line[2]; //Lookup based on concatenation, as before
        if(auto search = name_entries.find(str); search != std::end(name_entries))
        {
            ++total_entries;

            #define provider_block()\
            {\
                if(std::smatch m; std::regex_match(dat, m, r))\
                {\
                    if(auto search = provider_entries.find(m[1]); search != std::end(provider_entries)) ++search->second.second;\
                    else { ++unknown_providers; provider_entries.insert({m[1],{"UNKNOWN",1}}); }\
                }\
            }

            if(auto dat = line[6]; dat != "" && dat != "_"){ ATT     = ++total_ATT;  provider_block();    if(dat.find("N/A") == std::string::npos) val = true;}
            if(auto dat = line[7]; dat != "" && dat != "_"){ sprint  = ++total_sprint;  provider_block(); if(dat.find("N/A") == std::string::npos) val = true;}
            if(auto dat = line[8]; dat != "" && dat != "_"){ tmobile = ++total_tmobile; provider_block(); if(dat.find("N/A") == std::string::npos) val = true;}
            if(auto dat = line[9]; dat != "" && dat != "_"){ verizon = ++total_verizon; provider_block(); if(dat.find("N/A") == std::string::npos) val = true;}

            #undef provider_block

            //Update exclusive blocks
            if(ATT && !sprint && !tmobile && !verizon) ++only_ATT;
            if(!ATT && sprint && !tmobile && !verizon) ++only_sprint;
            if(!ATT && !sprint && tmobile && !verizon) ++only_tmobile;
            if(!ATT && !sprint && !tmobile && verizon) ++only_verizon;

            //Update blocks on all carriers
            if(ATT && sprint && tmobile && verizon) ++all;

            //Add to list of projects that will be in final throughput sheet
            if(val) D2S.push_back(line);
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
}

int main()
{
    std::cout << "\u001b[36;1m\n--------------------------------------------------------------------------\u001b[0m\n";
    std::cout << "|\u001b[33;1mDaily QA Sheet generator\u001b[0m\n";
    std::cout << "\u001b[36;1m--------------------------------------------------------------------------\u001b[0m\n";

    std::cout << "opening \u001b[35;1minput/names.csv\u001b[0m, \u001b[35;1minput/data.csv\u001b[0m, and \u001b[35;1minput/throughput.csv\u001b[0m\n";

    DailyQA& doc = DailyQA::get_singleton("input/names.csv", "input/data.csv","input/throughput.csv", "input/providers.csv", "input/thresholds.csv",
                                          "input/evening_data.csv", "input/afternoon_data.csv");

    std::cout << "\u001b[32;1mSuccessfully opened aforementioned files.\u001b[0m\n\nRunning main program.\n";

    doc.run();

    std::cout << "\u001b[36;1m--------------------------------------------------------------------------\u001b[0m\n";
    std::cout << "\nFinished. Find results in \u001b[35;1moutput/output.csv\u001b[0m and throughput results in \u001b[35;1moutput/t_output.csv\u001b[0m. Terminating.\n\n";

    return 0;
}