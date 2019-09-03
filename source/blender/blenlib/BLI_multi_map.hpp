/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup bli
 *
 * A multimap is a map that allows storing multiple values per key.
 */

#pragma once

#include "BLI_map.hpp"
#include "BLI_array_ref.hpp"
#include "BLI_vector.hpp"

namespace BLI {

template<typename K, typename V, uint N = 4> class MultiMap {
 private:
  struct Entry {
    uint offset;
    uint length;
    uint capacity;

    ArrayRef<V> get_slice(ArrayRef<V> array) const
    {
      return array.slice(offset, length);
    }
  };

  Map<K, Entry> m_map;
  Vector<V, N> m_elements;

 public:
  MultiMap() = default;

  uint key_amount() const
  {
    return m_map.size();
  }

  uint value_amount(const K &key) const
  {
    return this->lookup_default(key).size();
  }

  bool add(const K &key, const V &value)
  {
    BLI_STATIC_ASSERT(std::is_trivially_destructible<V>::value, "");
    bool newly_inserted = m_map.add_or_modify(
        key,
        /* Insert new key with value. */
        [this, &key, &value]() -> Entry {
          UNUSED_VARS(key);
          uint offset = m_elements.size();
          m_elements.append(value);
          return {offset, 1, 1};
        },
        /* Append new value for existing key. */
        [this, &value](Entry &entry) {
          if (entry.length < entry.capacity) {
            m_elements[entry.offset + entry.length] = value;
            entry.length += 1;
          }
          else {
            uint new_offset = m_elements.size();
            uint new_capacity = entry.capacity * 2;

            m_elements.reserve(m_elements.size() + new_capacity);

            /* Copy the existing elements to the end. */
            ArrayRef<V> old_values = entry.get_slice(m_elements);
            m_elements.extend_unchecked(old_values);

            /* Insert the new value. */
            m_elements.append_unchecked(value);

            /* Reserve the remaining capacity for this entry. */
            m_elements.increase_size_unchecked(entry.length - 1);

            entry.offset = new_offset;
            entry.length += 1;
            entry.capacity = new_capacity;
          }
        });
    return newly_inserted;
  }

  void add_multiple(const K &key, ArrayRef<V> values)
  {
    for (const V &value : values) {
      this->add(key, value);
    }
  }

  void add_new(const K &key, const V &value)
  {
    BLI_assert(!m_map.contains(key));
    uint offset = m_elements.size();
    m_elements.append(value);
    m_map.add_new(key, {offset, 1, 1});
  }

  void add_multiple_new(const K &key, ArrayRef<V> values)
  {
    BLI_assert(!m_map.contains(key));
    uint offset = m_elements.size();
    m_elements.extend(values);
    m_map.add_new(key, {offset, values.size(), values.size()});
  }

  ArrayRef<V> lookup(const K &key) const
  {
    BLI_assert(m_map.contains(key));
    return this->lookup_default(key);
  }

  ArrayRef<V> lookup_default(const K &key, ArrayRef<V> default_array = ArrayRef<V>()) const
  {
    const Entry *entry = m_map.lookup_ptr(key);
    if (entry == nullptr) {
      return default_array;
    }
    else {
      return entry->get_slice(m_elements);
    }
  }

  bool contains(const K &key) const
  {
    return m_map.contains(key);
  }

  typename Map<K, Entry>::KeyIterator keys() const
  {
    return m_map.keys();
  }

  template<typename FuncT> void foreach_value(const FuncT &func)
  {
    for (Entry &entry : m_map.values()) {
      for (const V &value : entry.get_slice(m_elements)) {
        func(value);
      }
    }
  }
};

} /* namespace BLI */