#include "utility_csv.hpp"
#include <fstream>
#include <sstream>

vector<array<float,1536>> read_features(string file_path) 
{ 
    // File pointer 
    fstream fin; 
  
    // Open an existing file 
    fin.open(file_path, ios::in); 
  
    int idx; 
    vector<array<float,1536>> features; 
    string line, word, temp; 
  
    while (fin >> temp) { 
        array<float,1536> feature;
        // read an entire row and 
        // store it in a string variable 'line' 
        getline(fin, line); 
  
        // used for breaking words 
        stringstream s(line); 
  
        // read every column data of a row and 
        // store it in a string variable, 'word' 
        idx=-2;
        while (getline(s, word, ',')) { 
            // add all the column data 
            // of a row to a vector 
            if( idx >= 0)
                feature[idx] = atof(word.c_str()); 
            idx++;
        }
        features.push_back(feature);
    } 
    return features;
} 