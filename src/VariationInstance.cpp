// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

using namespace Variation;

bool Variation::Compare_Instance(Instance first, Instance second)
{
	// Comparison function that, taking two values of the same type than those 
	// contained in the list object, returns true if the first argument goes before the 
	// second argument in the specific order, and false otherwise. 
	auto firstCoords = first.GetNormalizedFixed2_14Coordinates();
	auto secondCoords = second.GetNormalizedFixed2_14Coordinates();
	assert(firstCoords.size() == secondCoords.size());
	for (size_t i = 0; i < firstCoords.size(); ++i)
	{
		if (firstCoords[i].GetRawValue() < secondCoords[i].GetRawValue())
			return true;
		else if (firstCoords[i].GetRawValue() > secondCoords[i].GetRawValue())
			return false;
		// else go to next coord (if any)
	}
	return false;
}

void Variation::InstanceManager::ComputeAxisPointExtremes()
{
	axisPointMin_.clear();
	axisPointMax_.clear();

	// Compute the min/max axis points
	axisPointMin_.resize(axisCount_, 0);
	axisPointMax_.resize(axisCount_, 0);

	for (const auto & instance : instances_)
	{
		// exactally one non default coordinate makes it an "axis point"
		if (instance.Num() == 1)
		{
			auto nondefaultAxis = instance.Index(0);
			if (nondefaultAxis < axisCount_)
			{
				auto value = instance.Value(nondefaultAxis);
				if (value > 0)
					axisPointMax_[nondefaultAxis] = std::max(value, axisPointMax_[nondefaultAxis]);
				else
					axisPointMin_[nondefaultAxis] = std::min(value, axisPointMin_[nondefaultAxis]);
			}
		}
	}

	// Walk through instances and set max/min as approproiate.
	for (auto & instance : instances_)
	{
		if (instance.Num() == 1)
		{
			auto nondefaultAxis = instance.Index(0);
			auto value = instance.Value(nondefaultAxis);
			if (value > 0)
			{
				if (nondefaultAxis < axisPointMax_.size() && axisPointMax_[nondefaultAxis] == value)
					instance.SetExtreme(AxisExtreme::Positive, nondefaultAxis);
			}
			else
			{
				if (nondefaultAxis < axisPointMin_.size() && axisPointMin_[nondefaultAxis] == value)
					instance.SetExtreme(AxisExtreme::Negative, nondefaultAxis);
			}
		}
		else
		{
			instance.SetExtreme(AxisExtreme::None); 
		}
	}

	extremeValid_ = true; 
}

// Side door access to instances that are relavent to cvar table. 
std::shared_ptr<std::deque<Variation::CvarTuple*>> Variation::InstanceManager::GetCvarTuples()
{
	if (cvarTuples_ == nullptr)
	{
		cvarTuples_ = std::make_shared<std::deque<Variation::CvarTuple*>>();
	}

	cvarTuples_->clear(); 

	for (uint16_t index = 0; index < instances_.size(); index++)
	{
		Variation::Instance *instance = this->at_ptr(index);
		if (instance->IsCvar())
		{
			cvarTuples_->push_back(instance); 
		}
	}

	return cvarTuples_; 
}

// Side door access to instances copies that are relavent to cvar table.
std::deque<Variation::CvarTuple> Variation::InstanceManager::GetCvarTuplesCopy() 
{
	std::deque<Variation::CvarTuple> tuples; 

	for (uint16_t index = 0; index < instances_.size(); index++)
	{
		Variation::Instance instance = this->at(index);
		if (instance.IsCvar())
		{
			tuples.push_back(instance);
		}
	}

	return tuples;
}

// Side door access to instances that are relavent to TSIC table. 
// This is cvar tuples plus named tuples plus user tuples.
std::shared_ptr<std::deque<Variation::Instance*>> Variation::InstanceManager::GetPrivateCvarInstances()
{
	if (tsicInstances_ == nullptr)
	{
		tsicInstances_ = std::make_shared<std::deque<Variation::Instance*>>();
	}

	tsicInstances_->clear();

	for (uint16_t index = 0; index < instances_.size(); index++)
	{
		Variation::Instance *instance = this->at_ptr(index);
		if (instance->IsCvar() || instance->IsUser())
		{
			tsicInstances_->push_back(instance);
		}
	}

	return tsicInstances_;
	
}








