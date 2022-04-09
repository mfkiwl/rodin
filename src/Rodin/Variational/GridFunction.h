/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_VARIATIONAL_GRIDFUNCTION_H
#define RODIN_VARIATIONAL_GRIDFUNCTION_H

#include <cmath>
#include <utility>
#include <fstream>
#include <functional>
#include <boost/filesystem.hpp>
#include <type_traits>

#include <mfem.hpp>

#include "Rodin/Core.h"
#include "Rodin/Alert.h"
#include "Rodin/Mesh/SubMesh.h"

#include "ForwardDecls.h"
#include "Restriction.h"
#include "ScalarFunction.h"
#include "VectorFunction.h"
#include "MatrixFunction.h"

namespace Rodin::Variational
{
   /**
    * @internal
    * @brief Abstract class for GridFunction objects.
    */
   class GridFunctionBase
   {
      public:
         void update()
         {
            return getHandle().Update();
         }

         double max() const
         {
            return getHandle().Max();
         }

         double min() const
         {
            return getHandle().Min();
         }

         /**
          * @brief Gets the underlying handle to the mfem::GridFunction object.
          * @returns Reference to the underlying object.
          */
         virtual mfem::GridFunction& getHandle() = 0;

         /**
          * @internal
          * @brief Gets the underlying handle to the mfem::GridFunction object.
          * @returns Constant reference to the underlying object.
          */
         virtual const mfem::GridFunction& getHandle() const = 0;

         virtual FiniteElementSpaceBase& getFiniteElementSpace() = 0;

         virtual const FiniteElementSpaceBase& getFiniteElementSpace() const = 0;

         virtual GridFunctionBase& operator*=(double t) = 0;
         virtual GridFunctionBase& operator/=(double t) = 0;

         virtual GridFunctionBase& project(const ScalarFunctionBase& s, int attr) = 0;
         virtual GridFunctionBase& project(const VectorFunctionBase& s, int attr) = 0;
         virtual GridFunctionBase& project(const ScalarFunctionBase& s, const std::set<int>& attrs) = 0;
         virtual GridFunctionBase& project(const VectorFunctionBase& s, const std::set<int>& attrs) = 0;

         virtual GridFunctionBase& projectOnBoundary(const ScalarFunctionBase& s, int attr) = 0;
         virtual GridFunctionBase& projectOnBoundary(const VectorFunctionBase& s, int attr) = 0;
         virtual GridFunctionBase& projectOnBoundary(const ScalarFunctionBase& s, const std::set<int>& attrs) = 0;
         virtual GridFunctionBase& projectOnBoundary(const VectorFunctionBase& s, const std::set<int>& attrs) = 0;

         virtual std::pair<const double*, int> getData() const = 0;
   };

   /**
    * @brief Represents a grid function which does not yet have an associated
    * finite element space.
    *
    * To obtain the full functionality of the GridFunction class one must call
    * the @ref setFiniteElementSpace(FiniteElementSpace&) method.
    */
   class IncompleteGridFunction
   {
      public:
         /**
          * @brief Constructs an empty grid function with no associated finite
          * element space.
          */
         IncompleteGridFunction() = default;

         /**
          * @brief Associates a finite element space to the function.
          * @param[in] fes Finite element space to which the function belongs
          * to.
          * @tparam FES Finite element space associated
          */
         template <class FEC, class Trait>
         GridFunction<FEC, Trait> setFiniteElementSpace(FiniteElementSpace<FEC, Trait>& fes)
         {
            GridFunction<FEC, Trait> res(fes);
            int size = m_gf.Size();
            res.getHandle().SetDataAndSize(m_gf.StealData(), size);
            return res;
         }

         IncompleteGridFunction& setVectorDimension(int vdim)
         {
            m_vdim = vdim;
            return *this;
         }

         int getVectorDimension() const
         {
            assert(m_vdim);
            return *m_vdim;
         }

         mfem::GridFunction& getHandle()
         {
            return m_gf;
         }

         const mfem::GridFunction& getHandle() const
         {
            return m_gf;
         }

      private:
         mfem::GridFunction m_gf;
         std::optional<int> m_vdim;
   };

