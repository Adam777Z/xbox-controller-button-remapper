#pragma once

#include <string>
#include <memory>
#include <vector>

namespace handy
{
    namespace io
    {
    	// Notes:
        // Duplicate sections will be merged
        // Duplicate item names will overwrite the first

        class INIFile
        {
            public:
                INIFile();
                ~INIFile();

				// Open a file, load its contents, and close it.
				// Returns whether this was successful.
                bool loadFile (const std::string& filePath);

				// Save the current settings to the specified file.
				// Note that formatting and comments aren't preserved
				// and shall be destroyed by calling this function.
				bool saveFile (const std::string& filePath);

				// Retrieve an item of the appropriate type. An item of the correct name but wrong type
				// is actually considered a different item altogether- indeed, it is possible to have
				// multiple entries of the same name differing only by value, but this is not sensible.
				//
				// There is a possible problem due to this- if an item is supposed to be a string but
				// happens to be "true" or "42", it will be read as a different type.
				//
				// Optionally, set *wasRead to true if the value was read correctly, or false if the default
				// was used instead.
                int64_t            getInteger (const std::string& section, const std::string& item, int64_t            defaultVal, bool* wasRead = nullptr) const;
                double             getDouble  (const std::string& section, const std::string& item, double             defaultVal, bool* wasRead = nullptr) const;
                bool               getBool    (const std::string& section, const std::string& item, bool               defaultVal, bool* wasRead = nullptr) const;
                const std::string& getString  (const std::string& section, const std::string& item, const std::string& defaultVal, bool* wasRead = nullptr) const;

				// Set an item of the appropriate type, returning success.
				// Existing items of the same section, name, and type will
				// be overwritten with no warning.
				//
				// Shan't be written to any file until saveFile() is called.
                bool setInteger (const std::string& section, const std::string& item, int64_t            value);
                bool setDouble  (const std::string& section, const std::string& item, double             value);
                bool setBool    (const std::string& section, const std::string& item, bool               value);
                bool setString  (const std::string& section, const std::string& item, const std::string& value);


            private:
            	INIFile(const INIFile& noCopyingAllowed);
            	INIFile& operator=(const INIFile& noCopyingAllowed);
            	bool loadLines(const std::vector<std::string>& lines);
                struct INIFileImpl;
                std::unique_ptr<INIFileImpl> impl;

        };

    }
}

