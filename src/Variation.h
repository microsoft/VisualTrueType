// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

struct FVarTableHeader;
struct AxisVariationHeader; 

namespace Variation
{	
	bool IsFixed2_14CoordEqual(const std::vector<Fixed2_14>& first, const std::vector<Fixed2_14>& second, int16_t epsilon = 1);

	template<typename GenericTuple1, typename GenericTuple2>
	bool IsTupleCoordsEqual(const GenericTuple1 &first, const GenericTuple2 &second)
	{
		return (IsFixed2_14CoordEqual(first.peakF2Dot14, second.peakF2Dot14)); 
	}

	template<typename GenericTuple1, typename GenericTuple2>
	bool IsCoordinatesExisting(std::deque<GenericTuple1> &tuples, const GenericTuple2 &tuple)
	{
		bool found = false;
		for (auto tuplesIt = tuples.cbegin(); tuplesIt != tuples.cend() && !found; ++tuplesIt)
		{
			if (IsFixed2_14CoordEqual(tuplesIt->peakF2Dot14, tuple.peakF2Dot14) &&
				IsFixed2_14CoordEqual(tuplesIt->intermediateStartF2Dot14, tuple.intermediateStartF2Dot14) &&
				IsFixed2_14CoordEqual(tuplesIt->intermediateEndF2Dot14, tuple.intermediateEndF2Dot14))
			{
				found = true;
			}
		}

		return found;
	}

	template<typename GenericTuple, typename GenericTupleIt>
	void ExclusiveAdd(std::deque<GenericTuple> &tuples, GenericTupleIt location, const GenericTuple &tuple)
	{
		if (!IsCoordinatesExisting(tuples, tuple))
			tuples.insert(location, tuple);
	}

	class Tuple {
	public:
		Tuple() {}
		virtual ~Tuple() {}
		Tuple(const Tuple &tuple) : peakFloat(tuple.peakFloat), peakF2Dot14(tuple.peakF2Dot14),
			intermediateStartFloat(tuple.intermediateStartFloat), intermediateStartF2Dot14(tuple.intermediateStartF2Dot14),
			intermediateEndFloat(tuple.intermediateEndFloat), intermediateEndF2Dot14(tuple.intermediateEndF2Dot14) {}

		void CopyCoordinates(const Tuple& tuple)
		{
			peakFloat = tuple.peakFloat;
			peakF2Dot14 = tuple.peakF2Dot14; 
			CopyIntermediates(tuple);
		}

		void CopyIntermediates(const Tuple& tuple)
		{
			intermediateStartFloat = tuple.intermediateStartFloat; 
			intermediateStartF2Dot14 = tuple.intermediateStartF2Dot14;
			intermediateEndFloat = tuple.intermediateEndFloat;
			intermediateEndF2Dot14 = tuple.intermediateEndF2Dot14;
		}

		void ClearCoordinates()
		{
			peakFloat.clear();
			peakF2Dot14.clear();
			intermediateStartFloat.clear();
			intermediateStartF2Dot14.clear();
			intermediateEndFloat.clear();
			intermediateEndF2Dot14.clear();
		}

		// Returns number coordinates not default
		uint16_t Num() const
		{
			uint16_t count = 0;
			for (const auto & value : peakFloat)
				if (value != 0.0) // this not default
					count++;

			return count;
		}

		bool IsDefault() const
		{
			return (Num() == 0);
		}

		// Return dataIndex'ed non default index
		uint16_t Index(uint16_t dataIndex) const
		{
			uint16_t internalIndex = 0;
			uint16_t internalDataIndex = 0;
			for (const auto & value : peakFloat)
			{
				if (value != 0.0) // this not default
				{
					if (dataIndex == internalDataIndex)
						break;
					internalDataIndex++;
				}
				internalIndex++;
			}

			return internalIndex;
		}

		float Value(uint16_t index) const
		{
			assert(index < peakFloat.size());
			return peakFloat[index];
		}

