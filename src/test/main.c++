#include "fabric.h++"
#include <queue>

#define CYCLE_COUNT 10

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
    const uint64_t key;
    const control_request request;
    const control_response response;

public:
    control_message(uint64_t _key,
                    const control_request& _request,
                    const control_response& _response);

    static ptr random(void);
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
    std::map<tile_id_t, std::shared_ptr<std::queue<control_message::ptr>>>
        _tile_to_resp;

public:
    test_fabric(void);

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
    dut->step(CYCLE_COUNT);

    return 0;
}

uint64_t control_message::__key = 0;

control_message::control_message(uint64_t _key,
                                 const control_request& _request,
                                 const control_response& _response)
    : key(_key),
      request(_request),
      response(_response)
{
}

control_message::ptr control_message::random(void)
{
    return std::make_shared<control_message>(
        __key++,
        control_request::random(),
        control_response::random()
        );
}

test_fabric::test_fabric(void)
{
    for (auto tile: this->tiles()) {
        _tile_in_flight[tile] =
            std::make_shared<std::queue<control_message::ptr>>();
        _tile_to_resp[tile] =
            std::make_shared<std::queue<control_message::ptr>>();
    }
}        

#pragma GCC diagnostic ignored "-Wunused-parameter"

bool test_fabric::host_enq_cb(size_t cycle)
{
    if (cycle > (CYCLE_COUNT - 5))
        return false;

    auto message = control_message::random();
    if (host_enq(message->request) == false)
        return false;

    printf("host_enq_cb(): host enqueue '%lx %lx %lx -> %lx'\n",
           message->request.type,
           message->request.mask,
           message->request.data,
           message->response.data
        );

    _control_in_flight.push(message);

    for (auto tile: this->tiles()) {
        auto l = _tile_in_flight.find(tile);
        if (l == _tile_in_flight.end()) {
            fprintf(stderr, "Bad TID: %lu\n", tile);
            abort();
        }
        auto q = l->second;
        q->push(message);
    }

    return true;
}

bool test_fabric::host_deq_cb(size_t cycle, const control_response& pkt)
{
    fprintf(stderr, "host_deq_cb(): host dequeue %lu\n",
            pkt.data
        );

    auto expected = _control_in_flight.front();

    if (expected->response != pkt) {
        fprintf(stderr, "host_deq_cb(): expected %lx, got %lx\n",
                expected->response.data,
                pkt.data
            );
        return false;
    }

    _control_in_flight.pop();
    return true;
}

bool test_fabric::tile_enq_cb(size_t cycle, tile_id_t tid)
{
    auto l = _tile_to_resp.find(tid);
    if (l == _tile_to_resp.end())
        return false;
    auto q = l->second;

    if (q->size() == 0)
        return false;

    auto message = q->front();
    if (tile_enq(tid, message->response) == false) {
        printf("tile_enq_cb(%lu): tile enqueue %lx %lx %lx -> %lx\n",
               tid,
               message->request.type,
               message->request.mask,
               message->request.data,
               message->response.data
            );

        return false;
    }

    q->pop();
    return true;
}

bool test_fabric::tile_deq_cb(size_t cycle, tile_id_t tid, const control_request& pkt)
{
    fprintf(stderr, "tile_deq_cb(%lu): tile dequeue '%lx %lx %lx'\n",
            tid,
            pkt.type,
            pkt.mask,
            pkt.data
        );

    auto l = _tile_in_flight.find(tid);
    if (l == _tile_in_flight.end()) {
        fprintf(stderr, "tile_deq_cb(%lu, %lu, ...): Bad TID\n",
                cycle,
                tid
            );
        abort();
    }
    auto q = l->second;

    if (q->size() == 0) {
        fprintf(stderr, "Unexpected tile dequeue\n");
        return false;
    }

    auto message = q->front();
    if (message->request != pkt) {
        fprintf(stderr, "tile_deq_cb(): expected %lx %lx %lx, got %lx %lx %lx\n",
                message->request.type,
                message->request.mask,
                message->request.data,
                pkt.type,
                pkt.mask,
                pkt.data
            );
        return false;
    }
    q->pop();

    {
        auto l = _tile_to_resp.find(tid);
        if (l == _tile_to_resp.end()) {
            fprintf(stderr, "Bad TID\n");
            abort();
        }
        auto q = l->second;

        q->push(message);
    }
        
    return true;
}
