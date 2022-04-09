/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_TRACE_H
#define RODIN_VARIATIONAL_TRACE_H

#include "ForwardDecls.h"

#include "ScalarFunction.h"

namespace Rodin::Variational
{
   /**
    * @brief Represents the trace @f$ \mathrm{tr}(A) @f$ of some square matrix
    * @f$ A @f$.
    *
    * The trace of an @f$ n \times n @f$ matrix @f$ A @f$ is defined as
    * @f[
    *    \mathrm{tr}(A) = \sum_{i = 1}^n A_{ii}
    * @f]
    */
   class Trace : public ScalarFunctionBase
   {
      public:
         /**
          * @brief Constructs the Trace of the given matrix
          * @param[in] m Square matrix
          */
         Trace(const MatrixFunctionBase& m);

         Trace(const Trace& other);

         double getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) const override;

         Trace* copy() const noexcept override
         {
            return new Trace(*this);
         }
      private:
         std::unique_ptr<MatrixFunctionBase> m_matrix;
   };
}

#endif
