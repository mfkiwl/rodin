#include <iostream>

#include <Rodin/Mesh.h>
#include <RodinExternal/MMG.h>

using namespace Rodin;
using namespace Rodin::External;

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    std::cout << "Usage:\t" << argv[0] << " <filename>.mesh" << std::endl;
    std::exit(EXIT_FAILURE);
  }

  MMG::Mesh2D mmgMesh = MMG::Mesh2D::load(argv[1]);

  Rodin::Mesh rodinMesh = Cast(mmgMesh).to<Rodin::Mesh<Traits::Serial>>();

  rodinMesh.save("rodin.mesh");

  return 0;
}
