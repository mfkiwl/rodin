/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_MESH_MESH_H
#define RODIN_MESH_MESH_H

#include <set>
#include <string>

#include <mfem.hpp>

#include "Rodin/Configure.h"

#ifdef RODIN_USE_MPI
#include <boost/mpi.hpp>
#endif
#include <boost/filesystem.hpp>

#include "Rodin/Variational/ForwardDecls.h"

#include "ForwardDecls.h"

namespace Rodin
{
   class MeshBase
   {
      public:
         /**
          * @brief Gets the dimension of the ambient space
          * @returns Dimension of the space which the mesh is embedded in
          * @see getDimension() const
          */
         int getSpaceDimension() const;

         /**
          * @brief Gets the dimension of the elements
          * @returns Dimension of the elements
          * @see getSpaceDimension() const
          */
         int getDimension() const;

         /**
          * @brief Performs a uniform refinement over all mesh elements
          */
         void refine();

         std::set<int> getAttributes() const;

         std::set<int> getBoundaryAttributes() const;

         virtual void save(const boost::filesystem::path& filename);

         /**
          * @brief Displaces the mesh nodes by the displacement @f$ u @f$.
          * @param[in] u Displacement at each node
          *
          * Given a grid function @f$ u @f$, the method will perform the
          * displacement
          * @f[
          *    x \mapsto x + u(x)
          * @f]
          * at each node @f$ x @f$ of the mesh.
          *
          * @note The range dimension of @f$ u @f$ must be equal to the
          * mesh dimension.
          *
          * @returns Reference to this (for method chaining)
          */
         MeshBase& displace(const Variational::GridFunctionBase& u);

         /**
          * @brief Gets the maximum number @f$ t @f$ by which the mesh will
          * remain valid, when displacing by @f$ u @f$.
          * @param[in] u Displacement at each node
          *
          * This function will calculate the maximum number @f$ t @f$ so that
          * the displacement
          * @f[
          *    x \mapsto x + t u(x)
          * @f]
          * gives a valid mesh without actually displacing the mesh.
          *
          * @note The range dimension of @f$ u @f$ must be equal to the
          * mesh dimension.
          *
          * @returns Maximum time so that the mesh remains valid.
          */
         double getMaximumDisplacement(const Variational::GridFunctionBase& u);

         /**
          * @brief Gets the total volume of the mesh.
          */
         double getVolume();

         /**
          * @brief Gets the sum of the volumes of the elements given by the
          * specified attribute.
          * @param[in] attr Attribute of elements
          * @returns Sum of element volumes
          * @note If the element attribute does not exist then this function
          * will return 0 as the volume.
          */
         double getVolume(int attr);

         /**
          * @brief Indicates whether the mesh is a submesh or not.
          * @returns True if mesh is a submesh, false otherwise.
          *
          * A Mesh which is also a SubMesh may be casted into down to access
          * the SubMesh functionality. For example:
          * @code{.cpp}
          * if (mesh.isSubMesh())
          * {
          *    // Cast is well defined
          *    auto& submesh = static_cast<SubMesh&>(mesh);
          * }
          * @endcode
          *
          */
         virtual bool isSubMesh() const = 0;

         virtual bool isParallel() const = 0;

         /**
          * @internal
          * @brief Gets the underlying handle for the internal mesh.
          * @returns Reference to the underlying mfem::Mesh.
          */
         virtual mfem::Mesh& getHandle() = 0;

         /**
          * @internal
          * @brief Gets the underlying handle for the internal mesh.
          * @returns Constant reference to the underlying mfem::Mesh.
          */
         virtual const mfem::Mesh& getHandle() const = 0;
   };

   /**
    * @brief Represents the subdivision of some domain into faces of (possibly)
    * different geometries.
    */
   template <>
   class Mesh<Traits::Serial> : public MeshBase
   {
      public:
         /**
          * @brief Loads a mesh from file.
          * @param[in] filename Name of file to read
          */
         Mesh& load(const boost::filesystem::path& filename);

         /**
          * @brief Constructs an empty mesh with no elements.
          */
         Mesh() = default;

         /**
          * @brief Move constructs the mesh from another mesh.
          */
         Mesh(Mesh&& other) = default;

         /**
          * @brief Performs a deep copy of another mesh.
          */
         Mesh(const Mesh& other);

         /**
          * @brief Move assigns the mesh from another mesh.
          */
         Mesh& operator=(Mesh&& other) = default;

         /**
          * @brief Trims the elements with the given material reference.
          * @param[in] attr Attribute to trim
          * @param[in] bdr Boundary label which will be assigned to the
          * exterior boundary.
          * @returns SubMesh object to the remaining region of the mesh
          *
          * Convenience function to call trim(const std::set<int>&, int) with
          * only one attribute.
          */
         SubMesh<Traits::Serial> trim(int attr, int bdrLabel);

         /**
          * @brief Trims the elements with the given material references.
          * @param[in] attr Attributes to trim
          * @param[in] bdr Boundary label which will be assigned to the
          * exterior boundary.
          * @returns SubMesh object to the remaining region of the mesh
          *
          * This function will trim the current mesh and return a Submesh
          * object containing the elements which were not trimmed from the
          * original mesh.
          */
         SubMesh<Traits::Serial> trim(const std::set<int>& attrs, int bdrLabel);

         /**
          * @brief Skins the mesh to obtain its boundary mesh
          * @returns SubMesh object to the boundary region of the mesh
          *
          * This function will "skin" the mesh to return the mesh boundary as a
          * new SubMesh object. The new mesh will be embedded in the original
          * space dimension.
          */
         SubMesh<Traits::Serial> skin();

         virtual bool isSubMesh() const override
         {
            return false;
         }

         bool isParallel() const override
         {
            return false;
         }

         mfem::Mesh& getHandle() override;

         const mfem::Mesh& getHandle() const override;

         /**
          * @internal
          * @brief Move constructs a Rodin::Mesh from an mfem::Mesh.
          */
         explicit
         Mesh(mfem::Mesh&& mesh);

#ifdef RODIN_USE_MPI
         /**
          * @brief Set the MPI Communicator
          */
         Mesh<Traits::Parallel> parallelize(boost::mpi::communicator comm);
#endif
      private:
         mfem::Mesh m_mesh;
   };

#ifdef RODIN_USE_MPI
   template <>
   class Mesh<Traits::Parallel> : public MeshBase
   {
      public:
         /**
          * @internal
          */
         Mesh(boost::mpi::communicator comm, Mesh<Traits::Serial>& serialMesh)
            :  m_comm(comm),
               m_mesh(mfem::ParMesh(m_comm, serialMesh.getHandle()))
         {}

         Mesh(const Mesh&) = delete;

         Mesh(Mesh&&) = default;

         const boost::mpi::communicator& getMPIComm() const
         {
            return m_comm;
         }

         virtual bool isSubMesh() const override
         {
            return false;
         }

         bool isParallel() const override
         {
            return false;
         }

         mfem::ParMesh& getHandle() override
         {
            return m_mesh;
         }

         const mfem::ParMesh& getHandle() const override
         {
            return m_mesh;
         }
      private:
         boost::mpi::communicator m_comm;
         mfem::ParMesh m_mesh;
   };
#endif
}

#endif
