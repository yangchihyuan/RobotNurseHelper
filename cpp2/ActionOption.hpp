#ifndef __ACTION_OPTION_HPP__
#define __ACTION_OPTION_HPP__

struct ActionOption
{
    enum MOVE_MODE
    {
        MOVE_BODY = 0,
        MOVE_HEAD = 1,
    };

    MOVE_MODE move_mode;
};

#endif