   /**
    * @brief Represents a grid function which belongs to some finite element space.
    *
    * @tparam FES Finite element collection to which the function belongs.
    *
    * @note Note that the FES template parameter is typically inferred when
    * initializing the grid function, hence it is not necessary to make it
    * explicit.
    */
   template <class FEC>
   class GridFunction<FEC, Traits::Serial> : public GridFunctionBase
   {
      static_assert(
            std::is_base_of_v<FiniteElementSpaceBase, FiniteElementSpace<FEC>>,
            "FiniteElementSpace<FEC> must be derived from FiniteElementSpaceBase");
      public:
         /**
          * @brief Constructs a grid function on a finite element space.
          * @param[in] fes Finite element space to which the function belongs
          * to.
          */
         GridFunction(FiniteElementSpace<FEC>& fes)
            :  m_fes(fes),
               m_gf(&fes.getHandle())
         {
            m_gf = 0.0;
         }

         /**
          * @brief Copies the grid function.
          * @param[in] other Other grid function to copy.
          */
         GridFunction(const GridFunction& other)
            :  m_fes(other.m_fes),
               m_gf(other.m_gf)
         {}

         GridFunction(GridFunction&& other)
            : m_fes(other.m_fes),
              m_gf(std::move(other.m_gf))
         {}

         GridFunction& operator=(const GridFunction&) = delete;

         /**
          * @brief Move assignment operator.
          */
         GridFunction& operator=(GridFunction&&) = default;

         FiniteElementSpace<FEC>& getFiniteElementSpace() override
         {
            return m_fes;
         }

         /**
          * @brief Gets the finite element space to which the function
          * belongs to.
          * @returns Constant reference to finite element space to which the
          * function belongs to.
          */
         const FiniteElementSpace<FEC>& getFiniteElementSpace() const override
         {
            return m_fes;
         }

         /**
          * @brief Loads the grid function without assigning a finite element
          * space.
          * @param[in] filename Name of file to which the grid function will be
          * written to.
          */
         static IncompleteGridFunction load(const boost::filesystem::path& filename)
         {
            std::ifstream in(filename.string());
            IncompleteGridFunction res;
            int l = 0;

            // Read VDim manually
            std::string buff;
            constexpr std::string_view kw("VDim: ");
            std::optional<int> vdim;
            while (std::getline(in, buff))
            {
               l++;
               std::string::size_type n = buff.find(kw);
               if (n != std::string::npos)
               {
                  vdim = std::stoi(buff.substr(kw.length()));
                  break;
               }
            }
            if (vdim)
               res.setVectorDimension(*vdim);
            else
               Alert::Exception(
                  "VDim keyword not found while loading GridFunction").raise();

            // Advance until we see the "Ordering" keyword
            while (std::getline(in, buff))
            {
               l++;
               if (buff.find("Ordering: ") == std::string::npos)
                  break;
            }

            // Skip empty lines
            while (std::getline(in, buff))
            {
               l++;
               if (!buff.empty())
                  break;
            }

            // Count lines
            int size = 1;
            while (std::getline(in, buff))
               size++;

            // Load actual file
            in.clear();
            in.seekg(0);
            for (int i = 0; i < l - 1; i++)
               std::getline(in, buff);
            res.getHandle().Load(in, size);

            return res;
         }

         /**
          * @brief Saves the grid function in MFEMv1.0 format.
          * @param[in] filename Name of file to which the solution file will be
          * written.
          */
         void save(const boost::filesystem::path& filename)
         {
            m_gf.Save(filename.string().c_str());
         }

         /**
          * @brief Sets the data of the grid function and assumes ownership.
          *
          * @param[in] data Data array
          * @param[in] size Size of the data array
          *
          * @returns Reference to self (for method chaining)
          */
         GridFunction& setData(std::unique_ptr<double[]> data, int size)
         {
            m_gf.SetDataAndSize(data.release(), size);
            return *this;
         }

         /**
          * @brief Gets the raw data and size of the grid function.
          * @returns `std::pair{data, size}`
          */
         std::pair<const double*, int> getData() const override
         {
            return {static_cast<const double*>(m_gf.GetData()), m_gf.Size()};
         }

         GridFunction& operator=(double v)
         {
            getHandle() = v;
            return *this;
         }

