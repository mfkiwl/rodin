/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_PROBLEM_HPP
#define RODIN_VARIATIONAL_PROBLEM_HPP

#include "Rodin/Utility.h"

#include "GridFunction.h"
#include "DirichletBC.h"

#include "Problem.h"

namespace Rodin::Variational
{
   template <class TrialFEC, class TestFEC, class OperatorType>
   Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>
   ::Problem(
         TrialFunction<TrialFEC, Traits::Serial>& u,
         TestFunction<TestFEC, Traits::Serial>& v,
         OperatorType* op)
      :  m_bilinearForm(u, v),
         m_linearForm(v),
         m_trialFunctions{{u.getUUID(), std::ref(u)}},
         m_testFunctions{{v.getUUID(), std::ref(v)}},
         m_stiffnessOp(op)
   {
      m_guess = 0.0;
   }

   template <class TrialFEC, class TestFEC, class OperatorType>
   Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>&
   Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>::operator=(const ProblemBody& rhs)
   {
      m_pb.reset(rhs.copy());

      for (auto& bfi : m_pb->getBilinearFormDomainIntegratorList())
         m_bilinearForm.add(*bfi);

      // The LinearFormIntegrator instances have already been moved to the LHS
      for (auto& lfi : m_pb->getLinearFormDomainIntegratorList())
         m_linearForm.add(*lfi);
      for (auto& lfi : m_pb->getLinearFormBoundaryIntegratorList())
         m_linearForm.add(*lfi);

      // Emplace all the solutions
      for (auto& [uuid, u] : m_trialFunctions)
         u.get().emplaceGridFunction();

      return *this;
   }

   template <class TrialFEC, class TestFEC, class OperatorType>
   void Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>::assemble()
   {
      m_linearForm.assemble();
      m_bilinearForm.assemble();

      // We don't support PDE systems (yet)
      {
         assert(m_trialFunctions.size() == 1);
         assert(m_testFunctions.size() == 1);
      }

      auto& [uuid, u] = *m_trialFunctions.begin();

      m_bilinearForm.getHandle()
       .FormLinearSystem(
             m_essTrueDofList,
             u.get().getGridFunction().getHandle(),
             m_linearForm.getHandle(),
             m_stiffnessOp,
             m_guess,
             m_massVector);
   }

   template <class TrialFEC, class TestFEC, class OperatorType>
   Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>&
   Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>::update()
   {
      // We don't support PDE systems (yet)
      {
         assert(m_trialFunctions.size() == 1);
         assert(m_testFunctions.size() == 1);
      }

      // Update all components of the problem
      for (auto& [uuid, u] : m_trialFunctions)
      {
         u.get().getFiniteElementSpace().update();
         u.get().getGridFunction().update();
      }
      m_linearForm.update();
      m_bilinearForm.update();

      // Project values onto the essential boundary and compute essential dofs
      m_essTrueDofList.DeleteAll();

      for (const auto& [uuid, tfValue] : getEssentialBoundary().getTFMap())
      {
         assert(m_trialFunctions.count(uuid) == 1);
         auto& u = m_trialFunctions.at(uuid);
         auto& bdrAttr = tfValue.attributes;
         std::visit(
               [&](auto&& v){ u.get().getGridFunction().projectOnBoundary(*v, bdrAttr); },
               tfValue.value);
         m_essTrueDofList.Append(u.get().getFiniteElementSpace().getEssentialTrueDOFs(bdrAttr));
      }

      for (const auto& [uuid, compMap] : getEssentialBoundary().getTFCompMap())
      {
         assert(m_trialFunctions.count(uuid) == 1);
         auto& u = m_trialFunctions.at(uuid);
         for (const auto& [component, compValue] : compMap)
         {
            auto& bdrAttr = compValue.attributes;
            auto comp = Component(u.get().getGridFunction(), component);
            comp.projectOnBoundary(*compValue.value, bdrAttr);
            m_essTrueDofList.Append(
                  u.get().getFiniteElementSpace().getEssentialTrueDOFs(
                     bdrAttr, component));
         }
      }

      m_essTrueDofList.Sort();
      m_essTrueDofList.Unique();

      return *this;
   }

   template <class TrialFEC, class TestFEC, class OperatorType>
   void Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>::recoverSolution()
   {
      auto& [uuid, u] = *m_trialFunctions.begin();
      auto& a = m_bilinearForm;
      auto& l = m_linearForm;
      a.getHandle().RecoverFEMSolution(m_guess,
            l.getHandle(), u.get().getGridFunction().getHandle());
   }

   template <class TrialFEC, class TestFEC, class OperatorType>
   EssentialBoundary&
   Problem<TrialFEC, TestFEC, OperatorType, Traits::Serial>::getEssentialBoundary()
   {
      return m_pb->getEssentialBoundary();
   }
}

#endif
