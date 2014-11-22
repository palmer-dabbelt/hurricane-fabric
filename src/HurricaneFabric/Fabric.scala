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
    val upstream = new uncore.TileLinkIO().asOutput

    // The fabric is just an interconnect: the actual Hurricane tiles
    // live outside of here.  Here we have an interfact to connect
    // ourselves to every tile.
    val tiles = Vec.fill(tile_count){new TileIO()}
  }
  val io = new IO
}