		// Test if axis not default
		bool IsAxis(uint16_t axis) const
		{
			if (axis >= peakFloat.size())
				return false;

			return peakFloat[axis] != 0;
		}

		bool HasIntermediate() const
		{
			return (intermediateStartFloat.size() > 0 || intermediateStartF2Dot14.size() > 0); 
		}

		// All coordinates normalized. 
		std::vector<float> peakFloat;
		std::vector<Fixed2_14> peakF2Dot14;
		std::vector<float> intermediateStartFloat;
		std::vector<Fixed2_14> intermediateStartF2Dot14;
		std::vector<float> intermediateEndFloat;
		std::vector<Fixed2_14> intermediateEndF2Dot14;
	};

	class GvarTuple : public Tuple {
	public:
	private:
	};

	class InterpolatedCvtValue
	{
	public:
		InterpolatedCvtValue() : value_(0) {}

		template <typename T>
		InterpolatedCvtValue(T value) : value_(value) {}

		template <typename T>
		void ApplyDelta(T delta)
		{
			value_ += delta;
		}

		template <typename T>
		T Value() const
		{
			return value_;
		}

		template <typename T>
		void Update(T value)
		{
			value_ = value;
		}

	private:
		int32_t value_;
	};

	class EditedCvtValue
	{
	public:
		EditedCvtValue(): value_(0), edited_(false) {}

		template <typename T>
		EditedCvtValue(T value) : value_(value), edited_(true) {}

		template <typename T>
		EditedCvtValue(T value, bool edited) : value_(value), edited_(edited) {}

		template <typename T>
		T Value() const
		{
			return value_;
		}

		bool Edited() const
		{
			return edited_;
		}

		void Clear()
		{
			value_ = 0;
			edited_ = false;
		}

		template <typename T>
		void Update(T value)
		{
			value_ = value;
			edited_ = true;
		}

	private:
		int32_t value_ = 0;
		bool edited_ = false; 
	};

	class CvarTuple : public Tuple {
	public:
		
		CvarTuple()
		{
			master_ = false;
			cvar_ = false; 
			writeOrder_ = 0; 
		}

		CvarTuple(const Tuple& tuple) : Tuple(tuple)
		{
			master_ = false;
			cvar_ = false;
			writeOrder_ = 0;
		}

		CvarTuple(const GvarTuple& tuple) : Tuple(tuple)
		{
			master_ = true;
			cvar_ = false;
			writeOrder_ = 0; 
		}

		// Raw values from cvar table.
		std::vector<uint16_t> cvt;
		std::vector<int16_t> delta;

		// Interpolated values for UI. 
		std::vector<InterpolatedCvtValue> interpolatedCvtValues;

		// User edited values
		std::vector<EditedCvtValue> editedCvtValues;

		void SetAsMaster(bool master = true)
		{
			master_ = master;
		}

		bool IsMaster() const
		{
			return master_;
		}

		void SetAsCvar(bool cvar = true)
		{
			cvar_ = cvar;
		}

		bool IsCvar() const
		{
			return cvar_;
		}

		// Does this tuple have delta data or is it just a point in the axis coordinate space?
		bool HasData() const
		{
			assert(cvt.size() == delta.size());
			return cvt.size() > 0;
		}

		bool HasData(uint16_t cv) const
		{
			for (uint16_t i = 0; i < cvt.size(); i++)
			{
				if (cvt[i] == cv)
				{
					return delta[i] != 0; 
				}
			}

			return false; 
		}

		void SetWriteOrder(uint16_t index)
		{
			writeOrder_ = index; 
		}
		uint16_t GetWriteOrder() const 
		{
			return writeOrder_;
		}

	private:
		bool master_ = false;
		bool cvar_ = false;
		uint16_t writeOrder_ = 0; 
	};

	class CVTVariationInterpolator1 
	{
	public:
		CVTVariationInterpolator1()			
		{
		}

		bool ReverseInterpolate(const std::vector<int16_t> &cvtDefaults, uint16_t axisCount, std::deque<CvarTuple*> &tuples);
		
	private:
		
	};

} // namespace Variation
