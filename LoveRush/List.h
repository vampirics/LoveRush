#pragma once

#include <stdint.h>

template< typename Type, uint8_t Capacity >
class List
{
public:
	static_assert(Capacity > 0, "Attempt to create List with Capacity less than 1");
	static_assert(Capacity < 128, "Attempt to create List with Capacity greater than 127");

	using ItemType = Type;
	using IndexType = uint8_t;
	using IndexOfType = int8_t;

	using ValueType = Type;

	constexpr static const IndexType FirstIndex = 0;
	constexpr static const IndexType LastIndex = Capacity - 1;

private:
	ItemType items[Capacity];
	IndexType next;

public:
	List(void);

	bool isEmpty(void) const; // O(1)
	bool isFull(void) const; // O(1)

	IndexType getCount(void) const; // O(1)
	constexpr IndexType getCapacity(void) const; // O(1)
	constexpr IndexType getFirstIndex(void) const; // O(1)
	// Note that this is different to the internal LastIndex,
	// this version is based on getCount rather than getCapacity
	IndexType getLastIndex(void) const; // O(1)

	bool add(ItemType && item); // O(1)
	bool add(const ItemType & item); // O(1)
	bool remove(const ItemType & item); // O(n)
	void removeAt(const IndexType & index); // O(n)
	bool insert(const IndexType & index, const ItemType & item); // O(n)
	
	bool resize(const IndexType & newSize); // O(n)
	
	void clear(void); // O(n)
	bool contains(const ItemType & item) const; // O(n)

	// Returns -1 if item not found
	IndexOfType indexOf(const ItemType & item) const; // O(n)

	ItemType & operator [] (const IndexType & index); // O(1)
	const ItemType & operator [] (const IndexType & index) const; // O(1)
};


//
// Definition
//

template< typename ItemType, uint8_t Capacity >
List< ItemType, Capacity >::List(void)
	: items(), next(0)
{
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::isEmpty(void) const // O(1)
{
	return (this->next == FirstIndex);
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::isFull(void) const // O(1)
{
	return (this->next == this->getCapacity());
}

template< typename ItemType, uint8_t Capacity >
typename List< ItemType, Capacity >::IndexType List< ItemType, Capacity >::getCount(void) const // O(1)
{
	return this->next;
}

template< typename ItemType, uint8_t Capacity >
constexpr typename List< ItemType, Capacity >::IndexType List< ItemType, Capacity >::getCapacity(void) const // O(1)
{
	return static_cast<IndexType>(Capacity);
}

template< typename ItemType, uint8_t Capacity >
constexpr typename List< ItemType, Capacity >::IndexType List< ItemType, Capacity >::getFirstIndex(void) const // O(1)
{
	return static_cast<IndexType>(FirstIndex);
}

template< typename ItemType, uint8_t Capacity >
typename List< ItemType, Capacity >::IndexType List< ItemType, Capacity >::getLastIndex(void) const // O(1)
{
	// Returns firstIndex if the list is empty to allow use in reverse iteration
	return this->isEmpty() ? this->getFirstIndex() : static_cast<IndexType>(this->next - 1);
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::add(ItemType && item) // O(1)
{
	if (this->isFull())
		return false;

	this->items[this->next] = item; // should be std::move
	++this->next;
	return true;
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::add(const ItemType & item) // O(1)
{
	if (this->isFull())
		return false;

	this->items[this->next] = item;
	++this->next;
	return true;
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::remove(const ItemType & item) // O(n)
{
	for(IndexType i = 0; i < this->next; ++i)
		if (this->items[i] == item)
		{
			--this->next;
			while (i < this->next)
			{
				this->items[i] = this->items[i + 1]; // should be std::move
				++i;
			}
			(&this->items[this->next])->~ItemType();
			return true;
		}
	return false;
}

template< typename ItemType, uint8_t Capacity >
void List< ItemType, Capacity >::removeAt(const typename List< ItemType, Capacity >::IndexType & index) // O(n)
{
	//DEBUG_ASSERT(index < this->getCapacity());
	//DEBUG_ASSERT(index < this->next);

	--this->next;
	for (IndexType i = index; i < this->next; ++i)
			this->items[i] = this->items[i + 1];// should be std::move
	(&this->items[this->next])->~ItemType();
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::insert(const typename List< ItemType, Capacity >::IndexType & index, const ItemType & item) // O(n)
{
	//DEBUG_ASSERT(index < this->getCapacity());
	//DEBUG_ASSERT(index < this->next);

	if (this->isFull())
		return false;

	++this->next;
	for (IndexType i = index + 1; i < this->next; ++i)
		this->items[i] = Move(this->items[i - 1]);
	this->items[index] = item;
	return true;
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::resize(const typename List< ItemType, Capacity >::IndexType & newSize) // O(n)
{
	if(newSize == this->getCount())
		return true;
		
	if(newSize > this->getCapacity())
		return false;
		
	if(newSize < this->getCount())
	{
		for (IndexType i = this->next - 1; i >= newSize; --i)
			(&this->items[i])->~ItemType();
	}
	else
	{
		for (IndexType i = this->next; i < newSize; ++i)
			this->items[i] = ItemType();
	}
	this->next = newSize;
	return true;
}

template< typename ItemType, uint8_t Capacity >
void List< ItemType, Capacity >::clear(void) // O(n)
{
	for (IndexType i = 0; i < this->next; ++i)
		(&this->items[i])->~ItemType();
	this->next = 0;
}

template< typename ItemType, uint8_t Capacity >
bool List< ItemType, Capacity >::contains(const ItemType & item) const // O(n)
{
	return this->indexOf(item) != -1;
}

template< typename ItemType, uint8_t Capacity >
typename List< ItemType, Capacity >::IndexOfType List< ItemType, Capacity >::indexOf(const ItemType & item) const // O(n)
{
	for (IndexType i = 0; i < this->next; ++i)
		if (this->items[i] == item)
			return i;
	return -1;
}

template< typename ItemType, uint8_t Capacity >
ItemType & List< ItemType, Capacity >::operator [] (const typename List< ItemType, Capacity >::IndexType & index) // O(1)
{
	//DEBUG_ASSERT(index < this->getCapacity());
	//DEBUG_ASSERT(index < this->next);

	return this->items[index];
}

template< typename ItemType, uint8_t Capacity >
const ItemType & List< ItemType, Capacity >::operator [] (const typename List< ItemType, Capacity >::IndexType & index) const // O(1)
{
	//DEBUG_ASSERT(index < this->getCapacity());
	//DEBUG_ASSERT(index < this->next);

	return this->items[index];
}