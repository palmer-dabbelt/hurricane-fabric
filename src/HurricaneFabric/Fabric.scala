package HurricaneFabric

import Chisel._

// The absolute size of the Hurricane array that will be generated
case object Width extends Field[Int]
case object Height extends Field[Int]

abstract trait FabricParameters extends UsesParameters {
  val width  = params(Width)
  val height = params(Height)
}

// A single Hurricane virtual cache -- note that this cache doesn't
// actually store any data itself, it just causes the distributed and
// completely uncoherent Hurricane data caches to appear as a single
// cache to the rest of the world.
class Fabric extends Module with FabricParameters {
  class IO extends Bundle {
    // A single Tile Link
    val tile_link = new uncore.TileLinkIO().asOutput
  }
  val io = new IO
}
