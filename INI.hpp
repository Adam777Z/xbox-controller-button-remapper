#pragma once

#include "StrictScopedPtr.hpp"
#include "NoCopy.hpp"

#include <string>

namespace handy
{
    namespace io
    {
        // Duplicate sections will be merged
        // Duplicate item names will overwrite the first

        class INIFile : public util::NoCopy
        {
            public:
                INIFile();
                ~INIFile();

                bool      loadFile   (const std::string& filePath)                   ;
                bool      isValid    ()                                        const ;

                int64_t   getInteger (const std::string& sectionName, const std::string& itemName, int64_t defaultValue)                const ;
                int64_t   getInteger (const std::string& sectionName, const std::string& itemName, int64_t defaultValue, bool& wasRead) const ;

                double    getDouble  (const std::string& sectionName, const std::string& itemName, double defaultValue)                 const ;
                double    getDouble  (const std::string& sectionName, const std::string& itemName, double  defaultValue, bool& wasRead) const ;

				bool      getBool    (const std::string& sectionName, const std::string& itemName, bool defaultValue)                   const ;
                bool      getBool    (const std::string& sectionName, const std::string& itemName, bool  defaultValue, bool& wasRead)   const ;

				const std::string& getString   (const std::string& sectionName, const std::string& itemName, const std::string& defaultValue)                const ;
                const std::string& getString   (const std::string& sectionName, const std::string& itemName, const std::string& defaultValue, bool& wasRead) const ;


                //



            private:
                struct INIFileImpl;
                util::StrictScopedPtr<INIFileImpl> impl;

        };

    }
}

