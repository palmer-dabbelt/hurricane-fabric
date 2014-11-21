object Main {
  def main(in_args: Array[String]): Unit = {
    val args = in_args ++ Array[String](
      "--debug",
      "--vcd")

    Chisel.Driver.chiselConfigMode = Some("instance")
    Chisel.Driver.chiselConfigClassName = Some("ExampleConfig")
    Chisel.Driver.chiselProjectName = Some("HurricaneFabric")

    Chisel.chiselMain.run(
      args,
      () => new HurricaneFabric.Fabric
    )
  }
}
