/*
 * Copyright (c) 2017, Philipp Ringler philipp@x-plane.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
*/

#pragma once

namespace xpilot {
	constexpr long DRE_MSG_ADD_DATAREF = 0x01000000;
	const char* const DRE_PLUGIN_SINATURE = "xplanesdk.examples.DataRefEditor";

	/**
  * @brief RWType distinguishes the three types of acess to datarefs.
  *
  * For simdata, it specifies if incoming data may be written to X-Plane
  * and if data should be readable.
  * For owned data, it specifies if the data is readable or writeable
  * to OTHER plugins (or anything that accesses datarefs e.g. panels).
  * @todo Should there be two enums for simdata and owned data?
  */
	enum RWType {
		ReadOnly = 1,
		WriteOnly = 2,
		ReadWrite = WriteOnly | ReadOnly
	};

	/**
	  * A generic base exception that is thrown when anything
	  * with looking up an X-Plane dataref and binding it goes wrong
	  */
	class LookupException : public std::runtime_error {
	public:
		LookupException(const std::string& msg) :
			runtime_error(msg) {
		}
	};

	/**
	  * The dataref identifier could not be found in the plugin system's table
	  * of datarefs, neither X-Plane nor a plugin publishes it.
	  */
	class NotFoundException : public LookupException {
	public:
		NotFoundException(const std::string& msg) :
			LookupException(msg) {
		}
	};

	/**
	  * The requested data type does not match the type specified in the
	  * plugin system's table of datarefs
	  */
	class IncompatibleTypeException : public LookupException {
	public:
		IncompatibleTypeException(const std::string& msg) :
			LookupException(msg) {
		}
	};

	/**
	 * Indicates that a write-acess (write-only or read/write) to a
	  * read-only dataref was requested
	  */
	class NotWriteableException : public LookupException {
	public:
		NotWriteableException(const std::string& msg) :
			LookupException(msg) {
		}
	};

	template <typename T>
	struct dataref_trait {
		typedef T BasicType;
	};

	template<>
	struct dataref_trait<std::vector<float> > {
		typedef float BasicType;
		mutable std::vector<float> cache_;
	};

	template<>
	struct dataref_trait<std::vector<int> > {
		typedef int BasicType;
		mutable std::vector<int> cache_;
	};

	template<>
	struct dataref_trait<std::string> {
		typedef char BasicType;
		mutable std::vector<char> cache_;
	};

	/**
	 * @brief Wrapper for access to datarefs published by XP itself or by other plugins.
	 *
	 * It wraps the lookup of datarefs, type-safe getting and setting of data, and
	 * obeying to X-Planes writeability restrictions.
	 * By creating a DataRef instance, it is bound to a specific dataref
	 * specified in the constructor.
	 * @author (c) 2009-2015 by Philipp Muenzel
	 * @version 2.0
	 */
	template <typename SimType>
	class DataRefAccess : private dataref_trait<SimType>
	{
	public:

		/**
		  * Sets up the dataref connection.
		  * looks up the datarefs and stores handler locally, also checks for correct
		  * type of data (sim-type) and correct read-/writeability
		  * @param identifier The identifier as in datarefs.txt as std::string
		  * @param writeability the writeability as defined by RWType
		  * @param share treat the dataref as a shared dataref, i.e. register the user as a shared dataref participant
		  * @exception LookupException is thrown if one of the following happens
		  * a) DataRef can not be found
		  * b) Data type is invalid (trying to access an int DataRef through float functions)
		  * c) data was requested to be writeable, but X-Plane says it is read-only
		  */
		DataRefAccess(const std::string& identifier, RWType writeability = ReadOnly, bool share = false, bool publish_in_dre = false);
		DataRefAccess(const DataRefAccess&) = delete;

		/**
		 * unshare dataref if it was created as shared
		 */
		virtual ~DataRefAccess();

		/**
		  * read the current value from X-Plane's plugin system
		  */
		operator SimType() const;

		/**
		  * write value to X-Plane
		  */
		const DataRefAccess& operator=(const SimType& rhs);

		/**
		  * read value from other dataref and write it to this dataref
		  */
		const DataRefAccess& operator=(const DataRefAccess& rhs);

		/**
		 * @return does the actual value differ from the last history value
		 */
		bool hasChanged() const;

		/**
		 * save the actual value to the history
		 */
		void save();

		/**
		 * overwrite the actual value with the last history value
		 */
		void undo();

		/**
		 * offset history value from actual value, so the next hasChanged returns true.
		 * Does not change the actual value of the dataref.
		 */
		void forceChanged();

		/**
		  * read the current value from X-Plane and access element in vector data
		  * @note is the same as operator SimType() for non-vector data
		  * @todo is there a more elegant way to do this?
		  */
		typename dataref_trait<SimType>::BasicType operator[](std::size_t index) const;

