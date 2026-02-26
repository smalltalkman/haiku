/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */

#include "AllocationInfo.h"
#include "Attribute.h"
#include "Misc.h"
#include "Node.h"
#include "Volume.h"


Attribute::Attribute(Volume *volume, Node *node, const char *name,
					 uint32 type)
	: DataContainer(volume),
	  fNode(node),
	  fName(name),
	  fType(type),
	  fIndex(NULL),
	  fInIndex(false),
	  fIterators()
{
}


Attribute::~Attribute()
{
	ASSERT(fNode == NULL && fIndex == NULL);
}


status_t
Attribute::InitCheck() const
{
	return fName.GetString() ? B_OK : B_NO_INIT;
}


void
Attribute::SetNode(Node *node)
{
	if (fNode != NULL) {
		if (fIndex != NULL)
			fIndex->Removed(this);

		_NotifyRemoved();
	}

	fNode = node;

	if (fNode != NULL) {
		AttributeIndex* index = GetVolume()->FindAttributeIndex(GetName(), fType);
		if (index != NULL)
			index->Added(this);

		_NotifyAdded();
	}
}


void
Attribute::SetType(uint32 type)
{
	if (type == fType)
		return;

	if (fIndex != NULL)
		fIndex->Removed(this);
	_NotifyRemoved();

	fType = type;

	AttributeIndex *index = GetVolume()->FindAttributeIndex(GetName(), fType);
	if (index != NULL)
		index->Added(this);
	_NotifyAdded();
}


status_t
Attribute::SetSize(off_t newSize)
{
	off_t oldSize = DataContainer::GetSize();
	if (newSize == oldSize)
		return B_OK;

	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength = kMaxIndexKeyLength;
	GetKey(oldKey, &oldLength);

	status_t error = DataContainer::Resize(newSize);
	if (error != B_OK)
		return error;

	off_t changeOffset = (newSize < oldSize) ? newSize : oldSize;
	_Changed(oldKey, oldLength, changeOffset, newSize - oldSize);
	return B_OK;
}


status_t
Attribute::WriteAt(off_t offset, const void *buffer, size_t size, size_t *bytesWritten)
{
	// store the current key for the attribute
	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength = kMaxIndexKeyLength;
	GetKey(oldKey, &oldLength);

	// write the new value
	status_t error = DataContainer::WriteAt(offset, buffer, size, bytesWritten);
	if (error != B_OK)
		return error;

	// update index and send notifications
	_Changed(oldKey, oldLength, offset, size);
	return B_OK;
}


void
Attribute::_NotifyAdded()
{
	// notify node monitor
	notify_attribute_changed(GetVolume()->GetID(), -1, fNode->GetID(), GetName(),
		B_ATTR_CREATED);

	// update live queries
	uint8 newKey[kMaxIndexKeyLength];
	size_t newLength;
	GetKey(newKey, &newLength);
	GetVolume()->UpdateLiveQueries(NULL, fNode, GetName(),
		fType, NULL, 0, newKey, newLength);
}


void
Attribute::_NotifyRemoved()
{
	// notify node monitor
	notify_attribute_changed(GetVolume()->GetID(), -1, fNode->GetID(), GetName(),
		B_ATTR_REMOVED);

	// update live queries
	uint8 oldKey[kMaxIndexKeyLength];
	size_t oldLength;
	GetKey(oldKey, &oldLength);
	GetVolume()->UpdateLiveQueries(NULL, fNode, GetName(),
		fType, oldKey, oldLength, NULL, 0);
}


void
Attribute::_Changed(uint8* oldKey, size_t oldLength, off_t changeOffset, ssize_t changeSize)
{
	// If there is an index and a change has been made within the key, notify
	// the index.
	if (fIndex != NULL && changeOffset < (off_t)kMaxIndexKeyLength && changeSize != 0)
		fIndex->Changed(this, oldKey, oldLength);

	// notify node monitor
	notify_attribute_changed(GetVolume()->GetID(), -1, fNode->GetID(), GetName(),
		B_ATTR_CHANGED);

	// update live queries
	uint8 newKey[kMaxIndexKeyLength];
	size_t newLength;
	GetKey(newKey, &newLength);
	GetVolume()->UpdateLiveQueries(NULL, fNode, GetName(), fType, oldKey,
		oldLength, newKey, newLength);

	// node has been changed
	if (fNode != NULL && changeSize != 0)
		fNode->MarkModified(B_STAT_MODIFICATION_TIME);
}


void
Attribute::SetIndex(AttributeIndex *index, bool inIndex)
{
	ASSERT(fIndex == NULL || index == NULL || fIndex == index);
	ASSERT(!fInIndex || fInIndex != inIndex);

	fIndex = index;
	fInIndex = inIndex;
}


void
Attribute::GetKey(uint8 key[kMaxIndexKeyLength], size_t *length)
{
	ReadAt(0, key, kMaxIndexKeyLength, length);
}


void
Attribute::AttachAttributeIterator(AttributeIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && !iterator->IsSuspended())
		fIterators.Insert(iterator);
}


void
Attribute::DetachAttributeIterator(AttributeIterator *iterator)
{
	if (iterator && iterator->GetCurrent() == this && iterator->IsSuspended())
		fIterators.Remove(iterator);
}


void
Attribute::GetAllocationInfo(AllocationInfo &info)
{
	info.AddAttributeAllocation(DataContainer::GetCommittedSize());
	info.AddStringAllocation(fName.GetLength());
}
