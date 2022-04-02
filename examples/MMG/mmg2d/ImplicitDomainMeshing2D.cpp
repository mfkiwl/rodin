/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#include <iostream>

#include <Rodin/Mesh.h>
#include <RodinExternal/MMG.h>

using namespace Rodin;
using namespace Rodin::External::MMG;

int main(int argc, char** argv)
{
  int Omega = 0;
  int Interior = 1, Exterior = 2;
  int Boundary = 4;

  auto box = Mesh2D::load(argv[1]);
  auto ls  = ScalarSolution2D::load(argv[2]).setMesh(box);
  auto mesh = ImplicitDomainMesher2D().split(Omega, {Interior, Exterior})
                                           .setHMax(0.1)
                                           .setRMC()
                                           .setBoundaryReference(Boundary)
                                           .discretize(ls);
  mesh.save("Omega.mesh");

  return 0;
}
