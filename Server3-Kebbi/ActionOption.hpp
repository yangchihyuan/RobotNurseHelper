#ifndef __ACTION_OPTION_HPP__
#define __ACTION_OPTION_HPP__

struct ActionOption
{
    enum MOVE_MODE
    {
        MOVE_MANUAL = 0,
        MOVE_BODY = 1,
        MOVE_HEAD = 2,
    };

    MOVE_MODE move_mode;
};

#endif