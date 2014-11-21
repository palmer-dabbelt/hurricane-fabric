package HurricaneFabric

import Chisel._

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
    val tile_link = new uncore.TileLinkIO()
  }
  val io = new IO
}