		/**
		 * callback that invokes the template function specified in a sub-class,
		 * if the value of a shared dataref has been changed by someone else
		 */
		void notify() { doNotify(); }

		std::string name() const { return identifier_; }

		/**
		  * write value of vector element to X-Plane
		  * @note is the same as operator=() for non-vector data
		  * @todo is there a more elegant way to do this?
		  */
		void setVal(std::size_t, typename dataref_trait<SimType>::BasicType val) {
			operator =(val);
		}

		/**
		 * reserve as many elements in the history value array as X-Plane says the vector can hold at maximum
		 * @note does nothing for non-vector data
		 */
		void reserve() {}

		/**
		 * reserve this many elements in the history value array
		 * @note does nothing for non-vector data
		 */
		void reserve(std::size_t) {}

	private:

		static void NotifactionFunc(void* refcon) {
			DataRefAccess* self = static_cast<DataRefAccess*>(refcon);
			self->notify();
		}

		void shareDataRef(const std::string& identifier, bool publish_in_dre);

		void publishInDRE();

		void share(int success, bool publish_in_dre);

		void unshareData();

		void lookUp(const std::string& identifier);

		void checkWriteabilityIsValid();

		void checkDataType();

		void logErrorWithDataRef(const std::string& error_msg, const std::string& dataref_identifier);

		virtual void doNotify() {}  //!< override this in a subclass if you want callback functionality

	private:
		XPLMDataRef m_data_ref; //!< opaque handle to X-Plane's data
		RWType m_read_write;    //!< is the data required to be write- or readable
		SimType m_history;      //!< stores the last value to detect changes
		bool shared_;
		std::string identifier_;
	};

	template <typename SimType>
	DataRefAccess<SimType>::DataRefAccess(const std::string& identifier,
		RWType writeability,
		bool share,
		bool publish_in_dre) :
		m_data_ref(nullptr),
		m_read_write(writeability),
		shared_(false),
		identifier_(identifier) {
		try {
			if (share && XPLMFindDataRef(identifier.c_str()) == nullptr)
				shareDataRef(identifier, publish_in_dre);
			lookUp(identifier);
			checkDataType();
			checkWriteabilityIsValid();
		}
		catch (LookupException& ex) {}
	}

	template <typename SimType>
	DataRefAccess<SimType>::~DataRefAccess() {
		if (shared_)
			unshareData();
	}

	template <typename SimType>
	const DataRefAccess<SimType>& DataRefAccess<SimType>::operator=(const DataRefAccess<SimType>& rhs) {
		return operator=(SimType(rhs));
	}

	template <typename SimType>
	bool DataRefAccess<SimType>::hasChanged() const {
		return true;
	}

	template <>
	bool DataRefAccess<int>::hasChanged() const;

	template <>
	bool DataRefAccess<float>::hasChanged() const;

	template <>
	bool DataRefAccess<double>::hasChanged() const;

	template <>
	bool DataRefAccess<std::vector<int> >::hasChanged() const;

	template <>
	bool DataRefAccess<std::vector<float> >::hasChanged() const;

	template <>
	bool DataRefAccess<std::string>::hasChanged() const;

	template <typename SimType>
	void DataRefAccess<SimType>::forceChanged() {
		m_history = std::numeric_limits<SimType>::max();
	}

	template <>
	void DataRefAccess<std::vector<int> >::forceChanged();

	template <>
	void DataRefAccess<std::vector<float> >::forceChanged();

	template <>
	void DataRefAccess<std::string>::forceChanged();

	template <typename SimType>
	void DataRefAccess<SimType>::save() {
		m_history = operator SimType();
	}

	template <typename SimType>
	void DataRefAccess<SimType>::undo() {
		operator=(m_history);
	}

	template <typename SimType>
	void DataRefAccess<SimType>::shareDataRef(const std::string&, bool) {
		throw IncompatibleTypeException("No type defined for sharing data");
	}

	template<>
	void DataRefAccess<int>::shareDataRef(const std::string&, bool publish_in_dre);
	template<>
	void DataRefAccess<float>::shareDataRef(const std::string&, bool publish_in_dre);
	template<>
	void DataRefAccess<double>::shareDataRef(const std::string&, bool publish_in_dre);
	template<>
	void DataRefAccess<std::vector<int> >::shareDataRef(const std::string&, bool publish_in_dre);
	template<>
	void DataRefAccess<std::vector<float> >::shareDataRef(const std::string&, bool publish_in_dre);
	template<>
	void DataRefAccess<std::string>::shareDataRef(const std::string&, bool publish_in_dre);

	template <typename SimType>
	void DataRefAccess<SimType>::unshareData() {
		throw IncompatibleTypeException("No type defined for sharing data");
	}

	template <typename SimType>
	void DataRefAccess<SimType>::share(int success, bool publish_in_dre) {
		if (!success)
			throw IncompatibleTypeException("Could not share data " + identifier_ + " type mismatch with already existing data.");
		shared_ = true;
		if (publish_in_dre)
			publishInDRE();
	}

