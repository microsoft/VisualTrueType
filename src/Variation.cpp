// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

using namespace Variation;

template <typename T>
T const& clamp(const T& value, const T& minValue, const T& maxValue)
{
	return std::min<T>(std::max<T>(value, minValue), maxValue);
}

bool Variation::IsFixed2_14CoordEqual(const std::vector<Fixed2_14> &first, const std::vector<Fixed2_14> &second, int16_t epsilon)
{
	bool result = false; 

	if (first.size() == second.size())
	{
		bool equal = true; 

		auto firstIt = first.cbegin();
		auto secondIt = second.cbegin(); 

		for (; equal && firstIt != first.end() && secondIt != second.end(); ++firstIt, ++secondIt)
		{
			if (!(abs(firstIt->GetRawValue() - secondIt->GetRawValue()) <= epsilon))
				equal = false;
		}
		result = equal; 
	}

	return result;
}

bool CVTVariationInterpolator1::ReverseInterpolate(const std::vector<int16_t> &defaultCvts, uint16_t axisCount, std::deque<CvarTuple*> &tuples)
{
	Model variationModel;
	std::deque<Location> axisLocations; // including default

	// Build locations for model 
	axisLocations.clear();

	std::deque<std::vector<int16_t>> masterValues;

	for (uint16_t i = 0; i < tuples.size(); i++)
	{
		// Make sure tuple is marked as active meaning it applies to cvar generation.
		if (!tuples[i]->IsCvar())
			continue; 

		// Copy location data from tuple, recording index of tuple.
		Variation::Location loc(*tuples[i], static_cast<uint16_t>(axisLocations.size()), i);
		axisLocations.push_back(loc);

		// Copy master values from tuple
		std::vector<int16_t> interpValues;
		size_t numCvts = tuples[i]->interpolatedCvtValues.size();
		assert(numCvts == defaultCvts.size());
		interpValues.resize(numCvts, 0);

		for (uint16_t j = 0; j < numCvts; j++)
		{
			interpValues[j] = tuples[i]->interpolatedCvtValues[j].Value<int16_t>();
		}
		masterValues.push_back(interpValues);
	}

	// Model requires base master be represented in locations
	bool baseFound = false;
	for (const auto & loc : axisLocations)
	{
		baseFound |= loc.IsDefault();
	}

	// If we have not found a base master add one. 
	if (!baseFound)
	{
		Variation::Location loc(static_cast<uint16_t>(axisLocations.size()), static_cast<uint16_t>(tuples.size()));
		loc.peakFloat.resize(axisCount, 0); 
		loc.peakF2Dot14.resize(axisCount, Fixed2_14(0));		
		axisLocations.push_back(loc);

		// Base master is default cvts. 
		masterValues.push_back(defaultCvts);
	}

	// Initialize the model with the location data
	variationModel.Init(axisLocations, axisCount);

	// Run the model on our master values
	std::deque<std::vector<float>> deltaValues = variationModel.GetDeltas(masterValues);

	// Get the sorted locations from the model
	std::deque<Location>& sortedAxisLocations = variationModel.GetSortedLocations(); 

	// Get supports from model so we can determine any intermediate start/end values. 
	std::deque<std::vector<std::vector<float>>>& supports = variationModel.GetSortedSupports(); 

	// Copy deltas from model to tuples including write order.
	// Generate start/end for intermediates if needed.
	// Model sorts locations so save that order in the tuples so tuples can be written in that order.
	// Start at 1 because model sorts base master to position 0 and we don't represent base in our tuple array. 
	for (uint16_t i = 1; i < sortedAxisLocations.size(); i++)
	{
		// Which tuple?
		uint16_t tupleIndex = sortedAxisLocations[i].TupleIndex();

		// Clear the delta data.
		tuples[tupleIndex]->cvt.clear();
		tuples[tupleIndex]->delta.clear();
		tuples[tupleIndex]->SetWriteOrder(i - 1); // zero based

		// Clear any previous intermediate coordinates. 
		tuples[tupleIndex]->intermediateStartFloat.clear();
		tuples[tupleIndex]->intermediateEndFloat.clear();
		tuples[tupleIndex]->intermediateStartF2Dot14.clear();
		tuples[tupleIndex]->intermediateEndF2Dot14.clear();

		// std::vector<float> &peak = sortedAxisLocations[i].peakFloat; 

		// Determine the intermediate start/end if needed.		
		bool intermediateNeeded = false; 
		for (uint16_t j = 0; j < axisCount; j++)
		{
			float minValue = 0, value = 0, maxValue = 0;
			std::vector<float> axisSupport = supports[i][j];
			if (axisSupport.size() > 0)
			{
				assert(axisSupport.size() == 3); 
				minValue = axisSupport[0];
				value = axisSupport[1];
				maxValue = axisSupport[2];
			}

			float defaultMinValue = std::min(value, 0.0f);
			float defaultMaxValue = std::max(value, 0.0f);
			if (minValue != defaultMinValue || maxValue != defaultMaxValue)
			{
				intermediateNeeded = true;
				break;
			}
		}

		if (intermediateNeeded)
		{
			tuples[tupleIndex]->intermediateStartFloat.resize(axisCount);
			tuples[tupleIndex]->intermediateEndFloat.resize(axisCount);
			tuples[tupleIndex]->intermediateStartF2Dot14.resize(axisCount);
			tuples[tupleIndex]->intermediateEndF2Dot14.resize(axisCount);

			for (uint16_t j = 0; j < axisCount; j++)
			{
				float minValue = 0, maxValue = 0;
				// float value = 0; 
				std::vector<float> axisSupport = supports[i][j];
				if (axisSupport.size() > 0)
				{
					assert(axisSupport.size() == 3);
					minValue = axisSupport[0];
					// value = axisSupport[1];
					maxValue = axisSupport[2];
				}
				
				tuples[tupleIndex]->intermediateStartFloat[j] = minValue;
				tuples[tupleIndex]->intermediateEndFloat[j] = maxValue;
				Fixed16_16 minValueFixed = Fixed16_16::FromFloat(minValue);
				Fixed16_16 maxValueFixed = Fixed16_16::FromFloat(maxValue);
				tuples[tupleIndex]->intermediateStartF2Dot14[j] = minValueFixed.ToFixed2_14();
				tuples[tupleIndex]->intermediateEndF2Dot14[j] = maxValueFixed.ToFixed2_14();
			}
		}

		// Apply the deltas from model to tuple. 
		std::vector<float>& deltas = deltaValues[i]; 
		for (uint16_t j = 0; j < deltas.size(); j++)
		{
			if (deltas[j] != 0)
			{
				tuples[tupleIndex]->cvt.push_back(j);
				tuples[tupleIndex]->delta.push_back(static_cast<int16_t>(std::roundf(deltas[j])));
			}
		}
	}

	return true; 
}
