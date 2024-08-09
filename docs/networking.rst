==========
Networking
==========

TODO Explain the NUClear network mesh

Serialisation
*************

Serialisation is used to convert data to a type that can be written to files or sent over a network connection and be
usable by the receiving device. See `Wikipedia <https://en.wikipedia.org/wiki/Serialization>`_ for further information.

NUClear provides helper functions for dealing with serialisation, from the namespace ``NUClear::util::serialise`` there
are ``Searialise<T>::serialise``, ``Searialise<T>::deserialise`` and ``Searialise<T>::hash`` for serialisation,
deserialisation and hashing the type's name respectively. The functions are used internally by NUClear to try to
serialise/deserialise data sent/received via ``Network`` or ``UDP``. These functions are only defined for specific
types.

Trivial Data Types
------------------

NUClear defines serialisation for `Trivial Types <https://en.cppreference.com/w/cpp/named_req/TrivialType>`_ with some
caveats. The serialisation of trivial data is dependant on both the
`endianness <https://en.wikipedia.org/wiki/Endianness>`_ and the
`alignment <https://en.cppreference.com/w/cpp/language/object#Alignment>`_ of the data on the computer running the code.

NUClear also defines serialisation of iterators of trivial data types.

Google Protobuf Types
---------------------

NUClear wraps the serialisation and deserialisation of google
`protobuf <https://developers.google.com/protocol-buffers/>`_ types. Use protobuf over trivial data types when
communicating between machines or programs.

Custom Types
------------

To add another type to be able to be serialised add another partial specialisation to ``Serialise<T, Check>`` declaring
the type of ``Check``. The easiest ``Check`` is `is_same <https://en.cppreference.com/w/cpp/types/is_same>`_ as this
checks explicitly for an explicit type. Be careful about multiple declarations.

For this partial specialisation three static methods need to be defined.

.. codeblock:: c++
    static std::vector<uint8_t> serialise(const T& in)

    static T deserialise(const std::vector<uint8_t>& in)

    static uint64_t hash()
