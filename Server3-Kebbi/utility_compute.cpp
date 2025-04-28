#include "utility_compute.hpp"
#include <math.h>
#include <algorithm>


float ComputeL2Norm(array<float,1536> array1, array<float,1536> array2)
{
    float temp;
    unsigned int size = array1.size();
    float sum = 0;
    for(unsigned int i=0 ; i<size ; i++)
    {
        temp = (array1[i] - array2[i]);
        sum += temp*temp;
    }
    return sqrt(sum);
}

//if descend, the larger the distance, the smaller the index
//template <class X> vector<int> SortIndex(vector<X> input_vector, bool descend)
vector<int> SortIndex(vector<float> input_vector, bool descend)
{
    vector<int> index_vector(input_vector.size(), 0);
    for (unsigned int i = 0 ; i != index_vector.size() ; i++) {
        index_vector[i] = i;
    }

    if( descend)
    {
        sort(index_vector.begin(), index_vector.end(),
            [&](const int& a, const int& b) {
                return (input_vector[a] > input_vector[b]);
            }
        );
    }
    else
    {
        sort(index_vector.begin(), index_vector.end(),
            [&](const int& a, const int& b) {
                return (input_vector[a] < input_vector[b]);
            }
        );
    }
    
    return index_vector;
}
