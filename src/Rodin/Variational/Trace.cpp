#include "MatrixFunction.h"

#include "Trace.h"

namespace Rodin::Variational
{
   Trace::Trace(const MatrixFunctionBase& m)
      : m_matrix(m.copy())
   {
      assert(m.getColumns() == m.getRows());
   }

   Trace::Trace(const Trace& other)
      :  m_matrix(other.m_matrix->copy())
   {}

   double Trace::getValue(mfem::ElementTransformation& trans, const mfem::IntegrationPoint& ip) const
   {
      mfem::DenseMatrix mat;
      m_matrix->getValue(mat, trans, ip);
      return mat.Trace();
   }
}
