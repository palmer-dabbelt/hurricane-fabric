#ifndef CONTROL_RESPONSE_HXX
#define CONTROL_RESPONSE_HXX

#include "bit_types.h++"

/* A C++ wrapper for control response packets.  These are essentially
 * just structs, as you have to have pretty good visibility into them
 * anyway to write a test harness. */
class control_response {
public:
    control_word_t data;

public:
    static control_response random(void)
        {
            control_response out;
            out.data = rand();
            return out;
        }
};

static inline bool operator==(const control_response& a, const control_response& b)
{
    return a.data == b.data;
}

static inline bool operator!=(const control_response& a, const control_response& b)
{
    return !(a == b);
}


#endif
