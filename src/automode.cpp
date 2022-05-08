#include "automode.h"

const ProgramEntry programIndex[3] = {
    {TYPE_LEN, 2,  {}},
    {TYPE_HAND, 0, {}},
    {TYPE_AUTO, 3, {
        {TIME(0, 0, 0, 0), TIME(20*24, 0, 0, 0), 37.7f, 53, 12},
        {TIME(20*24, 0, 0, 0), TIME(20*24 + 12, 0, 0, 0), 37, 78, 0},
        {TIME(20*24 + 12, 0, 0, 0), TIME(21*24, 0, 0, 0), 36, 60, 0}
    }}
};
