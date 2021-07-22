#pragma once

namespace handy
{
    namespace util
    {
        class NoCopy
        {
            protected:
                NoCopy() {}
                ~NoCopy() {}
            private:  // emphasize the following members are private
                NoCopy( const NoCopy& );
                const NoCopy& operator=( const NoCopy& );
        };
    }

}