	template<>
	void DataRefAccess<int>::unshareData();
	template<>
	void DataRefAccess<float>::unshareData();
	template<>
	void DataRefAccess<double>::unshareData();
	template<>
	void DataRefAccess<std::vector<int> >::unshareData();
	template<>
	void DataRefAccess<std::vector<float> >::unshareData();
	template<>
	void DataRefAccess<std::string>::unshareData();

	template <typename SimType>
	void DataRefAccess<SimType>::publishInDRE() {
		XPLMPluginID PluginID = XPLMFindPluginBySignature(DRE_PLUGIN_SINATURE);
		if (PluginID != XPLM_NO_PLUGIN_ID)
			XPLMSendMessageToPlugin(PluginID, DRE_MSG_ADD_DATAREF, static_cast<void*>(const_cast<char*>(identifier_.c_str())));
	}

	template <typename SimType>
	void DataRefAccess<SimType>::lookUp(const std::string& identifier) {
		m_data_ref = XPLMFindDataRef(identifier.c_str());
		if (!m_data_ref)
			throw NotFoundException(identifier + " not found in X-Plane's available Datarefs.");
	}

	template <typename SimType>
	void DataRefAccess<SimType>::checkWriteabilityIsValid() {
		if (m_read_write == WriteOnly || m_read_write == ReadWrite)
			if (!XPLMCanWriteDataRef(m_data_ref))
				throw NotWriteableException("Declared to be writeable, but X-Plane says it is read-only.");
	}

	template <typename SimType>
	void DataRefAccess<SimType>::checkDataType() {
		throw IncompatibleTypeException("No type defined.");
	}

	template <typename SimType>
	void DataRefAccess<SimType>::logErrorWithDataRef(const std::string& error_msg, const std::string& dataref_identifier) {

	}

	template <>
	void DataRefAccess<int>::checkDataType();

	template <>
	void DataRefAccess<float>::checkDataType();

	template <>
	void DataRefAccess<double>::checkDataType();

	template <>
	void DataRefAccess<std::vector<float> >::checkDataType();

	template <>
	void DataRefAccess<std::vector<int> >::checkDataType();

	template <>
	void DataRefAccess<std::string>::checkDataType();

	template <>
	DataRefAccess<int>::operator int() const;

	template <>
	DataRefAccess<float>::operator float() const;

	template <>
	DataRefAccess<double>::operator double() const;

	template <>
	DataRefAccess<std::vector<float> >::operator std::vector<float>() const;

	template <>
	DataRefAccess<std::vector<int> >::operator std::vector<int>() const;

	template <>
	DataRefAccess<std::string>::operator std::string() const;

	template <>
	const DataRefAccess<int>& DataRefAccess<int>::operator=(const int&);

	template <>
	const DataRefAccess<float>& DataRefAccess<float>::operator=(const float&);

	template <>
	const DataRefAccess<double>& DataRefAccess<double>::operator=(const double&);

	template <>
	const DataRefAccess<std::vector<float> >& DataRefAccess<std::vector<float> >::operator=(const std::vector<float>&);

	template <>
	const DataRefAccess<std::vector<int> >& DataRefAccess<std::vector<int> >::operator=(const std::vector<int>&);

	template <>
	const DataRefAccess<std::string>& DataRefAccess<std::string>::operator=(const std::string&);

	template <typename SimType>
	typename dataref_trait<SimType>::BasicType DataRefAccess<SimType>::operator[](std::size_t) const {
		typedef typename dataref_trait<SimType>::BasicType T;
		return T(*this);
	}

	template<>
	dataref_trait<std::vector<float> >::BasicType DataRefAccess<std::vector<float> >::operator[](std::size_t index) const;

	template<>
	dataref_trait<std::vector<int> >::BasicType DataRefAccess<std::vector<int> >::operator[](std::size_t index) const;

	template<>
	dataref_trait<std::string>::BasicType DataRefAccess<std::string>::operator[](std::size_t index) const;

	template<>
	void DataRefAccess<std::vector<int> >::setVal(std::size_t pos, int val);
	template<>
	void DataRefAccess<std::vector<float> >::setVal(std::size_t pos, float val);
	template<>
	void DataRefAccess<std::string >::setVal(std::size_t pos, char val);

	template<>
	void DataRefAccess<std::vector<int> >::reserve(std::size_t i);
	template<>
	void DataRefAccess<std::vector<float> >::reserve(std::size_t i);
	template<>
	void DataRefAccess<std::string>::reserve(std::size_t i);

	template<>
	void DataRefAccess<std::vector<int> >::reserve();
	template<>
	void DataRefAccess<std::vector<float> >::reserve();
	template<>
	void DataRefAccess<std::string>::reserve();
}