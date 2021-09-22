/*****
*
*	VariationModels.cpp
*
*	Copyright (c) Microsoft Corporation.
*   Licensed under the MIT License.
*
*	Concepts from models.py in fonttools project. 
*
*
*****/
#include "pch.h"

using namespace Variation; 

class CompareLocations {
public:
	static std::vector<std::vector<float>> axisPoints; 

	static bool Compare_Locations(const Location & first, const Location & second)
	{
		// Comparison function that, taking two values of the same type than those 
		// contained in the list object, returns true if the first argument goes before the 
		// second argument in the specific order, and false otherwise. 

		uint16_t firstRank = first.Num();
		uint16_t secondRank = second.Num();
		// # First, order by increasing rank
		if (firstRank != secondRank)
		{
			return firstRank < secondRank;
		}

		uint64_t firstAxisHash = first.AxisSortHash();
		uint64_t secondAxisHash = second.AxisSortHash();
		if (firstAxisHash != secondAxisHash)
		{
			return firstAxisHash < secondAxisHash; 
		}

		//std::vector<bool> onPointAxesFirst, onPointAxesSecond;
		//onPointAxesFirst.resize(axisPoints.size(), false);
		//onPointAxesSecond.resize(axisPoints.size(), false);
		uint16_t firstCount = 0; 
		uint16_t secondCount = 0; 

		for (uint16_t index = 0; index < axisPoints.size(); index++)
		{
			//onPointAxesFirst[index] = first.IsAxis(index) && In(first.Value(index), axisPoints[index]);
			firstCount += first.IsAxis(index) && In(first.Value(index), axisPoints[index]) ? 1 : 0;
			//onPointAxesSecond[index] = second.IsAxis(index) && In(second.Value(index), axisPoints[index]);
			secondCount += second.IsAxis(index) && In(second.Value(index), axisPoints[index]) ? 1 : 0;
		}

		// # Next, by decreasing number of onPoint axes
		if (firstCount != secondCount)
			return firstCount > secondCount; 

		float firstSum = 0; 
		float firstSgn = 0; 
		for (const auto & value : first.peakFloat)
		{
			firstSum += std::abs(value); 
			firstSgn += sgn(value); 
		}

		float secondSum = 0;
		float secondSgn = 0; 
		for (const auto & value : second.peakFloat)
		{
			secondSum += std::abs(value); 
			secondSgn += sgn(value); 
		}

		// # Next, by signs of axis values
		if (firstSgn != secondSgn)
			return firstSgn < secondSgn;

		// # Next, by absolute value of axis values
		return firstSum < secondSum; 
	}
};
std::vector<std::vector<float>> CompareLocations::axisPoints; 

// Returns the scalar multiplier at location, for a master with support
float SupportScalar(const Location &location, const std::vector<std::vector<float>> &support)
{
	float scalar = 1.0; 
	// each axis could have a triple
	for (uint16_t i = 0; i < support.size(); i++)
	{
		if (support[i].size() > 0)
		{
			auto lower = support[i][0]; 
			auto peak = support[i][1]; 
			auto upper = support[i][2]; 

			if (peak == 0)
				continue; 
			if (lower > peak || peak > upper)
				continue; 
			if (lower < 0 && upper > 0)
				continue; 

			auto v = location.peakFloat[i]; 

			if (v == peak)
				continue;
			if (v <= lower || upper <= v)
			{
				scalar = 0;
				break;
			}
			if (v < peak)
				scalar *= (v - lower) / (peak - lower);
			else // v > peak
				scalar *= (v - upper) / (peak - upper);
		}
	}

	return scalar; 
}

