#ifndef CONTROL_REQUEST_HXX
#define CONTROL_REQUEST_HXX

#include "bit_types.h++"

/* A C++ wrapper for control response packets.  These are essentially
 * just structs, as you have to have pretty good visibility into them
 * anyway to write a test harness. */
class control_request {
public:
    control_type_t type;
    control_mask_t mask;
    control_word_t data;

public:
    static control_request random(void)
        {
            control_request out;
            out.type = rand() & 0x3;
            out.mask = rand() & 0xFFFF;
            out.data = rand();
            return out;
        }
};

static inline bool operator==(const control_request& a, const control_request& b)
{
    if (a.type != b.type)
        return false;
    if (a.mask != b.mask)
        return false;
    if (a.data != b.data)
        return false;

    return true;
}

static inline bool operator!=(const control_request& a, const control_request& b)
{
    return !(a == b);
}

#endif
