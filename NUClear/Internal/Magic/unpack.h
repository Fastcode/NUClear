#ifndef NUCLEAR_INTERNAL_MAGIC_UNPACK_H
#define NUCLEAR_INTERNAL_MAGIC_UNPACK_H
namespace NUClear {
namespace Internal {
namespace Magic {
    /**
     * @brief This function is used to execute a series of function calls from a variardic template pack. 
     * @detail 
     *  This function is to be used as a helper to expand a variardic template pack into a series of function calls.
     *  As the variardic function pack can only be expanded in the situation where they are comma seperated (and the
     *  comma is a syntactic seperator not the comma operator) the only place this can expand is within a function call.
     *  This function serves that purpose by allowing a series of function calls to be expanded as it's parameters. This
     *  will execute each of them without having to recursivly evaluate the pack. e.g.
     *      unpack(function(TType)...);
     *
     *  In the case where the return type of the functions is void, then using the comma operator will allow the functions
     *  to be run because it changes the return type of the expansion (function(TType), 0) to int rather then void e.g.
     *      unpack((function(TType), 0)...);
     *  
     */
    template <typename... Ts>
    void unpack(Ts...) {}
}
}
}
#endif
