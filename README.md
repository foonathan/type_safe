# type_safe 

![Project Status](https://img.shields.io/endpoint?url=https%3A%2F%2Fwww.jonathanmueller.dev%2Fproject%2Ftype_safe%2Findex.json)
![Build Status](https://github.com/foonathan/type_safe/workflows/Main%20CI/badge.svg)

type_safe provides zero overhead abstractions that use the C++ type system to prevent bugs.

> Zero overhead abstractions here and in following mean abstractions that have no cost with optimizations enabled,
> but may lead to slightly lower runtime in debug mode,
> especially when assertions for this library are enabled.

The library features cannot really be explained in the scope of this readme,
I highly suggest that you check out [the first](https://www.foonathan.net/2016/10/type-safe/) and [second blog post](https://www.foonathan.net/2016/10/strong-typedefs/) and the examples.

> |[![](https://www.jonathanmueller.dev/embarcadero-logo.png)](https://www.embarcadero.com/de/products/cbuilder/starter) | Sponsored by [Embarcadero C++Builder](https://www.embarcadero.com/de/products/cbuilder/starter). |
> |-------------------------------------|----------------|
>
> If you like this project, consider [supporting me](https://jonathanmueller.dev/support-me/).

## Features

### Improved built-in types

* `ts::integer<T>` - a zero overhead wrapper over a built-in integer type
    * no default constructor to force meaningful initialization
    * no "lossy" conversions (i.e. from a bigger type or a type with a different signedness)
    * no mixed arithmetic with floating points or integer types of a different signedness
    * no mixed comparison with floating points
    * over/underflow is undefined behavior in release mode - even for `unsigned` integers,
      enabling compiler optimizations
* `ts::floating_point<T>` - a zero overhead wrapper over a built-in floating point
    * no default constructor to force meaningful initialization
    * no "lossy"  conversion (i.e. from a bigger type)
    * no "lossy" comparisons
    * no mixed arithmetic/comparison with integers
* `ts::boolean` - a zero overhead wrapper over `bool`
    * no default constructor to force meaningful initialization
    * no conversion from integer values
    * no arithmetic operators
* aliases like `ts::uint32_t` or `ts::size_t` that are either wrapper or built-in type depending on macro
* literal operators for those aliases like `342_u32` or `0_usize`

### Vocabulary types

* `ts::object_ref<T>` - a non-null pointer
* `ts::index_t` and `ts::distance_t` - index and distance integer types with only a subset of operations available
* `ts::array_ref<T>` - non-null reference to contigous storage
* `ts::function_ref<T>` - non-null reference to a function
* `ts::flag` - an improved flag type, better than a regular `bool` or `ts::boolean`
* `ts::flag_set<Enum>` - a set of flags
* `ts::output_parameter<T>` - an improved output parameter compared to the naive lvalue reference

### Optional & Variant

* `ts::basic_optional<StoragePolicy>` - a generic, improved `std::optional` that is fully monadic,
  also `ts::optional<T>` and `ts::optional_ref<T>` implementations
* `ts::compact_optional` implementation for no space overhead optionals
* `ts::basic_variant<VariantPolicy, Types...>` - a generic, improved `std::variant`, also `ts::variant` and `ts::fallback_variant` implementations

### Type safe building blocks

* `ts::constrained_type<T, Constraint, Verifier>` - a wrapper over some type that verifies that a certain constraint is always fulfilled
    * `ts::constraints::*` - predefined constraints like `non_null`, `non_empty`, ...
    * `ts::tagged_type<T, Constraint>` - constrained type without checking, useful for tagging
    * `ts::bounded_type<T>` - constrained type that ensures a value in a certain interval
    * `ts::clamped_type<T>` - constrained type that clamps a value to ensure that it is in the certain interval
* `ts::strong_typedef` - a generic facility to create strong typedefs more easily
* `ts::deferred_construction<T>` - create an object without initializing it yet

## Installation

Header-only, just copy the files in your project.
You need to add the type_safe `include` directory to your include path as well as make [debug_assert.hpp](https://github.com/foonathan/debug_assert) available.
The repository is included.
You also need to enable C++11.

Behavior can be customized with the following macros:

* `TYPE_SAFE_ENABLE_ASSERTIONS` (default is `1`): whether or not assertions are enabled in this library
* `TYPE_SAFE_ENABLE_WRAPPER` (default is `1`): whether or not the typedefs in `type_safe/types.hpp` use the wrapper classes
* `TYPE_SAFE_ARITHMETIC_POLICY` (`ub`/`checked`/`default`, default is `ub`): whether under/overflow in the better integer types is UB, an exception, or the default behavior

If you're using CMake there is the target `type_safe` available after you've called `add_subdirectory(path/to/type_safe)`.
Simply link this target to your target and it will setup everything automagically.
For convenience the macros are also mapped to CMake options of the same name.

## Documentation

You can find the full documentation generated by [standardese](https://github.com/standardese/standardese) [here](https://type_safe.foonathan.net/).

## Acknowledgements

This project is greatly supported by my [patrons](https://patreon.com/foonathan).
In particular thanks to the individual supporters:

* Mark Atkinson
* Reiner Eiteljörge 

And big thanks to the main contributors as well:

* Johel Ernesto Guerrero Peña [@johelegp](https://github.com/johelegp)
    
