#include "fabric.h++"
#include <numeric>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/preprocessor/repeat.hpp>

#define TILE_COUNT 16
#define RESET_CYCLES 10

fabric::fabric(void)
    : _dut(),
      _tiles(boost::counting_iterator<tile_id_t>(0),
             boost::counting_iterator<tile_id_t>(TILE_COUNT)
          )
{
    for (int cycle = 0; cycle < RESET_CYCLES; cycle++)
        _dut.clock(true);
}

void fabric::step(size_t cycles)
{
    FILE *vcd = fopen("fabric.vcd", "w");
    auto req = control_request::random();
    auto resp = control_response::random();
    req.type = 0;
    req.mask = 0;
    req.data = 0;
    resp.data = 0;

    for (auto cycle = 0*cycles; cycle < cycles; cycle++) {
        printf("cycle: %lu\n", cycle);

        control_req_valid(false);
        control_resp_ready(false);
        control_req_bits(req);
        for (auto tid: _tiles) {
            tile_control_resp_valid(tid, false);
            tile_control_req_ready(tid, false);
            tile_control_resp_bits(tid, resp);
        }

        _dut.clock_lo(false);

        host_enq_cb(cycle);

        if (control_resp_valid()) {
            auto bits = control_resp_bits();
            auto handled = host_deq_cb(cycle, bits);
            control_resp_ready(handled);
        }

        for (auto tid: _tiles) {
            tile_enq_cb(cycle, tid);

            if (tile_control_req_valid(tid)) {
                auto bits = tile_control_req_bits(tid);
                auto handled = tile_deq_cb(cycle, tid, bits);
                tile_control_req_ready(tid, handled);
            }
        }

        _dut.clock_lo(false);
        _dut.dump(vcd, cycle*2+0);
        _dut.clock_hi(false);
        _dut.dump(vcd, cycle*2+1);
    }

    fclose(vcd);
}

bool fabric::host_enq(const control_request& pkt)
{
    if (control_req_ready() == false)
        return false;

    control_req_bits(pkt);
    control_req_valid(true);
    return true;
}

bool fabric::tile_enq(tile_id_t tid, const control_response& pkt)
{
    if (tile_control_resp_ready(tid) == false)
        return false;

    tile_control_resp_bits(tid, pkt);
    tile_control_resp_valid(tid, true);
    return true;
}

bool fabric::control_req_ready(void) const
{
    return _dut.Fabric__io_control_req_ready.values[0] == 1;
}

bool fabric::control_resp_valid(void) const
{
    return _dut.Fabric__io_control_resp_valid.values[0] == 1;
}

void fabric::control_req_bits(const control_request& pkt)
{
    _dut.Fabric__io_control_req_bits_message_type.values[0] = pkt.type;
    _dut.Fabric__io_control_req_bits_mask.values[0] = pkt.mask;
    _dut.Fabric__io_control_req_bits_data.values[0] = pkt.data;
}

control_response fabric::control_resp_bits(void) const
{
    control_response out;
    out.data = _dut.Fabric__io_control_resp_bits_data.values[0];
    return out;
}

void fabric::control_req_valid(bool value)
{
    _dut.Fabric__io_control_req_valid = value;
}

void fabric::control_resp_ready(bool value)
{
    _dut.Fabric__io_control_resp_ready = value;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"

bool fabric::tile_control_resp_ready(tile_id_t tid) const
{
#define CASERET(_, I, __)                                               \
    case I:                                                             \
        return _dut.Fabric__io_tiles_ ## I ## _control_resp_ready.values[0] == 1;

    switch (tid) {
        BOOST_PP_REPEAT(TILE_COUNT, CASERET, _)
    }

    fprintf(stderr, "Unknown TID: %lu\n", tid);
    abort();
    return false;
#undef CASERET
}

bool fabric::tile_control_req_valid(tile_id_t tid) const
{
#define CASERET(_, I, __)                                               \
    case I:                                                             \
        return _dut.Fabric__io_tiles_ ## I ## _control_req_valid.values[0] == 1;

    switch (tid) {
        BOOST_PP_REPEAT(TILE_COUNT, CASERET, _)
    }

    fprintf(stderr, "Unknown TID: %lu\n", tid);
    abort();
    return false;
#undef CASERET
}

void fabric::tile_control_resp_bits(tile_id_t tid, const control_response& pkt)
{
#define CASERET(_, I, __)                                               \
    case I:                                                             \
        _dut.Fabric__io_tiles_ ## I ## _control_resp_bits_data = pkt.data; \
        return;

    switch (tid) {
        BOOST_PP_REPEAT(TILE_COUNT, CASERET, _)
    }

    fprintf(stderr, "Unknown TID: %lu\n", tid);
    abort();
#undef CASERET
}

control_request fabric::tile_control_req_bits(tile_id_t tid) const
{
    control_request out;

#define CASERET(_, I, __)                                               \
    case I:                                                             \
        out.type = _dut.Fabric__io_tiles_ ## I ## _control_req_bits_message_type.values[0]; \
        out.mask = _dut.Fabric__io_tiles_ ## I ## _control_req_bits_mask.values[0]; \
        out.data = _dut.Fabric__io_tiles_ ## I ## _control_req_bits_data.values[0]; \
        return out;

    switch (tid) {
        BOOST_PP_REPEAT(TILE_COUNT, CASERET, _)
    }

    fprintf(stderr, "Unknown TID: %lu\n", tid);
    abort();
#undef CASERET

    return out;
}

void fabric::tile_control_resp_valid(tile_id_t tid, bool value)
{
#define CASERET(_, I, __)                                             \
    case I:                                                           \
        _dut.Fabric__io_tiles_ ## I ## _control_resp_valid = value;   \
        return;

    switch (tid) {
        BOOST_PP_REPEAT(TILE_COUNT, CASERET, _)
    }

    fprintf(stderr, "Unknown TID: %lu\n", tid);
    abort();
#undef CASERET
}

void fabric::tile_control_req_ready(tile_id_t tid, bool value)
{
#define CASERET(_, I, __)                                               \
    case I:                                                             \
        _dut.Fabric__io_tiles_ ## I ## _control_req_ready = value;      \
        return;

    switch (tid) {
        BOOST_PP_REPEAT(TILE_COUNT, CASERET, _)
    }

    fprintf(stderr, "Unknown TID: %lu\n", tid);
    abort();
#undef CASERET
}
