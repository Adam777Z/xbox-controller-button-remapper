#pragma once

#include "NoCopy.hpp"
#include "CheckedDelete.hpp"

namespace handy
{
    namespace util
    {

        // comparable to unique_ptr, but cannot be moved, must be initialized and does not provide a reset().
        // Intended for pimpl use.
        template <typename T>
        class StrictScopedPtr
        {
            public:
                StrictScopedPtr(T* pointer) : data(pointer)
                {
                    // Work done in init list
                }

                ~StrictScopedPtr()
                {
                    CheckedDelete(data);
                    data = nullptr;
                }

                inline void reset ()
                {
                    CheckedDelete(data);
                    data = nullptr;
                }

                inline T* operator-> () const
                {
                    return data;
                }

                inline T& operator* () const
                {
                    return *data;
                }

            private:
                T* data;

                // No copy or assign
                StrictScopedPtr(const StrictScopedPtr&);
                const StrictScopedPtr& operator=(const StrictScopedPtr&);

                #if __cplusplus > 199711L
                    // If C++11, then no moving either, for that matter.
                    // shan't be triggered for some old versions of GCC, which
                    // support some 11 features including rvalues, but don't
                    // increment _cplusplus. No attempt is made to support this.
                    StrictScopedPtr(const StrictScopedPtr&&);
                    const StrictScopedPtr&& operator=(const StrictScopedPtr&&);
                #else
					//#error fdfdf
                #endif




        };
    }

}
