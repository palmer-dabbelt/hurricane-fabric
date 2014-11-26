package HurricaneFabric

import Chisel._

// The absolute size of the Hurricane array that will be generated
case object Width extends Field[Int]
case object Height extends Field[Int]

abstract trait FabricParameters extends UsesParameters {
  val width      = params(Width)
  val height     = params(Height)
  val tile_count = width * height
}

// A single Hurricane virtual cache -- note that this cache doesn't
// actually store any data itself, it just causes the distributed and
// completely uncoherent Hurricane data caches to appear as a single
// cache to the rest of the world.
class Fabric extends Module with FabricParameters {
  class IO extends Bundle {
    // A single Tile Link, which acts as a connection to the rest of
    // the system.
    val upstream = new uncore.TileLinkIO()

    // The fabric is just an interconnect: the actual Hurricane tiles
    // live outside of here.  Here we have an interfact to connect
    // ourselves to every tile.
    val tiles = Vec.fill(tile_count){(new TileIO()).flip}

    // The fabric multiplexes control words from the host to each tile
    // in the fabric, only returning a success code when the entire
    // fabric has responded to each command.
    val control_req  = Decoupled(new ControlRequest).flip
    val control_resp = Decoupled(new ControlResponse)
  }
  val io = new IO

  // A state machine for the control -- note that this is currently
  // _very_ simple in that it only allows one request/response pair in
  // the system at a time.
    val host_req :: tile_req :: tile_resp :: host_resp :: Nil = Enum(UInt(), 4)
    val state = Reg(init = host_req)

    val sent      = Vec.fill(tile_count){ Reg(init = Bool(false)) }
    val responded = Vec.fill(tile_count){ Reg(init = Bool(false)) }

    val request  = Reg(new ControlRequest)
    val response = Vec.fill(tile_count){ Reg(new ControlResponse) }

    // By default don't allow anything to pass through at all
    io.control_req .ready := Bool(false)
    io.control_resp.valid := Bool(false)
    io.control_resp.bits.data := response.map{r => r.data}.fold(Bits(0)){ _ | _ }
    io.tiles.foreach{ tile =>
      tile.control_req.valid  := Bool(false)
      tile.control_req.bits   := request
      tile.control_resp.ready := Bool(false)
    }

    switch (state) {
      is (host_req) {
        io.control_req.ready := Bool(true)

        request := io.control_req.bits

        sent      := UInt(0)
        responded := UInt(0)

        when (io.control_req.valid === Bool(true)) {
          state := tile_req
        }
      }

      is (tile_req) {
        // Upon entrance to this state, the "sent" register has been
        // set to TRUE for every tile that has successfully had a
        // control request sent to it, and FALSE for every other tile.
        // This attempts to enqueue a request for every sent-FALSE
        // tile and then sets the sent bit when enqueueing a request.
        (sent, io.tiles).zipped.map{ (sent, tile) =>
          tile.control_req.valid := !sent
          when (tile.control_req.ready === Bool(true)) {
            sent := Bool(true)
          }
        }

        when (sent.fold(Bool(true)){_ && _} === Bool(true)) {
          state := tile_resp
        }
      }

      is (tile_resp) {
        (responded, io.tiles, response).zipped.map{ (responded, tile, response) =>
          tile.control_resp.ready := !responded
          when (tile.control_resp.valid === Bool(true)) {
            responded := Bool(true)
            response  := tile.control_resp.bits
          }
        }

        when (responded.fold(Bool(true)){_ && _} === Bool(true)) {
          state := host_resp
        }
      }

      is (host_resp) {
        io.control_resp.valid := Bool(true)

        when (io.control_resp.ready === Bool(true)) {
          state := host_req
        }
      }
    }
}
