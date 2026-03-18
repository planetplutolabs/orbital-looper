/* =========================================================================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#pragma once

namespace BinaryData
{
    extern const char*   BoogalooRegular_ttf;
    const int            BoogalooRegular_ttfSize = 33960;

    extern const char*   OrbitronVariable_ttf;
    const int            OrbitronVariable_ttfSize = 38576;

    extern const char*   Exo2Variable_ttf;
    const int            Exo2Variable_ttfSize = 303940;

    extern const char*   Exo2ItalicVariable_ttf;
    const int            Exo2ItalicVariable_ttfSize = 302556;

    extern const char*   SpaceGroteskVariable_ttf;
    const int            SpaceGroteskVariable_ttfSize = 136676;

    // Number of elements in the namedResourceList and originalFileNames arrays.
    const int namedResourceListSize = 5;

    // Points to the start of a list of resource names.
    extern const char* namedResourceList[];

    // Points to the start of a list of resource filenames.
    extern const char* originalFilenames[];

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding data and its size (or a null pointer if the name isn't found).
    const char* getNamedResource (const char* resourceNameUTF8, int& dataSizeInBytes);

    // If you provide the name of one of the binary resource variables above, this function will
    // return the corresponding original, non-mangled filename (or a null pointer if the name isn't found).
    const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
}
