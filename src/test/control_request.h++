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
            out.type = rand();
            out.mask = rand();
            out.data = rand();
            return out;
        }
};

#endif
