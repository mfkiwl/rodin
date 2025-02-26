/*
 *          Copyright Carlos BRITO PACHECO 2021 - 2022.
 * Distributed under the Boost Software License, Version 1.0.
 *       (See accompanying file LICENSE or copy at
 *          https://www.boost.org/LICENSE_1_0.txt)
 */
#ifndef RODIN_GEOMETRY_INDEXGENERATOR_H
#define RODIN_GEOMETRY_INDEXGENERATOR_H

#include <set>
#include <memory>
#include <utility>

#include "Rodin/Copyable.h"
#include "Rodin/Moveable.h"

#include "ForwardDecls.h"
#include "Polytope.h"
#include "Types.h"

namespace Rodin
{
  struct DefaultSentinelT {};
  inline constexpr DefaultSentinelT DefaultSentinel;
}

namespace Rodin::Geometry
{
  class IndexGeneratorBase : public Copyable, public Moveable
  {
    public:
      virtual ~IndexGeneratorBase() = default;
      virtual bool end() const = 0;
      virtual IndexGeneratorBase& operator++() = 0;
      virtual Index operator*() const noexcept = 0;
      virtual IndexGeneratorBase* copy() const noexcept override = 0;
      virtual IndexGeneratorBase* move() noexcept override = 0;
  };

  class EmptyIndexGenerator final : public IndexGeneratorBase
  {
    public:
      constexpr EmptyIndexGenerator() = default;

      constexpr EmptyIndexGenerator(EmptyIndexGenerator&& other)
        :  IndexGeneratorBase(std::move(other))
      {}

      constexpr EmptyIndexGenerator(const EmptyIndexGenerator& other)
        :  IndexGeneratorBase(other)
      {}

      bool end() const override
      {
        return true;
      }

      EmptyIndexGenerator& operator++() override
      {
        assert(false);
        return *this;
      }

      Index operator*() const noexcept override
      {
        assert(false);
        return 0;
      }

      EmptyIndexGenerator* copy() const noexcept override
      {
        return new EmptyIndexGenerator(*this);
      }

      EmptyIndexGenerator* move() noexcept override
      {
        return new EmptyIndexGenerator(std::move(*this));
      }
  };

  class BoundedIndexGenerator final : public IndexGeneratorBase
  {
    public:
      constexpr
      BoundedIndexGenerator(Index start, Index end)
        : m_start(start), m_end(end), m_curr(start)
      {}

      constexpr
      BoundedIndexGenerator(BoundedIndexGenerator&& other)
        :  IndexGeneratorBase(std::move(other)),
          m_start(other.m_start), m_end(other.m_end), m_curr(other.m_curr)
      {}

      constexpr
      BoundedIndexGenerator(const BoundedIndexGenerator& other)
        :  IndexGeneratorBase(other),
          m_start(other.m_start), m_end(other.m_end), m_curr(other.m_end)
      {}

      bool end() const override
      {
        return m_curr == m_end;
      }

      BoundedIndexGenerator& operator++() override
      {
        ++m_curr;
        return *this;
      }

      Index operator*() const noexcept override
      {
        assert(!end());
        return m_curr;
      }

      BoundedIndexGenerator* copy() const noexcept override
      {
        return new BoundedIndexGenerator(*this);
      }

      BoundedIndexGenerator* move() noexcept override
      {
        return new BoundedIndexGenerator(std::move(*this));
      }

    private:
      const Index m_start;
      const Index m_end;
      Index m_curr;
  };

  template <class Iterator>
  class IteratorIndexGenerator : public IndexGeneratorBase
  {
    public:
      IteratorIndexGenerator(Iterator it, Iterator end)
        : m_it(it), m_end(end)
      {}

      IteratorIndexGenerator(IteratorIndexGenerator&& other)
        : IndexGeneratorBase(std::move(other)),
          m_it(std::move(other.m_it)), m_end(std::move(other.m_end))
      {}

      IteratorIndexGenerator(const IteratorIndexGenerator& other)
        : IndexGeneratorBase(other),
          m_it(other.m_it), m_end(other.m_end)
      {}

      bool end() const override
      {
        return m_it == m_end;
      }

      IteratorIndexGenerator& operator++() override
      {
        ++m_it;
        return *this;
      }

      Index operator*() const noexcept override
      {
        assert(!end());
        return *m_it;
      }

      IteratorIndexGenerator* copy() const noexcept override
      {
        return new IteratorIndexGenerator(*this);
      }

      IteratorIndexGenerator* move() noexcept override
      {
        return new IteratorIndexGenerator(std::move(*this));
      }

    private:
      Iterator m_it;
      Iterator m_end;
  };

  class VectorIndexGenerator : public IndexGeneratorBase
  {
    public:
      VectorIndexGenerator(std::vector<Index>&& indices)
        : m_indices(std::move(indices)),
          m_it(m_indices.begin())
      {}

      VectorIndexGenerator(VectorIndexGenerator&& other)
        : IndexGeneratorBase(std::move(other)),
          m_indices(std::move(other.m_indices)),
          m_it(std::move(other.m_it))
      {}

      VectorIndexGenerator(const VectorIndexGenerator& other)
        : IndexGeneratorBase(other),
          m_indices(other.m_indices),
          m_it(other.m_it)
      {}

      bool end() const override
      {
        return m_it == m_indices.end();
      }

      VectorIndexGenerator& operator++() override
      {
        ++m_it;
        return *this;
      }

      Index operator*() const noexcept override
      {
        assert(!end());
        return *m_it;
      }

      VectorIndexGenerator* copy() const noexcept override
      {
        return new VectorIndexGenerator(*this);
      }

      VectorIndexGenerator* move() noexcept override
      {
        return new VectorIndexGenerator(std::move(*this));
      }

    private:
      std::vector<Index> m_indices;
      std::vector<Index>::iterator m_it;
  };

  class SetIndexGenerator : public IndexGeneratorBase
  {
    public:
      SetIndexGenerator(const IndexSet& indices)
        : m_indices(indices),
          m_it(m_indices.begin())
      {}

      SetIndexGenerator(IndexSet&& indices)
        : m_indices(std::move(indices)),
          m_it(m_indices.begin())
      {}

      SetIndexGenerator(SetIndexGenerator&& other)
        : IndexGeneratorBase(std::move(other)),
          m_indices(std::move(other.m_indices)),
          m_it(std::move(other.m_it))
      {}

      SetIndexGenerator(const SetIndexGenerator& other)
        : IndexGeneratorBase(other),
          m_indices(other.m_indices),
          m_it(other.m_it)
      {}

      bool end() const override
      {
        return m_it == m_indices.end();
      }

      SetIndexGenerator& operator++() override
      {
        ++m_it;
        return *this;
      }

      Index operator*() const noexcept override
      {
        assert(!end());
        return *m_it;
      }

      SetIndexGenerator* copy() const noexcept override
      {
        return new SetIndexGenerator(*this);
      }

      SetIndexGenerator* move() noexcept override
      {
        return new SetIndexGenerator(std::move(*this));
      }

    private:
      IndexSet m_indices;
      IndexSet::iterator m_it;
  };
}

#endif
