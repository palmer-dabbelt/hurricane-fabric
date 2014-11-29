package HurricaneFabric

// These are probably not necessary, but I'm not going to get rid of
// them for now...
package RocketChip {
  import Chisel._
  import uncore._
  import rocket._
  import rocket.Util._

  case object NTiles extends Field[Int]
  case object NBanks extends Field[Int]
  case object NOutstandingMemReqs extends Field[Int]
  case object BankIdLSB extends Field[Int]
  case object CacheBlockBytes extends Field[Int]
  case object CacheBlockOffsetBits extends Field[Int]
  case object UseBackupMemoryPort extends Field[Boolean]
  case object BuildCoherenceMaster extends Field[(Int) => CoherenceAgent]
  case object BuildTile extends Field[(Bool)=>Tile]
}

// You can't actually instantiate a Hurricane fabric without knowing
// some basic paramaters about the tiles that compose that fabric.  In
// order to avoid depending on a particular Hurricane tile, this
// configuration object specifies sane values for those that can then
// be used for testing.
class ExampleConfig extends Chisel.ChiselConfig (
  topDefinitions = { (pname,site,here) => 
    type PF = PartialFunction[Any,Any]
    def findBy(sname:Any):Any = here[PF](site[Any](sname))(pname)
    pname match {
      case HurricaneFabric.Width => 4
      case HurricaneFabric.Height => 4

      case uncore.TLAddrBits => 1
      case uncore.TLDataBits => 1

      case RocketChip.NTiles => Chisel.Knob("NTILES")
      case uncore.NClients => site(RocketChip.NTiles) + 1
      case uncore.TLCoherence =>
        new uncore.MSICoherence(
          () => new uncore.NullRepresentation(site(uncore.NClients))
        )

      case RocketChip.NBanks => Chisel.Knob("NBANKS")
      case uncore.LNMasters => site(RocketChip.NBanks)
      case uncore.LNClients => site(RocketChip.NTiles)+1
      case uncore.LNEndpoints => site(uncore.LNMasters) + site(uncore.LNClients)

      case uncore.TLClientXactIdBits => 1
      case uncore.TLMasterXactIdBits => 1

      case HurricaneFabric.ICacheLineSizeBits => 512
      case HurricaneFabric.DCacheWordSizeBits => 64
      case HurricaneFabric.NetworkWordSizeBits => site(HurricaneFabric.DCacheWordSizeBits)
      case HurricaneFabric.NetworkPortCount => 4

      case HurricaneFabric.PhysicalAddressSizeBits => 43
      case HurricaneFabric.MemoryTransactionTagSizeBits => 1
      case ControlTypeSizeBits => 4
      case ControlWordSizeBits => 64

      case HurricaneFabric.NetworkQueueDepth => 3
  }},
  knobValues = {
    case "NTILES" => 2
    case "NBANKS" => 1
  }
)