         template <class T>
         std::enable_if_t<
            std::is_base_of_v<ScalarFunctionBase, std::remove_reference_t<T>> ||
            std::is_base_of_v<VectorFunctionBase, std::remove_reference_t<T>>, GridFunction&>
         operator=(T&& v)
         {
            return project(std::forward<T>(v));
         }

         GridFunction& project(const ScalarFunctionBase& s, int attr) override
         {
            return project(s, std::set<int>{attr});
         }

         GridFunction& project(const VectorFunctionBase& v, int attr) override
         {
            return project(v, std::set<int>{attr});
         }

         GridFunction& projectOnBoundary(const ScalarFunctionBase& s, int attr) override
         {
            return projectOnBoundary(s, std::set<int>{attr});
         }

         GridFunction& projectOnBoundary(const VectorFunctionBase& v, int attr) override
         {
            return projectOnBoundary(v, std::set<int>{attr});
         }

         GridFunction& project(const ScalarFunctionBase& s, const std::set<int>& attrs = {}) override
         {
            assert(getFiniteElementSpace().getVectorDimension() == 1);
            auto iv = s.build();

            if (attrs.size() == 0)
               getHandle().ProjectCoefficient(*iv);
            else
            {
               int maxAttr = getFiniteElementSpace()
                            .getMesh()
                            .getHandle().attributes.Max();
               mfem::Array<int> marker(maxAttr);
               marker = 0;
               for (const auto& attr : attrs)
               {
                  assert(attr - 1 < maxAttr);
                  marker[attr - 1] = 1;
               }
               getHandle().ProjectCoefficient(*iv, marker);
            }
            return *this;
         }

         GridFunction& project(
               const VectorFunctionBase& s,
               const std::set<int>& attrs = {}) override
         {
            assert(getFiniteElementSpace().getVectorDimension() == s.getDimension());
            auto iv = s.build();

            if (attrs.size() == 0)
               getHandle().ProjectCoefficient(*iv);
            else
            {
               int maxAttr = getFiniteElementSpace()
                            .getMesh()
                            .getHandle().attributes.Max();
               mfem::Array<int> marker(maxAttr);
               marker = 0;
               for (const auto& attr : attrs)
               {
                  assert(attr - 1 < maxAttr);
                  marker[attr - 1] = 1;
               }
               getHandle().ProjectCoefficient(*iv, marker);
            }
            return *this;
         }

         GridFunction& projectOnBoundary(
               const ScalarFunctionBase& s, const std::set<int>& attrs = {}) override
         {
            assert(getFiniteElementSpace().getVectorDimension() == 1);
            auto iv = s.build();
            int maxBdrAttr = getFiniteElementSpace()
                            .getMesh()
                            .getHandle().bdr_attributes.Max();
            mfem::Array<int> marker(maxBdrAttr);
            if (attrs.size() == 0)
            {
               marker = 1;
               getHandle().ProjectBdrCoefficient(*iv, marker);
            }
            else
            {
               marker = 0;
               for (const auto& attr : attrs)
               {
                  assert(attr - 1 < maxBdrAttr);
                  marker[attr - 1] = 1;
               }
               getHandle().ProjectBdrCoefficient(*iv, marker);
            }
            return *this;
         }

         GridFunction& projectOnBoundary(
               const VectorFunctionBase& v, const std::set<int>& attrs = {}) override
         {
            assert(getFiniteElementSpace().getVectorDimension() == v.getDimension());
            auto iv = v.build();
            int maxBdrAttr = getFiniteElementSpace()
                            .getMesh()
                            .getHandle().bdr_attributes.Max();
            mfem::Array<int> marker(maxBdrAttr);
            if (attrs.size() == 0)
            {
               marker = 1;
               getHandle().ProjectBdrCoefficient(*iv, marker);
            }
            else
            {
               marker = 0;
               for (const auto& attr : attrs)
               {
                  assert(attr - 1 < maxBdrAttr);
                  marker[attr - 1] = 1;
               }
               getHandle().ProjectBdrCoefficient(*iv, marker);
            }
            return *this;
         }