// Initialize the model with locations in variation space. 
bool Model::Init(std::deque<Location> &locations, uint16_t axisCount)
{
	axisCount_ = axisCount; 

	// set axisPoints to number of axis
	axisPoints_.resize(axisCount);

	// compute axisPoints
	for (const auto & loc : locations)
	{
		// one non default coordinate
		if (loc.Num() != 1)
			continue;

		// non default coordinate represents axis
		uint16_t nonDefaultCoord = loc.Index(0);
		assert(nonDefaultCoord < loc.peakFloat.size());

		const float zero = 0; 
		if(!In(zero, axisPoints_[nonDefaultCoord]))
			axisPoints_[nonDefaultCoord].push_back(zero);

		if (!In(loc.peakFloat[nonDefaultCoord], axisPoints_[nonDefaultCoord]))
			axisPoints_[nonDefaultCoord].push_back(loc.peakFloat[nonDefaultCoord]);
	}

	// sort locations 
	sortedLocations_ = locations;

	// Sort function requires axisPoints_ so attach to compare function class
	CompareLocations compareLocs; 
	compareLocs.axisPoints = axisPoints_;

	std::stable_sort(sortedLocations_.begin(), sortedLocations_.end(), CompareLocations::Compare_Locations); 		
	
	return ComputeMasterSupport();
}

template <typename T>
T minx(T value, const std::vector<T> lst)
{
	T returnValue = value;
	std::vector<T> values = lst;

	values.push_back(value); 

	auto min = std::min_element(values.begin(), values.end());
	if (min != values.end())
		returnValue = *min;

	return returnValue;
}

template <typename T>
T maxx(T value, const std::vector<T> lst)
{
	T returnValue = value;
	std::vector<T> values = lst;

	values.push_back(value);

	auto max = std::max_element(values.begin(), values.end());
	if (max != values.end())
		returnValue = *max;

	return returnValue;
}

// Compute supports and delta weights from sorted locations
bool Model::ComputeMasterSupport()
{
	for (uint16_t i = 0; i < sortedLocations_.size(); i++)
	{
		std::vector<std::vector<float>> box;
		box.resize(axisCount_); 
		bool haveBox = false; 

		auto loc = sortedLocations_[i];

		assert(axisPoints_.size() == axisCount_);
		assert(loc.peakFloat.size() == 0 || loc.peakFloat.size() == axisCount_); 

		// Account for axisPoints first
		for (uint16_t axisIndex = 0; axisIndex < axisPoints_.size(); axisIndex++)
		{
			// if not axis in loc
			if (!(axisPoints_[axisIndex].size() > 0 && loc.peakFloat.size() > 0 && loc.peakFloat[axisIndex] != 0))
				continue;

			float locV = loc.peakFloat[axisIndex];

			std::vector<float> boxAxis;
			if (locV > 0)
			{
				boxAxis.push_back(0);
				boxAxis.push_back(locV);
				boxAxis.push_back(maxx(locV, axisPoints_[axisIndex]));
			}
			else
			{
				boxAxis.push_back(minx(locV, axisPoints_[axisIndex]));
				boxAxis.push_back(locV);
				boxAxis.push_back(0);
			}
			box[axisIndex] = boxAxis;
			haveBox = true; 
		}
		 
		// Start Paul's model tweak to make sure we have a reasonable starting box even if no axisPoints from above. 
		if (!haveBox && loc.Num() > 0)
		{
			for (uint16_t axisIndex = 0; axisIndex < loc.peakFloat.size(); axisIndex++)
			{
				std::vector<float> boxAxis;
				float locV = loc.peakFloat[axisIndex];
				if (locV > 0)
				{
					boxAxis.push_back(0);
					boxAxis.push_back(locV);
					boxAxis.push_back(std::min(locV, 1.0f));
				}
				else
				{
					boxAxis.push_back(std::max(locV, -1.0f));
					boxAxis.push_back(locV);
					boxAxis.push_back(0);
				}
				box[axisIndex] = boxAxis;
				haveBox = true;
			}
		}
		// End Paul's model tweak

		// Walk over previous masters now
		for (uint16_t j = 0; j < i; j++)
		{
			auto m = sortedLocations_[j];
			//Master with extra axes do not participate
			if (!m.IsAxisSubset(loc))
				continue;
			// If it's NOT in the current box, it does not participate
			bool relavent = true;
			for (uint16_t k = 0; k < box.size(); k++)
			{
				if (box[k].size() > 0)
				{
					auto lower = box[k][0];
					auto peak = box[k][1];
					auto upper = box[k][2];

					// if axis not in m or not (m[axis] == peak or lower < m[axis] < upper)
					if (!m.IsAxis(k) || !((m.peakFloat[k] == peak) || ((lower < m.peakFloat[k]) && (m.peakFloat[k] < upper))))
					{
						relavent = false;
						break;
					}
				}
			}
			if (!relavent)
				continue;
			// Split the box for new master; split in whatever direction
			// that results in less area - ratio lost.
			uint16_t bestAxis = 0;
			bool haveBestAxis = false; 
			float bestRatio = -1; 
			float bestLower = 0;
			float bestUpper = 0; 
			float bestlocV = 0; 

			// When adding a master and computing its support, when hitting another master,
			// we don't need to limit our box in every direction; we just need to limit in
			// one so we don't hit the older master with our support. Implement heuristic
			// that takes the first axis that minimizes the area loss.

			// Split the box for new master; split in whatever direction
			// that has largest range ratio.

			for (int16_t k = 0; k < (int16_t)m.peakFloat.size(); k++)
			{
				auto val = m.peakFloat[k];
				if (m.IsAxis(k) && box[k].size() > 0)
				{
					float lower = box[k][0];
					float locV = box[k][1];
					float upper = box[k][2];
					float newLower = lower;
					float newUpper = upper;
					float ratio = -1; 
					if (val < locV)
					{
						newLower = val;
						ratio = (val - locV) / (lower - locV); 
					}
					else if (locV < val)
					{
						newUpper = val;
						ratio = (val - locV) / (upper - locV); 
					}
					else // locV == val
					{
						// Can't split box in this direction.
						continue; 
					}
					
					if (ratio > bestRatio)
					{
						haveBestAxis = true; 
						bestRatio = ratio;
						bestAxis = k;
						bestLower = newLower;
						bestUpper = newUpper;
						bestlocV = locV;
					}
				}
			}

			if (haveBestAxis)
			{
				box[bestAxis][0] = bestLower;
				box[bestAxis][1] = bestlocV;
				box[bestAxis][2] = bestUpper; 
			}
		}
		supports_.push_back(box); 

		std::vector<float> deltaWeight;
		deltaWeight.resize(sortedLocations_.size(), 0.0); 

		// Walk over previous masters now, populate deltaWeight
		for (uint16_t j = 0; j < i; j++)
		{
			float scalar = SupportScalar(loc, supports_[j]); 
			if (scalar)
				deltaWeight[j] = scalar; 
		}
		deltaWeights_.push_back(deltaWeight); 
	}
	return true; 
}

