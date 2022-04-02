#ifndef RODIN_VARIATIONAL_BILINEARFORMINTEGRATOR_H
#define RODIN_VARIATIONAL_BILINEARFORMINTEGRATOR_H

#include <set>
#include <memory>
#include <mfem.hpp>

#include "FormLanguage/Base.h"

#include "ForwardDecls.h"
#include "ShapeFunction.h"

namespace Rodin::Variational
{
   class BilinearFormIntegratorBase
      : public FormLanguage::Buildable<mfem::BilinearFormIntegrator>
   {
      public:
         virtual ~BilinearFormIntegratorBase() = default;

         std::unique_ptr<mfem::BilinearFormIntegrator> build() const override;

         virtual const ShapeFunctionBase<Trial>& getTrialFunction() const = 0;

         virtual const ShapeFunctionBase<Test>& getTestFunction() const = 0;

         /**
          * @brief Gets the attributes of the elements being integrated.
          */
         virtual const std::set<int>& getAttributes() const = 0;

         /**
          * @brief Gets the integration region.
          */
         virtual IntegratorRegion getIntegratorRegion() const = 0;

         virtual void getElementMatrix(
               const mfem::FiniteElement& trial, const mfem::FiniteElement& test,
               mfem::ElementTransformation& trans, mfem::DenseMatrix& mat) const = 0;

         virtual BilinearFormIntegratorBase* copy() const noexcept override = 0;
   };

   class BilinearFormDomainIntegrator : public BilinearFormIntegratorBase
   {
      public:
         BilinearFormDomainIntegrator(
               const ShapeFunctionBase<Trial>& u, const ShapeFunctionBase<Test>& v)
            :  m_u(u.copy()), m_v(v.copy())
         {}

         BilinearFormDomainIntegrator(const BilinearFormDomainIntegrator& other)
            :  BilinearFormIntegratorBase(other),
               m_u(other.m_u->copy()), m_v(other.m_v->copy()),
               m_attrs(other.m_attrs)
         {}

         BilinearFormDomainIntegrator(BilinearFormDomainIntegrator&& other)
            :  BilinearFormIntegratorBase(std::move(other)),
               m_u(std::move(other.m_u)), m_v(std::move(other.m_v)),
               m_attrs(std::move(other.m_attrs))
         {}

         IntegratorRegion getIntegratorRegion() const override
         {
            return IntegratorRegion::Domain;
         }

         /**
          * @brief Specifies the material reference over which to integrate.
          * @returns Reference to self (for method chaining)
          *
          * Specifies the material reference over which the integration should
          * take place.
          */
         BilinearFormDomainIntegrator& over(int attr)
         {
            return over(std::set<int>{attr});
         }

         /**
          * @brief Specifies the material references over which to integrate.
          * @returns Reference to self (for method chaining)
          *
          * Specifies the material references over which the integration should
          * take place.
          */
         BilinearFormDomainIntegrator& over(const std::set<int>& attrs)
         {
            assert(attrs.size() > 0);
            m_attrs = attrs;
            return *this;
         }

         const ShapeFunctionBase<Trial>& getTrialFunction() const override
         {
            return *m_u;
         }

         const ShapeFunctionBase<Test>& getTestFunction() const override
         {
            return *m_v;
         }

         const std::set<int>& getAttributes() const override
         {
            return m_attrs;
         }

         virtual BilinearFormDomainIntegrator* copy() const noexcept override = 0;

      private:
         std::unique_ptr<ShapeFunctionBase<Trial>> m_u;
         std::unique_ptr<ShapeFunctionBase<Test>>  m_v;
         std::set<int> m_attrs;
   };
}

namespace Rodin::Variational::Internal
{
   class ProxyBilinearFormIntegrator : public mfem::BilinearFormIntegrator
  {
      public:
         ProxyBilinearFormIntegrator(const BilinearFormIntegratorBase& bfi)
            : m_bfi(bfi)
         {}

         ProxyBilinearFormIntegrator(const ProxyBilinearFormIntegrator& other)
            : mfem::BilinearFormIntegrator(other),
              m_bfi(other.m_bfi)
         {}

         ProxyBilinearFormIntegrator(ProxyBilinearFormIntegrator&& other)
            : mfem::BilinearFormIntegrator(std::move(other)),
              m_bfi(other.m_bfi)
         {}

         void AssembleElementMatrix(
               const mfem::FiniteElement& fe,
               mfem::ElementTransformation& trans, mfem::DenseMatrix& mat) override
         {
            m_bfi.getElementMatrix(fe, fe, trans, mat);
         }

         void AssembleElementMatrix2(
               const mfem::FiniteElement& trial, const mfem::FiniteElement& test,
                  mfem::ElementTransformation& trans, mfem::DenseMatrix& mat) override
         {
            m_bfi.getElementMatrix(trial, test, trans, mat);
         }
      private:
         const BilinearFormIntegratorBase& m_bfi;
   };
}

#endif
