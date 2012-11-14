/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2012 Lawrence Livermore National Laboratory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#ifndef HASH_H
#define HASH_H

#include <string>

#include "assert.h"
#include "ptr.h"

#include "hash-function.h"  // typedef ns3::Hash32_t, ns3::Hash64_t
#include "hash-murmur3.h"
#include "hash-fnv.h"

namespace ns3 {

/**
 *  \ingroup hash
 *
 *  \brief Generic Hash function interface
 *
 *  This class provides a generic interface for computing hashes
 *  of buffers.  Various getters return hashes of different lengths.
 *  The choice of hash function can be made at construction by
 *    \code
 *    Hasher hasher = Hasher ( Create<Hash::Function::Fnv1a> () );
 *    Hash32_t hash = Hasher.GetHash32 (data);
 *    \endcode
 *
 *  The available implementations are documented in group hash.
 *  The default implementation is Murmur3.  FNV1a is also available.
 *
 *  In addition to this class interface, global functions are
 *  defined which use the default hash implementation.
 *
 * \internal
 *
 *  Would be nice to offer longer hashes.  \c uint128_t looks doable,
 *  except that our fallback \c int64x64_t implementation doesn't
 *  offer \c unsigned.
 *
 *  Longer hashes require returning a byte buffer of some sort,
 *  but our \ref buffer class seems a bit overkill for this case.
 *
 */
class Hasher
{
public:
  /**
   * Constructor using the default implementation
   */
  Hasher ();
  /**
   * Constructor using the supplied implementation
   *
   * \param [in] hp Ptr<Hash::Implementation> to the desired implementation
   */
  Hasher (Ptr<Hash::Implementation> hp);
  /**
   * Compute 32-bit hash of a byte buffer
   *
   * \param [in] buffer pointer to the beginning of the buffer
   * \param [in] size length of the buffer, in bytes
   * \return 32-bit hash of the buffer
   */
  Hash32_t  GetHash32  (const char * buffer, const size_t size);
  /**
   * Compute 64-bit hash of a byte buffer
   *
   * \param [in] buffer pointer to the beginning of the buffer
   * \param [in] size length of the buffer, in bytes
   * \return 64-bit hash of the buffer
   */
  Hash64_t  GetHash64  (const char * buffer, const size_t size);

  /**
   * Compute 32-bit hash of a string
   *
   * \param [in] s string to hash
   * \return 32-bit hash of the string
   */
  Hash32_t  GetHash32  (const std::string s);
  /**
   * Compute 64-bit hash of a string
   *
   * \param [in] s string to hash
   * \return 64-bit hash of the string
   */
  Hash64_t  GetHash64  (const std::string s);
  /**
   * Restore initial state
   *
   * \return this
   */
  Hasher & clear (void);
  
private:
  Ptr<Hash::Implementation> m_impl;    /** Hash implementation */
  
};  // Hasher


/*************************************************
 **  Global functions declarations
 ************************************************/

/**
 * \ingroup hash
 *
 * Compute 32-bit hash of a byte buffer, using the default hash function
 *
 * \param [in] buffer pointer to the beginning of the buffer
 * \param [in] size length of the buffer, in bytes
 * \return 32-bit hash of the buffer
 */
Hash32_t Hash32 (const char * buffer, const size_t size);
/**
 * \ingroup hash
 *
 * Compute 64-bit hash of a byte buffer, using the default hash function
 *
 * \param [in] buffer pointer to the beginning of the buffer
 * \param [in] size length of the buffer, in bytes
 * \return 64-bit hash of the buffer
 */
Hash64_t Hash64 (const char * buffer, const size_t size);

/**
 * \ingroup hash
 *
 * Compute 32-bit hash of a string, using the default hash function
 *
 * \param [in] s string to hash
 * \return 32-bit hash of the string
 */
Hash32_t Hash32 (const std::string s);
/**
 * \ingroup hash
 *
 * Compute 64-bit hash of a string, using the default hash function
 *
 * \param [in] s string to hash
 * \return 64-bit hash of the string
 */
Hash64_t Hash64 (const std::string s);

}  // namespace ns3


/*************************************************
 **  Inline implementations for rvo
 ************************************************/

namespace ns3 {
  
/*************************************************
 class Hash implementation, inlined for rvo
*/

inline
Hash32_t
Hasher::GetHash32  (const char * buffer, const size_t size)
{
  NS_ASSERT (m_impl != 0);
  return m_impl->GetHash32  (buffer, size);
}

inline
Hash64_t
Hasher::GetHash64  (const char * buffer, const size_t size)
{
  NS_ASSERT (m_impl != 0);
  return m_impl->GetHash64  (buffer, size);
}

inline
Hash32_t
Hasher::GetHash32  (const std::string s)
{
  NS_ASSERT (m_impl != 0);
  return m_impl->GetHash32  (s.c_str (), s.size ());
}

inline
Hash64_t
Hasher::GetHash64  (const std::string s)
{
  NS_ASSERT (m_impl != 0);
  return m_impl->GetHash64  (s.c_str (), s.size ());
}


/*************************************************
  Global hash functions, inlined for rvo
*/

inline
Hash32_t
Hash32 (const char * buffer, const size_t size)
{
  return Hasher().GetHash32 (buffer, size);
}

inline
Hash64_t
Hash64 (const char * buffer, const size_t size)
{
  return Hasher().GetHash64 (buffer, size);
}

inline
Hash32_t
Hash32 (const std::string s)
{
  return Hasher().GetHash32 (s);
}

inline
Hash64_t
Hash64 (const std::string s)
{
  return Hasher().GetHash64 (s);
}


}  // namespace ns3

#endif /* HASH_H */