         /**
          * @brief Projects the restriction of a scalar coefficient on the given GridFunction.
          * @note The GridFunction must be scalar valued.
          * @param[in] s Scalar coefficient to project
          * @returns Reference to self
          */
         GridFunction& project(const Restriction<ScalarFunctionBase>& s)
         {
            assert(getFiniteElementSpace().getVectorDimension() == 1);
            auto iv = s.getScalarFunction().build();
            getHandle() = std::numeric_limits<double>::quiet_NaN();
            mfem::Array<int> vdofs;
            mfem::Vector vals;
            const auto& fes = getFiniteElementSpace().getHandle();
            const auto& attrs = s.getAttributes();
            for (int i = 0; i < fes.GetNE(); i++)
            {
               if (attrs.count(fes.GetAttribute(i)) > 0)
               {
                  fes.GetElementVDofs(i, vdofs);
                  vals.SetSize(vdofs.Size());
                  fes.GetFE(i)->Project(
                        *iv, *fes.GetElementTransformation(i), vals);
                  getHandle().SetSubVector(vdofs, vals);
               }
            }
            return *this;
         }

         /**
          * @brief Transfers the grid function from one finite element space to
          * another.
          */
         template <class OtherFEC>
         void transfer(GridFunction<OtherFEC, Traits::Serial>& other)
         {
            assert(getFiniteElementSpace().getVectorDimension() ==
                  other.getFiniteElementSpace().getVectorDimension());
            if (getFiniteElementSpace().getMesh().isSubMesh())
            {
               // If we are here the this means that we are in a submesh of the
               // underlying target finite element space. Hence we should seek
               // out to copy the grid function at the corresponding nodes
               // given by the vertex map given in the Submesh object.
               auto& submesh = static_cast<const SubMesh<Traits::Serial>&>(
                     getFiniteElementSpace().getMesh());
               if (&submesh.getParent() == &other.getFiniteElementSpace().getMesh())
               {
                  int vdim = getFiniteElementSpace().getVectorDimension();
                  const auto& s2pv = submesh.getVertexMap();
                  if (vdim == 1)
                  {
                     int size = getHandle().Size();
                     for (int i = 0; i < size; i++)
                        other.getHandle()[i] = getHandle()[s2pv.at(i)];
                  }
                  else
                  {
                     int nv = getFiniteElementSpace().getHandle().GetNV();
                     int pnv = other.getFiniteElementSpace().getHandle().GetNV();

                     assert(getFiniteElementSpace().getHandle().GetOrdering() ==
                              getFiniteElementSpace().getHandle().GetOrdering());
                     switch(getFiniteElementSpace().getHandle().GetOrdering())
                     {
                        case mfem::Ordering::byNODES:
                        {
                           for (int i = 0; i < vdim; i++)
                              for (int j = 0; j < nv; j++)
                                 other.getHandle()[s2pv.at(j) + i * pnv] = getHandle()[j + i * nv];
                           return;
                        }
                        case mfem::Ordering::byVDIM:
                        {
                           for (int i = 0; i < nv; i++)
                              for (int j = 0; j < vdim; j++)
                                 other.getHandle()[s2pv.at(i) * vdim + j] = getHandle()[i * vdim + j];
                           return;
                        }
                     }
                  }
               }
            }
            else
            {
               // If the meshes are equal or where obtained from refinements
               // one could use the mfem functionality to make a GridTransfer.
               // Alternatively, if the mesh is equal but the finite element
               // spaces are not, mfem also contains the TransferOperator class
               // which can come in useful.
               Alert::Exception("Unimplemented. Sorry.").raise();
            }
         }

         GridFunction& operator*=(double t) override
         {
            m_gf *= t;
            return *this;
         }

         GridFunction& operator/=(double t) override
         {
            m_gf /= t;
            return *this;
         }

         mfem::GridFunction& getHandle() override
         {
            return m_gf;
         }

         const mfem::GridFunction& getHandle() const override
         {
            return m_gf;
         }
      private:
         FiniteElementSpace<FEC>& m_fes;
         mfem::GridFunction m_gf;
   };
   template <class FEC, class Trait>
   GridFunction(FiniteElementSpace<FEC, Trait>&) -> GridFunction<FEC, Trait>;
}

#endif
