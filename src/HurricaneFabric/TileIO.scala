package HurricaneFabric

import Chisel._

// The size of a physical address in this system
case object PhysicalAddressSizeBits extends Field[Int]

// The ICache will be loaded an entire line at a time because it's not
// coherent at all.
case object ICacheLineSizeBits extends Field[Int]

// The DCache is also not coherent, but it achieves this by not
// caching at all.  This means that the DCache will only work on a
// per-word granularity.
case object DCacheWordSizeBits extends Field[Int]

// We currently expose an explicit on-chip network to each tile, the
// configuration of which is exposed here.
case object NetworkWordSizeBits extends Field[Int]
case object NetworkPortCount extends Field[Int]

// Memory subsystem requests and responses are tagged with a value
// that allows for them to be reordered.  This is the size of that
// tag.
case object MemoryTransactionTagSizeBits extends Field[Int]

// The control FIFO pairs contain a 
case object ControlTypeSizeBits extends Field[Int]
case object ControlWordSizeBits extends Field[Int]

// The number of Hurricane tiles in this fabric
case object TileCount extends Field[Int]

abstract trait TileIOParameters extends UsesParameters {
  val icache_line_size_bits      = params(ICacheLineSizeBits)
  val dcache_word_size_bits      = params(DCacheWordSizeBits)
  val network_word_size_bits     = params(NetworkWordSizeBits)
  val network_port_count         = params(NetworkPortCount)
  val physical_address_size_bits = params(PhysicalAddressSizeBits)
  val memory_transaction_tag_size_bits = params(MemoryTransactionTagSizeBits)
  val control_type_size_bits     = params(ControlTypeSizeBits)
  val control_word_size_bits     = params(ControlWordSizeBits)
  val tile_count                 = params(Width) * params(Height)
}

class NetworkPacket extends Bundle with TileIOParameters {
  val bits = Bits(width = network_word_size_bits)
}

// Memory is handled as a pair of FIFOs, one for request and one for
// response.
class MemoryRequest(data_size: Int) extends Bundle with TileIOParameters {
  val paddr = UInt(width = physical_address_size_bits)
  val tag   = UInt(width = memory_transaction_tag_size_bits)
  val data  = Bits(width = data_size)
  val wen   = Bool() // TRUE means this is a write, FALSE a read

  override def clone() = new MemoryRequest(data_size).asInstanceOf[this.type]
}

class MemoryResponse(data_size: Int) extends Bundle with TileIOParameters {
  val tag  = UInt(width = memory_transaction_tag_size_bits)
  val data = Bits(width = data_size)

  override def clone() = new MemoryResponse(data_size).asInstanceOf[this.type]
}

// The control messages are essentially tagged unions, but handled
// manually.
object ControlMessageType extends Enumeration {
  type ControlMessageType = Value
  val Halt, // Halts the execution of this processor, drains the
            // pipeline, and flushes all the memory queues.

  LongJump // Treates the data as a virtual PC, jumps to that address,
           // and begins executing.
  = Value
}

// The meaning of the bits for various control request/response pairs
// are included below.

// A Halt request/response, which doesn't actually need any bits at
// all.  The response can only be asserted after the tile has both
// flushed and halted.
class ControlHaltRequestBits extends Bundle with TileIOParameters {
  val unused = Bits(width = control_word_size_bits)
}
class ControlHaltResponseBits extends Bundle with TileIOParameters {
  val unused = Bits(width = control_word_size_bits)
}

// LongJump control messages cause the targeted tiles to jump to an
// address and begin executing.
class ControlLongJumpRequestBits extends Bundle with TileIOParameters {
  val virtual_target_pc = Bits(width = control_word_size_bits)
}
class ControlLongJumpResponseBits extends Bundle with TileIOParameters {
  val unused = Bits(width = control_word_size_bits)
}

// Generic control request/response FIFO values -- you shouldn't be
// using the data values here directly, but should instead use the
// tagged union and cast correctly.  Note that control requests should
// be ignored (and a message with data bits of 0 should be enqueued)
// if the mask does not contain this tile's ID set.
class ControlRequest extends Bundle with TileIOParameters {
  val message_type = Bits(OUTPUT, width = control_type_size_bits)
  val mask = Bits(width = tile_count)
  val data = Bits(width = control_word_size_bits)
}

class ControlResponse extends Bundle with TileIOParameters {
  val data = Bits(width = control_word_size_bits)
}

// The interface that a single Hurricane tile (which contains the core
// along with the I and D caches) provides to the rest of the
// Hurricane fabric.
class TileIO extends Bundle with TileIOParameters {
  // The explicit network input/output ports
  // FIXME: Remove these and replace them with in-memory queues
  val net_to_tile = Vec.fill(network_port_count){(new DecoupledIO(new NetworkPacket))}
  val net_to_fabric = Vec.fill(network_port_count){(new DecoupledIO(new NetworkPacket))}

  // The ICache memory interface, which satisfies entire cache lines
  // at a time.  When you get a response back from this interface it's
  // safe to cache it forever.
  val icache_resp = Decoupled(new MemoryResponse(icache_line_size_bits))
  val icache_req  = Decoupled(new MemoryRequest(icache_line_size_bits))

  // The DCache memory interface, which only loads single words at a
  // time.  This consists of two FIFO pairs: one that handles requests
  // from the tile to the rest of the fabric, and one that handles
  // requests from the rest of the system to the tile.
  val dcache_to_tile_req    = Decoupled(new MemoryRequest(dcache_word_size_bits))
  val dcache_to_tile_resp   = Decoupled(new MemoryResponse(dcache_word_size_bits))
  val dcache_to_fabric_req  = Decoupled(new MemoryRequest(dcache_word_size_bits))
  val dcache_to_fabric_resp = Decoupled(new MemoryResponse(dcache_word_size_bits))

  // The control FIFO pairs, which deal with getting arbitrary extra
  // information into each tile.  Each control request must produce
  // exactly one response, and these message pairs must be kept in
  // order.  All control requests are broadcast to all tiles, and the
  // value of the control response bits are ORed together to produce a
  // single response back to the host.
  val control_req  = Decoupled(new ControlRequest).flip
  val control_resp = Decoupled(new ControlResponse)
}
