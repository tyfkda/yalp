yalp
====

Yet Another List Processor

Under construction.

## Features

* Lisp-1 (same name space for function and variable)
* Case sensitive symbol
* Continuation
* Macro / Read macro
* Self hosting compiler
* Garbage collection (mark & sweep)

## No
* Bignum / Rational number / Complex number
* Keyword

## How to build
### Mac, Linux
* `make`
* `make test`

### Windows (Visual Studio 2012)
* Open build/yalp.vcproj
* [Build]


## Design

* Keep simple
* Embeddable
* Practical
* Can be used for shell scripting language

## Release
### v006
* Generate loop code for tail recursion

### v005
* Add function binder
* Chaned to Common Lisp like syntax
* Support inline function

### v004
* Add multiple values
* Add stream
* Add read macro (set-macro-character)
