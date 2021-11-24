//main.cpp
#include <iostream>
#include "DailyQA.h"

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
