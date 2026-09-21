#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    vector<string> msg {"congrats", "on", "running", "c++ code!"};

    for (const string& word: msg) {
        cout << word << " ";
    }

    cout << endl;
}   