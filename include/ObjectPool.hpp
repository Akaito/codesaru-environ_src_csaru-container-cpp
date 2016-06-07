#pragma once

#include <cstdint>

/*
	Allocated: | generation | object    |
	Free:      | generation | undefined |
*/

namespace CSaruContainer {

template <typename T_Type>
class ObjectPool {
public: // Types and constants.
	struct BlockEntry {
		uint32_t  generation = 0; // Significantly faster to ditch this and memset on pool block alloc?
		uint8_t   object[sizeof(T_Type)];
	};


	struct Handle {
		ObjectPool * pool       = nullptr;
		uint32_t     index      = 0;
		uint32_t     generation = 0;

		T_Type * Get () {
			if (!pool || !generation)
				return nullptr;
			return pool->Get(index, generation);
		}
	};

private: // Data.
	BlockEntry * m_pool            = nullptr; // multiple blocks not yet supported; here's just a single one
	uint32_t *   m_indices         = nullptr; // indices of alloc'd/free entries in the pool
	uint32_t     m_freeIndex       = 0;       // index of first free object index
	uint32_t     m_objectsPerBlock = 0;

private: // Helpers.
	bool AllocPoolBlock () {
		if (m_pool)
			return false;

		m_pool    = new BlockEntry[m_objectsPerBlock];
		m_indices = new uint32_t[m_objectsPerBlock];

		// prepare indices
		for (uint32_t i = 0; i < m_objectsPerBlock; ++i)
			m_indices[i] = i;

		return true;
	}

public: // Construction.
	ObjectPool (
		uint64_t objectsPerBlock = 4096 / sizeof(BlockEntry)
	) :
		// could also allow this to be set anytime the pool is empty
		m_objectsPerBlock(objectsPerBlock)
	{}

	~ObjectPool () {
		DestroyAll();
	}

public: // Queries.

public: // Commands.
	bool Prepare () {
		if (!AllocPoolBlock())
			return false;
		m_freeIndex = 0;
		return true;
	}


	void DestroyAll () {
		while (m_freeIndex > 0)
			Free(m_indices[m_freeIndex - 1]);

		delete [] m_indices;
		m_indices = nullptr;
		delete [] m_pool;
		m_pool = nullptr;
	}


	template <typename... T_Args>
	T_Type * Alloc (Handle * optHandleOut, T_Args... constructorArgs) {
		if (!m_pool || !m_indices)
			return nullptr;

		if (m_freeIndex >= m_objectsPerBlock) {
			if (optHandleOut) {
				optHandleOut->pool       = this;
				optHandleOut->index      = uint32_t(-1);
				optHandleOut->generation = 0;
			}
			return nullptr;
		}

		uint32_t     index = m_indices[m_freeIndex++];
		BlockEntry & entry = m_pool[index];
		++entry.generation;
		T_Type * obj = new (&entry.object) T_Type(constructorArgs...);

		if (optHandleOut) {
			optHandleOut->pool       = this;
			optHandleOut->index      = index;
			optHandleOut->generation = entry.generation;
		}
		return obj;
	}


	T_Type * Get (uint32_t index, uint32_t generation = 0) {
		if (index >= m_objectsPerBlock)
			return nullptr;

		BlockEntry & entry = m_pool[index];
		if (generation && generation != entry.generation)
			return nullptr;

		T_Type * obj = reinterpret_cast<T_Type *>(&entry.object);
		return obj;
	}


	T_Type * Enum (uint32_t * enumIndexInOut) {
		if (!enumIndexInOut)
			return nullptr;

		if (*enumIndexInOut >= m_freeIndex)
			return nullptr;

		return Get(m_indices[*enumIndexInOut++]);
	}


	bool Free (uint32_t index, uint32_t generation = 0) {
		if (!m_freeIndex)
			return false;

		BlockEntry & entry = m_pool[index];
		if (generation && generation != entry.generation)
			return false;

		// Find index in m_indices, to swap the last alloc'd index-index with the one just free'd.
		for (uint32_t indexIndex = m_freeIndex - 1; indexIndex < m_freeIndex; --indexIndex) {
			if (m_indices[indexIndex] != index)
				continue;

			T_Type * obj = reinterpret_cast<T_Type *>(&entry.object);
			obj->~T_Type();

			// swap last still-alive index with just-free'd index
			uint32_t stillAliveIndex = m_indices[m_freeIndex - 1];
			m_indices[m_freeIndex - 1] = index;
			m_indices[indexIndex]      = stillAliveIndex;
			--m_freeIndex;

			return true;
		}

		return false;
	}

};

} // namespace CSaruContainer

