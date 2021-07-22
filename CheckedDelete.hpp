#pragma once

namespace handy
{
    namespace util
    {
        template<class T>
        inline void CheckedDelete(T* x)
        {
            // READ:
            // If you are seeing an error here when using StrictScopedPtr for pimpl,
            // be sure that the public class containing the impl defines a destructor.

            // That is, for a Foo containing a StrictScopedPtr<FooImpl>, Foo::~Foo must be
            // present and available to StrictScoptedPtr.
            typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
            (void) sizeof(type_must_be_complete);
            delete x;
        }
    }
}
