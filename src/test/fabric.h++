#ifndef FABRIC_HXX
#define FABRIC_HXX

#include "Fabric.ExampleConfig.h"

#include "control_response.h++"
#include "control_request.h++"

#include <memory>

/* A C++ wrapper class for the Hurricane fabric. */
class fabric {
public:
    typedef std::shared_ptr<fabric> ptr;

private:
    Fabric_t _dut;
    std::vector<tile_id_t> _tiles;

public:
    fabric(void);

public:
    /* Steps the simulation of this fabric forward N cycles. */
    void step(size_t cycles = 1);

    /* Functions to actually enqueue packets into the fabric.  Note
     * that all of these can fail, in which case you'll have try again
     * next cycle. */
    bool host_enq(const control_request& pkt);
    bool tile_enq(const control_response& pkt, size_t cycle);

    /* These are essentially callback functions -- though they're
     * actually virtual instead.  */
    virtual bool host_enq_cb(size_t cycle) = 0;
    virtual bool host_deq_cb(size_t cycle, const control_response& pkt) = 0;
    virtual bool tile_enq_cb(size_t cycle, tile_id_t tid) = 0;
    virtual bool tile_deq_cb(size_t cycle, tile_id_t tid, const control_request& pkt) = 0;

private:
    /* Easy accessor methods to allow access to the internal DUT
     * fields, which aim to be type (and probably more importantly,
     * direction) safe. */
    bool control_req_ready(void) const;
    bool control_resp_valid(void) const;
    void control_req_bits(const control_request& pkt);
    control_response control_resp_bits(void) const;
    void control_req_valid(bool value);
    void control_resp_ready(bool value);
};

#endif
