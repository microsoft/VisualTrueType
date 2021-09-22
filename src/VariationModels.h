/*****
*
*	VariationModels.h
*
*	 Copyright (c) Microsoft Corporation.
*	Licensed under the MIT License.
*
*	Concepts from models.py in fonttools project.
*
*	Model (derived from MutatorMath) sorts masters first according to their descending number of on-axis 
*	coordinates and then distance from origin (kinda sorta), then allocates the support area of each master 
*	such that it does NOT affect the masters that came before it. For this reason, the linear equation system 
*	can trivially be solved by each delta calculation just subtracting the influence of deltas before it.
*
*
*****/
#pragma once

namespace Variation
{
	// For sorting according to sign
	template <typename T>
	int sgn(T val)
	{
		return (T(0) < val) - (val < T(0));
	}

	// Test if Element is in vector Vec
	template <typename T>
	const bool In(const T& Element, std::vector<T>& Vec)
	{
		if (std::find(Vec.begin(), Vec.end(), Element) != Vec.end())
			return true;

		return false;
	}

	class Location : public Tuple {
	public:
		Location() = delete;
		Location(uint16_t sortMap, uint16_t tupleIndex) :Tuple(), preSortLocIdx_(sortMap), tupleIdx_(tupleIndex)
		{}
		Location(const CvarTuple &cvarTuple, uint16_t sortMap, uint16_t tupleIndex) :Tuple(cvarTuple), preSortLocIdx_(sortMap), tupleIdx_(tupleIndex)
		{}
		Location(const CvarTuple &cvarTuple) = delete;

		// This can be done better so we don't potentially produce such large numbers for fonts with many axis. 
		uint64_t AxisSortHash() const
		{
			uint64_t hash = 0;
			uint64_t pos = 1;
			for (uint16_t i = 0; i < peakFloat.size(); i++)
			{
				if (IsAxis(i))
				{
					hash += pos; 
				}
				pos *= 10;
			}
			
			return hash; 
		}

		// Test if this is subset of b
		bool IsAxisSubset(const Location & b) const
		{
			if (peakFloat.size() == 0)
				return true;

			assert(peakFloat.size() == b.peakFloat.size());
			for (uint16_t i = 0; i < peakFloat.size(); i++)
			{
				if (IsAxis(i) && !b.IsAxis(i))
					return false;
			}

			return true;
		}

		// Return presort index
		uint16_t PreSortIndex()
		{
			return preSortLocIdx_;
		}

		uint16_t TupleIndex()
		{
			return tupleIdx_; 
		}

	private:
		uint16_t preSortLocIdx_; // bookkeeping to save pre sort index
		uint16_t tupleIdx_;      // save original tuple index
	};

	class Model {
	public:
		Model() :axisCount_(0) {}
		virtual ~Model() {}

		bool Init(std::deque<Location> &tuples, uint16_t axisCount);
		std::deque<std::vector<float>> GetDeltas(const std::deque<std::vector<int16_t>> &masterValues);
		std::deque<float> GetDeltas(const std::deque<int16_t> &masterValues);

		std::deque<Location> &GetSortedLocations()
		{
			return sortedLocations_;
		}

		std::deque<std::vector<std::vector<float>>> &GetSortedSupports()
		{
			return supports_; 
		}

	private:
		bool ComputeMasterSupport();

		std::vector<std::vector<float>> axisPoints_;
		uint16_t axisCount_;

		std::deque<Location> sortedLocations_;

		std::deque<std::vector<std::vector<float>>> supports_;
		std::deque<std::vector<float>> deltaWeights_;
	};

} // namespace Variation
