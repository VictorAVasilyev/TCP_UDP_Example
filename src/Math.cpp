#include <string>
#include <sstream>
#include <future>
#include <algorithm>

#include <Math.h>

using namespace std;

int sum_string(const string& str, string& sum_str)
{
    // stringstream is not thread-safe, need lock here
    mutex m;
    lock_guard<mutex> lock{m};

    sum_str = "";

    stringstream ssRead;
    // Storing the whole string into string stream
    ssRead << str;
    // Running loop till the end of the stream
    string temp;
    double sum = 0;
    double found;
    bool hasNumbers = false;
    while (!ssRead.eof()) {
        // extracting word by word from stream
        ssRead >> temp;
        // Checking the given word is number or not 
        if (stringstream(temp) >> found) {
            sum += found;
            hasNumbers = true;
        }
        // To save from space at the end of string
        temp = "";
    }
    if (hasNumbers) {
      stringstream ssWrite;
      ssWrite << sum;
      ssWrite >> sum_str;
    }
    else {
      // if no numbers found then return initial string
      sum_str = str;
    }
    return 0;
}