// Compute deltas from array of master value arrays. Outer array same order as locations passed to Init.
// Output outer array sorted by model so require mapping to determine index.
std::deque<std::vector<float>> Model::GetDeltas(const std::deque<std::vector<int16_t>> &masterValues)
{
	assert(masterValues.size() == deltaWeights_.size()); 
	std::deque<std::vector<float>> out; 
	for (size_t i = 0; i < deltaWeights_.size(); i++)
	{
		uint16_t mapping = sortedLocations_[i].PreSortIndex();

		std::vector<float> delta;
		delta.resize(masterValues[mapping].size(), 0);
		for(size_t k = 0; k < masterValues[mapping].size(); k++)
			delta[k]= masterValues[mapping][k];

		for (size_t j = 0; j < deltaWeights_[i].size(); j++)
		{
			float weight = deltaWeights_[i][j]; 
			if (!(weight != 0))
				continue; 

			for (size_t k = 0; k < delta.size(); k++)
				delta[k] -= out[j][k] * weight; 
		}
		out.push_back(delta); 
	} 

	return out; 
}

// Compute individual deltas
std::deque<float> Model::GetDeltas(const std::deque<int16_t> &masterValues)
{
	assert(masterValues.size() == deltaWeights_.size());
	std::deque<float> out;
	for (size_t i = 0; i < deltaWeights_.size(); i++)
	{
		uint16_t mapping = sortedLocations_[i].PreSortIndex();
		float delta = masterValues[mapping];
		for (size_t j = 0; j < deltaWeights_[i].size(); j++)
		{
			float weight = deltaWeights_[i][j];
			delta -= out[j] * weight;
		}
		out.push_back(delta);
	}

	return out;
}

