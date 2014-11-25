#include "fabric.h++"
#include <queue>

/* This neatly wraps up the control message request/response pair into
 * something we can have in a queue for testing.  The correctness
 * criteria here is simply that messages remain in order through the
 * traversal of the machine, and that each tile sees an in-order
 * message stream. */
class control_message {
public:
    typedef std::shared_ptr<control_message> ptr;

private:
    static uint64_t __key;

public:
    const uint64_t _key;
    const control_request request;
    const control_response response;

public:
    control_message(void);
};

class test_fabric : public fabric {
public:
    typedef std::shared_ptr<fabric> ptr;

private:
    /* A queue that contains the order of messages, with the head of
     * the queue containing the next message that must come out of the
     * top-level fabric control port. */
    std::queue<control_message::ptr> _control_in_flight;

    /* A set of queues, one for each tile in the system.  The head of
     * each of these queues contains the next message that must appear
     * at this tile, and ensures that all tiles get all messages in
     * order. */
    std::map<tile_id_t, std::shared_ptr<std::queue<control_message::ptr>>>
        _tile_in_flight;

    /* A map from the tile ID to the next message to write -- this
     * exists to allow me to delay these signals arbitrarily so I can
     * make sure the fabric is faithful about blocking. */
    std::map<tile_id_t, control_message> _control_to_resp;

public:
    virtual bool host_enq_cb(size_t cycle) override;
    virtual bool host_deq_cb(size_t cycle, const control_response& pkt) override;
    virtual bool tile_enq_cb(size_t cycle, tile_id_t tid) override;
    virtual bool tile_deq_cb(size_t cycle, tile_id_t tid, const control_request& pkt) override;
};

int main(int argc __attribute__((unused)),
         char **argv __attribute__((unused))
    )
{
    srand(0);

    auto dut = std::make_shared<test_fabric>();
    dut->step(100);

    return 0;
}

uint64_t control_message::__key = 0;

control_message::control_message(void)
    : _key(__key++),
      request(control_request::random()),
      response(control_response::random())
{
}

#pragma GCC diagnostic ignored "-Wunused-parameter"

bool test_fabric::host_enq_cb(size_t cycle)
{
    if (cycle > 50)
        return false;

    auto message = std::make_shared<control_message>();

    if (host_enq(message->request) == true) {
        printf("host enqueue: %lx %lx %lx\n",
               message->request.type,
               message->request.mask,
               message->request.data
            );

        _control_in_flight.push(message);
        return true;
    }

    return false;
}

bool test_fabric::host_deq_cb(size_t cycle, const control_response& pkt)
{
    auto expected = _control_in_flight.front();

    if (expected->response != pkt)
        fprintf(stderr, "Expected %lx, got %lx\n",
                expected->response.data,
                pkt.data
            );

    _control_in_flight.pop();
    return true;
}

bool test_fabric::tile_enq_cb(size_t cycle, tile_id_t tid)
{
    return false;
}

bool test_fabric::tile_deq_cb(size_t cycle, tile_id_t tid, const control_request& pkt)
{
    return false;
}
