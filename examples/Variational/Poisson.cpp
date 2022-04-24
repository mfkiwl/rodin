/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#include <chrono>
#include <Rodin/Mesh.h>
#include <Rodin/Solver.h>
#include <Rodin/Variational.h>

using namespace Rodin;
using namespace Rodin::Variational;

int main(int, char**)
{
  const char* meshFile = "../resources/mfem/poisson-example.mesh";

  // Define boundary attributes
  int Gamma = 1;

  // Load mesh
  Mesh Omega;
  Omega.load(meshFile);

  // Functions
  FiniteElementSpace<H1> Vh(Omega);
  TrialFunction u(Vh);
  TestFunction  v(Vh);

  // Define problem
  auto f = ScalarFunction(1.0);
  auto g = ScalarFunction(0.0);

  Problem poisson(u, v);
  poisson = Integral(Grad(u), Grad(v))
          - Integral(f, v)
          + DirichletBC(u, g).on(Gamma);

  // Solve problem
  Solver::CG().setMaxIterations(200)
              .setRelativeTolerance(1e-12)
              .printIterations(true)
              .solve(poisson);

  // Save solution
  u.getGridFunction().save("u.gf");
  Omega.save("Omega.mesh");

  return 0;
}
