#pragma once



// We don't want explicit dependencies on windows only headers
// Some day I want this to run on linux.
// Therefore quick and dirty little thing
template<typename T>
class ComPtr
{
public:

	ComPtr() : Ptr(nullptr) {};
	ComPtr(T* p) : Ptr(p) {}
	ComPtr(ComPtr<T>& rhs) : Ptr(rhs.Ptr)
	{
		if (Ptr)
		{
			Ptr->AddRef();
		}
	}
	~ComPtr()
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = nullptr;
		}
	}

	inline ComPtr<T>& operator = (T* rhs)
	{
		if (Ptr != rhs)
		{
			if (Ptr)
			{
				Ptr->Release();
			}
			Ptr = rhs;
			if (Ptr)
			{
				Ptr->AddRef();
			}
		}
		return *this;
	}

	inline ComPtr<T>& operator=(const ComPtr<T>& rhs)
	{
		return operator=(rhs.Ptr); // Calls the T* assignment overload
	}

	inline void UnsafeSet(T* inPtr)
	{
		Ptr = inPtr;
	}

	inline T* operator ->()
	{
		return Ptr;
	}

	inline T** operator &()
	{
		return ReleaseAndGetAddressOf();
	}

	inline const T* operator ->() const
	{
		return Ptr;
	}

	inline operator T* ()
	{
		return Ptr;
	}

	inline operator const T* () const
	{
		return Ptr;
	}

	inline T** ReleaseAndGetAddressOf()
	{
		if (Ptr)
		{
			Ptr->Release();
			Ptr = nullptr;
		}
		return &Ptr;
	}


	T* Ptr;

};