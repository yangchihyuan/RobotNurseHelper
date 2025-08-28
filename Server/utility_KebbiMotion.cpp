#include "utility_KebbiMotion.hpp"
#include <vector>
#include <iostream>

//The function checks if the motion is a head movement.
//If a head movement is involved, return true. Otherwise, return false.
bool KebbiMoveHeadDuringMotion(string sMotionName)
{
    vector<string> vHeadMove = {"666_SP_Cheer", "666_SA_Think", "666_BA_Nodhead", "666_DA_LookFor", "666_DA_Take","666_PE_Drums", "666_PE_Harmonica",
    "666_PE_Sorcery", "666_PE_Hug"};
    for( auto & head_motion : vHeadMove )
    {
        if( sMotionName == head_motion )
        {
            cout << "KebbiMoveHeadDuringMotion: " << sMotionName << endl;
            return true;
        }
    }
    return false;
}

