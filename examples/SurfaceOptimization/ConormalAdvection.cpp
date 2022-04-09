/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#include <Rodin/Mesh.h>
#include <Rodin/Solver.h>
#include <Rodin/Variational.h>
#include <RodinExternal/MMG.h>

using namespace Rodin;
using namespace Rodin::Variational;
using namespace Rodin::External;

int main(int, char**)
{
  const char* meshFile = "rodin.mesh";
  const double pi = std::atan(1) * 4;

  int Interior = 2, Exterior = 3;
  int Gamma = 4;

  Mesh Omega;
  Omega.load(meshFile);

  // H1 Vh(Omega, 2);

  // GridFunction f(Vh);
  // f = VectorFunction{
  //   [](const double* x, int){ return 3 * x[0] * x[0] + sin(10 * x[1]); },
  //   [](const double* x, int){ return 3 * x[0] + cos(10 * x[1]); }};

  // Omega.save("Omega.mesh");
  // f.save("f.gf");

  // auto mmgMesh = MMG::MeshS::load("surface.mesh");
  // auto mmgSol = MMG::ScalarSolutionS::load("surface.sol").setMesh(mmgMesh);

  // mmgMesh.save("lel.mesh");
  // mmgSol.save("lel.sol");

  // MMG::ScalarSolution2D tmp(mmgSol);
  // mmgMesh.save("mmg.mesh");
  // tmp.save("mmg.sol");

  // // Sphere radius
  // double r = 1;

  // // Hole radius
  // double rr = 0.2;

  // // Hole centers on sphere
  // std::vector<std::array<double, 3>> cs = {
  //   { r * sin(0) * cos(0), r * sin(0) * sin(0), r * cos(0) },
  //   { r * sin(pi / 2) * cos(pi), r * sin(pi / 2) * sin(pi), r * cos(pi / 2) },
  //   { r * sin(pi / 2) * cos(pi / 2), r * sin(pi / 2) * sin(pi / 2), r * cos(pi / 2) }
  // };

  // // Geodesic distance
  // auto gd = [&](const double* x, const double* c, int)
  //           {
  //             return std::acos((x[0] * c[0] + x[1] * c[1] + x[2] * c[2]));
  //           };

  // // Function for generating holes
  // auto f = ScalarCoefficient(
  //     [&](const double* x, int dim) -> double
  //     {
  //       double d = std::numeric_limits<double>::max();
  //       for (const auto& c : cs)
  //       {
  //         double dd = gd(x, c.data(), dim) - rr;
  //         d = std::min(d, dd);
  //       }
  //       if (d <= rr)
  //         return 1;
  //       else
  //         return -1;
  //     });
  // GridFunction ls(Vh);
  // ls = f;

  FiniteElementSpace<H1> Vh(Omega);
  FiniteElementSpace<H1> Th(Omega, 3);

  auto mmgMesh = Cast(Omega).to<MMG::MeshS>();
  auto mmgDist = MMG::DistancerS().distance(mmgMesh).setMesh(mmgMesh);

  auto phi = Cast(mmgDist).to<IncompleteGridFunction>().setFiniteElementSpace(Vh);

  // Compute extended conormal
  auto n0 = VectorFunction{Dx(phi), Dy(phi), Dz(phi)};

  double alpha = 0.1;

  TrialFunction nx(Vh);
  TrialFunction ny(Vh);
  TrialFunction nz(Vh);
  TestFunction  v(Vh);

  auto solver = Solver::UMFPack();

  Problem velextX(nx, v);
  velextX = Integral(alpha * Grad(nx), Grad(v))
          + Integral(nx, v)
          - Integral(n0.x(), v);
  solver.solve(velextX);

  Problem velextY(ny, v);
  velextY = Integral(alpha * Grad(ny), Grad(v))
          + Integral(ny, v)
          - Integral(n0.y(), v);
  solver.solve(velextY);

  Problem velextZ(nz, v);
  velextZ = Integral(alpha * Grad(nz), Grad(v))
          + Integral(nz, v)
          - Integral(n0.z(), v);
  solver.solve(velextZ);

  auto n = VectorFunction{nx.getGridFunction(), ny.getGridFunction(), nz.getGridFunction()};
  auto norm = Pow(n.x() * n.x() + n.y() * n.y() + n.z() * n.z(), 0.5);

  GridFunction conormal(Th);
  conormal = n / norm;

  phi.save("phi.gf");
  conormal.save("conormal.gf");
  Omega.save("Omega.mesh");

  auto mmgCo = Cast(conormal).to<MMG::IncompleteVectorSolutionS>().setMesh(mmgMesh);
  mmgCo.save("mmg.sol");

  return 0;
}
