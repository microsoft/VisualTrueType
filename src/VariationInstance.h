// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace Variation
{
	enum class AxisExtreme { None, Positive, Negative };

	class Instance : public CvarTuple
	{
	public:
		static const size_t minUserNameLength = 2; 

		Instance() : CvarTuple()
		{}

		virtual ~Instance()
		{}

		Instance(const Tuple& tuple) : CvarTuple(tuple)
		{
		}

		Instance(const Tuple &tuple, const std::wstring& name, bool user = false) : CvarTuple(tuple)
		{
			nameString_ = name; 
			user_ = user;
		}

		//Instance(const Tuple &tuple, bool user = false) : CvarTuple(tuple)
		//{
		//	user_ = user;
		//}

		Instance(const CvarTuple &tuple) : CvarTuple(tuple)
		{
		}

		Instance(const std::vector<Fixed2_14> &normalizedFixed2_14Coordinates) : CvarTuple()
		{
			peakFloat.resize(normalizedFixed2_14Coordinates.size()); 
			for (size_t i = 0; i < normalizedFixed2_14Coordinates.size(); i++)
			{
				Fixed16_16 value = Fixed16_16::FromFixed2_14(normalizedFixed2_14Coordinates.at(i)); 
				peakFloat.at(i) = value.ToFloat(); 
			}

			peakF2Dot14 = normalizedFixed2_14Coordinates;
			
		}

		Instance(const std::vector<float> &normalizedAxisCoordinates, const std::vector<Fixed2_14> &normalizedFixed2_14Coordinates) : CvarTuple()
		{
			peakFloat = normalizedAxisCoordinates;
			peakF2Dot14 = normalizedFixed2_14Coordinates;
		}

		Instance(
			const std::vector<float> &normalizedAxisCoordinates,
			const std::vector<Fixed2_14> &normalizedFixed2_14Coordinates,
			const std::vector<float> &normalizedIntermediateStartCoordinates, 
			const std::vector<Fixed2_14> &normalizedIntermediateStartFixed2_14Coordinates,
			const std::vector<float> &normalizedIntermediateEndCoordinates,
			const std::vector<Fixed2_14> &normalizedIntermediateEndFixed2_14Coordinates) : CvarTuple()
		{
			peakFloat = normalizedAxisCoordinates; 
			peakF2Dot14 = normalizedFixed2_14Coordinates; 
			intermediateStartFloat = normalizedIntermediateStartCoordinates; 
			intermediateStartF2Dot14 = normalizedIntermediateStartFixed2_14Coordinates; 
			intermediateEndFloat = normalizedIntermediateEndCoordinates;
			intermediateEndF2Dot14 = normalizedIntermediateEndFixed2_14Coordinates; 
		}

		void SetCoordinates(const Tuple &tuple)
		{
			peakFloat = tuple.peakFloat;
			peakF2Dot14 = tuple.peakF2Dot14;
			intermediateStartFloat = tuple.intermediateStartFloat;
			intermediateStartF2Dot14 = tuple.intermediateStartF2Dot14;
			intermediateEndFloat = tuple.intermediateEndFloat;
			intermediateEndF2Dot14 = tuple.intermediateEndF2Dot14;
		}

		void SetCoordinates(const std::vector<float> &normalizedAxisCoordinates, const std::vector<Fixed2_14> &normalizedFixed2_14Coordinates, bool namedInstance)
		{
			peakFloat = normalizedAxisCoordinates;
			peakF2Dot14 = normalizedFixed2_14Coordinates;
			intermediateStartFloat.clear(); 
			intermediateStartF2Dot14.clear(); 
			intermediateEndFloat.clear();
			intermediateEndF2Dot14.clear();
			namedInstance_ = namedInstance;

		}

		void SetCoordinates(
			const std::vector<float> &normalizedAxisCoordinates,
			const std::vector<Fixed2_14> &normalizedFixed2_14Coordinates,
			const std::vector<float> &normalizedIntermediateStartCoordinates,
			const std::vector<Fixed2_14> &normalizedIntermediateStartFixed2_14Coordinates,
			const std::vector<float> &normalizedIntermediateEndCoordinates,
			const std::vector<Fixed2_14> &normalizedIntermediateEndFixed2_14Coordinates,
			bool namedInstance)
		{
			peakFloat = normalizedAxisCoordinates;
			peakF2Dot14 = normalizedFixed2_14Coordinates;
			intermediateStartFloat = normalizedIntermediateStartCoordinates;
			intermediateStartF2Dot14 = normalizedIntermediateStartFixed2_14Coordinates;
			intermediateEndFloat = normalizedIntermediateEndCoordinates;
			intermediateEndF2Dot14 = normalizedIntermediateEndFixed2_14Coordinates;
			namedInstance_ = namedInstance;
		}

		std::vector<float> GetNormalizedAxisCoordinates() const
		{
			return peakFloat;
		}

		std::vector<Fixed2_14> GetNormalizedFixed2_14Coordinates() const
		{
			return peakF2Dot14;
		}

		Tuple& GetTuple() 
		{
			return *this; 
		}

		CvarTuple& GetCvarTuple()
		{
			return *this; 
		}
		
		void SetName(const std::wstring &name)
		{
			nameString_ = name; 
		}

		std::wstring GetName() const
		{
			return nameString_; 
		}

		bool IsNamedInstance() const
		{
			return namedInstance_;
		}

		/*void SetAsNamedInstance(bool named = true)
		{
			namedInstance_ = named; 
		}*/ 

		bool IsUser() const
		{
			return user_; 
		}

		/*void SetAsUser(bool user = true)
		{
			user_ = user;
		}*/ 

		AxisExtreme GetExtreme() const
		{
			return extreme_;
		}

		uint16_t GetExtremeAxis() const
		{
			return extremeAxis_; 
		}

		void SetExtreme(AxisExtreme extreme, uint16_t axis = 0)
		{
			extreme_ = extreme;
			extremeAxis_ = axis; 
		}

	private:		
		std::wstring nameString_;		
		bool user_ = false;
		bool namedInstance_ = false; 
		AxisExtreme extreme_ = AxisExtreme::None; 
		uint16_t extremeAxis_ = 0; 
	};

	bool Compare_Instance(Instance first, Instance second);

	class InstanceManager
	{
	public:
		InstanceManager() {}
		virtual ~InstanceManager(){}

		void SetAxisCount(uint16_t count)
		{
			axisCount_ = count;
		}

		void Add(const Instance &instance)
		{
			instances_.push_back(instance);
			extremeValid_ = false;
		}

		void AddAndSort(const Instance &instance)
		{
			this->Add(instance); 
			this->Sort();
		}

		void EraseAndSort(uint16_t pos)
		{
			instances_.erase(instances_.begin() + pos);
			this->Sort();
			extremeValid_ = false;
		}

		uint16_t size() const
		{
			return static_cast<uint16_t>(instances_.size());
		}

		Instance& at(uint16_t index)
		{
			assert(index < instances_.size()); 
			if (!extremeValid_)
			{
				// Lazy compute
				ComputeAxisPointExtremes(); 
			}
			return instances_.at(index); 
		}

		Instance* at_ptr(uint16_t index)
		{
			assert(index < instances_.size());
			if (!extremeValid_)
			{
				// Lazy compute
				ComputeAxisPointExtremes();
			}
			return &instances_.at(index);
		}

		void clear()
		{
			extremeValid_ = false;
			instances_.clear(); 
		}

		bool IsInitialized() const
		{
			return instances_.size() > 0; 
		}

		void Sort()
		{
			std::stable_sort(instances_.begin(), instances_.end(), Compare_Instance);
		}

		// Side door access to instances that are relavent to cvar table.
		std::shared_ptr<std::deque<Variation::CvarTuple*>> GetCvarTuples(); 

		// Side door access to instances copies that are relavent to cvar table.
		// Copies needed when building cvar so can be sorted without touching original order. 
		std::deque<Variation::CvarTuple> GetCvarTuplesCopy();		

		// Side door access to instances that are relavent to TSIC table. 
		// This is cvar tuples plus named tuples plus user tuples.
		std::shared_ptr<std::deque<Variation::Instance*>> GetPrivateCvarInstances();
		
	private:
		std::deque<Instance> instances_; 
		std::shared_ptr<std::deque<Variation::CvarTuple*>> cvarTuples_ = nullptr;
		std::shared_ptr<std::deque<Variation::Instance*>> tsicInstances_ = nullptr;
		
		void ComputeAxisPointExtremes();

		uint16_t axisCount_ = 0; 
		std::vector<float> axisPointMin_;
		std::vector<float> axisPointMax_;
		bool extremeValid_ = false; 
	};
} // namespace Variation